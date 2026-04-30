#include "CursorProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/CookieImporter.h"
#include "../../models/CursorUsageSummary.h"

#include <QNetworkCookie>

const QStringList CursorWebStrategy::s_knownCookieNames = {
    "WorkosCursorSessionToken",
    "next-auth.session-token",
    "wos-session",
    "authjs.session-token",
    "__Secure-WorkosCursorSessionToken",
    "__Secure-next-auth.session-token",
    "__Secure-wos-session",
    "__Secure-authjs.session-token"
};

CursorProvider::CursorProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> CursorProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new CursorWebStrategy() };
}

CursorWebStrategy::CursorWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool CursorWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        return true;
    }
    for (auto browser : CookieImporter::importOrder()) {
        if (CookieImporter::isBrowserInstalled(browser)) return true;
    }
    return false;
}

bool CursorWebStrategy::shouldFallback(const ProviderFetchResult& result,
                                        const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

QVector<QNetworkCookie> CursorWebStrategy::importSessionCookies() {
    QStringList domains = {"cursor.com"};
    for (auto browser : CookieImporter::importOrder()) {
        if (!CookieImporter::isBrowserInstalled(browser)) continue;
        QVector<QNetworkCookie> cookies = CookieImporter::importCookies(browser, domains);
        if (!cookies.isEmpty()) return cookies;
    }
    return {};
}

std::optional<QString> CursorWebStrategy::findSessionCookie(const QVector<QNetworkCookie>& cookies) {
    for (const auto& name : s_knownCookieNames) {
        for (const auto& cookie : cookies) {
            if (cookie.name() == name.toUtf8()) {
                QString value = QString::fromUtf8(cookie.value()).trimmed();
                if (!value.isEmpty()) return value;
            }
        }
    }

    for (const auto& cookie : cookies) {
        QString name = QString::fromUtf8(cookie.name());
        QString domain = cookie.domain();
        if (domain.contains("cursor.com") && !cookie.value().isEmpty()) {
            return QString::fromUtf8(cookie.value());
        }
    }

    return std::nullopt;
}

QString CursorWebStrategy::buildCookieHeader(const QVector<QNetworkCookie>& cookies) {
    QStringList parts;
    for (const auto& cookie : cookies) {
        parts.append(QString::fromUtf8(cookie.name()) + "=" + QString::fromUtf8(cookie.value()));
    }
    return parts.join("; ");
}

ProviderFetchResult CursorWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QVector<QNetworkCookie> allCookies;
    QString cookieHeader;

    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        cookieHeader = *ctx.manualCookieHeader;
    } else {
        allCookies = importSessionCookies();
        if (allCookies.isEmpty()) {
            result.success = false;
            result.errorMessage = "No Cursor session cookies found in browser.";
            return result;
        }
        auto sessionValue = findSessionCookie(allCookies);
        if (!sessionValue.has_value()) {
            result.success = false;
            result.errorMessage = "No Cursor session cookie found.";
            return result;
        }
        cookieHeader = buildCookieHeader(allCookies);
    }

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "application/json";

    QJsonObject usageJson = NetworkManager::instance().getJsonSync(
        QUrl("https://cursor.com/api/usage-summary"), headers, ctx.networkTimeoutMs);

    if (usageJson.isEmpty()) {
        result.success = false;
        result.errorMessage = "empty response from Cursor usage API";
        return result;
    }

    CursorUsageSummary summary = parseCursorUsageSummary(usageJson);

    QJsonObject meJson = NetworkManager::instance().getJsonSync(
        QUrl("https://cursor.com/api/auth/me"), headers, ctx.networkTimeoutMs);
    CursorUserInfo userInfo;
    if (!meJson.isEmpty()) {
        userInfo = parseCursorUserInfo(meJson);
    }

    QJsonObject subJson = NetworkManager::instance().getJsonSync(
        QUrl("https://cursor.com/api/stripe/subscription"), headers, ctx.networkTimeoutMs);
    CursorSubscriptionInfo subInfo;
    if (!subJson.isEmpty()) {
        subInfo = parseCursorSubscription(subJson);
    }

    CursorUsageSnapshot snap = buildCursorSnapshot(summary, userInfo, subInfo);

    if (!snap.planPercentUsed.has_value() && !snap.apiPercentUsed.has_value()) {
        result.success = false;
        result.errorMessage = "no usage data in Cursor response";
        return result;
    }

    result.usage = snap.toUsageSnapshot();
    result.success = true;
    return result;
}
