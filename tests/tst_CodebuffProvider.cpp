#include <QtTest/QtTest>

#include "providers/codebuff/CodebuffProvider.h"

#include <QJsonDocument>
#include <QTemporaryDir>
#include <QFile>

class tst_CodebuffProvider : public QObject {
    Q_OBJECT

private slots:
    void parsesUsageAndSubscriptionPayloads();
    void readsCredentialsFileToken();
};

void tst_CodebuffProvider::parsesUsageAndSubscriptionPayloads()
{
    const QByteArray usageJson = R"({
        "usage": 25,
        "quota": 100,
        "remainingBalance": 75,
        "next_quota_reset": "2026-05-12T00:00:00Z",
        "autoTopupEnabled": true
    })";
    const QByteArray subscriptionJson = R"({
        "email": "user@example.com",
        "subscription": {
            "displayName": "Pro",
            "status": "active",
            "billingPeriodEnd": "2026-06-01T00:00:00Z"
        },
        "rateLimit": {
            "weeklyUsed": 40,
            "weeklyLimit": 200,
            "weeklyResetsAt": "2026-05-06T00:00:00Z"
        }
    })";

    const CodebuffUsagePayload usage = CodebuffAPIStrategy::parseUsagePayloadForTesting(
        QJsonDocument::fromJson(usageJson).object());
    const CodebuffSubscriptionPayload subscription = CodebuffAPIStrategy::parseSubscriptionPayloadForTesting(
        QJsonDocument::fromJson(subscriptionJson).object());
    const UsageSnapshot snapshot = CodebuffAPIStrategy::makeSnapshotForTesting(usage, subscription);

    QVERIFY(snapshot.primary.has_value());
    QCOMPARE(snapshot.primary->usedPercent, 25.0);
    QVERIFY(snapshot.secondary.has_value());
    QCOMPARE(snapshot.secondary->usedPercent, 20.0);
    QVERIFY(snapshot.identity.has_value());
    QCOMPARE(snapshot.identity->accountEmail.value(), QStringLiteral("user@example.com"));
    QVERIFY(snapshot.identity->loginMethod.has_value());
    QVERIFY(snapshot.identity->loginMethod->contains(QStringLiteral("Pro")));
    QVERIFY(snapshot.identity->loginMethod->contains(QStringLiteral("75")));
}

void tst_CodebuffProvider::readsCredentialsFileToken()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir home(dir.path());
    QVERIFY(home.mkpath(QStringLiteral(".config/manicode")));

    QFile file(home.filePath(QStringLiteral(".config/manicode/credentials.json")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"default":{"authToken":" file-token "}})");
    file.close();

    QCOMPARE(CodebuffAPIStrategy::authFileTokenForTesting(dir.path()), QStringLiteral("file-token"));
}

QTEST_MAIN(tst_CodebuffProvider)
#include "tst_CodebuffProvider.moc"
