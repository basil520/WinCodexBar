#include "ProviderRuntimeManager.h"
#include "AugmentRuntime.h"
#include "CodexRuntime.h"
#include "GenericRuntime.h"
#include "../providers/ProviderRegistry.h"
#include "../providers/IProvider.h"
#include <QDebug>
#include <QSet>

ProviderRuntimeManager::ProviderRuntimeManager(QObject* parent)
    : QObject(parent)
{
}

ProviderRuntimeManager* ProviderRuntimeManager::instance()
{
    static ProviderRuntimeManager* s_instance = new ProviderRuntimeManager();
    return s_instance;
}

void ProviderRuntimeManager::registerRuntime(const QString& providerId, IProviderRuntime* runtime)
{
    if (!runtime) return;
    if (m_runtimes.contains(providerId)) {
        qWarning() << "ProviderRuntimeManager: runtime already registered for" << providerId;
        return;
    }
    m_runtimes[providerId] = runtime;
    connect(runtime, &IProviderRuntime::stateChanged, this, [this, providerId](RuntimeState state) {
        emit runtimeStateChanged(providerId, state);
    });
}

void ProviderRuntimeManager::unregisterRuntime(const QString& providerId)
{
    auto it = m_runtimes.find(providerId);
    if (it != m_runtimes.end()) {
        it.value()->deleteLater();
        m_runtimes.erase(it);
    }
}

IProviderRuntime* ProviderRuntimeManager::runtimeFor(const QString& providerId) const
{
    return m_runtimes.value(providerId, nullptr);
}

QStringList ProviderRuntimeManager::registeredRuntimeIds() const
{
    return m_runtimes.keys();
}

bool ProviderRuntimeManager::hasRuntime(const QString& providerId) const
{
    return m_runtimes.contains(providerId);
}

IProviderRuntime* ProviderRuntimeManager::ensureRuntimeForProvider(const QString& providerId)
{
    if (providerId.isEmpty()) {
        return nullptr;
    }

    if (auto* existing = runtimeFor(providerId)) {
        return existing;
    }

    IProviderRuntime* runtime = nullptr;
    if (providerId == QLatin1String("codex")) {
        runtime = new CodexRuntime();
    } else if (providerId == QLatin1String("augment")) {
        runtime = new AugmentRuntime();
    } else if (auto* provider = ProviderRegistry::instance().provider(providerId)) {
        runtime = new GenericRuntime(provider);
    }

    if (!runtime) {
        qWarning() << "ProviderRuntimeManager: no provider registered for runtime" << providerId;
        return nullptr;
    }

    registerRuntime(providerId, runtime);
    return runtime;
}

void ProviderRuntimeManager::setProviderRuntimeEnabled(const QString& providerId, bool enabled)
{
    if (enabled) {
        IProviderRuntime* runtime = ensureRuntimeForProvider(providerId);
        if (!runtime) {
            return;
        }

        runtime->setEnabled(true);
        if (runtime->state() == RuntimeState::Stopped) {
            runtime->start();
        } else if (runtime->state() == RuntimeState::Paused) {
            runtime->resume();
        }
        return;
    }

    IProviderRuntime* runtime = runtimeFor(providerId);
    if (!runtime) {
        return;
    }

    runtime->setEnabled(false);
    runtime->stop();
}

void ProviderRuntimeManager::syncEnabledProviderRuntimes()
{
    const QVector<QString> enabledIds = ProviderRegistry::instance().enabledProviderIDs();
    QSet<QString> enabled;
    enabled.reserve(enabledIds.size());
    for (const QString& providerId : enabledIds) {
        enabled.insert(providerId);
    }

    const QStringList registeredIds = registeredRuntimeIds();
    for (const QString& providerId : registeredIds) {
        if (!enabled.contains(providerId)) {
            setProviderRuntimeEnabled(providerId, false);
        }
    }

    for (const QString& providerId : enabledIds) {
        setProviderRuntimeEnabled(providerId, true);
    }
}

void ProviderRuntimeManager::startAll()
{
    for (auto* runtime : m_runtimes) {
        if (runtime && runtime->isEnabled()) {
            runtime->start();
        }
    }
}

void ProviderRuntimeManager::stopAll()
{
    for (auto* runtime : m_runtimes) {
        if (runtime) {
            runtime->stop();
        }
    }
}

void ProviderRuntimeManager::pauseAll()
{
    for (auto* runtime : m_runtimes) {
        if (runtime) {
            runtime->pause();
        }
    }
}

void ProviderRuntimeManager::resumeAll()
{
    for (auto* runtime : m_runtimes) {
        if (runtime) {
            runtime->resume();
        }
    }
}

void ProviderRuntimeManager::setBackgroundRefreshInterval(int intervalMs)
{
    m_refreshIntervalMs = qMax(10000, intervalMs); // Min 10 seconds
    if (m_refreshTimer && m_refreshTimer->isActive()) {
        m_refreshTimer->setInterval(m_refreshIntervalMs);
    }
}

void ProviderRuntimeManager::startBackgroundRefresh()
{
    if (!m_refreshTimer) {
        m_refreshTimer = new QTimer(this);
        connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
            emit backgroundRefreshTick();
            for (auto it = m_runtimes.begin(); it != m_runtimes.end(); ++it) {
                IProviderRuntime* runtime = it.value();
                if (runtime && runtime->isEnabled() && runtime->state() == RuntimeState::Running) {
                    ProviderFetchContext ctx;
                    ctx.providerId = it.key();
                    ctx.isAppRuntime = true;
                    ctx.allowInteractiveAuth = false;
                    runtime->fetch(ctx);
                }
            }
        });
    }
    m_refreshTimer->start(m_refreshIntervalMs);
}

void ProviderRuntimeManager::stopBackgroundRefresh()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

bool ProviderRuntimeManager::isBackgroundRefreshRunning() const
{
    return m_refreshTimer && m_refreshTimer->isActive();
}

void ProviderRuntimeManager::registerLoginFlow(const QString& providerId, IProviderLoginFlow* flow)
{
    if (!flow) return;
    if (m_loginFlows.contains(providerId)) {
        m_loginFlows[providerId]->deleteLater();
    }
    m_loginFlows[providerId] = flow;
}

void ProviderRuntimeManager::unregisterLoginFlow(const QString& providerId)
{
    auto it = m_loginFlows.find(providerId);
    if (it != m_loginFlows.end()) {
        it.value()->deleteLater();
        m_loginFlows.erase(it);
    }
}

IProviderLoginFlow* ProviderRuntimeManager::loginFlowFor(const QString& providerId) const
{
    return m_loginFlows.value(providerId, nullptr);
}

QList<ProviderRuntimeManager::RuntimeStatus> ProviderRuntimeManager::allStatuses() const
{
    QList<RuntimeStatus> result;
    for (auto it = m_runtimes.constBegin(); it != m_runtimes.constEnd(); ++it) {
        IProviderRuntime* runtime = it.value();
        if (runtime) {
            RuntimeStatus status;
            status.providerId = it.key();
            status.state = runtime->state();
            status.lastError = runtime->lastError();
            status.lastFetchTime = runtime->lastFetchTime();
            result.append(status);
        }
    }
    return result;
}
