#include "KimiProvider.h"
#include "KimiAPIError.h"
#include "KimiTokenResolver.h"
#include "KimiModels.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/CookieImporter.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <utility>

KimiProvider::KimiProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> KimiProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new KimiWebStrategy() };
}

QVector<ProviderSettingsDescriptor> KimiProvider::settingsDescriptors() const {
    return {
        {"cookieSource", "Cookie source", "picker", "auto",
         {{"auto", "Automatic"}, {"manual", "Manual"}}, {}, {}, {}, "Browser or manual cookie", false, false},
        {"manualCookieHeader", "Manual cookie", "secret", QVariant(),
         {}, {}, "KIMI_AUTH_TOKEN", "kimi-auth=eyJ...", "Paste kimi-auth cookie or JWT token", false, true}
    };
}

KimiWebStrategy::KimiWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool KimiWebStrategy::shouldFallback(const ProviderFetchResult& result,
                                      const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    if (!result.success) {
        QString msg = result.errorMessage.toLower();
        if (msg.contains("missing") || msg.contains("invalid")) return false;
    }
    return true;
}

bool KimiWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return resolveAuthToken(ctx).has_value();
}

std::optional<QString> KimiWebStrategy::resolveAuthToken(const ProviderFetchContext& ctx) {
    // 1. Manual cookie header from settings
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        auto token = KimiTokenResolver::extractKimiAuthToken(*ctx.manualCookieHeader);
        if (token.has_value()) return token;
    }

    // 2. Environment variable
    for (const auto& key : {"KIMI_AUTH_TOKEN", "KIMI_MANUAL_COOKIE", "kimi_auth_token"}) {
        if (ctx.env.contains(key)) {
            auto token = KimiTokenResolver::extractKimiAuthToken(ctx.env[key]);
            if (token.has_value()) return token;
        }
    }

    // 3. Browser cookie import
    QStringList domains = {"kimi.com", "www.kimi.com"};
    for (auto browser : CookieImporter::importOrder()) {
        if (!CookieImporter::isBrowserInstalled(browser)) continue;
        QVector<QNetworkCookie> cookies = CookieImporter::importCookies(browser, domains);
        for (const auto& cookie : cookies) {
            if (cookie.name() == "kimi-auth") {
                QString value = QString::fromUtf8(cookie.value()).trimmed();
                if (!value.isEmpty()) return value;
            }
        }
    }

    return std::nullopt;
}

ProviderFetchResult KimiWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    auto tokenOpt = resolveAuthToken(ctx);
    if (!tokenOpt.has_value()) {
        result.success = false;
        result.errorMessage = kimiErrorMessage(KimiAPIError::MissingToken);
        return result;
    }

    QString token = *tokenOpt;

    QHash<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer " + token;
    headers["Cookie"] = "kimi-auth=" + token;
    headers["Origin"] = "https://www.kimi.com";
    headers["Referer"] = "https://www.kimi.com/code/console";
    headers["Accept"] = "*/*";
    headers["Accept-Language"] = "en-US,en;q=0.9";
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/143.0.0.0 Safari/537.36";
    headers["connect-protocol-version"] = "1";
    headers["x-language"] = "en-US";
    headers["x-msh-platform"] = "web";
    headers["r-timezone"] = QDateTime::currentDateTime().timeZoneAbbreviation();

    // Decode JWT for session headers
    QJsonObject jwtPayload = KimiTokenResolver::decodeJWTPayload(token);
    if (!jwtPayload.isEmpty()) {
        QString deviceId = jwtPayload.value("device_id").toString();
        QString sessionId = jwtPayload.value("ssid").toString();
        QString trafficId = jwtPayload.value("sub").toString();
        if (!deviceId.isEmpty()) headers["x-msh-device-id"] = deviceId;
        if (!sessionId.isEmpty()) headers["x-msh-session-id"] = sessionId;
        if (!trafficId.isEmpty()) headers["x-traffic-id"] = trafficId;
    }

    QJsonObject body;
    QJsonArray scopeArr;
    scopeArr.append("FEATURE_CODING");
    body["scope"] = scopeArr;

    auto [json, httpStatus] = NetworkManager::instance().postJsonSyncWithStatus(
        QUrl("https://www.kimi.com/apiv2/kimi.gateway.billing.v1.BillingService/GetUsages"),
        body, headers, ctx.networkTimeoutMs, false);

    if (httpStatus == 0) {
        result.success = false;
        result.errorMessage = kimiErrorMessage(KimiAPIError::NetworkError,
            "Kimi API unreachable or request timed out");
        return result;
    }

    if (httpStatus == 401 || httpStatus == 403) {
        result.success = false;
        result.errorMessage = kimiErrorMessage(KimiAPIError::InvalidToken,
            QStringLiteral("HTTP %1").arg(httpStatus));
        return result;
    }

    if (httpStatus >= 400) {
        result.success = false;
        QString detail = json.contains("message")
            ? json.value("message").toString()
            : QStringLiteral("HTTP %1").arg(httpStatus);
        result.errorMessage = kimiErrorMessage(KimiAPIError::APIError, detail);
        return result;
    }

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = kimiErrorMessage(KimiAPIError::ParseFailed,
            "empty or invalid JSON response");
        return result;
    }

    if (!json.contains("usages")) {
        result.success = false;
        result.errorMessage = kimiErrorMessage(KimiAPIError::ParseFailed,
            "missing 'usages' field in response");
        return result;
    }

    result.usage = parseKimiResponse(json);
    result.success = result.usage.primary.has_value() || result.usage.identity.has_value();
    if (!result.success) {
        result.errorMessage = kimiErrorMessage(KimiAPIError::ParseFailed,
            "no FEATURE_CODING usage data found");
        qDebug() << "[KimiProvider] Parsed usage has no primary data. Raw response:"
                 << QJsonDocument(json).toJson(QJsonDocument::Compact);
    }
    return result;
}

static RateWindow makeKimiWindow(const KimiUsageDetail& detail, int windowMinutes = 0, const QString& resetDesc = QString()) {
    RateWindow rw;

    if (!detail.limit.has_value() || *detail.limit <= 0) return rw;

    double limit = *detail.limit;
    double used = 0;
    if (detail.used.has_value()) {
        used = *detail.used;
    } else if (detail.remaining.has_value()) {
        used = limit - *detail.remaining;
    }

    rw.usedPercent = std::clamp(used / limit * 100.0, 0.0, 100.0);
    if (windowMinutes > 0) rw.windowMinutes = windowMinutes;

    QDateTime resetDt = parseKimiResetTime(detail.resetTime);
    if (resetDt.isValid()) rw.resetsAt = resetDt;

    if (!resetDesc.isEmpty()) {
        rw.resetDescription = resetDesc;
    }

    return rw;
}

UsageSnapshot KimiWebStrategy::parseKimiResponse(const QJsonObject& json) {
    UsageSnapshot snapshot;
    snapshot.updatedAt = QDateTime::currentDateTime();

    QJsonArray usages = json.value("usages").toArray();
    std::optional<KimiCodingUsage> codingUsage;
    for (const auto& val : usages) {
        auto maybe = KimiCodingUsage::fromJson(val.toObject());
        if (maybe.has_value()) {
            codingUsage = maybe;
            break;
        }
    }

    if (!codingUsage.has_value()) return snapshot;

    // Primary = overall weekly quota from detail
    {
        const auto& detail = codingUsage->detail;
        if (detail.limit.has_value() && *detail.limit > 0) {
            double limitVal = *detail.limit;
            double usedVal = detail.used.has_value() ? *detail.used
                : (detail.remaining.has_value() ? (limitVal - *detail.remaining) : 0);
            QString desc = QCoreApplication::translate("ProviderLabels", "%1/%2 requests").arg(usedVal, 0, 'f', 0).arg(limitVal, 0, 'f', 0);
            RateWindow rw = makeKimiWindow(detail, 0, desc);
            snapshot.primary = rw;
        }
    }

    // Secondary = rate limit (first limits entry, usually 5h window)
    // Upstream: uses limits.first?.detail, windowMinutes = 300
    if (!codingUsage->limits.isEmpty()) {
        const auto& rl = codingUsage->limits[0];
        const auto& detail = rl.detail;
        if (detail.limit.has_value() && *detail.limit > 0) {
            double limitVal = *detail.limit;
            double usedVal = detail.used.has_value() ? *detail.used
                : (detail.remaining.has_value() ? (limitVal - *detail.remaining) : 0);
            QString desc = QCoreApplication::translate("ProviderLabels", "Rate: %1/%2 per 5 hours")
                .arg(usedVal, 0, 'f', 0).arg(limitVal, 0, 'f', 0);
            RateWindow rw = makeKimiWindow(detail, 300, desc);
            snapshot.secondary = rw;
        }
    }

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::kimi;
    snapshot.identity = identity;

    return snapshot;
}
