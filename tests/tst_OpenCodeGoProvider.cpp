#include <QtTest/QtTest>
#include "../src/providers/opencode/OpenCodeGoProvider.h"
#include "../src/models/UsageSnapshot.h"
#include "../src/models/RateWindow.h"

class tst_OpenCodeGoProvider : public QObject {
    Q_OBJECT
private slots:
    void parseStandardJSON() {
        QString json = R"({"rollingUsage":{"usagePercent":42.5,"resetInSec":18000},"weeklyUsage":{"usagePercent":65.0,"resetInSec":604800}})";
        auto snap = OpenCodeGoWebStrategy::parseUsage(json);
        QVERIFY(snap.primary.has_value());
        QVERIFY(snap.secondary.has_value());
        QVERIFY(snap.primary->windowMinutes.has_value());
        QCOMPARE(snap.primary->windowMinutes.value(), 300);
        QVERIFY(snap.secondary->windowMinutes.has_value());
        QCOMPARE(snap.secondary->windowMinutes.value(), 10080);
    }

    void parseNestedJSON() {
        QString json = R"({"data":{"result":{"usage":{"rollingUsage":{"usagePercent":23},"weeklyUsage":{"usagePercent":78}}}}})";
        auto snap = OpenCodeGoWebStrategy::parseUsage(json);
        QVERIFY(snap.primary.has_value());
        QVERIFY(snap.secondary.has_value());
    }

    void parseDeepRecursionJSON() {
        QString json = R"({"a":{"b":{"c":{"rollingUsage":{"usagePercent":12},"weeklyUsage":{"usagePercent":34}}}}})";
        auto snap = OpenCodeGoWebStrategy::parseUsage(json);
        QVERIFY(snap.primary.has_value());
        QVERIFY(snap.secondary.has_value());
    }

    void parseRegexFallback() {
        QString text = R"({"rollingUsage":{"usagePercent":55,"resetInSec":14400},"weeklyUsage":{"usagePercent":88,"resetInSec":432000}})";
        auto snap = OpenCodeGoWebStrategy::parseUsage(text);
        QVERIFY(snap.primary.has_value());
        QVERIFY(snap.secondary.has_value());
    }

    void normalizePercentSmallValues() {
        QString json = R"({"rollingUsage":{"usagePercent":0.45},"weeklyUsage":{"usagePercent":0.12}})";
        auto snap = OpenCodeGoWebStrategy::parseUsage(json);
        QVERIFY(snap.primary.has_value());
        QCOMPARE(snap.primary->usedPercent, 45.0);
        QVERIFY(snap.secondary.has_value());
        QCOMPARE(snap.secondary->usedPercent, 12.0);
    }

    void missingResetHasNoWindowMinutes() {
        QString json = R"({"rollingUsage":{"usagePercent":30},"weeklyUsage":{"usagePercent":50}})";
        auto snap = OpenCodeGoWebStrategy::parseUsage(json);
        QVERIFY(snap.primary.has_value());
        QVERIFY(!snap.primary->windowMinutes.has_value());
        QVERIFY(!snap.primary->resetsAt.has_value());
    }

    void parseMonthlyWindow() {
        QString json = R"({"rollingUsage":{"usagePercent":10},"weeklyUsage":{"usagePercent":20},"monthlyUsage":{"usagePercent":30}})";
        auto snap = OpenCodeGoWebStrategy::parseUsage(json);
        QVERIFY(snap.tertiary.has_value());
        QCOMPARE(snap.tertiary->usedPercent, 30.0);
    }

    void identitySet() {
        QString json = R"({"rollingUsage":{"usagePercent":10},"weeklyUsage":{"usagePercent":20}})";
        auto snap = OpenCodeGoWebStrategy::parseUsage(json);
        QVERIFY(snap.identity.has_value());
        QCOMPARE(snap.identity->providerID.value(), UsageProvider::opencodego);
    }
};

QTEST_MAIN(tst_OpenCodeGoProvider)
#include "tst_OpenCodeGoProvider.moc"
