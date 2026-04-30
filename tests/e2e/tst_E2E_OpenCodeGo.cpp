#include <QtTest/QtTest>
#include "E2ETestUtils.h"

class tst_E2E_OpenCodeGo : public QObject {
    Q_OBJECT

private slots:
    void testRealAccount() {
        if (!E2ETestUtils::hasRealCredentials("opencodego")) {
            QSKIP("OpenCode Go cookie not available. Set CODEXBAR_E2E_OPENCODEGO_COOKIE.");
        }

        QHash<QString, QVariant> overrides;
        QString cookie = qgetenv("CODEXBAR_E2E_OPENCODEGO_COOKIE");
        if (cookie.isEmpty()) cookie = qgetenv("OPENCODE_COOKIE");
        if (cookie.isEmpty()) cookie = qgetenv("OPENCODE_AUTH");
        overrides["manualCookieHeader"] = cookie;

        auto result = E2ETestUtils::runProviderFetch("opencodego", overrides);

        if (!result.success) {
            qDebug() << "OpenCode Go E2E failed:" << E2ETestUtils::maskSensitive(result.errorMessage);
        }

        QVERIFY2(result.success,
                 qPrintable("OpenCode Go fetch failed: " + E2ETestUtils::maskSensitive(result.errorMessage)));

        QVERIFY(result.usage.primary.has_value() || result.usage.secondary.has_value() || result.usage.tertiary.has_value());
    }
};

QTEST_MAIN(tst_E2E_OpenCodeGo)
#include "tst_E2E_OpenCodeGo.moc"
