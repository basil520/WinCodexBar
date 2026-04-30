#include "KiloProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>
#include <algorithm>
#include <cmath>

KiloProvider::KiloProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> KiloProvider::createStrategies(const ProviderFetchContext& ctx) {
    return { new KiloAPIStrategy(const_cast<KiloProvider*>(this)) };
}

KiloAPIStrategy::KiloAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

static QString resolveKiloApiKey(const ProviderFetchContext& ctx) {
    if (ctx.env.contains("KILO_API_KEY")) return ctx.env["KILO_API_KEY"].trimmed();
    auto cred = ProviderCredentialStore::read("com.codexbar.apikey.kilo");
    if (cred.has_value()) return QString::fromUtf8(cred.value());
    return {};
}

bool KiloAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveKiloApiKey(ctx).isEmpty();
}

static double findMoneyAmount(const QJsonObject& obj) {
    QStringList usdKeys = { "amount", "used", "remaining", "total" };
    for (auto& k : usdKeys) {
        if (obj.contains(k)) return obj.value(k).toDouble(0);
    }
    QStringList centsKeys = { "amountCents", "usedCents", "remainingCents", "bonusCents" };
    for (auto& k : centsKeys) {
        if (obj.contains(k)) return obj.value(k).toDouble(0) / 100.0;
    }
    QStringList usdMicroKeys = { "amount_mUsd", "used_mUsd", "remaining_mUsd", "bonus_mUsd",
                                  "currentPeriodUsageUsd", "currentPeriodBaseCreditsUsd",
                                  "currentPeriodBonusCreditsUsd" };
    for (auto& k : usdMicroKeys) {
        if (obj.contains(k)) return obj.value(k).toDouble(0) / 1000000.0;
    }
    return 0;
}

ProviderFetchResult KiloAPIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    QString apiKey = resolveKiloApiKey(ctx);
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

        QJsonObject input;
        input["0"] = QJsonObject{{"json",QJsonValue::Null}};
        input["1"] = QJsonObject{{"json",QJsonValue::Null}};
        input["2"] = QJsonObject{{"json",QJsonValue::Null}};

        QString inputStr = QString(QJsonDocument(input).toJson(QJsonDocument::Compact));
        QString url = QString("https://app.kilo.ai/api/trpc/user.getCreditBlocks,kiloPass.getState,user.getAutoTopUpPaymentMethod?batch=1&input=%1").arg(inputStr);

        QHash<QString,QString> headers;
        headers["Authorization"] = "Bearer " + apiKey;
        headers["Accept"] = "application/json";

        QJsonObject json = NetworkManager::instance().getJsonSync(QUrl(url), headers, ctx.networkTimeoutMs);
        if (json.isEmpty()) { result.errorMessage = "empty response"; return result; }

        QJsonArray results;
        for (int i = 0; i <= 2; i++) {
            auto v = json[QString::number(i)];
            if (!v.isUndefined()) results.append(v);
        }
        if (results.isEmpty()) {
            result.errorMessage = "no results in tRPC response"; return result;
        }

        QJsonObject creditData, passData;
        double creditTotal = 0, creditRemaining = 0;

        for (int i = 0; i < results.size(); i++) {
            auto entry = results[i].toObject();
            auto resData = entry["result"].toObject()["data"].toObject()["json"].toObject();

            if (resData.contains("creditBlocks")) {
                QJsonArray blocks = resData["creditBlocks"].toArray();
                for (int bi = 0; bi < blocks.size(); bi++) {
                    auto bo = blocks[bi].toObject();
                    creditTotal += bo["amount_mUsd"].toDouble(0);
                    creditRemaining += bo["balance_mUsd"].toDouble(0);
                }
                creditTotal /= 1000000.0;
                creditRemaining /= 1000000.0;
            } else if (resData.contains("subscription")) {
                passData = resData;
            }
        }

        double creditUsed = std::max(0.0, creditTotal - creditRemaining);
        double creditPct = creditTotal > 0 ? std::clamp(creditUsed / creditTotal * 100.0, 0.0, 100.0) : 0;

        UsageSnapshot snapshot;
        snapshot.updatedAt = QDateTime::currentDateTime();

        RateWindow primary;
        primary.usedPercent = creditPct;
        primary.resetDescription = QString("$%1 / $%2").arg(creditUsed, 0, 'f', 2).arg(creditTotal, 0, 'f', 2);
        snapshot.primary = primary;

        if (!passData.isEmpty()) {
            auto sub = passData["subscription"].toObject();
            double passUsed = sub["currentPeriodUsageUsd"].toDouble(0) / 1000000.0;
            double baseCredits = sub["currentPeriodBaseCreditsUsd"].toDouble(0) / 1000000.0;
            double bonusCredits = sub["currentPeriodBonusCreditsUsd"].toDouble(0) / 1000000.0;
            double passTotal = baseCredits + bonusCredits;
            double passPct = passTotal > 0 ? std::clamp(passUsed / passTotal * 100.0, 0.0, 100.0) : 0;

            RateWindow secondary;
            secondary.usedPercent = passPct;
            secondary.resetDescription = QString("$%1 / $%2").arg(passUsed, 0, 'f', 2).arg(passTotal, 0, 'f', 2);
            snapshot.secondary = secondary;
        }

        QString tier = passData["subscription"].toObject()["tier"].toString();
        QString plan = tier.isEmpty() ? "Kilo" : tier;
        ProviderIdentitySnapshot identity;
        identity.providerID = UsageProvider::kilo;
        identity.loginMethod = plan;
        snapshot.identity = identity;

        result.usage = snapshot;
        result.success = true;
        return result;
}
