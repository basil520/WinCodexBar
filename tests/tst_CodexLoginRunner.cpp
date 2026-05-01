#include <QtTest/QtTest>

#include "../src/providers/codex/CodexLoginRunner.h"
#include "../src/providers/codex/ManagedCodexAccountService.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

QString jwtWithPayload(const QJsonObject& payload)
{
    QByteArray encoded = QJsonDocument(payload).toJson(QJsonDocument::Compact)
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return QStringLiteral("e30.%1.sig").arg(QString::fromLatin1(encoded));
}

void writeAuthJson(const QString& codexHome, const QString& email, const QString& accountId)
{
    QVERIFY(QDir().mkpath(codexHome));
    QJsonObject tokens;
    tokens["access_token"] = "access-token";
    tokens["refresh_token"] = "refresh-token";
    tokens["account_id"] = accountId;
    tokens["id_token"] = jwtWithPayload(QJsonObject{
        {"email", email},
        {"https://api.openai.com/profile", QJsonObject{{"email", email}}}
    });

    QJsonObject root;
    root["tokens"] = tokens;

    QFile file(QDir(codexHome).filePath("auth.json"));
    QVERIFY2(file.open(QIODevice::WriteOnly),
             qPrintable(file.errorString() + " " + file.fileName()));
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

QString managedAccountStorePath()
{
    return QDir::currentPath() + "/codex-login-managed-accounts.json";
}

QString testDirPath(const QString& name)
{
    return QDir::currentPath() + "/" + name;
}

} // namespace

class tst_CodexLoginRunner : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        qputenv("CODEXBAR_MANAGED_CODEX_ACCOUNTS_PATH",
                QDir::toNativeSeparators(managedAccountStorePath()).toUtf8());
    }

    void cleanup() {
        QFile::remove(managedAccountStorePath());
        QDir(testDirPath("codex-login-user-home")).removeRecursively();
        QDir(testDirPath("codex-login-home")).removeRecursively();
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
             + "/managed-codex-homes").removeRecursively();
    }

    void cleanupTestCase() {
        qunsetenv("CODEXBAR_MANAGED_CODEX_ACCOUNTS_PATH");
        QFile::remove(managedAccountStorePath());
        QDir(testDirPath("codex-login-user-home")).removeRecursively();
        QDir(testDirPath("codex-login-home")).removeRecursively();
    }

    void parsesDeviceAuthPromptWithUrlAndCode() {
        QString output =
            "Open https://auth.openai.com/activate and enter code ABCD-EFGH to continue.";

        auto prompt = CodexLoginRunner::parseDeviceAuthPrompt(output);

        QVERIFY(prompt.has_value());
        QCOMPARE(prompt->verificationUri, QString("https://auth.openai.com/activate"));
        QCOMPARE(prompt->userCode, QString("ABCD-EFGH"));
    }

    void parsesDeviceAuthPromptAfterAnsiOutput() {
        QString output =
            "\x1b[32mVisit https://auth.openai.com/activate.\x1b[0m\r\n"
            "User code: WXYZ-1234";

        auto prompt = CodexLoginRunner::parseDeviceAuthPrompt(output);

        QVERIFY(prompt.has_value());
        QCOMPARE(prompt->verificationUri, QString("https://auth.openai.com/activate"));
        QCOMPARE(prompt->userCode, QString("WXYZ-1234"));
    }

    void ignoresAuthorizationTextBeforeRealDeviceCode() {
        QString output =
            "Complete device code authorization at https://auth.openai.com/activate.\n"
            "Then enter code ABCD-EFGH.";

        auto prompt = CodexLoginRunner::parseDeviceAuthPrompt(output);

        QVERIFY(prompt.has_value());
        QCOMPARE(prompt->verificationUri, QString("https://auth.openai.com/activate"));
        QCOMPARE(prompt->userCode, QString("ABCD-EFGH"));
    }

    void missingDeviceAuthCodeIsNotPrompt() {
        auto prompt = CodexLoginRunner::parseDeviceAuthPrompt("Starting login...");

        QVERIFY(!prompt.has_value());
    }

    void managedAccountUsesEmailFromIdToken() {
        const QString userHome = testDirPath("codex-login-user-home");
        const QString codexHome = testDirPath("codex-login-home");
        QVERIFY(QDir().mkpath(userHome));
        QVERIFY(QDir().mkpath(codexHome));
        writeAuthJson(codexHome, "dev@example.com", "acct_123");

        QHash<QString, QString> env;
        env["USERPROFILE"] = userHome;

        ManagedCodexAccountService service(env);
        QVERIFY(service.addAccount("", codexHome));

        auto accounts = service.visibleAccounts();
        QCOMPARE(accounts.size(), 1);
        QCOMPARE(accounts.first().email, QString("dev@example.com"));
        QCOMPARE(accounts.first().displayName, QString("dev@example.com"));
    }

    void test_parseNoUrlInOutput() {
        auto prompt = CodexLoginRunner::parseDeviceAuthPrompt(
            "Some random text without any URL at all.");
        QVERIFY(!prompt.has_value());
    }

    void test_parseUrlButNoCode() {
        auto prompt = CodexLoginRunner::parseDeviceAuthPrompt(
            "Visit https://example.com/setup to configure your account.");
        QVERIFY(!prompt.has_value());
    }

    void test_parseSpacedUserCode() {
        auto prompt = CodexLoginRunner::parseDeviceAuthPrompt(
            "Open https://auth.openai.com/activate\nUser code: ABCD EFGH");
        QVERIFY(prompt.has_value());
        QCOMPARE(prompt->userCode, QString("ABCD-EFGH"));
        QCOMPARE(prompt->verificationUri, QString("https://auth.openai.com/activate"));
    }

    void test_parseLowercaseCode() {
        auto prompt = CodexLoginRunner::parseDeviceAuthPrompt(
            "Go to https://auth.codex.ai/device\nenter the code xyz9-1234 to continue");
        QVERIFY(prompt.has_value());
        QCOMPARE(prompt->userCode, QString("XYZ9-1234"));
    }

    void test_emptyOutputReturnsNullopt() {
        auto prompt = CodexLoginRunner::parseDeviceAuthPrompt(QString());
        QVERIFY(!prompt.has_value());
    }
};

QTEST_MAIN(tst_CodexLoginRunner)
#include "tst_CodexLoginRunner.moc"
