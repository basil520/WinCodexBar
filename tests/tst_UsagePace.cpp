#include <QtTest>
#include "models/UsagePace.h"

class UsagePaceTest : public QObject {
    Q_OBJECT

private slots:
    void testDefaultStage() {
        UsagePace pace;
        QCOMPARE(pace.stage, UsagePace::Stage::onTrack);
        QCOMPARE(pace.deltaPercent, 0.0);
        QVERIFY(pace.willLastToReset);
    }

    void testAheadStage() {
        UsagePace pace;
        pace.stage = UsagePace::Stage::ahead;
        pace.actualUsedPercent = 60.0;
        pace.expectedUsedPercent = 40.0;
        pace.deltaPercent = 20.0;
        QCOMPARE(pace.stage, UsagePace::Stage::ahead);
        QCOMPARE(pace.deltaPercent, 20.0);
    }

    void testBehindStage() {
        UsagePace pace;
        pace.stage = UsagePace::Stage::behind;
        pace.actualUsedPercent = 80.0;
        pace.expectedUsedPercent = 40.0;
        pace.deltaPercent = 40.0;
        QCOMPARE(pace.stage, UsagePace::Stage::behind);
    }

    void testETA() {
        UsagePace pace;
        pace.etaSeconds = 3600;
        pace.willLastToReset = false;
        QCOMPARE(pace.etaSeconds.value(), 3600);
        QVERIFY(!pace.willLastToReset);
    }
};

QTEST_MAIN(UsagePaceTest)
#include "tst_UsagePace.moc"
