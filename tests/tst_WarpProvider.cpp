#include <QtTest/QtTest>
#include "../src/providers/warp/WarpProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_WarpProvider : public QObject {
    Q_OBJECT
private slots:
    void parseLimitedRequests() {
        QJsonObject data, user1, user, limitInfo;
        limitInfo["isUnlimited"] = false;
        limitInfo["requestLimit"] = 1000;
        limitInfo["requestsUsedSinceLastRefresh"] = 250;
        user["requestLimitInfo"] = limitInfo;
        QJsonArray bonusGrants;
        user["bonusGrants"] = bonusGrants;
        user1["user"] = user;
        data["user"] = user1;
        QJsonObject json;
        json["data"] = data;

        ProviderFetchResult r = WarpAPIStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 25.0);
    }

    void parseUnlimited() {
        QJsonObject data, user1, user, limitInfo;
        limitInfo["isUnlimited"] = true;
        limitInfo["requestLimit"] = 0;
        limitInfo["requestsUsedSinceLastRefresh"] = 500;
        user["requestLimitInfo"] = limitInfo;
        QJsonArray bonusGrants;
        user["bonusGrants"] = bonusGrants;
        user1["user"] = user;
        data["user"] = user1;
        QJsonObject json;
        json["data"] = data;

        ProviderFetchResult r = WarpAPIStrategy::parseResponse(json);
        QVERIFY(r.success);
        QCOMPARE(r.usage.primary->usedPercent, 0.0);
        QCOMPARE(r.usage.primary->resetDescription.value(), QString("Unlimited"));
    }
};

QTEST_MAIN(tst_WarpProvider)
#include "tst_WarpProvider.moc"
