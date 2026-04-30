#include "KimiProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/CookieImporter.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDateTime>

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

bool KimiWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return resolveAuthToken(ctx).has_value();
}

std::optional<QString> KimiWebStrategy::resolveAuthToken(const ProviderFetchContext& ctx) {
    // 1. Manual cookie header from settings
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        auto token = extractKimiAuthToken(*ctx.manualCookieHeader);
        if (token.has_value()) return token;
    }

    // 2. Environment variable
    for (const auto& key : {"KIMI_AUTH_TOKEN", "KIMI_MANUAL_COOKIE", "kimi_auth_token"}) {
        if (ctx.env.contains(key)) {
            auto token = extractKimiAuthToken(ctx.env[key]);
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

std::optional<QString> KimiWebStrategy::extractKimiAuthToken(const QString& raw) {
    QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) return std::nullopt;

    // Direct JWT token: eyJ... with 3 parts
    if (trimmed.startsWith("eyJ") && trimmed.split('.').count() == 3) {
        return trimmed;
    }

    // Cookie header patterns
    QRegularExpression re1(R"re((?i)kimi-auth=([A-Za-z0-9._\-+=/]+))re");
    QRegularExpressionMatch m1 = re1.match(trimmed);
    if (m1.hasMatch() && !m1.captured(1).isEmpty()) return m1.captured(1);

    QRegularExpression re2(R"re((?i)kimi-auth:\s*([A-Za-z0-9._\-+=/]+))re");
    QRegularExpressionMatch m2 = re2.match(trimmed);
    if (m2.hasMatch() && !m2.captured(1).isEmpty()) return m2.captured(1);

    return std::nullopt;
}

QJsonObject KimiWebStrategy::decodeJWTPayload(const QString& jwt) {
    QStringList parts = jwt.split('.');
    if (parts.size() != 3) return {};

    QString payload = parts[1];
    payload.replace('-', '+').replace('_', '/');
    while (payload.size() % 4 != 0) payload.append('=');

    QByteArray data = QByteArray::fromBase64(payload.toUtf8());
    if (data.isEmpty()) return {};

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return {};
    return doc.object();
}

ProviderFetchResult KimiWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    auto tokenOpt = resolveAuthToken(ctx);
    if (!tokenOpt.has_value()) {
        result.success = false;
        result.errorMessage = "No kimi-auth token found in browser cookies or manual input.";
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
    QJsonObject jwtPayload = decodeJWTPayload(token);
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

    QJsonObject json = NetworkManager::instance().postJsonSync(
        QUrl("https://www.kimi.com/apiv2/kimi.gateway.billing.v1.BillingService/GetUsages"),
        body, headers, ctx.networkTimeoutMs, false);

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "empty or invalid response from Kimi API";
        return result;
    }

    if (!json.contains("usages")) {
        result.success = false;
        result.errorMessage = "no usages field in Kimi response";
        return result;
    }

    result.usage = parseKimiResponse(json);
    result.success = result.usage.primary.has_value() || result.usage.identity.has_value();
    if (!result.success) {
        result.errorMessage = "no usage data in Kimi response";
    }
    return result;
}

static std::optional<double> parseKimiNumber(const QJsonObject& obj, const QString& key) {
    if (obj.contains(key)) {
        QJsonValue v = obj.value(key);
        if (v.isString()) {
            bool ok = false;
            double d = v.toString().toDouble(&ok);
            if (ok) return d;
        } else if (v.isDouble()) {
            return v.toDouble();
        }
    }
    return std::nullopt;
}

static QDateTime parseKimiResetTime(const QString& iso) {
    if (iso.isEmpty()) return {};
    // Kimi returns microsecond precision (e.g. 2026-05-07T02:43:45.585111Z)
    // Qt::ISODate handles up to milliseconds; strip extra digits.
    QString simplified = iso;
    int dotIdx = simplified.lastIndexOf('.');
    int zIdx = simplified.lastIndexOf('Z');
    if (dotIdx > 0 && zIdx > dotIdx) {
        QString ms = simplified.mid(dotIdx + 1, zIdx - dotIdx - 1);
        if (ms.length() > 3) {
            ms = ms.left(3);
            simplified = simplified.left(dotIdx + 1) + ms + "Z";
        }
    }
    QDateTime dt = QDateTime::fromString(simplified, Qt::ISODate);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(simplified, Qt::ISODateWithMs);
    }
    return dt;
}

static RateWindow makeKimiWindow(const QJsonObject& detail, int windowMinutes = 0) {
    RateWindow rw;
    auto limitOpt = parseKimiNumber(detail, "limit");
    auto usedOpt = parseKimiNumber(detail, "used");
    auto remainingOpt = parseKimiNumber(detail, "remaining");
    QString resetStr = detail.value("resetTime").toString();

    if (!limitOpt.has_value() || *limitOpt <= 0) return rw;

    double limit = *limitOpt;
    double used = 0;
    if (usedOpt.has_value()) {
        used = *usedOpt;
    } else if (remainingOpt.has_value()) {
        used = limit - *remainingOpt;
    }

    rw.usedPercent = std::clamp(used / limit * 100.0, 0.0, 100.0);
    if (windowMinutes > 0) rw.windowMinutes = windowMinutes;

    QDateTime resetDt = parseKimiResetTime(resetStr);
    if (resetDt.isValid()) rw.resetsAt = resetDt;

    return rw;
}

UsageSnapshot KimiWebStrategy::parseKimiResponse(const QJsonObject& json) {
    UsageSnapshot snapshot;
    snapshot.updatedAt = QDateTime::currentDateTime();

    QJsonArray usages = json.value("usages").toArray();
    QJsonObject codingUsage;
    for (const auto& val : usages) {
        QJsonObject usage = val.toObject();
        if (usage.value("scope").toString() == "FEATURE_CODING") {
            codingUsage = usage;
            break;
        }
    }

    if (codingUsage.isEmpty()) return snapshot;

    // limits[0] = rate limit window (e.g. 5 min) -> primary (session)
    QJsonArray limits = codingUsage.value("limits").toArray();
    if (!limits.isEmpty()) {
        QJsonObject firstLimit = limits[0].toObject();
        QJsonObject limitDetail = firstLimit.value("detail").toObject();
        QJsonObject window = firstLimit.value("window").toObject();
        int duration = window.value("duration").toInt(0);
        QString timeUnit = window.value("timeUnit").toString();
        int windowMinutes = duration;
        if (timeUnit == "TIME_UNIT_SECOND") windowMinutes = duration / 60;
        else if (timeUnit == "TIME_UNIT_HOUR") windowMinutes = duration * 60;
        else if (timeUnit == "TIME_UNIT_DAY") windowMinutes = duration * 24 * 60;
        // TIME_UNIT_MINUTE -> duration is already minutes

        RateWindow rw = makeKimiWindow(limitDetail, windowMinutes);
        if (rw.usedPercent > 0 || limitDetail.contains("limit")) {
            snapshot.primary = rw;
        }
    }

    // detail = overall quota -> secondary (weekly)
    QJsonObject detail = codingUsage.value("detail").toObject();
    {
        RateWindow rw = makeKimiWindow(detail, 10080); // 7 days in minutes
        if (rw.usedPercent > 0 || detail.contains("limit")) {
            snapshot.secondary = rw;
        }
    }

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::kimi;
    snapshot.identity = identity;

    return snapshot;
}
