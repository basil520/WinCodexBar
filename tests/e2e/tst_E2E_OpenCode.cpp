#include <QtTest/QtTest>
#include "E2ETestUtils.h"

class tst_E2E_OpenCode : public QObject {
    Q_OBJECT

private slots:
    void testRealAccount() {
        if (!E2ETestUtils::hasRealCredentials("opencode")) {
            QSKIP("OpenCode cookie not available. Set CODEXBAR_E2E_OPENCODE_COOKIE.");
        }

        QHash<QString, QVariant> overrides;
        QString cookie = qgetenv("CODEXBAR_E2E_OPENCODE_COOKIE");
        if (cookie.isEmpty()) cookie = qgetenv("OPENCODE_COOKIE");
        if (cookie.isEmpty()) cookie = qgetenv("OPENCODE_AUTH");
        overrides["manualCookieHeader"] = cookie;

        auto result = E2ETestUtils::runProviderFetch("opencode", overrides);

        if (!result.success) {
            // If the workspace has no subscription data, skip rather than fail
            if (result.errorMessage.contains("no subscription usage data", Qt::CaseInsensitive)) {
                QSKIP(qPrintable("OpenCode workspace has no subscription data: " + result.errorMessage));
            }
            qDebug() << "OpenCode E2E failed:" << E2ETestUtils::maskSensitive(result.errorMessage);
        }

        QVERIFY2(result.success,
                 qPrintable("OpenCode fetch failed: " + E2ETestUtils::maskSensitive(result.errorMessage)));

        QVERIFY(result.usage.primary.has_value() || result.usage.secondary.has_value() || result.usage.identity.has_value());
    }
};

QTEST_MAIN(tst_E2E_OpenCode)
#include "tst_E2E_OpenCode.moc"
