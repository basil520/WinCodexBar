#include <QtTest>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <algorithm>

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

    void testOpenCodeGoScan() {
        CostUsageScanner scanner;
        auto result = scanner.scanOpenCodeDB(QDate::currentDate().addDays(-29), QDate::currentDate());
        
        QFile logFile(QDir::tempPath() + "/opencode_go_scan.log");
        logFile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream log(&logFile);
        log << "OpenCode DB scan result:\n";
        log << "  providers: " << result.size() << "\n";
        log << "  opencodego tokens: " << result.value("opencodego").last30DaysTokens << "\n";
        log << "  kimi tokens: " << result.value("kimi").last30DaysTokens << "\n";
        
        auto dumpSnapshot = [&](const QString& name, const CostUsageSnapshot& snap) {
            log << "\n" << name << ":\n";
            log << "  last30DaysTokens: " << snap.last30DaysTokens << "\n";
            log << "  last30DaysCostUSD: " << snap.last30DaysCostUSD << "\n";
            log << "  daily count: " << snap.daily.size() << "\n";
            log << "  error: " << snap.errorMessage << "\n";
            for (auto& d : snap.daily) {
                log << "  day: " << d.date << " tokens: " << d.totalTokens() << " models: " << d.models.size() << "\n";
                for (auto& m : d.models) {
                    log << "    model: " << m.modelName << " tokens: " << m.totalTokens() << "\n";
                }
            }
        };
        
        for (auto it = result.constBegin(); it != result.constEnd(); ++it) {
            dumpSnapshot(it.key(), it.value());
        }
        logFile.close();

        QVERIFY(!result.contains("opencode-go"));
        QVERIFY(!result.contains("kimi-for-coding"));
    }

    void testOpenCodeGoPricingOnlyContainsSupportedModelFamilies() {
        auto keys = CostUsageScanner::opencodeGoPricingMap().keys();
        std::sort(keys.begin(), keys.end());

        const QStringList expected = {
            "glm-5",
            "glm-5.1",
            "k2p6",
            "kimi-k2.5",
            "kimi-k2.6",
            "minimax",
        };
        QCOMPARE(keys, expected);
    }
};

QTEST_MAIN(CostUsageScannerTest)
#include "tst_CostUsageScanner.moc"
