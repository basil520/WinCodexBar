#include "ZaiProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"
#include "../../models/UsagePace.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

ZaiProvider::ZaiProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> ZaiProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new ZaiAPIStrategy(const_cast<ZaiProvider*>(this)) };
}

ZaiAPIStrategy::ZaiAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

static QString resolveApiKey(const ProviderFetchContext& ctx) {
    if (ctx.env.contains("Z_AI_API_KEY")) {
        return ctx.env["Z_AI_API_KEY"];
    }
    if (ctx.accountCredentials.api.has_value() && ctx.accountCredentials.api->isValid()) {
        return ctx.accountCredentials.api->apiKey.toString().trimmed();
    }
    auto cred = ProviderCredentialStore::read("com.codexbar.apikey.zai");
    if (cred.has_value()) {
        return QString::fromUtf8(cred.value());
    }
    return {};
}

static QString resolveApiHost(const ProviderFetchContext& ctx) {
    if (ctx.env.contains("Z_AI_QUOTA_URL")) {
        return ctx.env["Z_AI_QUOTA_URL"];
    }
    QString host = ctx.env.value("Z_AI_API_HOST", QString());
    if (!host.isEmpty()) {
        return host + "/api/monitor/usage/quota/limit";
    }
    QString region = ctx.env.value("ZAI_API_REGION", "global");
    if (region == "bigmodelCN") {
        return "https://open.bigmodel.cn/api/monitor/usage/quota/limit";
    }
    return "https://api.z.ai/api/monitor/usage/quota/limit";
}

bool ZaiAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveApiKey(ctx).isEmpty();
}

bool ZaiAPIStrategy::shouldFallback(const ProviderFetchResult& result,
                                     const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

static ZaiLimitEntry parseZaiLimitEntry(const QJsonObject& limit) {
    ZaiLimitEntry entry;

    QString typeStr = limit.value("type").toString();
    if (typeStr == "TOKENS_LIMIT") entry.type = ZaiLimitType::TokensLimit;
    else if (typeStr == "TIME_LIMIT") entry.type = ZaiLimitType::TimeLimit;
    else entry.type = ZaiLimitType::Unknown;

    int unitVal = limit.value("unit").toInt(0);
    switch (unitVal) {
    case 1: entry.unit = ZaiLimitUnit::Days; break;
    case 3: entry.unit = ZaiLimitUnit::Hours; break;
    case 5: entry.unit = ZaiLimitUnit::Minutes; break;
    case 6: entry.unit = ZaiLimitUnit::Weeks; break;
    default: entry.unit = ZaiLimitUnit::Unknown; break;
    }

    entry.number = limit.value("number").toInt(0);
    entry.percentage = limit.value("percentage").toDouble(0);

    if (limit.contains("usage")) entry.usage = limit.value("usage").toInt(0);
    if (limit.contains("currentValue")) entry.currentValue = limit.value("currentValue").toInt(0);
    if (limit.contains("remaining")) entry.remaining = limit.value("remaining").toInt(0);

    if (limit.contains("usageDetails")) {
        QJsonArray details = limit.value("usageDetails").toArray();
        for (const auto& d : details) {
            QJsonObject obj = d.toObject();
            ZaiUsageDetail detail;
            detail.modelCode = obj.value("modelCode").toString();
            detail.usage = obj.value("usage").toInt(0);
            entry.usageDetails.append(detail);
        }
    }

    if (limit.contains("nextResetTime")) {
        qint64 epochMs = static_cast<qint64>(limit.value("nextResetTime").toDouble());
        if (epochMs > 0) entry.nextResetTime = QDateTime::fromMSecsSinceEpoch(epochMs);
    }

    return entry;
}

static UsageSnapshot parseZaiResponse(const QJsonObject& json) {
    int code = json.value("code").toInt(-1);
    bool success = json.value("success").toBool(false);
    if (code != 200 || !success) {
        return UsageSnapshot{};
    }

    QJsonObject data = json.value("data").toObject();
    QJsonArray limits = data.value("limits").toArray();
    if (limits.isEmpty()) {
        return UsageSnapshot{};
    }

    ZaiUsageSnapshot zaiSnap;
    zaiSnap.updatedAt = QDateTime::currentDateTime();

    QVector<ZaiLimitEntry> tokenLimits;
    std::optional<ZaiLimitEntry> timeLimitEntry;

    for (const auto& limitVal : limits) {
        ZaiLimitEntry entry = parseZaiLimitEntry(limitVal.toObject());
        if (entry.type == ZaiLimitType::TokensLimit) {
            tokenLimits.append(entry);
        } else if (entry.type == ZaiLimitType::TimeLimit) {
            timeLimitEntry = entry;
        }
    }

    std::sort(tokenLimits.begin(), tokenLimits.end(),
              [](const ZaiLimitEntry& a, const ZaiLimitEntry& b) {
                  return a.windowMinutes().value_or(0) < b.windowMinutes().value_or(0);
              });

    if (!tokenLimits.isEmpty()) {
        zaiSnap.tokenLimit = tokenLimits.last();
        if (tokenLimits.size() >= 2) {
            zaiSnap.sessionTokenLimit = tokenLimits.first();
        }
    }
    zaiSnap.timeLimit = timeLimitEntry;

    if (data.contains("planName")) zaiSnap.planName = data.value("planName").toString();
    else if (data.contains("plan")) zaiSnap.planName = data.value("plan").toString();
    else if (data.contains("plan_type")) zaiSnap.planName = data.value("plan_type").toString();
    else if (data.contains("packageName")) zaiSnap.planName = data.value("packageName").toString();

    return zaiSnap.toUsageSnapshot();
}

ProviderFetchResult ZaiAPIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    QString apiKey = resolveApiKey(ctx);
    QString url = resolveApiHost(ctx);

    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().getJsonSync(QUrl(url), headers, ctx.networkTimeoutMs);
    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "empty or invalid response";
        return result;
    }

    int code = json.value("code").toInt(-1);
    if (code != 200) {
        result.success = false;
        result.errorMessage = json.value("msg").toString("API error");
        return result;
    }

    result.usage = parseZaiResponse(json);
    result.success = true;
    return result;
}
