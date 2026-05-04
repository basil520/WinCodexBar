#include <QtTest/QtTest>
#include <QJsonObject>
#include <QJsonArray>
#include "providers/kimi/KimiProvider.h"
#include "providers/kimi/KimiTokenResolver.h"
#include "providers/kimi/KimiModels.h"
#include "providers/kimi/KimiAPIError.h"

class tst_KimiProvider : public QObject {
    Q_OBJECT

private slots:
    void labelsAreCorrect();
    void sourceModes();
    void statusPageURLIsEmpty();
    void extractTokenFromJWT();
    void extractTokenFromCookieHeader();
    void extractTokenFromCurlCookieHeader();
    void extractTokenFromCurlAuthHeader();
    void decodeJWTPayload();
    void isValidJWT();
    void parseKimiNumber();
    void kimiResetTimeParsing();
    void kimiModelsFromJson();
    void parseKimiResponseFull();
    void apiErrorMessages();
};

void tst_KimiProvider::labelsAreCorrect() {
    KimiProvider provider;
    QCOMPARE(provider.sessionLabel(), QString("Weekly"));
    QCOMPARE(provider.weeklyLabel(), QString("Rate Limit"));
    QCOMPARE(provider.displayName(), QString("Kimi"));
    QCOMPARE(provider.id(), QString("kimi"));
}

void tst_KimiProvider::sourceModes() {
    KimiProvider provider;
    auto modes = provider.supportedSourceModes();
    QVERIFY(modes.contains("auto"));
    QVERIFY(modes.contains("web"));
}

void tst_KimiProvider::statusPageURLIsEmpty() {
    KimiProvider provider;
    QVERIFY(provider.statusPageURL().isEmpty());
}

void tst_KimiProvider::extractTokenFromJWT() {
    QString jwt = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIn0.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c";
    auto token = KimiTokenResolver::extractKimiAuthToken(jwt);
    QVERIFY(token.has_value());
    QCOMPARE(token.value(), jwt);
}

void tst_KimiProvider::extractTokenFromCookieHeader() {
    QString cookie1 = "kimi-auth=abc123.def456.ghi789";
    auto token1 = KimiTokenResolver::extractKimiAuthToken(cookie1);
    QVERIFY(token1.has_value());
    QCOMPARE(token1.value(), QString("abc123.def456.ghi789"));

    QString cookie2 = "other=xyz; kimi-auth=abc123.def456.ghi789; foo=bar";
    auto token2 = KimiTokenResolver::extractKimiAuthToken(cookie2);
    QVERIFY(token2.has_value());
    QCOMPARE(token2.value(), QString("abc123.def456.ghi789"));
}

void tst_KimiProvider::extractTokenFromCurlCookieHeader() {
    QString curl1 = "-H 'Cookie: kimi-auth=abc123.def456.ghi789'";
    auto token1 = KimiTokenResolver::extractKimiAuthToken(curl1);
    QVERIFY(token1.has_value());
    QCOMPARE(token1.value(), QString("abc123.def456.ghi789"));

    QString curl2 = R"(-H "Cookie: other=xyz; kimi-auth=abc123.def456.ghi789")";
    auto token2 = KimiTokenResolver::extractKimiAuthToken(curl2);
    QVERIFY(token2.has_value());
    QCOMPARE(token2.value(), QString("abc123.def456.ghi789"));
}

void tst_KimiProvider::extractTokenFromCurlAuthHeader() {
    QString curl = "-H 'Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIn0.sig'";
    auto token = KimiTokenResolver::extractKimiAuthToken(curl);
    QVERIFY(token.has_value());
    QCOMPARE(token.value(), QString("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIn0.sig"));
}

void tst_KimiProvider::decodeJWTPayload() {
    // JWT with payload: {"sub":"user123","device_id":"dev456"}
    QString jwt = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyMTIzIiwiZGV2aWNlX2lkIjoiZGV2NDU2In0.sig";
    QJsonObject payload = KimiTokenResolver::decodeJWTPayload(jwt);
    QCOMPARE(payload.value("sub").toString(), QString("user123"));
    QCOMPARE(payload.value("device_id").toString(), QString("dev456"));
}

void tst_KimiProvider::isValidJWT() {
    QVERIFY(KimiTokenResolver::isValidJWT("eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxIn0.sig"));
    QVERIFY(!KimiTokenResolver::isValidJWT("not-a-jwt"));
    QVERIFY(!KimiTokenResolver::isValidJWT("eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxIn0")); // only 2 parts
    QVERIFY(!KimiTokenResolver::isValidJWT(""));
}

void tst_KimiProvider::parseKimiNumber() {
    QJsonObject obj;
    obj["limit"] = "200";
    obj["used"] = 45;
    obj["remaining"] = "155";

    auto detail = KimiUsageDetail::fromJson(obj);
    QVERIFY(detail.limit.has_value());
    QCOMPARE(*detail.limit, 200.0);
    QVERIFY(detail.used.has_value());
    QCOMPARE(*detail.used, 45.0);
    QVERIFY(detail.remaining.has_value());
    QCOMPARE(*detail.remaining, 155.0);
}

void tst_KimiProvider::kimiResetTimeParsing() {
    // Microsecond precision
    QDateTime dt1 = parseKimiResetTime("2026-05-07T02:43:45.585111Z");
    QVERIFY(dt1.isValid());
    QCOMPARE(dt1.toString(Qt::ISODate), QString("2026-05-07T02:43:45Z"));

    // Millisecond precision
    QDateTime dt2 = parseKimiResetTime("2026-05-07T02:43:45.585Z");
    QVERIFY(dt2.isValid());

    // No milliseconds
    QDateTime dt3 = parseKimiResetTime("2026-05-07T02:43:45Z");
    QVERIFY(dt3.isValid());

    // Empty
    QDateTime dt4 = parseKimiResetTime("");
    QVERIFY(!dt4.isValid());
}

void tst_KimiProvider::kimiModelsFromJson() {
    QJsonObject weeklyDetail;
    weeklyDetail["limit"] = "200";
    weeklyDetail["used"] = "45";
    weeklyDetail["remaining"] = "155";
    weeklyDetail["resetTime"] = "2026-05-10T00:00:00Z";

    QJsonObject rateDetail;
    rateDetail["limit"] = "50";
    rateDetail["used"] = "12";
    rateDetail["remaining"] = "38";
    rateDetail["resetTime"] = "2026-05-04T17:00:00Z";

    QJsonObject window;
    window["duration"] = 300;
    window["timeUnit"] = "TIME_UNIT_MINUTE";

    QJsonObject rateLimitObj;
    rateLimitObj["window"] = window;
    rateLimitObj["detail"] = rateDetail;

    QJsonArray limits;
    limits.append(rateLimitObj);

    QJsonObject codingUsageJson;
    codingUsageJson["scope"] = "FEATURE_CODING";
    codingUsageJson["detail"] = weeklyDetail;
    codingUsageJson["limits"] = limits;

    auto codingUsage = KimiCodingUsage::fromJson(codingUsageJson);
    QVERIFY(codingUsage.has_value());
    QCOMPARE(codingUsage->scope, QString("FEATURE_CODING"));
    QCOMPARE(codingUsage->limits.size(), 1);
    QCOMPARE(codingUsage->limits[0].windowDuration, 300);
    QCOMPARE(codingUsage->limits[0].windowTimeUnit, QString("TIME_UNIT_MINUTE"));

    // Non-FEATURE_CODING should return nullopt
    QJsonObject otherUsage;
    otherUsage["scope"] = "OTHER";
    auto other = KimiCodingUsage::fromJson(otherUsage);
    QVERIFY(!other.has_value());
}

void tst_KimiProvider::parseKimiResponseFull() {
    QJsonObject weeklyDetail;
    weeklyDetail["limit"] = "200";
    weeklyDetail["used"] = "50";
    weeklyDetail["remaining"] = "150";
    weeklyDetail["resetTime"] = "2026-05-10T00:00:00Z";

    QJsonObject rateDetail;
    rateDetail["limit"] = "50";
    rateDetail["used"] = "20";
    rateDetail["remaining"] = "30";
    rateDetail["resetTime"] = "2026-05-04T17:00:00Z";

    QJsonObject window;
    window["duration"] = 300;
    window["timeUnit"] = "TIME_UNIT_MINUTE";

    QJsonObject rateLimitObj;
    rateLimitObj["window"] = window;
    rateLimitObj["detail"] = rateDetail;

    QJsonArray limits;
    limits.append(rateLimitObj);

    QJsonObject codingUsageJson;
    codingUsageJson["scope"] = "FEATURE_CODING";
    codingUsageJson["detail"] = weeklyDetail;
    codingUsageJson["limits"] = limits;

    QJsonArray usages;
    usages.append(codingUsageJson);

    QJsonObject json;
    json["usages"] = usages;

    auto snap = KimiWebStrategy::parseKimiResponse(json);
    QVERIFY(snap.primary.has_value());
    QCOMPARE(snap.primary->usedPercent, 25.0);
    QVERIFY(!snap.primary->windowMinutes.has_value() || snap.primary->windowMinutes.value() == 0);
    QVERIFY(snap.primary->resetsAt.has_value());

    QVERIFY(snap.secondary.has_value());
    QCOMPARE(snap.secondary->usedPercent, 40.0);
    QCOMPARE(snap.secondary->windowMinutes.value(), 300);
    QVERIFY(snap.secondary->resetsAt.has_value());

    QVERIFY(snap.identity.has_value());
    QCOMPARE(snap.identity->providerID.value(), UsageProvider::kimi);
}

void tst_KimiProvider::apiErrorMessages() {
    QString msg1 = kimiErrorMessage(KimiAPIError::MissingToken);
    QVERIFY(msg1.contains("missing"));

    QString msg2 = kimiErrorMessage(KimiAPIError::InvalidToken, "HTTP 401");
    QVERIFY(msg2.contains("401"));

    QString msg3 = kimiErrorMessage(KimiAPIError::NetworkError, "timeout");
    QVERIFY(msg3.contains("timeout"));

    QString msg4 = kimiErrorMessage(KimiAPIError::ParseFailed, "bad json");
    QVERIFY(msg4.contains("bad json"));

    QString msg5 = kimiErrorMessage(KimiAPIError::APIError, "server error");
    QVERIFY(msg5.contains("server error"));
}

QTEST_MAIN(tst_KimiProvider)
#include "tst_KimiProvider.moc"
