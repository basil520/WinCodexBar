#include <QtTest/QtTest>

#include "../src/app/SettingsStore.h"
#include "../src/app/UsageStore.h"
#include "../src/providers/ProviderRegistry.h"
#include "../src/providers/claude/ClaudeProvider.h"
#include "../src/providers/codex/CodexProvider.h"
#include "../src/providers/zai/ZaiProvider.h"
#include "../src/providers/windsurf/WindsurfProvider.h"
#include "../src/providers/shared/ProviderCredentialStore.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QThread>
#include <QTemporaryDir>
#include <atomic>
#include <memory>

namespace {

QString managedAccountStorePath()
{
    return QDir::currentPath() + "/fetch-context-managed-codex-accounts.json";
}

} // namespace

class DisplaySettingsStrategy : public IFetchStrategy {
public:
    QString id() const override { return "display-test.strategy"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext&) const override { return true; }

    ProviderFetchResult fetchSync(const ProviderFetchContext&) override {
        UsageSnapshot snapshot;
        snapshot.updatedAt = QDateTime::fromString("2026-05-01T09:00:00", Qt::ISODate);

        RateWindow primary;
        primary.usedPercent = 25.0;
        primary.resetsAt = QDateTime::fromString("2026-05-01T12:34:00", Qt::ISODate);
        primary.resetDescription = "in 3 hours";
        snapshot.primary = primary;

        ProviderCostSnapshot providerCost;
        providerCost.used = 12.5;
        providerCost.limit = 50.0;
        providerCost.currencyCode = "USD";
        snapshot.providerCost = providerCost;

        ProviderFetchResult result;
        result.success = true;
        result.strategyID = id();
        result.strategyKind = kind();
        result.usage = snapshot;
        return result;
    }

    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override {
        return false;
    }
};

class DisplaySettingsProvider : public IProvider {
public:
    QString id() const override { return "display-test"; }
    QString displayName() const override { return "Display Test"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext&) override {
        return { new DisplaySettingsStrategy() };
    }

    QVector<QString> supportedSourceModes() const override { return {"api"}; }
};

class ConnectionLagStrategy : public IFetchStrategy {
public:
    QString id() const override { return "connection-lag.strategy"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext&) const override { return true; }

    ProviderFetchResult fetchSync(const ProviderFetchContext&) override {
        ProviderFetchResult result;
        result.success = true;
        result.strategyID = id();
        result.strategyKind = kind();
        UsageSnapshot snapshot;
        snapshot.updatedAt = QDateTime::currentDateTimeUtc();
        result.usage = snapshot;
        return result;
    }

    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override {
        return false;
    }
};

class ConnectionLagProvider : public IProvider {
public:
    QString id() const override { return "connection-lag"; }
    QString displayName() const override { return "Connection Lag"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    bool defaultEnabled() const override { return false; }

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"apiKey", "API Key", "secret", {}, {}, "com.codexbar.test.connection-lag",
             {}, "secret", "Stored in Windows Credential Manager", false, true}
        };
    }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext&) override {
        return { new ConnectionLagStrategy() };
    }

    QVector<QString> supportedSourceModes() const override { return {"api"}; }
};

class FastClaudeRefreshProvider : public IProvider {
public:
    QString id() const override { return "claude"; }
    QString displayName() const override { return "Claude Refresh Test"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext&) override {
        return { new ConnectionLagStrategy() };
    }

    QVector<QString> supportedSourceModes() const override { return {"api"}; }
};

class SlowReadCredentialBackend : public ProviderCredentialBackend {
public:
    bool write(const QString&, const QString&, const QByteArray&) override { return true; }

    std::optional<QByteArray> read(const QString&) override {
        ++readCount;
        QThread::msleep(220);
        return QByteArray("slow-secret");
    }

    bool remove(const QString&) override { return true; }
    bool exists(const QString&) override { return false; }

    int readCount = 0;
};

class SlowExistsCredentialBackend : public ProviderCredentialBackend {
public:
    bool write(const QString&, const QString&, const QByteArray&) override { return true; }
    std::optional<QByteArray> read(const QString&) override { return std::nullopt; }
    bool remove(const QString&) override { return true; }

    bool exists(const QString&) override {
        ++existsCount;
        QThread::msleep(220);
        return true;
    }

    std::atomic<int> existsCount = 0;
};

class tst_FetchContext : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        qputenv("CODEXBAR_MANAGED_CODEX_ACCOUNTS_PATH",
                QDir::toNativeSeparators(managedAccountStorePath()).toUtf8());
        UsageStore::rebuildSystemEnvCache();
        ProviderRegistry::instance().registerProvider(new ClaudeProvider());
        ProviderRegistry::instance().registerProvider(new CodexProvider());
        ProviderRegistry::instance().registerProvider(new ZaiProvider());
        ProviderRegistry::instance().registerProvider(new WindsurfProvider());
        ProviderRegistry::instance().registerProvider(new DisplaySettingsProvider());
        ProviderRegistry::instance().registerProvider(new ConnectionLagProvider());
    }

    void init() {
        ProviderCredentialStore::setBackendForTesting(std::make_shared<InMemoryCredentialBackend>());
        QFile::remove(managedAccountStorePath());
    }

    void cleanup() {
        ProviderCredentialStore::resetBackendForTesting();
        for (const auto& id : ProviderRegistry::instance().providerIDs()) {
            ProviderRegistry::instance().setProviderEnabled(id, false);
        }
        qunsetenv("Z_AI_API_KEY");
        qunsetenv("CODEX_HOME");
        qunsetenv("CLAUDE_CONFIG_DIR");
        UsageStore::rebuildSystemEnvCache();
        QFile::remove(managedAccountStorePath());
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
             + "/managed-codex-homes").removeRecursively();
    }

    void cleanupTestCase() {
        qunsetenv("CODEXBAR_MANAGED_CODEX_ACCOUNTS_PATH");
        QFile::remove(managedAccountStorePath());
    }

    void injectsEnvironmentAndZaiRegion() {
        qputenv("CODEXBAR_TEST_ENV", "present");
        UsageStore::rebuildSystemEnvCache();

        SettingsStore settings;
        settings.setProviderSetting("zai", "apiRegion", "bigmodelCN");

        UsageStore store;
        store.setSettingsStore(&settings);

        ProviderFetchContext ctx = store.buildFetchContextForProvider("zai");

        QCOMPARE(ctx.providerId, QString("zai"));
        QCOMPARE(ctx.env.value("CODEXBAR_TEST_ENV"), QString("present"));
        QCOMPARE(ctx.env.value("ZAI_API_REGION"), QString("bigmodelCN"));
        QCOMPARE(ctx.settings.get("apiRegion").toString(), QString("bigmodelCN"));
    }

    void rebuildSystemEnvCacheRefreshesFetchContextEnv() {
        qputenv("CODEXBAR_TEST_ENV", "updated-by-cache");
        UsageStore::rebuildSystemEnvCache();

        UsageStore store;
        ProviderFetchContext ctx = store.buildFetchContextForProvider("zai");

        QCOMPARE(ctx.env.value("CODEXBAR_TEST_ENV"), QString("updated-by-cache"));
    }

    void mapsSourceModeAndManualCookieSettings() {
        SettingsStore settings;
        settings.setProviderSetting("claude", "claudeDataSource", "web");
        settings.setProviderSetting("claude", "cookieSource", "manual");
        settings.setProviderSetting("claude", "manualCookieHeader", "sessionKey=sk-ant-test");
        settings.setProviderSetting("claude", "networkTimeoutMs", 1234);

        UsageStore store;
        store.setSettingsStore(&settings);

        ProviderFetchContext ctx = store.buildFetchContextForProvider("claude");

        QCOMPARE(ctx.sourceMode, ProviderSourceMode::Web);
        QCOMPARE(ctx.settings.get("sourceMode").toString(), QString("web"));
        QCOMPARE(ctx.settings.get("cookieSource").toString(), QString("manual"));
        QVERIFY(ctx.manualCookieHeader.has_value());
        QCOMPARE(*ctx.manualCookieHeader, QString("sessionKey=sk-ant-test"));
        QCOMPARE(ctx.networkTimeoutMs, 1234);
    }

    void readsManualCookieFromCredentialStore() {
        ProviderCredentialStore::write("com.codexbar.cookie.claude", {}, "sessionKey=sk-ant-credential");

        SettingsStore settings;
        UsageStore store;
        store.setSettingsStore(&settings);

        ProviderFetchContext ctx = store.buildFetchContextForProvider("claude");

        QVERIFY(ctx.manualCookieHeader.has_value());
        QCOMPARE(*ctx.manualCookieHeader, QString("sessionKey=sk-ant-credential"));
        QCOMPARE(ctx.settings.get("manualCookieHeader").toString(), QString("sessionKey=sk-ant-credential"));
    }

    void preservesProviderCookieSourceDefault() {
        SettingsStore settings;
        UsageStore store;
        store.setSettingsStore(&settings);

        ProviderFetchContext ctx = store.buildFetchContextForProvider("windsurf");

        QCOMPARE(ctx.settings.get("cookieSource").toString(), QString("off"));
        QVERIFY(!ctx.manualCookieHeader.has_value());
    }

    void codexActiveManagedAccountScopesFetchContext() {
        QTemporaryDir codexHome(QDir::currentPath() + "/fetch-context-codex-home-XXXXXX");
        QVERIFY(codexHome.isValid());

        SettingsStore settings;
        UsageStore store;
        store.setSettingsStore(&settings);

        QVERIFY(store.addCodexAccount("dev@example.com", codexHome.path()));

        QVariantMap state = store.codexAccountState();
        QCOMPARE(state.value("activeAccountID").toString(),
                 store.codexActiveAccountID());

        ProviderFetchContext ctx = store.buildFetchContextForProvider("codex");

        QCOMPARE(ctx.providerId, QString("codex"));
        QCOMPARE(ctx.env.value("CODEX_HOME"), codexHome.path());
        QCOMPARE(ctx.accountID, store.codexActiveAccountID());
    }

    void envSecretBeatsStoredCredential() {
        qputenv("Z_AI_API_KEY", "env-secret");
        UsageStore::rebuildSystemEnvCache();
        ProviderCredentialStore::write("com.codexbar.apikey.zai", {}, "stored-secret");

        SettingsStore settings;
        UsageStore store;
        store.setSettingsStore(&settings);

        ProviderFetchContext ctx = store.buildFetchContextForProvider("zai");

        QCOMPARE(ctx.settings.get("apiKey").toString(), QString("env-secret"));
    }

    void secretStatusReportsEnvAndCredentialSources() {
        SettingsStore settings;
        UsageStore store;
        store.setSettingsStore(&settings);

        ProviderCredentialStore::write("com.codexbar.apikey.zai", {}, "stored-secret");
        store.preloadCredentials();
        QVariantMap credentialStatus = store.providerSecretStatus("zai", "apiKey");
        QCOMPARE(credentialStatus.value("configured").toBool(), true);
        QCOMPARE(credentialStatus.value("source").toString(), QString("credential"));

        qputenv("Z_AI_API_KEY", "env-secret");
        UsageStore::rebuildSystemEnvCache();
        QVariantMap envStatus = store.providerSecretStatus("zai", "apiKey");
        QCOMPARE(envStatus.value("configured").toBool(), true);
        QCOMPARE(envStatus.value("source").toString(), QString("env"));
        QCOMPARE(envStatus.value("readOnly").toBool(), true);
    }

    void providerSecretStatusDoesNotBlockOnCredentialExists() {
        qunsetenv("Z_AI_API_KEY");
        UsageStore::rebuildSystemEnvCache();

        auto backend = std::make_shared<SlowExistsCredentialBackend>();
        ProviderCredentialStore::setBackendForTesting(backend);

        SettingsStore settings;
        UsageStore store;
        store.setSettingsStore(&settings);
        QSignalSpy secretSpy(&store, &UsageStore::providerSecretChanged);

        QElapsedTimer elapsed;
        elapsed.start();
        QVariantMap status = store.providerSecretStatus("zai", "apiKey");
        const qint64 returnedInMs = elapsed.elapsed();

        QVERIFY2(returnedInMs < 100,
                 qPrintable(QString("providerSecretStatus returned after %1 ms").arg(returnedInMs)));
        QCOMPARE(status.value("configured").toBool(), false);
        QCOMPARE(status.value("checking").toBool(), true);

        QTRY_VERIFY_WITH_TIMEOUT(backend->existsCount.load() > 0, 1000);
        QTRY_VERIFY_WITH_TIMEOUT(secretSpy.count() > 0, 1000);

        QVariantMap resolved = store.providerSecretStatus("zai", "apiKey");
        QCOMPARE(resolved.value("configured").toBool(), true);
        QCOMPARE(resolved.value("source").toString(), QString("credential"));
    }

    void descriptorUsesStringProviderId() {
        auto desc = ProviderRegistry::instance().descriptor("zai");
        QVERIFY(desc.has_value());
        QCOMPARE(desc->id, QString("zai"));
        QCOMPARE(desc->metadata.displayName, QString("z.ai"));
        QVERIFY(desc->fetchPlan.allowedSourceModes.contains("api"));
    }

    void displaySettingsAffectSnapshotData() {
        SettingsStore settings;
        settings.setUsageBarsShowUsed(false);
        settings.setResetTimesShowAbsolute(false);
        settings.setShowOptionalCreditsAndExtraUsage(true);

        UsageStore store;
        store.setSettingsStore(&settings);

        QSignalSpy spy(&store, &UsageStore::snapshotChanged);
        store.refreshProvider("display-test");
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, 5000);

        QVariantMap remainingData = store.snapshotData("display-test");
        QCOMPARE(remainingData.value("primaryDisplayPercent").toDouble(), 75.0);
        QCOMPARE(remainingData.value("primaryDisplayIsUsed").toBool(), false);
        QCOMPARE(remainingData.value("primaryResetDesc").toString(), QString("in 3 hours"));
        QCOMPARE(remainingData.value("hasProviderCost").toBool(), true);

        settings.setUsageBarsShowUsed(true);
        settings.setResetTimesShowAbsolute(true);
        QVariantMap usedAbsoluteData = store.snapshotData("display-test");
        QCOMPARE(usedAbsoluteData.value("primaryDisplayPercent").toDouble(), 25.0);
        QCOMPARE(usedAbsoluteData.value("primaryDisplayIsUsed").toBool(), true);
        QCOMPARE(usedAbsoluteData.value("primaryResetDesc").toString(),
                 QDateTime::fromString("2026-05-01T12:34:00", Qt::ISODate)
                    .toLocalTime()
                    .toString("yyyy-MM-dd hh:mm"));

        settings.setShowOptionalCreditsAndExtraUsage(false);
        QVariantMap hiddenExtrasData = store.snapshotData("display-test");
        QCOMPARE(hiddenExtrasData.value("hasProviderCost").toBool(), false);
        QVERIFY(!hiddenExtrasData.contains("providerCost"));
    }

    void displaySettingsNotifySnapshotConsumers() {
        SettingsStore settings;
        settings.setUsageBarsShowUsed(false);
        settings.setResetTimesShowAbsolute(false);
        settings.setShowOptionalCreditsAndExtraUsage(true);

        UsageStore store;
        store.setSettingsStore(&settings);

        QSignalSpy revisionSpy(&store, &UsageStore::snapshotRevisionChanged);

        settings.setUsageBarsShowUsed(true);
        QCOMPARE(revisionSpy.count(), 1);

        settings.setResetTimesShowAbsolute(true);
        QCOMPARE(revisionSpy.count(), 2);

        settings.setShowOptionalCreditsAndExtraUsage(false);
        QCOMPARE(revisionSpy.count(), 3);
        // snapshotChanged is no longer emitted for display setting changes;
        // TrayPanel/PlanUtilizationChart refresh via snapshotRevisionChanged binding.
    }

    void costUsageDoesNotStartWhenOnlyNonTokenProvidersAreEnabled() {
        SettingsStore settings;
        UsageStore store;
        store.setSettingsStore(&settings);
        store.setProviderEnabled("zai", true);

        QSignalSpy refreshingSpy(&store, &UsageStore::costUsageRefreshingChanged);

        store.setCostUsageEnabled(true);
        QTest::qWait(50);

        QCOMPARE(store.costUsageEnabled(), true);
        QCOMPARE(store.costUsageRefreshing(), false);
        QCOMPARE(refreshingSpy.count(), 0);
    }

    void providerRefreshDoesNotPiggybackCostUsageScan() {
        QTemporaryDir claudeHome(QDir::currentPath() + "/refresh-cost-claude-home-XXXXXX");
        QVERIFY(claudeHome.isValid());
        qputenv("CLAUDE_CONFIG_DIR", QDir::toNativeSeparators(claudeHome.path()).toUtf8());

        ProviderRegistry::instance().registerProvider(new FastClaudeRefreshProvider());

        SettingsStore settings;
        UsageStore store;
        store.setSettingsStore(&settings);
        store.setProviderEnabled("claude", true);

        QSignalSpy refreshingSpy(&store, &UsageStore::costUsageRefreshingChanged);
        store.setCostUsageEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(!store.costUsageRefreshing(), 3000);
        refreshingSpy.clear();

        store.refresh();
        QTest::qWait(50);

        QCOMPARE(store.costUsageRefreshing(), false);
        QCOMPARE(refreshingSpy.count(), 0);
    }

    void testProviderConnectionDoesNotBlockOnCredentialRead() {
        auto backend = std::make_shared<SlowReadCredentialBackend>();
        ProviderCredentialStore::setBackendForTesting(backend);

        UsageStore store;
        QSignalSpy connectionSpy(&store, &UsageStore::providerConnectionTestChanged);

        QElapsedTimer elapsed;
        elapsed.start();
        store.testProviderConnection("connection-lag");
        const qint64 returnedInMs = elapsed.elapsed();

        QVERIFY2(returnedInMs < 100,
                 qPrintable(QString("testProviderConnection returned after %1 ms").arg(returnedInMs)));
        QCOMPARE(connectionSpy.count(), 1);
        QCOMPARE(store.providerConnectionTest("connection-lag").value("state").toString(), QString("testing"));

        QTRY_VERIFY_WITH_TIMEOUT(connectionSpy.count() >= 2, 3000);
        QCOMPARE(store.providerConnectionTest("connection-lag").value("state").toString(), QString("succeeded"));
        QVERIFY(backend->readCount > 0);
    }
};

QTEST_MAIN(tst_FetchContext)
#include "tst_FetchContext.moc"
