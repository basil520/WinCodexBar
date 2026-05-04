#include <QtTest/QtTest>
#include "../src/providers/minimax/MiniMaxProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_MiniMaxProvider : public QObject {
    Q_OBJECT
private slots:
    void parseRemains() {
        QJsonObject json;
        QJsonObject baseResp;
        baseResp["status_code"] = 0;
        json["base_resp"] = baseResp;
        QJsonObject data;
        data["plan_name"] = "coding-plan-pro";
        QJsonArray remains;
        QJsonObject remain;
        remain["current_interval_total_count"] = 500;
        remain["current_interval_usage_count"] = 100;
        remain["start_time"] = 1714608000000.0;
        remain["end_time"] = 1715212800000.0;
        remains.append(remain);
        data["model_remains"] = remains;
        json["data"] = data;

        ProviderFetchResult r = MiniMaxAPIStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 20.0);
        QVERIFY(r.usage.primary->windowMinutes.has_value());
        QCOMPARE(r.usage.identity->loginMethod.value(), QString("coding-plan-pro"));
    }

    void apiError() {
        QJsonObject json;
        QJsonObject baseResp;
        baseResp["status_code"] = 401;
        baseResp["status_msg"] = "Unauthorized";
        json["base_resp"] = baseResp;

        ProviderFetchResult r = MiniMaxAPIStrategy::parseResponse(json);
        QVERIFY(!r.success);
        QCOMPARE(r.errorMessage, QString("Unauthorized"));
    }

    void providerMetadata() {
        MiniMaxProvider p;
        QCOMPARE(p.id(), QString("minimax"));
        QCOMPARE(p.displayName(), QString("MiniMax"));
        QCOMPARE(p.brandColor(), QString("#EC4899"));
    }
};

QTEST_MAIN(tst_MiniMaxProvider)
#include "tst_MiniMaxProvider.moc"
