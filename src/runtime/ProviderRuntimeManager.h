#pragma once

#include <QHash>
#include <QTimer>
#include "IProviderRuntime.h"
#include "IProviderLoginFlow.h"

class ProviderRuntimeManager : public QObject {
    Q_OBJECT
public:
    static ProviderRuntimeManager* instance();

    void registerRuntime(const QString& providerId, IProviderRuntime* runtime);
    void unregisterRuntime(const QString& providerId);
    IProviderRuntime* runtimeFor(const QString& providerId) const;
    QStringList registeredRuntimeIds() const;
    bool hasRuntime(const QString& providerId) const;
    IProviderRuntime* ensureRuntimeForProvider(const QString& providerId);
    void setProviderRuntimeEnabled(const QString& providerId, bool enabled);
    void syncEnabledProviderRuntimes();

    // Global lifecycle
    void startAll();
    void stopAll();
    void pauseAll();
    void resumeAll();

    // Background refresh
    void setBackgroundRefreshInterval(int intervalMs);
    void startBackgroundRefresh();
    void stopBackgroundRefresh();
    bool isBackgroundRefreshRunning() const;

    // Login flows
    void registerLoginFlow(const QString& providerId, IProviderLoginFlow* flow);
    void unregisterLoginFlow(const QString& providerId);
    IProviderLoginFlow* loginFlowFor(const QString& providerId) const;

    // Status aggregation
    struct RuntimeStatus {
        QString providerId;
        RuntimeState state;
        QString lastError;
        QDateTime lastFetchTime;
    };
    QList<RuntimeStatus> allStatuses() const;

signals:
    void runtimeStateChanged(const QString& providerId, RuntimeState state);
    void backgroundRefreshTick();

private:
    explicit ProviderRuntimeManager(QObject* parent = nullptr);
    QHash<QString, IProviderRuntime*> m_runtimes;
    QHash<QString, IProviderLoginFlow*> m_loginFlows;
    QTimer* m_refreshTimer = nullptr;
    int m_refreshIntervalMs = 300000; // 5 minutes default
};
