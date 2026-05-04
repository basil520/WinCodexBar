#include "MiniMaxProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MiniMaxProvider::MiniMaxProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> MiniMaxProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new MiniMaxAPIStrategy(this) };
}

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
        return "https://platform.minimaxi.com/v1/api/openplatform/coding_plan/remains";
    }
    return "https://platform.minimax.io/v1/api/openplatform/coding_plan/remains";
}

bool MiniMaxAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveApiKey(ctx).isEmpty();
}

bool MiniMaxAPIStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
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
        result.success = false;
        result.errorMessage = baseResp["status_msg"].toString("API error");
        return result;
    }

    QJsonObject data = json["data"].toObject();
    QJsonArray modelRemains = data["model_remains"].toArray();

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::minimax;
    identity.loginMethod = data["plan_name"].toString();
    snap.identity = identity;

    if (!modelRemains.isEmpty()) {
        QJsonObject first = modelRemains[0].toObject();
        int total = first["current_interval_total_count"].toInt(0);
        int used = first["current_interval_usage_count"].toInt(0);
        qint64 startTime = static_cast<qint64>(first["start_time"].toDouble());
        qint64 endTime = static_cast<qint64>(first["end_time"].toDouble());

        RateWindow rw;
        rw.usedPercent = (total > 0) ? (static_cast<double>(used) / total * 100.0) : 0;
        if (startTime > 0 && endTime > 0 && endTime > startTime) {
            rw.windowMinutes = static_cast<int>((endTime - startTime) / 60000);
            rw.resetsAt = QDateTime::fromMSecsSinceEpoch(endTime);
        }
        rw.resetDescription = QString("%1 / %2 prompts").arg(used).arg(total);
        snap.primary = rw;
    }

    result.usage = snap;
    result.success = true;
    return result;
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

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["MM-API-Source"] = "CodexBar";
    headers["Accept"] = "application/json";

    QString url = resolveEndpoint(ctx);
    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl(url), headers, ctx.networkTimeoutMs);

    return parseResponse(json);
}
