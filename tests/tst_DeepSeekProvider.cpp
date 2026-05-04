#include <QtTest/QtTest>
#include "../src/providers/deepseek/DeepSeekProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_DeepSeekProvider : public QObject {
    Q_OBJECT
private slots:
    void parseUSDbalance() {
        QJsonObject json;
        json["is_available"] = true;
        QJsonArray infos;
        QJsonObject info;
        info["currency"] = "USD"; info["total_balance"] = "49.50";
        info["granted_balance"] = "10.00"; info["topped_up_balance"] = "39.50";
        infos.append(info);
        json["balance_infos"] = infos;

        ProviderFetchResult r = DeepSeekAPIStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 0.0);
        QVERIFY(r.usage.primary->resetDescription.has_value());
        QVERIFY(r.usage.primary->resetDescription->contains("Paid"));
    }

    void parseCNYbalance() {
        QJsonObject json;
        json["is_available"] = true;
        QJsonArray infos;
        QJsonObject info;
        info["currency"] = "CNY"; info["total_balance"] = "100.00";
        info["granted_balance"] = "20.00"; info["topped_up_balance"] = "80.00";
        infos.append(info);
        json["balance_infos"] = infos;

        ProviderFetchResult r = DeepSeekAPIStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary->resetDescription->contains("\xC2\xA5"));
    }

    void unavailableAccount() {
        QJsonObject json;
        json["is_available"] = false;
        QJsonArray infos;
        QJsonObject info;
        info["currency"] = "USD"; info["total_balance"] = "50.00";
        info["granted_balance"] = "10.00"; info["topped_up_balance"] = "40.00";
        infos.append(info);
        json["balance_infos"] = infos;

        ProviderFetchResult r = DeepSeekAPIStrategy::parseResponse(json);
        QVERIFY(r.success);
        QCOMPARE(r.usage.primary->usedPercent, 100.0);
        QVERIFY(r.usage.primary->resetDescription->contains("unavailable"));
    }

    void zeroBalance() {
        QJsonObject json;
        json["is_available"] = true;
        QJsonArray infos;
        json["balance_infos"] = infos;

        ProviderFetchResult r = DeepSeekAPIStrategy::parseResponse(json);
        QVERIFY(r.success);
        QCOMPARE(r.usage.primary->usedPercent, 100.0);
        QVERIFY(r.usage.primary->resetDescription->contains("0.00"));
    }

    void emptyResponse() {
        QJsonObject json;
        ProviderFetchResult r = DeepSeekAPIStrategy::parseResponse(json);
        QVERIFY(!r.success);
    }
};

QTEST_MAIN(tst_DeepSeekProvider)
#include "tst_DeepSeekProvider.moc"
