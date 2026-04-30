#include <QtTest/QtTest>
#include "E2ETestUtils.h"

#include <QTemporaryDir>
#include <QDir>
#include <QFile>

class tst_E2E_Codex : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        qDebug() << "E2E Codex initTestCase";
    }

    void testDummy() {
        QVERIFY(true);
    }

    void testRealAccount() {
        if (!E2ETestUtils::hasRealCredentials("codex")) {
            QSKIP("Codex auth.json not available. Set up ~/.codex/auth.json or CODEX_HOME.");
        }

        // Isolate CODEX_HOME so token refresh (if triggered) writes to a temp dir
        // instead of the user's real auth.json.
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString realHome = QDir::homePath();
        QString realAuth = realHome + "/.codex/auth.json";
        QString tempCodexHome = tempDir.path() + "/.codex";
        QVERIFY(QDir().mkpath(tempCodexHome));
        QVERIFY(QFile::copy(realAuth, tempCodexHome + "/auth.json"));

        qputenv("CODEX_HOME", tempCodexHome.toUtf8());

        auto result = E2ETestUtils::runProviderFetch("codex");

        if (!result.success) {
            qDebug() << "Codex E2E failed:" << E2ETestUtils::maskSensitive(result.errorMessage);
        }

        QVERIFY2(result.success,
                 qPrintable("Codex fetch failed: " + E2ETestUtils::maskSensitive(result.errorMessage)));

        QVERIFY(result.usage.primary.has_value() || result.usage.identity.has_value());
    }
};

QTEST_MAIN(tst_E2E_Codex)
#include "tst_E2E_Codex.moc"
