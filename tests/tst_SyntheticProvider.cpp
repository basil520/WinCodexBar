#include <QtTest/QtTest>
#include "../src/providers/synthetic/SyntheticProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_SyntheticProvider : public QObject {
    Q_OBJECT
private slots:
    void parseQuotaSlots() {
        QJsonObject root;
        QJsonObject rolling;
        rolling["percentUsed"] = 0.65;
        QJsonObject weekly;
        weekly["usedPercent"] = 42.0;
        root["rollingFiveHourLimit"] = rolling;
        root["weeklyTokenLimit"] = weekly;

        ProviderFetchResult r = SyntheticAPIStrategy::parseResponse(root);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 65.0);
        QVERIFY(r.usage.secondary.has_value());
        QCOMPARE(r.usage.secondary->usedPercent, 42.0);
    }

    void emptyResponse() {
        QJsonObject json;
        ProviderFetchResult r = SyntheticAPIStrategy::parseResponse(json);
        QVERIFY(!r.success);
    }
};

QTEST_MAIN(tst_SyntheticProvider)
#include "tst_SyntheticProvider.moc"
