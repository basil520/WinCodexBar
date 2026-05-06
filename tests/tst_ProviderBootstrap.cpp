#include <QtTest/QtTest>

#include "providers/ProviderBootstrap.h"
#include "providers/ProviderRegistry.h"
#include "runtime/ProviderRuntimeManager.h"

class tst_ProviderBootstrap : public QObject {
    Q_OBJECT

private slots:
    void registersCodebuffProvider();
    void descriptorExposesProviderMetadata();
    void syncCreatesOnlyEnabledRuntimes();
};

void tst_ProviderBootstrap::registersCodebuffProvider()
{
    ProviderBootstrap::registerAllProviders();

    auto* provider = ProviderRegistry::instance().provider(QStringLiteral("codebuff"));
    QVERIFY(provider != nullptr);
    QCOMPARE(provider->displayName(), QStringLiteral("Codebuff"));

    auto descriptor = ProviderRegistry::instance().descriptor(QStringLiteral("codebuff"));
    QVERIFY(descriptor.has_value());
    QCOMPARE(descriptor->metadata.cliName, QStringLiteral("codebuff"));
    QVERIFY(descriptor->metadata.supportsCredits);

    auto* windsurf = ProviderRegistry::instance().provider(QStringLiteral("windsurf"));
    QVERIFY(windsurf != nullptr);
    QCOMPARE(windsurf->displayName(), QStringLiteral("Windsurf"));

    auto windsurfDescriptor = ProviderRegistry::instance().descriptor(QStringLiteral("windsurf"));
    QVERIFY(windsurfDescriptor.has_value());
    QCOMPARE(windsurfDescriptor->metadata.cliName, QStringLiteral("windsurf"));
    QVERIFY(windsurfDescriptor->fetchPlan.allowedSourceModes.contains(QStringLiteral("web")));
    QVERIFY(windsurfDescriptor->fetchPlan.allowedSourceModes.contains(QStringLiteral("cli")));
}

void tst_ProviderBootstrap::descriptorExposesProviderMetadata()
{
    ProviderBootstrap::registerAllProviders();

    auto codebuff = ProviderRegistry::instance().descriptor(QStringLiteral("codebuff"));
    QVERIFY(codebuff.has_value());
    QCOMPARE(codebuff->metadata.statusLinkURL, QString());
    QCOMPARE(codebuff->metadata.statusWorkspaceProductID, QString());
    QVERIFY(codebuff->tokenAccounts.supportsMultipleAccounts);
    QVERIFY(codebuff->tokenAccounts.requiredCredentialTypes.contains(QStringLiteral("apiKey")));
    QVERIFY(codebuff->fetchPlan.allowedSourceModes.contains(QStringLiteral("api")));

    auto alibaba = ProviderRegistry::instance().descriptor(QStringLiteral("alibaba"));
    QVERIFY(alibaba.has_value());
    QCOMPARE(alibaba->metadata.statusLinkURL, QStringLiteral("https://status.aliyun.com"));
}

void tst_ProviderBootstrap::syncCreatesOnlyEnabledRuntimes()
{
    ProviderBootstrap::registerAllProviders();
    auto* manager = ProviderRuntimeManager::instance();
    manager->unregisterRuntime(QStringLiteral("codebuff"));
    manager->unregisterRuntime(QStringLiteral("mistral"));

    ProviderRegistry::instance().setProviderEnabled(QStringLiteral("codebuff"), true);
    ProviderRegistry::instance().setProviderEnabled(QStringLiteral("mistral"), false);
    ProviderBootstrap::syncEnabledProviderRuntimes();

    QVERIFY(manager->hasRuntime(QStringLiteral("codebuff")));
    QVERIFY(!manager->hasRuntime(QStringLiteral("mistral")));
    QCOMPARE(manager->runtimeFor(QStringLiteral("codebuff"))->state(), RuntimeState::Running);

    manager->unregisterRuntime(QStringLiteral("codebuff"));
    ProviderRegistry::instance().setProviderEnabled(QStringLiteral("codebuff"), false);
}

QTEST_MAIN(tst_ProviderBootstrap)
#include "tst_ProviderBootstrap.moc"
