#include <QtTest/QtTest>
#include "../src/providers/augment/AugmentProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_AugmentProvider : public QObject {
    Q_OBJECT
private slots:
    void parseCreditsResponse() {
        QJsonObject json;
        json["usageUnitsRemaining"] = 375;
        json["usageUnitsConsumedThisBillingCycle"] = 125;
        json["usageUnitsAvailable"] = 500;

        ProviderFetchResult r = AugmentWebStrategy::parseCreditsResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 25.0);
    }

    void parseCLIoutput() {
        // Test that the provider has both strategies
        AugmentProvider p;
        ProviderFetchContext ctx;
        auto strategies = p.createStrategies(ctx);
        QCOMPARE(strategies.size(), 2);
        QCOMPARE(strategies[0]->id(), QString("augment.cli"));
        QCOMPARE(strategies[1]->id(), QString("augment.web"));
        qDeleteAll(strategies);
    }
};

QTEST_MAIN(tst_AugmentProvider)
#include "tst_AugmentProvider.moc"
