#include <QtTest/QtTest>
#include "../src/providers/factory/FactoryProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_FactoryProvider : public QObject {
    Q_OBJECT
private slots:
    void parseUsageResponse() {
        QJsonObject json;
        QJsonObject usage;
        QJsonObject standard, premium;
        standard["usedRatio"] = 0.05;
        premium["usedRatio"] = 0.12;
        usage["standard"] = standard;
        usage["premium"] = premium;
        json["usage"] = usage;

        ProviderFetchResult r = FactoryWebStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 5.0);
        QVERIFY(r.usage.secondary.has_value());
        QCOMPARE(r.usage.secondary->usedPercent, 12.0);
    }

    void unlimitedDetection() {
        QJsonObject json;
        QJsonObject usage;
        QJsonObject standard;
        standard["totalAllowance"] = 10000000000001.0; // > 1e12
        standard["orgTotalTokensUsed"] = 50000000.0;
        usage["standard"] = standard;
        json["usage"] = usage;

        ProviderFetchResult r = FactoryWebStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary->usedPercent > 0);
        QVERIFY(r.usage.primary->usedPercent < 100);
    }
};

QTEST_MAIN(tst_FactoryProvider)
#include "tst_FactoryProvider.moc"
