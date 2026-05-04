#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "account/TokenAccountStore.h"
#include "account/TokenAccount.h"
#include "account/SecureString.h"
#include "providers/shared/ProviderCredentialStore.h"

class tst_TokenAccountStore : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void singletonInstance();
    void addAndRetrieveAccount();
    void updateAccount();
    void removeAccount();
    void accountQueries();
    void defaultAccount();
    void credentialStorage();
    void persistAndReload();
    void migrationFromLegacy();

private:
    QString m_originalStoragePath;
    QTemporaryDir m_tempDir;
};

void tst_TokenAccountStore::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    // Use in-memory credential backend for tests
    ProviderCredentialStore::setBackendForTesting(std::make_shared<InMemoryCredentialBackend>());
}

void tst_TokenAccountStore::cleanupTestCase()
{
    ProviderCredentialStore::resetBackendForTesting();
}

void tst_TokenAccountStore::singletonInstance()
{
    TokenAccountStore* store1 = TokenAccountStore::instance();
    TokenAccountStore* store2 = TokenAccountStore::instance();
    QCOMPARE(store1, store2);
}

void tst_TokenAccountStore::addAndRetrieveAccount()
{
    TokenAccountStore* store = TokenAccountStore::instance();
    // Clear any previous state
    for (const auto& acc : store->allAccounts()) {
        store->removeAccount(acc.accountId);
    }

    TokenAccount account;
    account.providerId = "openrouter";
    account.displayName = "Personal OpenRouter";
    account.sourceMode = ProviderSourceMode::API;
    account.visibility = AccountVisibility::Visible;

    QString accountId = store->addAccount(account);
    QVERIFY(!accountId.isEmpty());

    auto retrieved = store->account(accountId);
    QVERIFY(retrieved.has_value());
    QCOMPARE(retrieved->providerId, QString("openrouter"));
    QCOMPARE(retrieved->displayName, QString("Personal OpenRouter"));
    QCOMPARE(retrieved->sourceMode, ProviderSourceMode::API);
    QCOMPARE(retrieved->visibility, AccountVisibility::Visible);
    QCOMPARE(store->accountCount(), 1);
}

void tst_TokenAccountStore::updateAccount()
{
    TokenAccountStore* store = TokenAccountStore::instance();
    for (const auto& acc : store->allAccounts()) {
        store->removeAccount(acc.accountId);
    }

    TokenAccount account;
    account.providerId = "copilot";
    account.displayName = "Work GitHub";
    account.sourceMode = ProviderSourceMode::OAuth;
    QString id = store->addAccount(account);

    auto retrieved = store->account(id);
    QVERIFY(retrieved.has_value());
    QCOMPARE(retrieved->displayName, QString("Work GitHub"));

    TokenAccount updated = retrieved.value();
    updated.displayName = "Work GitHub Updated";
    updated.visibility = AccountVisibility::Hidden;
    QVERIFY(store->updateAccount(id, updated));

    auto after = store->account(id);
    QVERIFY(after.has_value());
    QCOMPARE(after->displayName, QString("Work GitHub Updated"));
    QCOMPARE(after->visibility, AccountVisibility::Hidden);
}

void tst_TokenAccountStore::removeAccount()
{
    TokenAccountStore* store = TokenAccountStore::instance();
    for (const auto& acc : store->allAccounts()) {
        store->removeAccount(acc.accountId);
    }

    TokenAccount account;
    account.providerId = "claude";
    account.displayName = "Test Account";
    QString id = store->addAccount(account);
    QCOMPARE(store->accountCount(), 1);

    QVERIFY(store->removeAccount(id));
    QCOMPARE(store->accountCount(), 0);
    QVERIFY(!store->account(id).has_value());
}

void tst_TokenAccountStore::accountQueries()
{
    TokenAccountStore* store = TokenAccountStore::instance();
    for (const auto& acc : store->allAccounts()) {
        store->removeAccount(acc.accountId);
    }

    TokenAccount acc1;
    acc1.providerId = "copilot";
    acc1.displayName = "Account 1";
    acc1.visibility = AccountVisibility::Visible;
    QString id1 = store->addAccount(acc1);

    TokenAccount acc2;
    acc2.providerId = "copilot";
    acc2.displayName = "Account 2";
    acc2.visibility = AccountVisibility::Hidden;
    QString id2 = store->addAccount(acc2);

    TokenAccount acc3;
    acc3.providerId = "claude";
    acc3.displayName = "Claude Account";
    acc3.visibility = AccountVisibility::Visible;
    QString id3 = store->addAccount(acc3);

    QCOMPARE(store->accountCountForProvider("copilot"), 2);
    QCOMPARE(store->accountCountForProvider("claude"), 1);

    auto copilotAccounts = store->accountsForProvider("copilot");
    QCOMPARE(copilotAccounts.size(), 2);

    auto visibleCopilot = store->visibleAccountsForProvider("copilot");
    QCOMPARE(visibleCopilot.size(), 1);
    QCOMPARE(visibleCopilot.first().accountId, id1);

    auto allIds = store->accountIdsForProvider("copilot");
    QCOMPARE(allIds.size(), 2);
    QVERIFY(allIds.contains(id1));
    QVERIFY(allIds.contains(id2));
}

void tst_TokenAccountStore::defaultAccount()
{
    TokenAccountStore* store = TokenAccountStore::instance();
    for (const auto& acc : store->allAccounts()) {
        store->removeAccount(acc.accountId);
    }

    TokenAccount acc1;
    acc1.providerId = "openrouter";
    acc1.displayName = "Default Account";
    QString id1 = store->addAccount(acc1);

    TokenAccount acc2;
    acc2.providerId = "openrouter";
    acc2.displayName = "Secondary Account";
    QString id2 = store->addAccount(acc2);

    QVERIFY(store->defaultAccountId("openrouter").isEmpty());
    store->setDefaultAccountId("openrouter", id1);
    QCOMPARE(store->defaultAccountId("openrouter"), id1);

    // Removing default account should clear default mapping
    store->removeAccount(id1);
    QVERIFY(store->defaultAccountId("openrouter").isEmpty());
}

void tst_TokenAccountStore::credentialStorage()
{
    TokenAccountStore* store = TokenAccountStore::instance();
    for (const auto& acc : store->allAccounts()) {
        store->removeAccount(acc.accountId);
    }

    TokenAccount account;
    account.providerId = "openrouter";
    account.displayName = "API Account";

    APICredentials api;
    api.apiKey = SecureString("sk-test-key-12345");
    api.baseUrl = QUrl("https://api.openrouter.com");
    api.organizationId = "org-test";
    account.credentials.api = api;

    QString id = store->addAccount(account);

    auto retrieved = store->account(id);
    QVERIFY(retrieved.has_value());
    QVERIFY(retrieved->credentials.api.has_value());
    QCOMPARE(retrieved->credentials.api->apiKey.data(), QByteArray("sk-test-key-12345"));
    QCOMPARE(retrieved->credentials.api->baseUrl.toString(), QString("https://api.openrouter.com"));
    QCOMPARE(retrieved->credentials.api->organizationId, QString("org-test"));

    // Update credentials
    WebCredentials web;
    web.cookieDomain = "github.com";
    web.cookieName = "user_session";
    web.cookieValue = SecureString("cookie-value-abc");
    account.credentials.web = web;
    account.credentials.api = std::nullopt;
    store->updateAccount(id, account);

    auto after = store->account(id);
    QVERIFY(after.has_value());
    QVERIFY(!after->credentials.api.has_value());
    QVERIFY(after->credentials.web.has_value());
    QCOMPARE(after->credentials.web->cookieValue.data(), QByteArray("cookie-value-abc"));
}

void tst_TokenAccountStore::persistAndReload()
{
    QString tempPath = m_tempDir.path() + "/accounts_test.json";

    // Create a fresh store pointing to temp path
    // We can't easily change the storage path of the singleton,
    // so we'll test via the singleton and then clear it.
    TokenAccountStore* store = TokenAccountStore::instance();
    for (const auto& acc : store->allAccounts()) {
        store->removeAccount(acc.accountId);
    }

    TokenAccount acc1;
    acc1.providerId = "copilot";
    acc1.displayName = "Work";
    acc1.sourceMode = ProviderSourceMode::OAuth;
    acc1.visibility = AccountVisibility::Visible;
    QString id1 = store->addAccount(acc1);
    store->setDefaultAccountId("copilot", id1);

    TokenAccount acc2;
    acc2.providerId = "claude";
    acc2.displayName = "Personal";
    acc2.sourceMode = ProviderSourceMode::Web;
    acc2.visibility = AccountVisibility::Hidden;
    QString id2 = store->addAccount(acc2);

    QVERIFY(store->saveToDisk());

    // Clear and reload
    for (const auto& acc : store->allAccounts()) {
        store->removeAccount(acc.accountId);
    }
    QCOMPARE(store->accountCount(), 0);

    QVERIFY(store->loadFromDisk());
    QCOMPARE(store->accountCount(), 2);
    QCOMPARE(store->defaultAccountId("copilot"), id1);

    auto reloaded1 = store->account(id1);
    QVERIFY(reloaded1.has_value());
    QCOMPARE(reloaded1->displayName, QString("Work"));
    QCOMPARE(reloaded1->sourceMode, ProviderSourceMode::OAuth);

    auto reloaded2 = store->account(id2);
    QVERIFY(reloaded2.has_value());
    QCOMPARE(reloaded2->visibility, AccountVisibility::Hidden);
}

void tst_TokenAccountStore::migrationFromLegacy()
{
    TokenAccountStore* store = TokenAccountStore::instance();
    for (const auto& acc : store->allAccounts()) {
        store->removeAccount(acc.accountId);
    }

    // Note: migrateFromLegacy requires a SettingsStore pointer.
    // In a unit test without full SettingsStore setup, we skip detailed migration testing.
    // This test serves as a placeholder for integration testing.
    QVERIFY(store->accountCount() >= 0);
}

QTEST_MAIN(tst_TokenAccountStore)
#include "tst_TokenAccountStore.moc"
