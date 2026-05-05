#include "CodebuffProvider.h"

#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QLocale>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QtMath>
#include <algorithm>

namespace {

QString cleaned(const QString& raw)
{
    QString value = raw.trimmed();
    if (value.size() >= 2) {
        const QChar first = value.front();
        const QChar last = value.back();
        if ((first == QLatin1Char('"') && last == QLatin1Char('"')) ||
            (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
            value = value.mid(1, value.size() - 2).trimmed();
        }
    }
    return value;
}

std::optional<double> doubleFromJson(const QJsonValue& value)
{
    double raw = 0.0;
    if (value.isDouble()) {
        raw = value.toDouble();
    } else if (value.isString()) {
        bool ok = false;
        raw = value.toString().trimmed().toDouble(&ok);
        if (!ok) return std::nullopt;
    } else {
        return std::nullopt;
    }
    return std::isfinite(raw) ? std::optional<double>(raw) : std::nullopt;
}

std::optional<QString> stringFromJson(const QJsonValue& value)
{
    QString raw;
    if (value.isString()) {
        raw = value.toString();
    } else if (value.isDouble()) {
        raw = QLocale::c().toString(value.toDouble());
    } else {
        return std::nullopt;
    }
    raw = raw.trimmed();
    if (raw.isEmpty()) return std::nullopt;
    return raw;
}

QDateTime dateFromNumeric(double value)
{
    if (!std::isfinite(value)) return {};
    if (value > 10000000000.0) {
        return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(value), Qt::UTC);
    }
    return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(value), Qt::UTC);
}

std::optional<QDateTime> dateFromJson(const QJsonValue& value)
{
    if (auto number = doubleFromJson(value); number.has_value()) {
        QDateTime parsed = dateFromNumeric(*number);
        return parsed.isValid() ? std::optional<QDateTime>(parsed) : std::nullopt;
    }
    if (!value.isString()) return std::nullopt;

    const QString raw = value.toString().trimmed();
    if (raw.isEmpty()) return std::nullopt;

    QDateTime parsed = QDateTime::fromString(raw, Qt::ISODateWithMs);
    if (!parsed.isValid()) parsed = QDateTime::fromString(raw, Qt::ISODate);
    if (parsed.isValid()) {
        if (parsed.timeSpec() == Qt::LocalTime) parsed.setTimeSpec(Qt::UTC);
        return parsed;
    }

    bool ok = false;
    const double numeric = raw.toDouble(&ok);
    if (ok) {
        parsed = dateFromNumeric(numeric);
        if (parsed.isValid()) return parsed;
    }
    return std::nullopt;
}

QString compactNumber(double value)
{
    QLocale locale(QLocale::English, QLocale::UnitedStates);
    const int decimals = qAbs(value) >= 1000.0 ? 0 : 1;
    return locale.toString(value, 'f', decimals);
}

CodebuffUsagePayload parseUsagePayload(const QJsonObject& json)
{
    CodebuffUsagePayload payload;
    payload.used = doubleFromJson(json.value("usage"));
    if (!payload.used.has_value()) payload.used = doubleFromJson(json.value("used"));
    payload.total = doubleFromJson(json.value("quota"));
    if (!payload.total.has_value()) payload.total = doubleFromJson(json.value("limit"));
    payload.remaining = doubleFromJson(json.value("remainingBalance"));
    if (!payload.remaining.has_value()) payload.remaining = doubleFromJson(json.value("remaining"));
    payload.nextQuotaReset = dateFromJson(json.value("next_quota_reset"));
    if (json.value("autoTopupEnabled").isBool()) {
        payload.autoTopUpEnabled = json.value("autoTopupEnabled").toBool();
    } else if (json.value("auto_topup_enabled").isBool()) {
        payload.autoTopUpEnabled = json.value("auto_topup_enabled").toBool();
    }
    return payload;
}

CodebuffSubscriptionPayload parseSubscriptionPayload(const QJsonObject& json)
{
    CodebuffSubscriptionPayload payload;
    const QJsonObject subscription = json.value("subscription").toObject();
    const QJsonObject rateLimit = json.value("rateLimit").toObject();

    payload.tier = stringFromJson(subscription.value("displayName"));
    if (!payload.tier.has_value()) payload.tier = stringFromJson(json.value("displayName"));
    if (!payload.tier.has_value()) payload.tier = stringFromJson(subscription.value("tier"));
    if (!payload.tier.has_value()) payload.tier = stringFromJson(json.value("tier"));
    if (!payload.tier.has_value()) payload.tier = stringFromJson(subscription.value("scheduledTier"));
    payload.status = stringFromJson(subscription.value("status"));
    payload.email = stringFromJson(json.value("email"));
    if (!payload.email.has_value()) {
        payload.email = stringFromJson(json.value("user").toObject().value("email"));
    }
    payload.billingPeriodEnd = dateFromJson(subscription.value("billingPeriodEnd"));
    if (!payload.billingPeriodEnd.has_value()) {
        payload.billingPeriodEnd = dateFromJson(subscription.value("currentPeriodEnd"));
    }
    payload.weeklyUsed = doubleFromJson(rateLimit.value("weeklyUsed"));
    if (!payload.weeklyUsed.has_value()) payload.weeklyUsed = doubleFromJson(rateLimit.value("used"));
    payload.weeklyLimit = doubleFromJson(rateLimit.value("weeklyLimit"));
    if (!payload.weeklyLimit.has_value()) payload.weeklyLimit = doubleFromJson(rateLimit.value("limit"));
    payload.weeklyResetsAt = dateFromJson(rateLimit.value("weeklyResetsAt"));
    return payload;
}

UsageSnapshot makeSnapshot(const CodebuffUsagePayload& usage,
                           const std::optional<CodebuffSubscriptionPayload>& subscription)
{
    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::codebuff;
    if (subscription.has_value() && subscription->email.has_value()) {
        identity.accountEmail = subscription->email;
    }

    const std::optional<double> total = usage.total.has_value()
        ? std::optional<double>(std::max(0.0, *usage.total))
        : (usage.used.has_value() && usage.remaining.has_value()
               ? std::optional<double>(std::max(0.0, *usage.used + *usage.remaining))
               : std::nullopt);
    const double used = usage.used.has_value()
        ? std::max(0.0, *usage.used)
        : (total.has_value() && usage.remaining.has_value()
               ? std::max(0.0, *total - *usage.remaining)
               : 0.0);

    if (total.has_value() && *total > 0.0) {
        RateWindow primary;
        primary.usedPercent = qBound(0.0, (used / *total) * 100.0, 100.0);
        primary.resetsAt = usage.nextQuotaReset;
        if (usage.remaining.has_value()) {
            primary.resetDescription = QStringLiteral("%1 / %2 credits remaining")
                .arg(compactNumber(*usage.remaining), compactNumber(*total));
        }
        snap.primary = primary;
    } else if (usage.remaining.has_value() || usage.used.has_value()) {
        RateWindow primary;
        primary.usedPercent = 100.0;
        if (usage.remaining.has_value()) {
            primary.resetDescription = QStringLiteral("%1 credits remaining").arg(compactNumber(*usage.remaining));
        }
        snap.primary = primary;
    }

    if (subscription.has_value() && subscription->weeklyLimit.has_value() && *subscription->weeklyLimit > 0.0) {
        const double weeklyUsed = std::max(0.0, subscription->weeklyUsed.value_or(0.0));
        RateWindow weekly;
        weekly.usedPercent = qBound(0.0, (weeklyUsed / *subscription->weeklyLimit) * 100.0, 100.0);
        weekly.windowMinutes = 7 * 24 * 60;
        weekly.resetsAt = subscription->weeklyResetsAt;
        weekly.resetDescription = QStringLiteral("%1 / %2 weekly credits")
            .arg(compactNumber(weeklyUsed), compactNumber(*subscription->weeklyLimit));
        snap.secondary = weekly;
    }

    QStringList loginParts;
    if (subscription.has_value() && subscription->tier.has_value()) {
        loginParts.append(*subscription->tier);
    }
    if (usage.remaining.has_value()) {
        loginParts.append(QStringLiteral("%1 remaining").arg(compactNumber(*usage.remaining)));
    }
    if (usage.autoTopUpEnabled.value_or(false)) {
        loginParts.append(QStringLiteral("auto top-up"));
    }
    if (!loginParts.isEmpty()) {
        identity.loginMethod = loginParts.join(QStringLiteral(" | "));
    }
    snap.identity = identity;
    return snap;
}

QString envValue(const ProviderFetchContext& ctx, const QString& key)
{
    if (ctx.env.contains(key)) return cleaned(ctx.env.value(key));
    return cleaned(QProcessEnvironment::systemEnvironment().value(key));
}

QString defaultHomePath()
{
    const QString home = QDir::homePath();
    if (!home.isEmpty()) return home;
    const QString profile = QProcessEnvironment::systemEnvironment().value(QStringLiteral("USERPROFILE"));
    if (!profile.trimmed().isEmpty()) return profile.trimmed();
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

} // namespace

CodebuffProvider::CodebuffProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> CodebuffProvider::createStrategies(const ProviderFetchContext& ctx)
{
    Q_UNUSED(ctx)
    return { new CodebuffAPIStrategy(this) };
}

CodebuffAPIStrategy::CodebuffAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool CodebuffAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const
{
    return resolveToken(ctx).has_value();
}

bool CodebuffAPIStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const
{
    Q_UNUSED(result)
    Q_UNUSED(ctx)
    return false;
}

ProviderFetchResult CodebuffAPIStrategy::fetchSync(const ProviderFetchContext& ctx)
{
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = QStringLiteral("api");

    const auto tokenResolution = resolveToken(ctx);
    if (!tokenResolution.has_value()) {
        return errorResult(QStringLiteral("Codebuff API token not configured. Set CODEBUFF_API_KEY or run codebuff login."));
    }

    QHash<QString, QString> headers;
    headers["Authorization"] = QStringLiteral("Bearer ") + tokenResolution->token;
    headers["Accept"] = QStringLiteral("application/json");
    headers["Content-Type"] = QStringLiteral("application/json");

    QJsonObject body;
    body["fingerprintId"] = QStringLiteral("codexbar-usage");
    const QUrl base = apiBaseUrl(ctx);
    auto [usageJson, status] = NetworkManager::instance().postJsonSyncWithStatus(
        base.resolved(QUrl(QStringLiteral("/api/v1/usage"))),
        body,
        headers,
        ctx.networkTimeoutMs);

    if (status == 401 || status == 403) {
        return errorResult(QStringLiteral("Unauthorized. Please sign in to Codebuff again."));
    }
    if (status == 404) {
        return errorResult(QStringLiteral("Codebuff usage endpoint not found."));
    }
    if (status >= 500) {
        return errorResult(QStringLiteral("Codebuff API is temporarily unavailable (status %1).").arg(status));
    }
    if (status != 200) {
        return errorResult(status == 0
            ? QStringLiteral("Codebuff API request failed or timed out.")
            : QStringLiteral("Codebuff API returned an unexpected status (%1).").arg(status));
    }
    if (usageJson.isEmpty()) {
        return errorResult(QStringLiteral("Could not parse Codebuff usage response."));
    }

    std::optional<CodebuffSubscriptionPayload> subscription;
    if (tokenResolution->source == TokenSource::AuthFile) {
        QHash<QString, QString> subscriptionHeaders;
        subscriptionHeaders["Authorization"] = QStringLiteral("Bearer ") + tokenResolution->token;
        subscriptionHeaders["Accept"] = QStringLiteral("application/json");
        const int subscriptionTimeoutMs = qMin(ctx.networkTimeoutMs, 2000);
        const QJsonObject subscriptionJson = NetworkManager::instance().getJsonSync(
            base.resolved(QUrl(QStringLiteral("/api/user/subscription"))),
            subscriptionHeaders,
            subscriptionTimeoutMs);
        if (!subscriptionJson.isEmpty()) {
            subscription = parseSubscriptionPayload(subscriptionJson);
        }
    }

    result.usage = makeSnapshot(parseUsagePayload(usageJson), subscription);
    result.success = true;
    return result;
}

CodebuffUsagePayload CodebuffAPIStrategy::parseUsagePayloadForTesting(const QJsonObject& json)
{
    return parseUsagePayload(json);
}

CodebuffSubscriptionPayload CodebuffAPIStrategy::parseSubscriptionPayloadForTesting(const QJsonObject& json)
{
    return parseSubscriptionPayload(json);
}

UsageSnapshot CodebuffAPIStrategy::makeSnapshotForTesting(
    const CodebuffUsagePayload& usage,
    const std::optional<CodebuffSubscriptionPayload>& subscription)
{
    return makeSnapshot(usage, subscription);
}

QString CodebuffAPIStrategy::authFileTokenForTesting(const QString& homePath)
{
    return authFileToken(homePath);
}

std::optional<CodebuffAPIStrategy::TokenResolution> CodebuffAPIStrategy::resolveToken(
    const ProviderFetchContext& ctx)
{
    const QString envToken = envValue(ctx, QStringLiteral("CODEBUFF_API_KEY"));
    if (!envToken.isEmpty()) return TokenResolution{envToken, TokenSource::Environment};

    if (ctx.accountCredentials.api.has_value() && ctx.accountCredentials.api->isValid()) {
        const QString token = cleaned(ctx.accountCredentials.api->apiKey.toString());
        if (!token.isEmpty()) return TokenResolution{token, TokenSource::TokenAccount};
    }

    const QString settingsToken = cleaned(ctx.settings.get(QStringLiteral("apiKey")).toString());
    if (!settingsToken.isEmpty()) return TokenResolution{settingsToken, TokenSource::Settings};

    auto stored = ProviderCredentialStore::read(QStringLiteral("com.codexbar.apikey.codebuff"));
    if (stored.has_value()) {
        const QString token = cleaned(QString::fromUtf8(stored.value()));
        if (!token.isEmpty()) return TokenResolution{token, TokenSource::CredentialStore};
    }

    const QString fileToken = authFileToken(defaultHomePath());
    if (!fileToken.isEmpty()) return TokenResolution{fileToken, TokenSource::AuthFile};

    return std::nullopt;
}

QString CodebuffAPIStrategy::authFileToken(const QString& homePath)
{
    if (homePath.trimmed().isEmpty()) return {};
    QFile file(QDir(homePath).filePath(QStringLiteral(".config/manicode/credentials.json")));
    if (!file.open(QIODevice::ReadOnly)) return {};

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return {};

    const QJsonObject root = doc.object();
    const QString profileToken = cleaned(root.value("default").toObject().value("authToken").toString());
    if (!profileToken.isEmpty()) return profileToken;
    return cleaned(root.value("authToken").toString());
}

QUrl CodebuffAPIStrategy::apiBaseUrl(const ProviderFetchContext& ctx)
{
    const QString overrideUrl = envValue(ctx, QStringLiteral("CODEBUFF_API_URL"));
    if (!overrideUrl.isEmpty()) {
        QUrl url(overrideUrl);
        if (url.isValid() && !url.host().isEmpty()) return url;
    }
    return QUrl(QStringLiteral("https://www.codebuff.com"));
}

ProviderFetchResult CodebuffAPIStrategy::errorResult(const QString& message)
{
    ProviderFetchResult result;
    result.strategyID = QStringLiteral("codebuff.api");
    result.strategyKind = ProviderFetchKind::APIToken;
    result.sourceLabel = QStringLiteral("api");
    result.success = false;
    result.errorMessage = message;
    return result;
}
