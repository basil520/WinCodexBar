#include <QtTest/QtTest>
#include "../src/providers/minimax/MiniMaxProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_MiniMaxProvider : public QObject {
    Q_OBJECT
private slots:
    void parseRemainsAsRemainingNotUsed() {
        // Upstream: current_interval_usage_count = REMAINING, not used
        QJsonObject json;
        QJsonObject baseResp;
        baseResp["status_code"] = 0;
        json["base_resp"] = baseResp;
        QJsonObject data;
        data["plan_name"] = "coding-plan-pro";
        QJsonArray remains;
        QJsonObject remain;
        remain["current_interval_total_count"] = 100;
        remain["current_interval_usage_count"] = 17; // 17 remaining → 83 used
        remain["start_time"] = 1714608000000.0;
        remain["end_time"] = 1715212800000.0;
        remains.append(remain);
        data["model_remains"] = remains;
        json["data"] = data;

        ProviderFetchResult r = MiniMaxAPIStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 83.0); // (100-17)/100*100 = 83%
        QVERIFY(r.usage.primary->resetDescription.has_value());
        QVERIFY(r.usage.primary->resetDescription->contains("83"));
        QVERIFY(r.usage.primary->resetDescription->contains("100"));
    }

    void apiErrorStatusCode() {
        QJsonObject json;
        QJsonObject baseResp;
        baseResp["status_code"] = 1004;
        baseResp["status_msg"] = "Invalid session, please log in";
        json["base_resp"] = baseResp;

        ProviderFetchResult r = MiniMaxAPIStrategy::parseResponse(json);
        QVERIFY(!r.success);
        QVERIFY(r.errorMessage.toLower().contains("invalid"));
    }

    void planNameFromCandidates() {
        QJsonObject json;
        QJsonObject baseResp; baseResp["status_code"] = 0;
        json["base_resp"] = baseResp;
        QJsonObject data;
        data["current_subscribe_title"] = "Coding Plan Pro";
        data["plan_name"] = "pro-plan";
        QJsonArray remains;
        QJsonObject remain;
        remain["current_interval_total_count"] = 500;
        remain["current_interval_usage_count"] = 400;
        remains.append(remain);
        data["model_remains"] = remains;
        json["data"] = data;

        ProviderFetchResult r = MiniMaxAPIStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.identity.has_value());
        QCOMPARE(r.usage.identity->loginMethod.value(), QString("Coding Plan Pro"));
    }

    void dualStrategyMode() {
        MiniMaxProvider p;
        QCOMPARE(p.id(), QString("minimax"));
        QCOMPARE(p.displayName(), QString("MiniMax"));
        QCOMPARE(p.brandColor(), QString("#EC4899"));
        QCOMPARE(p.sessionLabel(), QString("Prompts"));
        QCOMPARE(p.weeklyLabel(), QString("Window"));

        auto modes = p.supportedSourceModes();
        QVERIFY(modes.contains("auto"));
        QVERIFY(modes.contains("web"));
        QVERIFY(modes.contains("api"));

        // Auto mode produces both strategies
        ProviderFetchContext ctx;
        auto s = p.createStrategies(ctx);
        QCOMPARE(s.size(), 2);
        QCOMPARE(s[0]->id(), QString("minimax.api"));
        QCOMPARE(s[1]->id(), QString("minimax.web"));
        qDeleteAll(s);
    }
};

QTEST_MAIN(tst_MiniMaxProvider)
#include "tst_MiniMaxProvider.moc"
