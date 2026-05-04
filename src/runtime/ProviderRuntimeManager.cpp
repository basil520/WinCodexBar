#include "ProviderRuntimeManager.h"
#include "../providers/ProviderRegistry.h"
#include "../providers/IProvider.h"
#include <QDebug>

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
