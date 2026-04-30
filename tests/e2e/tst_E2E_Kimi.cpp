#include <QtTest/QtTest>
#include "E2ETestUtils.h"

class tst_E2E_Kimi : public QObject {
    Q_OBJECT

private slots:
    void testRealAccount() {
        if (!E2ETestUtils::hasRealCredentials("kimi")) {
            QSKIP("Kimi cookie not available. Set CODEXBAR_E2E_KIMI_COOKIE.");
        }

        QHash<QString, QVariant> overrides;
        overrides["manualCookieHeader"] = qgetenv("CODEXBAR_E2E_KIMI_COOKIE");

        auto result = E2ETestUtils::runProviderFetch("kimi", overrides);

        if (!result.success) {
            qDebug() << "Kimi E2E failed:" << E2ETestUtils::maskSensitive(result.errorMessage);
        }

        QVERIFY2(result.success,
                 qPrintable("Kimi fetch failed: " + E2ETestUtils::maskSensitive(result.errorMessage)));

        QVERIFY(result.usage.primary.has_value() || result.usage.identity.has_value());
    }
};

QTEST_MAIN(tst_E2E_Kimi)
#include "tst_E2E_Kimi.moc"
