#include <QtTest>
#include "models/RateWindow.h"

class RateWindowTest : public QObject {
    Q_OBJECT

private slots:
    void testDefaultConstruction() {
        RateWindow rw;
        QCOMPARE(rw.usedPercent, 0.0);
        QVERIFY(!rw.windowMinutes.has_value());
        QVERIFY(!rw.resetsAt.has_value());
        QVERIFY(rw.isValid());
    }

    void testRemainingPercent() {
        RateWindow rw;
        rw.usedPercent = 25.0;
        QCOMPARE(rw.remainingPercent(), 75.0);
    }

    void testRemainingPercentNeverNegative() {
        RateWindow rw;
        rw.usedPercent = 150.0;
        QCOMPARE(rw.remainingPercent(), 0.0);
    }

    void testFullConstruction() {
        RateWindow rw;
        rw.usedPercent = 50.0;
        rw.windowMinutes = 300;
        rw.resetDescription = "in 2h 30m";
        rw.nextRegenPercent = 25.0;
        rw.resetsAt = QDateTime::currentDateTime().addSecs(9000);

        QCOMPARE(rw.usedPercent, 50.0);
        QCOMPARE(rw.remainingPercent(), 50.0);
        QCOMPARE(rw.windowMinutes.value(), 300);
        QCOMPARE(rw.resetDescription.value(), "in 2h 30m");
        QCOMPARE(rw.nextRegenPercent.value(), 25.0);
        QVERIFY(rw.isValid());
    }

    void testNamedRateWindow() {
        NamedRateWindow nw;
        nw.id = "code-review";
        nw.title = "Code Review";
        nw.window.usedPercent = 30.0;
        QCOMPARE(nw.id, "code-review");
        QCOMPARE(nw.title, "Code Review");
        QCOMPARE(nw.window.usedPercent, 30.0);
    }
};

QTEST_MAIN(RateWindowTest)
#include "tst_RateWindow.moc"
