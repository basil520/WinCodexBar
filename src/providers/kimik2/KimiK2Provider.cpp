#include "KimiK2Provider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"
#include <QJsonObject>
#include <algorithm>

KimiK2Provider::KimiK2Provider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> KimiK2Provider::createStrategies(const ProviderFetchContext& ctx) {
    return { new KimiK2APIStrategy(const_cast<KimiK2Provider*>(this)) };
}

KimiK2APIStrategy::KimiK2APIStrategy(QObject* parent) : IFetchStrategy(parent) {}

static QString resolveKimiK2ApiKey(const ProviderFetchContext& ctx) {
    for (auto& key : {"KIMI_K2_API_KEY", "KIMI_API_KEY", "KIMI_KEY"}) {
        if (ctx.env.contains(key)) return ctx.env[key].trimmed();
    }
    auto cred = ProviderCredentialStore::read("com.codexbar.apikey.kimik2");
    if (cred.has_value()) return QString::fromUtf8(cred.value());
    return {};
}

bool KimiK2APIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveKimiK2ApiKey(ctx).isEmpty();
}

static double findDouble(const QJsonObject& obj, const QStringList& keys) {
    for (auto& k : keys) {
        if (obj.contains(k)) return obj.value(k).toDouble(0);
    }
    return 0;
}

ProviderFetchResult KimiK2APIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    QString apiKey = resolveKimiK2ApiKey(ctx);
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://kimi-k2.ai/api/user/credits"), headers, ctx.networkTimeoutMs);

    if (json.isEmpty()) { result.errorMessage = "empty response"; return result; }

    auto data = json.value("data").toObject();
    auto usage = data.value("usage").toObject();
    auto creds = data.value("credits").toObject();
    QJsonObject contexts[] = { json, data, usage, creds,
        json.value("result").toObject(),
        json.value("result").toObject().value("usage").toObject() };

    double consumed = 0, remaining = 0;
    QStringList consumedKeys = { "total_credits_consumed","totalCreditsConsumed","total_credits_used",
        "totalCreditsUsed","credits_consumed","creditsConsumed","consumedCredits","total","consumed" };
    QStringList remainingKeys = { "credits_remaining","creditsRemaining","remaining_credits",
        "remainingCredits","available_credits","availableCredits","credits_left","creditsLeft","remaining" };

    for (auto& c : contexts) {
        if (consumed == 0) consumed = findDouble(c, consumedKeys);
        if (remaining == 0) remaining = findDouble(c, remainingKeys);
    }

    double total = consumed + remaining;
    double usedPercent = total > 0 ? std::clamp(consumed / total * 100.0, 0.0, 100.0) : 0;

    UsageSnapshot snapshot;
    snapshot.updatedAt = QDateTime::currentDateTime();
    RateWindow rw;
    rw.usedPercent = usedPercent;
    rw.resetDescription = QString("Credits: %1/%2").arg(consumed, 0, 'f', 0).arg(total, 0, 'f', 0);
    snapshot.primary = rw;

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::kimik2;
    snapshot.identity = identity;

    result.usage = snapshot;
    result.success = true;
    return result;
}
