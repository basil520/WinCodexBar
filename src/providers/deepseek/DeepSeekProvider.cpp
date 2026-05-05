#include "DeepSeekProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

DeepSeekProvider::DeepSeekProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> DeepSeekProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new DeepSeekAPIStrategy(this) };
}

DeepSeekAPIStrategy::DeepSeekAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString DeepSeekAPIStrategy::resolveApiKey(const ProviderFetchContext& ctx) {
    if (ctx.env.contains("DEEPSEEK_API_KEY")) return ctx.env["DEEPSEEK_API_KEY"];
    if (ctx.env.contains("DEEPSEEK_KEY")) return ctx.env["DEEPSEEK_KEY"];
    if (ctx.accountCredentials.api.has_value() && ctx.accountCredentials.api->isValid()) {
        return ctx.accountCredentials.api->apiKey.toString().trimmed();
    }
    auto cred = ProviderCredentialStore::read("com.codexbar.apikey.deepseek");
    if (cred.has_value()) return QString::fromUtf8(cred.value());
    return {};
}

bool DeepSeekAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveApiKey(ctx).isEmpty();
}

bool DeepSeekAPIStrategy::shouldFallback(const ProviderFetchResult& result,
                                          const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult DeepSeekAPIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

    QString apiKey = resolveApiKey(ctx);
    if (apiKey.isEmpty()) {
        result.success = false;
        result.errorMessage = "DeepSeek API key not configured.";
        return result;
    }

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://api.deepseek.com/user/balance"), headers, ctx.networkTimeoutMs);

    return parseResponse(json);
}

ProviderFetchResult DeepSeekAPIStrategy::parseResponse(const QJsonObject& json) {
    ProviderFetchResult result;
    result.strategyID = "deepseek.api";
    result.strategyKind = ProviderFetchKind::APIToken;
    result.sourceLabel = "api";

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty or invalid response from DeepSeek API";
        return result;
    }

    bool isAvailable = json["is_available"].toBool(false);
    QJsonArray infos = json["balance_infos"].toArray();

    QString currency = "USD";
    double total = 0, granted = 0, toppedUp = 0;

    for (const auto& v : infos) {
        QJsonObject info = v.toObject();
        if (info["currency"].toString() == "USD") {
            currency = "USD";
            total = info["total_balance"].toString().toDouble();
            granted = info["granted_balance"].toString().toDouble();
            toppedUp = info["topped_up_balance"].toString().toDouble();
            break;
        }
    }

    if (total == 0 && !infos.isEmpty()) {
        QJsonObject info = infos[0].toObject();
        currency = info["currency"].toString();
        total = info["total_balance"].toString().toDouble();
        granted = info["granted_balance"].toString().toDouble();
        toppedUp = info["topped_up_balance"].toString().toDouble();
    }

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::deepseek;
    snap.identity = identity;

    QString symbol = (currency == "CNY") ? "\xC2\xA5" : "$";
    RateWindow rw;
    if (total <= 0 || !isAvailable) {
        rw.usedPercent = 100;
        rw.resetDescription = !isAvailable
            ? QCoreApplication::translate("ProviderLabels", "Balance unavailable for API calls")
            : QCoreApplication::translate("ProviderLabels", "%1%2 — add credits at platform.deepseek.com")
                .arg(symbol).arg(total, 0, 'f', 2);
    } else {
        rw.usedPercent = 0;
        rw.resetDescription = QCoreApplication::translate("ProviderLabels", "%1%2 (Paid: %1%3 / Granted: %1%4)")
            .arg(symbol)
            .arg(total, 0, 'f', 2)
            .arg(toppedUp, 0, 'f', 2)
            .arg(granted, 0, 'f', 2);
    }
    snap.primary = rw;
    result.usage = snap;
    result.success = true;

    return result;
}
