#include "OpenRouterProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QUrlQuery>
#include <algorithm>

OpenRouterProvider::OpenRouterProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> OpenRouterProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new OpenRouterAPIStrategy(const_cast<OpenRouterProvider*>(this)) };
}

OpenRouterAPIStrategy::OpenRouterAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

static QString resolveOpenRouterApiKey(const ProviderFetchContext& ctx) {
    if (ctx.env.contains("OPENROUTER_API_KEY")) {
        return ctx.env["OPENROUTER_API_KEY"].trimmed().remove('"').remove('\'');
    }
    auto cred = ProviderCredentialStore::read("com.codexbar.apikey.openrouter");
    if (cred.has_value()) return QString::fromUtf8(cred.value());
    return {};
}

bool OpenRouterAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveOpenRouterApiKey(ctx).isEmpty();
}

bool OpenRouterAPIStrategy::shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const {
    return false;
}

ProviderFetchResult OpenRouterAPIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    QString apiKey = resolveOpenRouterApiKey(ctx);
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["Accept"] = "application/json";

    QJsonObject credJson = NetworkManager::instance().getJsonSync(
        QUrl("https://openrouter.ai/api/v1/credits"), headers, ctx.networkTimeoutMs);

    if (credJson.isEmpty()) {
        result.success = false;
        result.errorMessage = "empty credits response";
        return result;
    }

    QJsonObject data = credJson.value("data").toObject();
    double totalCredits = data.value("total_credits").toDouble(0);
    double totalUsage = data.value("total_usage").toDouble(0);
    double balance = std::max(0.0, totalCredits - totalUsage);
    double usedPercent = totalCredits > 0 ? std::min(100.0, (totalUsage / totalCredits) * 100.0) : 0.0;

    OpenRouterUsageSnapshot orSnap;
    orSnap.totalCredits = totalCredits;
    orSnap.totalUsage = totalUsage;
    orSnap.balance = balance;
    orSnap.usedPercent = usedPercent;
    orSnap.updatedAt = QDateTime::currentDateTime();

    QJsonObject keyJson = NetworkManager::instance().getJsonSync(
        QUrl("https://openrouter.ai/api/v1/key"), headers, ctx.networkTimeoutMs);

    if (!keyJson.isEmpty()) {
        QJsonObject keyData = keyJson.value("data").toObject();
        double keyLimit = keyData.value("limit").toDouble(0);
        double keyUsage = keyData.value("usage").toDouble(-1);

        if (keyLimit > 0) orSnap.keyLimit = keyLimit;
        if (keyUsage >= 0) orSnap.keyUsage = keyUsage;

        QJsonObject rateLimitObj = keyData.value("rate_limit").toObject();
        if (!rateLimitObj.isEmpty()) {
            OpenRouterRateLimit rl;
            rl.requests = rateLimitObj.value("requests").toInt(0);
            rl.interval = rateLimitObj.value("interval").toString();
            orSnap.rateLimit = rl;
        }
    }

    result.usage = orSnap.toUsageSnapshot();
    result.success = true;
    return result;
}
