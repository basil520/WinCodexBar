#pragma once

#include <QObject>
#include <QDateTime>
#include "../providers/ProviderFetchResult.h"
#include "../providers/ProviderFetchContext.h"

enum class RuntimeState {
    Stopped,
    Starting,
    Running,
    Paused,
    Error
};

class IProviderRuntime : public QObject {
    Q_OBJECT
public:
    explicit IProviderRuntime(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IProviderRuntime() = default;

    // Lifecycle
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;

    // State
    virtual RuntimeState state() const = 0;
    virtual QString lastError() const = 0;
    virtual QDateTime lastFetchTime() const = 0;

    // Fetch (executes synchronously, emits signals for async listeners)
    virtual ProviderFetchResult fetch(const ProviderFetchContext& ctx) = 0;

    // Configuration
    virtual bool isEnabled() const { return m_enabled; }
    virtual void setEnabled(bool enabled) { m_enabled = enabled; }

    // Failure tracking
    virtual int consecutiveFailures() const { return m_consecutiveFailures; }
    virtual void resetFailureCount() { m_consecutiveFailures = 0; }
    virtual int currentBackoffMs() const;

signals:
    void stateChanged(RuntimeState newState);
    void fetchCompleted(const ProviderFetchResult& result);
    void fetchFailed(const QString& errorMessage);
    void accountChanged(const QString& accountId);

protected:
    void setState(RuntimeState state);
    void recordFailure(const QString& error);
    void recordSuccess();

    RuntimeState m_state = RuntimeState::Stopped;
    QString m_lastError;
    QDateTime m_lastFetchTime;
    bool m_enabled = true;
    int m_consecutiveFailures = 0;
    static constexpr int BASE_BACKOFF_MS = 30000;
    static constexpr int MAX_BACKOFF_MS = 300000;
};
