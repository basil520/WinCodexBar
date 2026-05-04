#include <QtTest/QtTest>

#include "../src/app/SettingsStore.h"
#include "../src/app/UsageStore.h"
#include "../src/providers/ProviderRegistry.h"
#include "../src/providers/claude/ClaudeProvider.h"
#include "../src/providers/codex/CodexProvider.h"
#include "../src/providers/zai/ZaiProvider.h"
#include "../src/providers/shared/ProviderCredentialStore.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
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

class tst_FetchContext : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        qputenv("CODEXBAR_MANAGED_CODEX_ACCOUNTS_PATH",
                QDir::toNativeSeparators(managedAccountStorePath()).toUtf8());
        ProviderRegistry::instance().registerProvider(new ClaudeProvider());
        ProviderRegistry::instance().registerProvider(new CodexProvider());
        ProviderRegistry::instance().registerProvider(new ZaiProvider());
        ProviderRegistry::instance().registerProvider(new DisplaySettingsProvider());
    }

    void init() {
        ProviderCredentialStore::setBackendForTesting(std::make_shared<InMemoryCredentialBackend>());
        QFile::remove(managedAccountStorePath());
    }

    void cleanup() {
        ProviderCredentialStore::resetBackendForTesting();
        qunsetenv("Z_AI_API_KEY");
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
        QVariantMap credentialStatus = store.providerSecretStatus("zai", "apiKey");
        QCOMPARE(credentialStatus.value("configured").toBool(), true);
        QCOMPARE(credentialStatus.value("source").toString(), QString("credential"));

        qputenv("Z_AI_API_KEY", "env-secret");
        QVariantMap envStatus = store.providerSecretStatus("zai", "apiKey");
        QCOMPARE(envStatus.value("configured").toBool(), true);
        QCOMPARE(envStatus.value("source").toString(), QString("env"));
        QCOMPARE(envStatus.value("readOnly").toBool(), true);
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
};

QTEST_MAIN(tst_FetchContext)
#include "tst_FetchContext.moc"
