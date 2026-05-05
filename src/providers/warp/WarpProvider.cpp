#include "WarpProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

WarpProvider::WarpProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> WarpProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new WarpAPIStrategy(this) };
}

WarpAPIStrategy::WarpAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString WarpAPIStrategy::resolveApiKey(const ProviderFetchContext& ctx) {
    if (ctx.env.contains("WARP_API_KEY")) return ctx.env["WARP_API_KEY"];
    if (ctx.env.contains("WARP_TOKEN")) return ctx.env["WARP_TOKEN"];
    auto cred = ProviderCredentialStore::read("com.codexbar.apikey.warp");
    if (cred.has_value()) return QString::fromUtf8(cred.value());
    return {};
}

bool WarpAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveApiKey(ctx).isEmpty();
}

bool WarpAPIStrategy::shouldFallback(const ProviderFetchResult& result,
                                      const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult WarpAPIStrategy::parseResponse(const QJsonObject& json) {
    ProviderFetchResult result;
    result.strategyID = "warp.api";
    result.strategyKind = ProviderFetchKind::APIToken;
    result.sourceLabel = "api";

    QJsonObject data = json["data"].toObject();
    QJsonObject user1 = data["user"].toObject();
    QJsonObject user = user1["user"].toObject();

    QJsonObject limitInfo = user["requestLimitInfo"].toObject();
    bool isUnlimited = limitInfo["isUnlimited"].toBool(false);
    int requestLimit = limitInfo["requestLimit"].toInt(0);
    int requestsUsed = limitInfo["requestsUsedSinceLastRefresh"].toInt(0);
    qint64 nextRefresh = static_cast<qint64>(limitInfo["nextRefreshTime"].toDouble());

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::warp;
    snap.identity = identity;

    RateWindow primary;
    if (isUnlimited) {
        primary.usedPercent = 0;
        primary.resetDescription = QCoreApplication::translate("ProviderLabels", "Unlimited");
    } else if (requestLimit > 0) {
        primary.usedPercent = static_cast<double>(requestsUsed) / requestLimit * 100.0;
        primary.resetDescription = QCoreApplication::translate("ProviderLabels", "%1 / %2 requests").arg(requestsUsed).arg(requestLimit);
    }
    if (nextRefresh > 0) primary.resetsAt = QDateTime::fromMSecsSinceEpoch(nextRefresh);
    snap.primary = primary;

    // Bonus grants
    QJsonArray bonusGrants = user["bonusGrants"].toArray();
    double totalBonusRemaining = 0;
    double totalBonusGranted = 0;
    for (const auto& g : bonusGrants) {
        QJsonObject grant = g.toObject();
        totalBonusRemaining += grant["requestCreditsRemaining"].toDouble(0);
        totalBonusGranted += grant["requestCreditsGranted"].toDouble(0);
    }

    if (totalBonusGranted > 0) {
        RateWindow secondary;
        secondary.usedPercent = ((totalBonusGranted - totalBonusRemaining) / totalBonusGranted) * 100.0;
        secondary.resetDescription = QCoreApplication::translate("ProviderLabels", "%1 bonus credits").arg(totalBonusRemaining, 0, 'f', 0);
        snap.secondary = secondary;
    }

    result.usage = snap;
    result.success = true;
    return result;
}

static const char* GraphQLQuery = R"(
query GetRequestLimitInfo($requestContext: RequestContext!) {
  user(requestContext: $requestContext) {
    ... on UserOutput {
      user {
        requestLimitInfo { isUnlimited nextRefreshTime requestLimit requestsUsedSinceLastRefresh }
        bonusGrants { requestCreditsGranted requestCreditsRemaining expiration }
      }
    }
  }
}
)";

ProviderFetchResult WarpAPIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

    QString apiKey = resolveApiKey(ctx);
    if (apiKey.isEmpty()) {
        result.success = false;
        result.errorMessage = "Warp API key not configured.";
        return result;
    }

    QJsonObject variables;
    QJsonObject requestContext;
    variables["requestContext"] = requestContext;

    QJsonObject body;
    body["query"] = QString::fromUtf8(GraphQLQuery);
    body["variables"] = variables;

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["x-warp-client-id"] = "warp-app";
    headers["User-Agent"] = "Warp/1.0";
    headers["Content-Type"] = "application/json";

    QJsonObject json = NetworkManager::instance().postJsonSync(
        QUrl("https://app.warp.dev/graphql/v2?op=GetRequestLimitInfo"),
        body, headers, ctx.networkTimeoutMs);

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty response from Warp GraphQL API";
        return result;
    }

    if (json.contains("errors")) {
        result.success = false;
        QJsonArray errors = json["errors"].toArray();
        if (!errors.isEmpty()) {
            result.errorMessage = errors[0].toObject()["message"].toString();
        }
        if (result.errorMessage.isEmpty()) result.errorMessage = "GraphQL error";
        return result;
    }

    return parseResponse(json);
}
