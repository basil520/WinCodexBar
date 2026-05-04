#include "AbacusProvider.h"
#include "../../network/NetworkManager.h"
#include "../shared/CookieImporter.h"
#include "../shared/BrowserDetection.h"

#include <QJsonDocument>
#include <QJsonObject>

AbacusProvider::AbacusProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> AbacusProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new AbacusWebStrategy(this) };
}

AbacusWebStrategy::AbacusWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString AbacusWebStrategy::resolveCookieHeader(const ProviderFetchContext& ctx) {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty())
        return *ctx.manualCookieHeader;

    QString manual = ctx.settings.get("manualCookieHeader").toString().trimmed();
    if (!manual.isEmpty()) return manual;

    QStringList domains = {"abacus.ai", "apps.abacus.ai"};
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

bool AbacusWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveCookieHeader(ctx).isEmpty();
}

bool AbacusWebStrategy::shouldFallback(const ProviderFetchResult& result,
                                        const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult AbacusWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QString cookieHeader = resolveCookieHeader(ctx);
    if (cookieHeader.isEmpty()) {
        result.success = false;
        result.errorMessage = "No Abacus AI cookie available. Log in to apps.abacus.ai in your browser.";
        return result;
    }

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "application/json";

    QJsonObject creditsJson = NetworkManager::instance().getJsonSync(
        QUrl("https://apps.abacus.ai/api/_getOrganizationComputePoints"),
        headers, ctx.networkTimeoutMs);

    if (creditsJson.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty response from Abacus AI API";
        return result;
    }

    bool success = creditsJson["success"].toBool(false);
    if (!success) {
        result.success = false;
        result.errorMessage = "Abacus AI API error";
        return result;
    }

    QJsonObject creditsResult = creditsJson["result"].toObject();
    double totalPoints = creditsResult["totalComputePoints"].toDouble(0);
    double pointsLeft = creditsResult["computePointsLeft"].toDouble(0);

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::abacus;
    snap.identity = identity;

    RateWindow rw;
    rw.usedPercent = (totalPoints > 0)
        ? qBound(0.0, ((totalPoints - pointsLeft) / totalPoints) * 100.0, 100.0)
        : 0;
    rw.resetDescription = QString("%1 / %2 compute points")
        .arg(pointsLeft, 0, 'f', 1)
        .arg(totalPoints, 0, 'f', 1);
    snap.primary = rw;

    // Try to get billing info for reset date
    QJsonObject billingBody;
    QJsonObject billingJson = NetworkManager::instance().postJsonSync(
        QUrl("https://apps.abacus.ai/api/_getBillingInfo"),
        billingBody, headers, ctx.networkTimeoutMs);

    if (billingJson["success"].toBool(false)) {
        QJsonObject billingResult = billingJson["result"].toObject();
        QString nextBilling = billingResult["nextBillingDate"].toString();
        if (!nextBilling.isEmpty()) {
            snap.primary->resetsAt = QDateTime::fromString(nextBilling, Qt::ISODate);
        }
        identity.loginMethod = billingResult["currentTier"].toString();
        snap.identity = identity;
    }

    result.usage = snap;
    result.success = true;
    return result;
}
