#include <QtTest/QtTest>
#include "../src/models/CostUsageReport.h"

class PerProviderCostUsageTest : public QObject {
    Q_OBJECT

private slots:
    void testProviderCostUsageSnapshotConstruction() {
        ProviderCostUsageSnapshot pcs;
        pcs.providerId = "codex";
        pcs.snapshot.sessionTokens = 1000;
        pcs.snapshot.sessionCostUSD = 0.5;
        pcs.snapshot.last30DaysTokens = 50000;
        pcs.snapshot.last30DaysCostUSD = 25.0;

        CostUsageModelBreakdown model;
        model.modelName = "gpt-5.2-codex";
        model.inputTokens = 30000;
        model.outputTokens = 20000;
        model.costUSD = 20.0;
        pcs.modelSummary.append(model);

        QCOMPARE(pcs.providerId, "codex");
        QCOMPARE(pcs.snapshot.sessionTokens, 1000);
        QCOMPARE(pcs.modelSummary.size(), 1);
        QCOMPARE(pcs.modelSummary[0].modelName, "gpt-5.2-codex");
        QCOMPARE(pcs.modelSummary[0].totalTokens(), 50000);
    }

    void testModelBreakdownAggregation() {
        CostUsageModelBreakdown m1;
        m1.modelName = "gpt-5.2-codex";
        m1.inputTokens = 10000;
        m1.outputTokens = 5000;
        m1.costUSD = 5.0;

        CostUsageModelBreakdown m2;
        m2.modelName = "gpt-5.2-codex";
        m2.inputTokens = 20000;
        m2.outputTokens = 10000;
        m2.costUSD = 15.0;

        QHash<QString, CostUsageModelBreakdown> modelMap;
        for (auto& m : {m1, m2}) {
            auto& agg = modelMap[m.modelName];
            agg.modelName = m.modelName;
            agg.inputTokens += m.inputTokens;
            agg.outputTokens += m.outputTokens;
            agg.costUSD += m.costUSD;
        }

        QCOMPARE(modelMap.size(), 1);
        QCOMPARE(modelMap["gpt-5.2-codex"].inputTokens, 30000);
        QCOMPARE(modelMap["gpt-5.2-codex"].totalTokens(), 45000);
        QCOMPARE(modelMap["gpt-5.2-codex"].costUSD, 20.0);
    }

    void testProviderCostUsageSnapshotSorting() {
        ProviderCostUsageSnapshot p1;
        p1.providerId = "codex";
        p1.snapshot.last30DaysCostUSD = 10.0;

        ProviderCostUsageSnapshot p2;
        p2.providerId = "claude";
        p2.snapshot.last30DaysCostUSD = 25.0;

        QVector<ProviderCostUsageSnapshot> list = {p1, p2};
        std::sort(list.begin(), list.end(),
                  [](const ProviderCostUsageSnapshot& a, const ProviderCostUsageSnapshot& b) {
                      return a.snapshot.last30DaysCostUSD > b.snapshot.last30DaysCostUSD;
                  });

        QCOMPARE(list[0].providerId, "claude");
        QCOMPARE(list[1].providerId, "codex");
    }
};

QTEST_MAIN(PerProviderCostUsageTest)
#include "tst_PerProviderCostUsage.moc"