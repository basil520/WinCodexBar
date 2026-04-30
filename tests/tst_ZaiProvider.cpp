#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "models/UsageSnapshot.h"
#include "models/RateWindow.h"
#include "models/ProviderIdentitySnapshot.h"

class ZaiProviderTest : public QObject {
    Q_OBJECT

private:
    static UsageSnapshot parseZaiResponse(const QJsonObject& json);

private slots:
    void testParseBasicResponse() {
        QJsonObject json;
        json["code"] = 200;
        json["success"] = true;
        QJsonObject data;
        QJsonArray limits;

        QJsonObject tokenLimit;
        tokenLimit["type"] = "TOKENS_LIMIT";
        tokenLimit["unit"] = 3;
        tokenLimit["number"] = 5;
        tokenLimit["percentage"] = 34;
        limits.append(tokenLimit);

        QJsonObject timeLimit;
        timeLimit["type"] = "TIME_LIMIT";
        timeLimit["percentage"] = 12;
        limits.append(timeLimit);

        data["limits"] = limits;
        data["planName"] = "Pro";
        json["data"] = data;

        auto snapshot = parseZaiResponse(json);
        QVERIFY(snapshot.primary.has_value());
        QCOMPARE(snapshot.primary->usedPercent, 34.0);
        QVERIFY(snapshot.primary->windowMinutes.has_value());
        QCOMPARE(snapshot.primary->windowMinutes.value(), 300);
        QCOMPARE(snapshot.primary->remainingPercent(), 66.0);
        QVERIFY(snapshot.secondary.has_value());
        QCOMPARE(snapshot.secondary->usedPercent, 12.0);
        QVERIFY(snapshot.identity.has_value());
        QCOMPARE(snapshot.identity->loginMethod.value(), "Pro");
    }

    void testParseAuthError() {
        QJsonObject json;
        json["code"] = 1001;
        json["msg"] = "Authorization Token Missing";
        json["success"] = false;

        auto snapshot = parseZaiResponse(json);
        QVERIFY(!snapshot.primary.has_value());
        QVERIFY(!snapshot.identity.has_value());
    }

    void testParseNoData() {
        QJsonObject json;
        json["code"] = 200;
        json["success"] = true;

        auto snapshot = parseZaiResponse(json);
        QVERIFY(!snapshot.primary.has_value());
    }

    void testParseEmptyLimits() {
        QJsonObject json;
        json["code"] = 200;
        json["success"] = true;
        QJsonObject data;
        data["planName"] = "Pro";
        QJsonArray limits;
        data["limits"] = limits;
        json["data"] = data;

        auto snapshot = parseZaiResponse(json);
        QVERIFY(!snapshot.primary.has_value());
        QVERIFY(snapshot.identity.has_value());
    }

    void testParseThreeLimits() {
        QJsonObject json;
        json["code"] = 200;
        json["success"] = true;
        QJsonObject data;
        QJsonArray limits;

        QJsonObject shortTokens;
        shortTokens["type"] = "TOKENS_LIMIT";
        shortTokens["unit"] = 3;
        shortTokens["number"] = 5;
        shortTokens["percentage"] = 80;
        limits.append(shortTokens);

        QJsonObject longTokens;
        longTokens["type"] = "TOKENS_LIMIT";
        longTokens["unit"] = 1;
        longTokens["number"] = 7;
        longTokens["percentage"] = 30;
        limits.append(longTokens);

        QJsonObject timeLimit;
        timeLimit["type"] = "TIME_LIMIT";
        timeLimit["percentage"] = 45;
        limits.append(timeLimit);

        data["limits"] = limits;
        json["data"] = data;

        auto snapshot = parseZaiResponse(json);
        QVERIFY(snapshot.primary.has_value());
        QCOMPARE(snapshot.primary->usedPercent, 30.0);
        QVERIFY(snapshot.tertiary.has_value());
        QCOMPARE(snapshot.tertiary->usedPercent, 80.0);
        QVERIFY(snapshot.secondary.has_value());
        QCOMPARE(snapshot.secondary->usedPercent, 45.0);
    }

    void testRateWindowRemaining() {
        RateWindow rw;
        rw.usedPercent = 75.0;
        QCOMPARE(rw.remainingPercent(), 25.0);
    }

    void testRateWindowNeverNegative() {
        RateWindow rw;
        rw.usedPercent = 150.0;
        QCOMPARE(rw.remainingPercent(), 0.0);
    }
};

static int limitUnitToMinutes(int unit, int number) {
    switch (unit) {
    case 1: return number * 24 * 60;
    case 3: return number * 60;
    case 5: return number;
    case 6: return number * 7 * 24 * 60;
    default: return 0;
    }
}

static std::optional<RateWindow> makeRateWindow(const QJsonObject& limit) {
    double percentage = limit.value("percentage").toDouble(0);
    double usedPercent = std::clamp(percentage, 0.0, 100.0);

    std::optional<int> windowMinutes;
    if (limit.contains("unit") && limit.contains("number")) {
        int unit = limit.value("unit").toInt();
        int number = limit.value("number").toInt();
        int mins = limitUnitToMinutes(unit, number);
        if (mins > 0) windowMinutes = mins;
    }

    std::optional<QDateTime> resetsAt;
    if (limit.contains("nextResetTime")) {
        qint64 epochMs = static_cast<qint64>(limit.value("nextResetTime").toDouble());
        if (epochMs > 0) {
            resetsAt = QDateTime::fromMSecsSinceEpoch(epochMs);
        }
    }

    RateWindow rw;
    rw.usedPercent = usedPercent;
    rw.windowMinutes = windowMinutes;
    rw.resetsAt = resetsAt;
    return rw;
}

UsageSnapshot ZaiProviderTest::parseZaiResponse(const QJsonObject& json) {
    UsageSnapshot snapshot;
    snapshot.updatedAt = QDateTime::currentDateTime();

    int code = json.value("code").toInt(-1);
    bool success = json.value("success").toBool(false);
    if (code != 200 || !success) return snapshot;

    QJsonObject data = json.value("data").toObject();
    QJsonArray limits = data.value("limits").toArray();
    if (limits.isEmpty()) {
        QString planName;
        if (data.contains("planName")) planName = data.value("planName").toString();
        if (!planName.isEmpty()) {
            ProviderIdentitySnapshot identity;
            identity.providerID = UsageProvider::zai;
            identity.loginMethod = planName;
            snapshot.identity = identity;
        }
        return snapshot;
    }

    QVector<QJsonObject> tokenLimits;
    std::optional<QJsonObject> timeLimit;

    for (const auto& limitVal : limits) {
        QJsonObject limit = limitVal.toObject();
        QString type = limit.value("type").toString();
        if (type == "TOKENS_LIMIT") tokenLimits.append(limit);
        else if (type == "TIME_LIMIT") timeLimit = limit;
    }

    std::sort(tokenLimits.begin(), tokenLimits.end(),
              [](const QJsonObject& a, const QJsonObject& b) {
                  int aMins = limitUnitToMinutes(a.value("unit").toInt(), a.value("number").toInt());
                  int bMins = limitUnitToMinutes(b.value("unit").toInt(), b.value("number").toInt());
                  return aMins < bMins;
              });

    if (!tokenLimits.isEmpty()) {
        if (tokenLimits.size() >= 2) {
            snapshot.primary = makeRateWindow(tokenLimits.last());
            snapshot.tertiary = makeRateWindow(tokenLimits.first());
            if (timeLimit.has_value()) snapshot.secondary = makeRateWindow(timeLimit.value());
        } else {
            snapshot.primary = makeRateWindow(tokenLimits.first());
            if (timeLimit.has_value()) snapshot.secondary = makeRateWindow(timeLimit.value());
        }
    } else if (timeLimit.has_value()) {
        snapshot.primary = makeRateWindow(timeLimit.value());
    }

    QString planName;
    if (data.contains("planName")) planName = data.value("planName").toString();
    if (!planName.isEmpty()) {
        ProviderIdentitySnapshot identity;
        identity.providerID = UsageProvider::zai;
        identity.loginMethod = planName;
        snapshot.identity = identity;
    }
    return snapshot;
}

QTEST_MAIN(ZaiProviderTest)
#include "tst_ZaiProvider.moc"
