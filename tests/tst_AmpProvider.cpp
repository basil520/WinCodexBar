#include <QtTest/QtTest>
#include "../src/providers/amp/AmpProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_AmpProvider : public QObject {
    Q_OBJECT
private slots:
    void parseFreeTier() {
        QString html = R"(<script>window.freeTierUsage = {quota: 100, used: 25, hourlyReplenishment: 10, windowHours: 24};</script>)";
        ProviderFetchResult r = AmpWebStrategy::parseHTML(html);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 25.0);
    }

    void parseSignedOut() {
        QString html = R"(<html><body><a href="/auth/sign-in">Sign in</a></body></html>)";
        ProviderFetchResult r = AmpWebStrategy::parseHTML(html);
        QVERIFY(!r.success);
    }
};

QTEST_MAIN(tst_AmpProvider)
#include "tst_AmpProvider.moc"
