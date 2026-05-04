#include "CodexRuntime.h"
#include "../providers/ProviderPipeline.h"
#include "../providers/ProviderRegistry.h"
#include "../providers/codex/CodexProvider.h"
#include "../account/TokenAccountStore.h"
#include <QDebug>

CodexRuntime::CodexRuntime(QObject* parent)
    : IProviderRuntime(parent)
{
}

void CodexRuntime::start()
{
    // Discover Codex accounts from TokenAccountStore
    auto accounts = TokenAccountStore::instance()->accountsForProvider("codex");
    for (const auto& acc : accounts) {
        if (acc.displayName.contains("weekly", Qt::CaseInsensitive) ||
            acc.displayName.contains("slow", Qt::CaseInsensitive)) {
            m_weeklyAccountId = acc.accountId;
        } else {
            // Default to 5-hour for any non-weekly account
            m_fiveHourAccountId = acc.accountId;
        }
    }

    // If only one account exists, use it for both
    if (m_fiveHourAccountId.isEmpty() && !m_weeklyAccountId.isEmpty()) {
        m_fiveHourAccountId = m_weeklyAccountId;
    }
    if (m_weeklyAccountId.isEmpty() && !m_fiveHourAccountId.isEmpty()) {
        m_weeklyAccountId = m_fiveHourAccountId;
    }

    setState(RuntimeState::Running);
}

void CodexRuntime::stop()
{
    setState(RuntimeState::Stopped);
}

void CodexRuntime::pause()
{
    if (m_state == RuntimeState::Running) {
        setState(RuntimeState::Paused);
    }
}

void CodexRuntime::resume()
{
    if (m_state == RuntimeState::Paused) {
        setState(RuntimeState::Running);
    }
}

ProviderFetchResult CodexRuntime::fetch(const ProviderFetchContext& ctx)
{
    IProvider* provider = ProviderRegistry::instance().provider("codex");
    if (!provider) {
        recordFailure("Codex provider not found");
        emit fetchFailed(m_lastError);
        ProviderFetchResult r;
        r.success = false;
        r.errorMessage = m_lastError;
        return r;
    }

    if (m_state != RuntimeState::Running) {
        recordFailure("Runtime is not running");
        emit fetchFailed(m_lastError);
        ProviderFetchResult r;
        r.success = false;
        r.errorMessage = m_lastError;
        return r;
    }

    // Evaluate auto-switch before fetching
    if (m_autoSwitch) {
        evaluateAutoSwitch();
    }

    // Resolve active account and inject into context
    ProviderFetchContext modifiedCtx = ctx;
    TokenAccount activeAccount = resolveActiveAccount();
    if (activeAccount.isValid()) {
        modifiedCtx.accountID = activeAccount.accountId;
        modifiedCtx.accountCredentials = activeAccount.credentials;
    }

    ProviderPipeline pipeline;
    ProviderFetchResult result = pipeline.executeProvider(provider, modifiedCtx);

    if (result.success) {
        recordSuccess();
        emit fetchCompleted(result);
    } else {
        recordFailure(result.errorMessage.isEmpty() ? "Codex fetch failed" : result.errorMessage);
        emit fetchFailed(m_lastError);
    }
    return result;
}

void CodexRuntime::setAutoSwitchEnabled(bool enabled)
{
    m_autoSwitch = enabled;
}

void CodexRuntime::switchToAccount(AccountType type)
{
    if (m_currentAccountType != type) {
        m_currentAccountType = type;
        emit accountTypeChanged(type);
        emit accountChanged(accountIdForType(type));
    }
}

QString CodexRuntime::accountIdForType(AccountType type) const
{
    return (type == AccountType::Weekly) ? m_weeklyAccountId : m_fiveHourAccountId;
}

void CodexRuntime::evaluateAutoSwitch()
{
    // Placeholder for auto-switch logic.
    // Full implementation requires integration with usage snapshots
    // to detect when 5-hour quota is nearly exhausted.
    // For now, we maintain the current account unless explicitly switched.
}

bool CodexRuntime::shouldSwitchToWeekly() const
{
    // TODO: Implement based on 5-hour usage snapshot > 80%
    return false;
}

bool CodexRuntime::shouldSwitchToFiveHour() const
{
    // TODO: Implement based on 5-hour reset window
    return false;
}

TokenAccount CodexRuntime::resolveActiveAccount() const
{
    QString accountId = accountIdForType(m_currentAccountType);
    if (accountId.isEmpty()) {
        // Fallback to default account
        accountId = TokenAccountStore::instance()->defaultAccountId("codex");
    }
    if (!accountId.isEmpty()) {
        auto accOpt = TokenAccountStore::instance()->account(accountId);
        if (accOpt.has_value()) {
            return accOpt.value();
        }
    }
    return TokenAccount();
}
