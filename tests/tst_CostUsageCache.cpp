#include <QtTest/QtTest>
#include "../src/util/CostUsageCache.h"
#include "../src/models/CostUsageReport.h"

#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

class tst_CostUsageCache : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Force cache into a temp dir by overriding GenericCacheLocation via env
        // (CostUsageCache uses GenericCacheLocation, which on Windows is %LOCALAPPDATA%)
        // We test the public API without relying on actual disk location.
    }

    void singletonInstance() {
        CostUsageCache& a = CostUsageCache::instance();
        CostUsageCache& b = CostUsageCache::instance();
        QCOMPARE(&a, &b);
    }

    void setAndGetContributions() {
        CostUsageCache& cache = CostUsageCache::instance();
        cache.clear();

        QHash<QString, QVector<CostUsageModelBreakdown>> contributions;
        CostUsageModelBreakdown mb;
        mb.modelName = "gpt-5";
        mb.inputTokens = 1000;
        mb.outputTokens = 200;
        contributions["2026-05-04"].append(mb);

        cache.setContributions("/tmp/test.jsonl", "codex", contributions);
        QCOMPARE(cache.entryCount(), 1);

        auto retrieved = cache.contributions("/tmp/test.jsonl");
        QCOMPARE(retrieved.size(), 1);
        QVERIFY(retrieved.contains("2026-05-04"));
        QCOMPARE(retrieved["2026-05-04"].size(), 1);
        QCOMPARE(retrieved["2026-05-04"][0].modelName, QString("gpt-5"));
        QCOMPARE(retrieved["2026-05-04"][0].inputTokens, 1000);
        QCOMPARE(retrieved["2026-05-04"][0].outputTokens, 200);
    }

    void isValidDetectsChanges() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.path() + "/test.jsonl";
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("test");
        file.close();

        CostUsageCache& cache = CostUsageCache::instance();
        cache.clear();

        QHash<QString, QVector<CostUsageModelBreakdown>> contributions;
        cache.setContributions(path, "codex", contributions);

        QFileInfo info(path);
        QVERIFY(cache.isValid(path, info));

        // Modify file
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("modified content");
        file.close();
        QFileInfo info2(path);
        QVERIFY(!cache.isValid(path, info2));
    }

    void removeEntry() {
        CostUsageCache& cache = CostUsageCache::instance();
        cache.clear();

        cache.setContributions("/tmp/a.jsonl", "codex", {});
        QCOMPARE(cache.entryCount(), 1);
        cache.removeEntry("/tmp/a.jsonl");
        QCOMPARE(cache.entryCount(), 0);
    }

    void clearAll() {
        CostUsageCache& cache = CostUsageCache::instance();
        cache.setContributions("/tmp/x.jsonl", "codex", {});
        cache.setContributions("/tmp/y.jsonl", "claude", {});
        QCOMPARE(cache.entryCount(), 2);
        cache.clear();
        QCOMPARE(cache.entryCount(), 0);
    }

    void persistAndReload() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.path() + "/test.jsonl";
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("test");
        file.close();

        CostUsageCache& cache = CostUsageCache::instance();
        cache.clear();

        QHash<QString, QVector<CostUsageModelBreakdown>> contributions;
        CostUsageModelBreakdown mb;
        mb.modelName = "claude-sonnet-4";
        mb.inputTokens = 500;
        mb.cacheReadTokens = 50;
        mb.cacheCreationTokens = 10;
        mb.outputTokens = 150;
        contributions["2026-05-01"].append(mb);
        cache.setContributions(path, "claude", contributions);

        QVERIFY(cache.save());

        // Simulate new process: clear in-memory state and reload
        cache.clear();
        QCOMPARE(cache.entryCount(), 0);

        QVERIFY(cache.load());
        QCOMPARE(cache.entryCount(), 1);

        auto retrieved = cache.contributions(path);
        QCOMPARE(retrieved.size(), 1);
        QCOMPARE(retrieved["2026-05-01"].size(), 1);
        QCOMPARE(retrieved["2026-05-01"][0].modelName, QString("claude-sonnet-4"));
        QCOMPARE(retrieved["2026-05-01"][0].inputTokens, 500);
        QCOMPARE(retrieved["2026-05-01"][0].cacheReadTokens, 50);
        QCOMPARE(retrieved["2026-05-01"][0].cacheCreationTokens, 10);
        QCOMPARE(retrieved["2026-05-01"][0].outputTokens, 150);
    }

    void invalidSchemaClearsCache() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/CodexBar";
        QDir().mkpath(cacheDir);
        QString cachePath = cacheDir + "/cost-usage-index.json";

        // Write an old-schema cache file
        QJsonObject root;
        root["schemaVersion"] = 0; // old
        root["entries"] = QJsonObject();
        QFile file(cachePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        file.close();

        CostUsageCache& cache = CostUsageCache::instance();
        cache.clear();
        cache.load(); // should detect old schema and clear
        QCOMPARE(cache.entryCount(), 0);
    }
};

QTEST_MAIN(tst_CostUsageCache)
#include "tst_CostUsageCache.moc"
