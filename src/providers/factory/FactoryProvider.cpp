#include "FactoryProvider.h"
#include "../../network/NetworkManager.h"
#include "../shared/CookieImporter.h"
#include "../shared/BrowserDetection.h"

#include <QJsonDocument>
#include <QJsonObject>

FactoryProvider::FactoryProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> FactoryProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new FactoryWebStrategy(this) };
}

FactoryWebStrategy::FactoryWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString FactoryWebStrategy::resolveCookieHeader(const ProviderFetchContext& ctx) {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty())
        return *ctx.manualCookieHeader;

    QString manual = ctx.settings.get("manualCookieHeader").toString().trimmed();
    if (!manual.isEmpty()) return manual;

    QStringList domains = {"app.factory.ai", "factory.ai"};
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

bool FactoryWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveCookieHeader(ctx).isEmpty();
}

bool FactoryWebStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult FactoryWebStrategy::parseResponse(const QJsonObject& json) {
    ProviderFetchResult result;
    result.strategyID = "factory.web";
    result.strategyKind = ProviderFetchKind::Web;
    result.sourceLabel = "web";

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty response from Factory API";
        return result;
    }

    QJsonObject usage = json["usage"].toObject();
    QJsonObject standard = usage["standard"].toObject();
    QJsonObject premium = usage["premium"].toObject();

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::factory;
    snap.identity = identity;

    auto parseTier = [](const QJsonObject& tier) -> RateWindow {
        RateWindow rw;
        double usedRatio = tier["usedRatio"].toDouble(-1);
        if (usedRatio >= 0) {
            rw.usedPercent = usedRatio * 100.0;
        } else {
            double totalAllowance = tier["totalAllowance"].toDouble(0);
            double used = tier["orgTotalTokensUsed"].toDouble(0);
            if (totalAllowance > 0) {
                if (totalAllowance > 1e12) {
                    rw.usedPercent = (used / 1e8);
                } else {
                    rw.usedPercent = (used / totalAllowance) * 100.0;
                }
            }
        }
        rw.usedPercent = qBound(0.0, rw.usedPercent, 100.0);
        return rw;
    };

    snap.primary = parseTier(standard);
    snap.primary->resetDescription = "Standard";

    snap.secondary = parseTier(premium);
    snap.secondary->resetDescription = "Premium";

    qint64 startDate = static_cast<qint64>(usage["startDate"].toDouble());
    qint64 endDate = static_cast<qint64>(usage["endDate"].toDouble());
    if (endDate > 0) {
        snap.primary->resetsAt = QDateTime::fromMSecsSinceEpoch(endDate);
        snap.secondary->resetsAt = QDateTime::fromMSecsSinceEpoch(endDate);
    }

    result.usage = snap;
    result.success = true;
    return result;
}

ProviderFetchResult FactoryWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QString cookieHeader = resolveCookieHeader(ctx);
    if (cookieHeader.isEmpty()) {
        result.success = false;
        result.errorMessage = "No Factory cookie available. Log in to app.factory.ai in your browser.";
        return result;
    }

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "application/json";
    headers["x-factory-client"] = "web-app";
    headers["Origin"] = "https://app.factory.ai";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://app.factory.ai/api/organization/subscription/usage"),
        headers, ctx.networkTimeoutMs);

    return parseResponse(json);
}
