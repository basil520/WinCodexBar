#include "MiniMaxProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"
#include "../../providers/shared/CookieImporter.h"
#include "../../providers/shared/BrowserDetection.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

MiniMaxProvider::MiniMaxProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> MiniMaxProvider::createStrategies(const ProviderFetchContext& ctx) {
    QString mode = ctx.settings.get("sourceMode", "auto").toString();
    if (mode == "api") return { new MiniMaxAPIStrategy(this) };
    if (mode == "web") return { new MiniMaxWebStrategy(this) };
    // auto: API first, then web
    return { new MiniMaxAPIStrategy(this), new MiniMaxWebStrategy(this) };
}

// ============================================================================
// API Strategy
// ============================================================================

MiniMaxAPIStrategy::MiniMaxAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString MiniMaxAPIStrategy::resolveApiKey(const ProviderFetchContext& ctx) {
    if (ctx.env.contains("MINIMAX_API_KEY")) return ctx.env["MINIMAX_API_KEY"];
    auto cred = ProviderCredentialStore::read("com.codexbar.apikey.minimax");
    if (cred.has_value()) return QString::fromUtf8(cred.value());
    return {};
}

QString MiniMaxAPIStrategy::resolveEndpoint(const ProviderFetchContext& ctx) {
    QString region = ctx.env.value("MINIMAX_API_REGION",
                        ctx.settings.get("apiRegion", "global").toString());
    if (region == "cn") {
        return "https://api.minimaxi.com";
    }
    return "https://api.minimax.io";
}

bool MiniMaxAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveApiKey(ctx).isEmpty();
}

bool MiniMaxAPIStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    if (!result.success) {
        QString lower = result.errorMessage.toLower();
        // Only fallback for invalid credentials (token expired) or HTTP 404
        if (lower.contains("invalid") || lower.contains("expired") || lower.contains("http 404")) return true;
        return false;
    }
    return false;
}

ProviderFetchResult MiniMaxAPIStrategy::fetchOnce(const QString& apiKey, const QString& baseUrl,
                                                   const ProviderFetchContext& ctx) {
    QString path = "/v1/api/openplatform/coding_plan/remains";
    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["MM-API-Source"] = "CodexBar";
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl(baseUrl + path), headers, ctx.networkTimeoutMs);
    return parseResponse(json);
}

ProviderFetchResult MiniMaxAPIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

    QString apiKey = resolveApiKey(ctx);
    if (apiKey.isEmpty()) {
        result.success = false;
        result.errorMessage = "MiniMax API key not configured.";
        return result;
    }

    QString baseUrl = resolveEndpoint(ctx);

    result = fetchOnce(apiKey, baseUrl, ctx);
    if (result.success) return result;

    // Region retry: if global rejected, try China API
    if (baseUrl.contains("api.minimax.io")) {
        QString cnUrl = "https://api.minimaxi.com";
        ProviderFetchResult retry = fetchOnce(apiKey, cnUrl, ctx);
        if (retry.success) return retry;
        // Preserve original error
        result.errorMessage = "MiniMax API key is invalid or expired. Please check your API region setting.";
    }

    return result;
}

ProviderFetchResult MiniMaxAPIStrategy::parseResponse(const QJsonObject& json) {
    ProviderFetchResult result;
    result.strategyID = "minimax.api";
    result.strategyKind = ProviderFetchKind::APIToken;
    result.sourceLabel = "api";

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty or invalid response from MiniMax API";
        return result;
    }

    QJsonObject baseResp = json["base_resp"].toObject();
    int statusCode = baseResp["status_code"].toInt(-1);
    if (statusCode != 0) {
        QString msg = baseResp["status_msg"].toString("API error");
        result.success = false;
        result.errorMessage = msg;
        // Detect invalid credentials
        QString lower = msg.toLower();
        if (statusCode == 1004 || lower.contains("cookie") || lower.contains("login") || lower.contains("log in")) {
            result.errorMessage = "MiniMax API credentials are invalid or expired.";
        }
        return result;
    }

    QJsonObject data = json["data"].toObject();
    QJsonArray modelRemains = data["model_remains"].toArray();

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::minimax;

    // Parse plan name from multiple candidates (upstream order)
    auto tryString = [&](const QString& key) -> QString {
        if (data.contains(key)) return data[key].toString().trimmed();
        return {};
    };
    QStringList plans = { tryString("current_subscribe_title"), tryString("plan_name"),
                          tryString("combo_title"), tryString("current_plan_title") };
    for (const auto& p : plans) {
        if (!p.isEmpty()) { identity.loginMethod = p; break; }
    }
    QJsonObject comboCard = data["current_combo_card"].toObject();
    if (!identity.loginMethod.has_value() && comboCard.contains("title"))
        identity.loginMethod = comboCard["title"].toString().trimmed();
    snap.identity = identity;

    if (!modelRemains.isEmpty()) {
        QJsonObject first = modelRemains[0].toObject();
        // Upstream: current_interval_usage_count represents REMAINING (not used)
        int total = first["current_interval_total_count"].toInt(0);
        int remaining = first["current_interval_usage_count"].toInt(0);
        int used = (total > 0) ? qMax(0, total - remaining) : 0;

        qint64 startTime = static_cast<qint64>(first["start_time"].toDouble());
        qint64 endTime = static_cast<qint64>(first["end_time"].toDouble());
        int remainsTime = first["remains_time"].toInt(0);

        RateWindow rw;
        rw.usedPercent = (total > 0) ? (static_cast<double>(used) / total * 100.0) : 0;
        rw.usedPercent = qBound(0.0, rw.usedPercent, 100.0);

        if (startTime > 0 && endTime > 0 && endTime > startTime) {
            rw.windowMinutes = static_cast<int>((endTime - startTime) / 60000);
            rw.resetsAt = QDateTime::fromMSecsSinceEpoch(endTime);
        } else if (remainsTime > 0) {
            // remainsTime in milliseconds
            int sec = remainsTime > 1000000 ? remainsTime / 1000 : remainsTime;
            rw.resetsAt = QDateTime::currentDateTime().addSecs(sec);
        }
        rw.resetDescription = QCoreApplication::translate("ProviderLabels", "%1 / %2 prompts").arg(used).arg(total);
        snap.primary = rw;
    }

    result.usage = snap;
    result.success = true;
    return result;
}

// ============================================================================
// Web (Cookie) Strategy
// ============================================================================

MiniMaxWebStrategy::MiniMaxWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString MiniMaxWebStrategy::resolveCookieHeader(const ProviderFetchContext& ctx) {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty())
        return *ctx.manualCookieHeader;

    QString manual = ctx.settings.get("manualCookieHeader").toString().trimmed();
    if (!manual.isEmpty()) return manual;

    // Env var: MINIMAX_COOKIE
    for (const auto& key : {"MINIMAX_COOKIE", "MINIMAX_COOKIE_HEADER"}) {
        if (ctx.env.contains(key) && !ctx.env[key].isEmpty())
            return ctx.env[key];
    }

    // Browser cookie import
    QStringList domains = {"platform.minimax.io", "platform.minimaxi.com", "minimax.io", "minimaxi.com"};
    auto browsers = BrowserDetection::installedBrowsers();
    for (auto browser : browsers) {
        auto cookies = CookieImporter::importCookies(browser, domains);
        if (cookies.isEmpty()) continue;
        QString header;
        for (const auto& cookie : cookies) {
            if (!header.isEmpty()) header += "; ";
            header += cookie.name() + "=" + cookie.value();
        }
        if (!header.isEmpty()) return header;
    }
    return {};
}

bool MiniMaxWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveCookieHeader(ctx).isEmpty();
}

bool MiniMaxWebStrategy::shouldFallback(const ProviderFetchResult&,
                                         const ProviderFetchContext&) const {
    return false; // web strategy is last resort
}

ProviderFetchResult MiniMaxWebStrategy::fetchWithCookie(const QString& cookieHeader,
                                                         const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = "minimax.web";
    result.strategyKind = ProviderFetchKind::Web;
    result.sourceLabel = "web";

    QString region = ctx.env.value("MINIMAX_API_REGION",
                        ctx.settings.get("apiRegion", "global").toString());
    QString baseUrl = (region == "cn") ? "https://platform.minimaxi.com" : "https://platform.minimax.io";
    QString path = "/user-center/payment/coding-plan?cycle_type=3";
    QUrl url(baseUrl + path);

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/143.0.0.0 Safari/537.36";
    headers["Origin"] = baseUrl;

    QString html = NetworkManager::instance().getStringSync(url, headers, ctx.networkTimeoutMs);

    if (html.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty response from MiniMax coding plan page";
        return result;
    }

    // Check signed out
    QString lower = html.toLower();
    if (lower.contains("sign in") || lower.contains("log in") || lower.contains("登录")) {
        result.success = false;
        result.errorMessage = "MiniMax session expired. Please re-import your cookie.";
        return result;
    }

    // Try __NEXT_DATA__ embedded JSON (most reliable)
    QRegularExpression nextDataRe(R"(id="__NEXT_DATA__"[^>]*>\s*(\{[^<]*\})\s*</script>)",
                                   QRegularExpression::DotMatchesEverythingOption);
    auto m = nextDataRe.match(html);
    if (m.hasMatch()) {
        QString jsonStr = m.captured(1);
        QJsonObject json = QJsonDocument::fromJson(jsonStr.toUtf8()).object();
        // Search for model_remains in nested structure
        std::function<QJsonObject(const QJsonValue&)> findRemains = [&](const QJsonValue& v) -> QJsonObject {
            if (v.isObject()) {
                QJsonObject o = v.toObject();
                if (o.contains("model_remains") || o.contains("modelRemains"))
                    return o;
                for (auto it = o.begin(); it != o.end(); ++it) {
                    QJsonObject found = findRemains(it.value());
                    if (!found.isEmpty()) return found;
                }
            } else if (v.isArray()) {
                for (const auto& item : v.toArray()) {
                    QJsonObject found = findRemains(item);
                    if (!found.isEmpty()) return found;
                }
            }
            return {};
        };

        QJsonObject found = findRemains(json);
        if (!found.isEmpty()) {
            // Normalize camelCase → snake_case
            if (found.contains("modelRemains") && !found.contains("model_remains"))
                found["model_remains"] = found["modelRemains"];
            QJsonObject wrapper;
            QJsonObject data; data["model_remains"] = found["model_remains"];
            wrapper["data"] = data;
            return MiniMaxAPIStrategy::parseResponse(wrapper);
        }
    }

    // Fallback: try to find usage data via regex
    // "XX% used"
    QRegularExpression usedRe(R"(([0-9]{1,3}(?:\.[0-9]+)?)\s*%\s*used)", QRegularExpression::CaseInsensitiveOption);
    auto usedMatch = usedRe.match(html);
    if (usedMatch.hasMatch()) {
        double pct = usedMatch.captured(1).toDouble();
        UsageSnapshot snap;
        snap.updatedAt = QDateTime::currentDateTime();
        ProviderIdentitySnapshot identity;
        identity.providerID = UsageProvider::minimax;
        snap.identity = identity;
        RateWindow rw;
        rw.usedPercent = qBound(0.0, pct, 100.0);
        snap.primary = rw;
        result.usage = snap;
        result.success = true;
        return result;
    }

    // "available usage: N prompts / X hours"
    QRegularExpression availRe(R"(available\s+usage[:\s]*([0-9][0-9,]*)\s*prompts?\s*/\s*([0-9]+(?:\.[0-9]+)?)\s*(hours?|hrs?|h|minutes?|mins?|m|days?|d))",
                                QRegularExpression::CaseInsensitiveOption);
    auto availMatch = availRe.match(html);
    if (availMatch.hasMatch()) {
        int prompts = availMatch.captured(1).replace(",", "").toInt();
        UsageSnapshot snap;
        snap.updatedAt = QDateTime::currentDateTime();
        ProviderIdentitySnapshot identity;
        identity.providerID = UsageProvider::minimax;
        snap.identity = identity;
        RateWindow rw;
        rw.usedPercent = 0;
        rw.resetDescription = QString("%1 prompts / %2").arg(prompts).arg(availMatch.captured(2) + " " + availMatch.captured(3));
        snap.primary = rw;
        result.usage = snap;
        result.success = true;
        return result;
    }

    result.success = false;
    result.errorMessage = "Could not parse MiniMax coding plan data from page";
    return result;
}

ProviderFetchResult MiniMaxWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QString cookieHeader = resolveCookieHeader(ctx);
    if (cookieHeader.isEmpty()) {
        result.success = false;
        result.errorMessage = "No MiniMax cookie available. Log in to platform.minimax.io in your browser.";
        return result;
    }

    return fetchWithCookie(cookieHeader, ctx);
}
