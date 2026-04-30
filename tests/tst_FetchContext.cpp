#include <QtTest/QtTest>

#include "../src/app/SettingsStore.h"
#include "../src/app/UsageStore.h"
#include "../src/providers/ProviderRegistry.h"
#include "../src/providers/claude/ClaudeProvider.h"
#include "../src/providers/zai/ZaiProvider.h"
#include "../src/providers/shared/ProviderCredentialStore.h"

#include <memory>

class tst_FetchContext : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        ProviderRegistry::instance().registerProvider(new ClaudeProvider());
        ProviderRegistry::instance().registerProvider(new ZaiProvider());
    }

    void init() {
        ProviderCredentialStore::setBackendForTesting(std::make_shared<InMemoryCredentialBackend>());
    }

    void cleanup() {
        ProviderCredentialStore::resetBackendForTesting();
        qunsetenv("Z_AI_API_KEY");
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
};

QTEST_MAIN(tst_FetchContext)
#include "tst_FetchContext.moc"
