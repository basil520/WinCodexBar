#include <QtTest/QtTest>
#include "E2ETestUtils.h"

#include "../src/providers/shared/ProviderCredentialStore.h"

class tst_E2E_Copilot : public QObject {
    Q_OBJECT

private slots:
    void testRealAccount() {
        if (!E2ETestUtils::hasRealCredentials("copilot")) {
            QSKIP("Copilot token not available. Set CODEXBAR_E2E_COPILOT_TOKEN or store token in WinCred.");
        }

        QByteArray token = qgetenv("CODEXBAR_E2E_COPILOT_TOKEN");
        // If the env var is present, inject it into the in-memory credential backend
        // so we do not touch Windows Credential Manager during the test.
        std::shared_ptr<InMemoryCredentialBackend> memBackend;
        if (!token.isEmpty()) {
            memBackend = std::make_shared<InMemoryCredentialBackend>();
            ProviderCredentialStore::setBackendForTesting(memBackend);
            memBackend->write("com.codexbar.oauth.copilot", "", token);
        }

        auto result = E2ETestUtils::runProviderFetch("copilot");

        if (memBackend) {
            ProviderCredentialStore::resetBackendForTesting();
        }

        if (!result.success) {
            qDebug() << "Copilot E2E failed:" << E2ETestUtils::maskSensitive(result.errorMessage);
        }

        QVERIFY2(result.success,
                 qPrintable("Copilot fetch failed: " + E2ETestUtils::maskSensitive(result.errorMessage)));

        QVERIFY(result.usage.primary.has_value() || result.usage.identity.has_value());
    }
};

QTEST_MAIN(tst_E2E_Copilot)
#include "tst_E2E_Copilot.moc"
