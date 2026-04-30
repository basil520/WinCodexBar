#include "ClaudeProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/CookieImporter.h"
#include "../../models/ClaudeUsageSnapshot.h"

#include <QJsonDocument>
#include <QJsonArray>

ClaudeProvider::ClaudeProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> ClaudeProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new ClaudeOAuthStrategy(), new ClaudeWebStrategy() };
}

ClaudeOAuthStrategy::ClaudeOAuthStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool ClaudeOAuthStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return ClaudeOAuthCredentials::load(ctx.env).has_value();
}

bool ClaudeOAuthStrategy::shouldFallback(const ProviderFetchResult& result,
                                          const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult ClaudeOAuthStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "oauth";

    auto credsOpt = ClaudeOAuthCredentials::load(ctx.env);
    if (!credsOpt.has_value()) {
        result.success = false;
        result.errorMessage = "Claude OAuth credentials not found. Run `claude` to authenticate.";
        return result;
    }

    ClaudeOAuthCredentials creds = *credsOpt;

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + creds.accessToken;
    headers["Accept"] = "application/json";
    headers["Content-Type"] = "application/json";
    headers["anthropic-beta"] = "oauth-2025-04-20";
    headers["User-Agent"] = "claude-code/2.1.0";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://api.anthropic.com/api/oauth/usage"), headers, ctx.networkTimeoutMs);

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "empty or invalid response from Claude OAuth API";
        return result;
    }

    if (json.contains("error")) {
        result.success = false;
        result.errorMessage = json.value("error").toObject().value("message").toString("OAuth error");
        return result;
    }

    ClaudeUsageSnapshot snap = ClaudeUsageSnapshot::fromOAuthJson(json);
    if (!snap.isValid()) {
        result.success = false;
        result.errorMessage = "no usage data in Claude OAuth response";
        return result;
    }

    if (creds.rateLimitTier.has_value()) {
        snap.loginMethod = claudePlanDisplayName(claudePlanFromRateLimitTier(*creds.rateLimitTier));
    }

    result.usage = snap.toUsageSnapshot();
    result.success = true;
    return result;
}

ClaudeWebStrategy::ClaudeWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool ClaudeWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        return true;
    }
    for (auto browser : CookieImporter::importOrder()) {
        if (CookieImporter::isBrowserInstalled(browser)) return true;
    }
    return false;
}

bool ClaudeWebStrategy::shouldFallback(const ProviderFetchResult& result,
                                        const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

std::optional<QString> ClaudeWebStrategy::extractSessionKey(const QVector<QNetworkCookie>& cookies) {
    for (const auto& cookie : cookies) {
        if (cookie.name() == "sessionKey") {
            QString value = QString::fromUtf8(cookie.value()).trimmed();
            if (value.startsWith("sk-ant-")) return value;
        }
    }
    return std::nullopt;
}

QString ClaudeWebStrategy::fetchOrgId(const QString& sessionKey, int timeoutMs) {
    QHash<QString, QString> headers;
    headers["Cookie"] = "sessionKey=" + sessionKey;
    headers["Accept"] = "application/json";

    QString response = NetworkManager::instance().getStringSync(
        QUrl("https://claude.ai/api/organizations"), headers, timeoutMs);

    if (response.isEmpty()) return {};

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return {};

    QJsonArray orgs;
    if (doc.isArray()) {
        orgs = doc.array();
    } else if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains("uuid")) return obj.value("uuid").toString();
        QJsonValue data = obj.value("data");
        if (data.isArray()) orgs = data.toArray();
    }

    for (const auto& orgVal : orgs) {
        QJsonObject org = orgVal.toObject();
        QJsonArray caps = org.value("capabilities").toArray();
        bool hasChat = false;
        for (const auto& c : caps) {
            if (c.toString().toLower() == "chat") { hasChat = true; break; }
        }
        if (hasChat) return org.value("uuid").toString();
    }

    if (!orgs.isEmpty()) return orgs[0].toObject().value("uuid").toString();
    return {};
}

ClaudeUsageSnapshot ClaudeWebStrategy::fetchUsageData(const QString& orgId, const QString& sessionKey, int timeoutMs) {
    QHash<QString, QString> headers;
    headers["Cookie"] = "sessionKey=" + sessionKey;
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://claude.ai/api/organizations/" + orgId + "/usage"), headers, timeoutMs);

    return ClaudeUsageSnapshot::fromWebJson(json);
}

std::optional<ProviderCostSnapshot> ClaudeWebStrategy::fetchOverageCost(
    const QString& orgId, const QString& sessionKey, int timeoutMs)
{
    QHash<QString, QString> headers;
    headers["Cookie"] = "sessionKey=" + sessionKey;
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://claude.ai/api/organizations/" + orgId + "/overage_spend_limit"), headers, timeoutMs);

    if (json.isEmpty()) return std::nullopt;
    if (!json.value("is_enabled").toBool(false)) return std::nullopt;

    double usedCredits = json.value("used_credits").toDouble(0);
    double monthlyLimit = json.value("monthly_credit_limit").toDouble(0);
    QString currency = json.value("currency").toString("USD");

    if (usedCredits <= 0 && monthlyLimit <= 0) return std::nullopt;

    ProviderCostSnapshot cost;
    cost.used = usedCredits / 100.0;
    cost.limit = monthlyLimit / 100.0;
    cost.currencyCode = currency;
    cost.period = "Monthly";
    cost.updatedAt = QDateTime::currentDateTime();
    return cost;
}

std::optional<QString> ClaudeWebStrategy::fetchAccountEmail(const QString& sessionKey, int timeoutMs) {
    QHash<QString, QString> headers;
    headers["Cookie"] = "sessionKey=" + sessionKey;
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://claude.ai/api/account"), headers, timeoutMs);

    if (json.isEmpty()) return std::nullopt;
    QString email = json.value("email_address").toString().trimmed();
    if (email.isEmpty()) email = json.value("emailAddress").toString().trimmed();
    return email.isEmpty() ? std::optional<QString>{} : email;
}

ProviderFetchResult ClaudeWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QString sessionKey;

    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        QString header = *ctx.manualCookieHeader;
        for (const auto& pair : header.split(';', Qt::SkipEmptyParts)) {
            QString trimmed = pair.trimmed();
            int eq = trimmed.indexOf('=');
            if (eq > 0) {
                QString name = trimmed.left(eq).trimmed();
                QString value = trimmed.mid(eq + 1).trimmed();
                if (name == "sessionKey" && value.startsWith("sk-ant-")) {
                    sessionKey = value;
                    break;
                }
            }
        }
    }

    if (sessionKey.isEmpty()) {
        QStringList domains = {"claude.ai"};
        for (auto browser : CookieImporter::importOrder()) {
            if (!CookieImporter::isBrowserInstalled(browser)) continue;
            QVector<QNetworkCookie> cookies = CookieImporter::importCookies(browser, domains);
            auto key = extractSessionKey(cookies);
            if (key.has_value()) {
                sessionKey = *key;
                break;
            }
        }
    }

    if (sessionKey.isEmpty()) {
        result.success = false;
        result.errorMessage = "No Claude session key found in browser cookies.";
        return result;
    }

    QString orgId = fetchOrgId(sessionKey, ctx.networkTimeoutMs);
    if (orgId.isEmpty()) {
        result.success = false;
        result.errorMessage = "No Claude organization found.";
        return result;
    }

    ClaudeUsageSnapshot snap = fetchUsageData(orgId, sessionKey, ctx.networkTimeoutMs);
    if (!snap.isValid()) {
        result.success = false;
        result.errorMessage = "No usage data from Claude web API.";
        return result;
    }

    auto overage = fetchOverageCost(orgId, sessionKey, ctx.networkTimeoutMs);
    if (overage.has_value()) {
        snap.extraUsage = ClaudeExtraUsage{
            true,
            overage->limit * 100.0,
            overage->used * 100.0,
            std::nullopt,
            overage->currencyCode
        };
    }

    auto email = fetchAccountEmail(sessionKey, ctx.networkTimeoutMs);
    if (email.has_value()) snap.accountEmail = email;

    result.usage = snap.toUsageSnapshot();
    result.success = true;
    return result;
}
