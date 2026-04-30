#include <QtTest>
#include <QDir>

#include "util/CostUsageScanner.h"

class CostUsageScannerTest : public QObject {
    Q_OBJECT

private slots:
    void testCodexIsoTimestampTokenUsageIsCounted() {
        CostUsageScanner scanner;
        const auto snapshot = scanner.scanCodex(QStringLiteral(TEST_DATA_DIR) + "/cost_usage/codex",
                                                QDate(2026, 4, 1),
                                                QDate(2026, 4, 30));

        QCOMPARE(snapshot.daily.size(), 1);
        QCOMPARE(snapshot.last30DaysTokens, 1830);
        QCOMPARE(snapshot.daily.first().date, QString("2026-04-29"));
        QCOMPARE(snapshot.daily.first().inputTokens, 1500);
        QCOMPARE(snapshot.daily.first().cacheReadTokens, 250);
        QCOMPARE(snapshot.daily.first().outputTokens, 80);
    }
};

QTEST_MAIN(CostUsageScannerTest)
#include "tst_CostUsageScanner.moc"
