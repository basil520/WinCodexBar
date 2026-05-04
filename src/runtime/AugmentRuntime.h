#pragma once

#include "IProviderRuntime.h"
#include <QTimer>

/**
 * @brief Runtime for Augment provider with session keepalive.
 *
 * Augment sessions expire after 7 days of inactivity.
 * This runtime periodically pings the API to keep the session alive
 * and refreshes cookies before expiry.
 */
class AugmentRuntime : public IProviderRuntime {
    Q_OBJECT
public:
    explicit AugmentRuntime(QObject* parent = nullptr);
    ~AugmentRuntime() override;

    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;

    RuntimeState state() const override { return m_state; }
    QString lastError() const override { return m_lastError; }
    QDateTime lastFetchTime() const override { return m_lastFetchTime; }

    ProviderFetchResult fetch(const ProviderFetchContext& ctx) override;

    // Keepalive configuration
    void setKeepaliveIntervalHours(int hours);
    int keepaliveIntervalHours() const { return m_keepaliveIntervalHours; }

private:
    void performKeepalive();
    bool refreshSessionCookie();

    QTimer* m_keepaliveTimer = nullptr;
    int m_keepaliveIntervalHours = 24;
    QDateTime m_sessionExpiry;
    int m_keepaliveFailures = 0;
    static constexpr int MAX_KEEPALIVE_FAILURES = 3;
};
