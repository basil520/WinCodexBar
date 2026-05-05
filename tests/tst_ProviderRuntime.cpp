#include <QtTest/QtTest>
#include "runtime/ProviderRuntimeManager.h"
#include "runtime/GenericRuntime.h"
#include "runtime/IProviderRuntime.h"
#include "providers/IProvider.h"
#include "providers/ProviderRegistry.h"
#include <utility>

class MockProvider : public IProvider {
    Q_OBJECT
public:
    explicit MockProvider(QString providerId = "mock", bool enabledByDefault = true)
        : m_providerId(std::move(providerId))
        , m_enabledByDefault(enabledByDefault)
    {
    }

    QString id() const override { return m_providerId; }
    QString displayName() const override { return QStringLiteral("Mock Provider %1").arg(m_providerId); }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext&) override { return {}; }
    bool defaultEnabled() const override { return m_enabledByDefault; }

private:
    QString m_providerId;
    bool m_enabledByDefault = true;
};

class tst_ProviderRuntime : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void genericRuntimeLifecycle();
    void runtimeManagerRegistration();
    void runtimeFetchResult();
    void failureBackoff();
    void runtimeManagerStartsOnlyEnabledProviders();
    void runtimeManagerStopsRuntimeWhenProviderDisabled();

private:
    MockProvider* m_mockProvider = nullptr;
    MockProvider* m_enabledProvider = nullptr;
    MockProvider* m_disabledProvider = nullptr;
};

void tst_ProviderRuntime::initTestCase()
{
    m_mockProvider = new MockProvider();
    ProviderRegistry::instance().registerProvider(m_mockProvider);
    m_enabledProvider = new MockProvider(QStringLiteral("runtime-enabled"), true);
    ProviderRegistry::instance().registerProvider(m_enabledProvider);
    m_disabledProvider = new MockProvider(QStringLiteral("runtime-disabled"), false);
    ProviderRegistry::instance().registerProvider(m_disabledProvider);
}

void tst_ProviderRuntime::cleanupTestCase()
{
    // Cleanup handled by registry destruction
}

void tst_ProviderRuntime::genericRuntimeLifecycle()
{
    GenericRuntime runtime(m_mockProvider);
    QCOMPARE(runtime.state(), RuntimeState::Stopped);

    runtime.start();
    QCOMPARE(runtime.state(), RuntimeState::Running);

    runtime.pause();
    QCOMPARE(runtime.state(), RuntimeState::Paused);

    runtime.resume();
    QCOMPARE(runtime.state(), RuntimeState::Running);

    runtime.stop();
    QCOMPARE(runtime.state(), RuntimeState::Stopped);
}

void tst_ProviderRuntime::runtimeManagerRegistration()
{
    ProviderRuntimeManager* manager = ProviderRuntimeManager::instance();
    QVERIFY(!manager->hasRuntime("mock"));

    GenericRuntime* runtime = new GenericRuntime(m_mockProvider);
    manager->registerRuntime("mock", runtime);
    QVERIFY(manager->hasRuntime("mock"));
    QCOMPARE(manager->runtimeFor("mock"), runtime);

    manager->unregisterRuntime("mock");
    QVERIFY(!manager->hasRuntime("mock"));
}

void tst_ProviderRuntime::runtimeFetchResult()
{
    GenericRuntime runtime(m_mockProvider);
    runtime.start();

    ProviderFetchContext ctx;
    ctx.providerId = "mock";
    ProviderFetchResult result = runtime.fetch(ctx);

    // Mock provider has no strategies, so fetch should fail
    QVERIFY(!result.success);
    QCOMPARE(runtime.consecutiveFailures(), 1);
}

void tst_ProviderRuntime::failureBackoff()
{
    GenericRuntime runtime(m_mockProvider);
    runtime.start();

    ProviderFetchContext ctx;
    ctx.providerId = "mock";

    // First failure
    runtime.fetch(ctx);
    QCOMPARE(runtime.consecutiveFailures(), 1);
    QVERIFY(runtime.currentBackoffMs() > 0);

    // Second failure
    runtime.fetch(ctx);
    QCOMPARE(runtime.consecutiveFailures(), 2);
    int backoff2 = runtime.currentBackoffMs();
    QVERIFY(backoff2 > runtime.currentBackoffMs() / 2); // Should be larger

    // Success resets
    runtime.resetFailureCount();
    QCOMPARE(runtime.consecutiveFailures(), 0);
    QCOMPARE(runtime.currentBackoffMs(), 0);
}

void tst_ProviderRuntime::runtimeManagerStartsOnlyEnabledProviders()
{
    ProviderRuntimeManager* manager = ProviderRuntimeManager::instance();
    manager->unregisterRuntime("runtime-enabled");
    manager->unregisterRuntime("runtime-disabled");

    ProviderRegistry::instance().setProviderEnabled("runtime-enabled", true);
    ProviderRegistry::instance().setProviderEnabled("runtime-disabled", false);

    manager->syncEnabledProviderRuntimes();

    QVERIFY(manager->hasRuntime("runtime-enabled"));
    QVERIFY(!manager->hasRuntime("runtime-disabled"));
    QCOMPARE(manager->runtimeFor("runtime-enabled")->state(), RuntimeState::Running);

    manager->unregisterRuntime("runtime-enabled");
}

void tst_ProviderRuntime::runtimeManagerStopsRuntimeWhenProviderDisabled()
{
    ProviderRuntimeManager* manager = ProviderRuntimeManager::instance();
    manager->unregisterRuntime("runtime-enabled");

    ProviderRegistry::instance().setProviderEnabled("runtime-enabled", true);
    manager->setProviderRuntimeEnabled("runtime-enabled", true);
    QVERIFY(manager->hasRuntime("runtime-enabled"));

    ProviderRegistry::instance().setProviderEnabled("runtime-enabled", false);
    manager->setProviderRuntimeEnabled("runtime-enabled", false);
    QVERIFY(manager->hasRuntime("runtime-enabled"));
    QCOMPARE(manager->runtimeFor("runtime-enabled")->state(), RuntimeState::Stopped);
    QVERIFY(!manager->runtimeFor("runtime-enabled")->isEnabled());

    manager->unregisterRuntime("runtime-enabled");
}

QTEST_MAIN(tst_ProviderRuntime)
#include "tst_ProviderRuntime.moc"
