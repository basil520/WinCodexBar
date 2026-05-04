#pragma once

#include "IProviderRuntime.h"
#include "../account/TokenAccountStore.h"

/**
 * @brief Runtime for Codex provider with dual-account support.
 *
 * Codex has two account tiers: 5-hour (fast, limited) and weekly (slow, generous).
 * This runtime manages switching between them and coordinates with
 * ManagedCodexAccountService for the actual account lifecycle.
 */
class CodexRuntime : public IProviderRuntime {
    Q_OBJECT
public:
    enum class AccountType {
        FiveHour,
        Weekly
    };

    explicit CodexRuntime(QObject* parent = nullptr);

    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;

    RuntimeState state() const override { return m_state; }
    QString lastError() const override { return m_lastError; }
    QDateTime lastFetchTime() const override { return m_lastFetchTime; }

    ProviderFetchResult fetch(const ProviderFetchContext& ctx) override;

    // Account switching
    void setAutoSwitchEnabled(bool enabled);
    bool autoSwitchEnabled() const { return m_autoSwitch; }
    AccountType currentAccountType() const { return m_currentAccountType; }
    void switchToAccount(AccountType type);

    // Resolve which account ID to use for a given type
    QString accountIdForType(AccountType type) const;

signals:
    void accountTypeChanged(AccountType type);

private:
    void evaluateAutoSwitch();
    bool shouldSwitchToWeekly() const;
    bool shouldSwitchToFiveHour() const;
    TokenAccount resolveActiveAccount() const;

    AccountType m_currentAccountType = AccountType::FiveHour;
    bool m_autoSwitch = true;
    QString m_fiveHourAccountId;
    QString m_weeklyAccountId;
};
