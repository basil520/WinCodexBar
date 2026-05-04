#include <QtTest/QtTest>
#include "../src/providers/gemini/GeminiProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_GeminiProvider : public QObject {
    Q_OBJECT
private slots:
    void parseBuckets() {
        QJsonObject json;
        QJsonArray buckets;
        QJsonObject pro, flash, flashLite;
        pro["remainingFraction"] = 0.75; pro["modelId"] = "gemini-2.5-pro";
        flash["remainingFraction"] = 0.50; flash["modelId"] = "gemini-2.5-flash";
        flashLite["remainingFraction"] = 0.25; flashLite["modelId"] = "gemini-2.5-flash-lite";
        buckets.append(pro); buckets.append(flash); buckets.append(flashLite);
        json["buckets"] = buckets;

        ProviderFetchResult r = GeminiOAuthStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 25.0);
        QVERIFY(r.usage.secondary.has_value());
        QCOMPARE(r.usage.secondary->usedPercent, 50.0);
        QVERIFY(r.usage.tertiary.has_value());
        QCOMPARE(r.usage.tertiary->usedPercent, 75.0);
    }

    void emptyBuckets() {
        QJsonObject json;
        QJsonArray buckets;
        json["buckets"] = buckets;
        ProviderFetchResult r = GeminiOAuthStrategy::parseResponse(json);
        QVERIFY(!r.success);
    }

    void dualStrategy() {
        GeminiProvider p;
        ProviderFetchContext ctx;
        auto s = p.createStrategies(ctx);
        QCOMPARE(s.size(), 2);
        qDeleteAll(s);
    }
};

QTEST_MAIN(tst_GeminiProvider)
#include "tst_GeminiProvider.moc"
