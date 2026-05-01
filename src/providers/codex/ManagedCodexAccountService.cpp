#include "ManagedCodexAccountService.h"
#include "CodexOAuthCredentials.h"
#include "CodexHomeScope.h"

#include <QDebug>
#include <QUuid>

ManagedCodexAccountService::ManagedCodexAccountService(const QHash<QString, QString>& env, QObject* parent)
    : QObject(parent)
    , m_env(env)
    , m_isAuthenticating(false)
    , m_isRemoving(false)
{
    refresh();
}

QVector<CodexVisibleAccount> ManagedCodexAccountService::visibleAccounts() const
{
    QVector<CodexVisibleAccount> accounts;

    // Add stored managed accounts
    for (const auto& stored : m_snapshot.storedAccounts) {
        CodexVisibleAccount visible;
        visible.id = stored.id;
        visible.email = stored.email;
        visible.workspaceLabel = stored.workspaceLabel;
        visible.isLive = false;
        visible.isActive = (stored.id == m_activeAccountID);
        visible.storedAccountID = stored.id;
        visible.displayName = resolveDisplayName(stored);
        accounts.append(visible);
    }

    // Add live system account if not already in stored accounts
    if (m_snapshot.liveSystemAccount.has_value()) {
        bool foundInStored = false;
        for (const auto& visible : accounts) {
            if (visible.isLive) {
                foundInStored = true;
                break;
            }
        }

        if (!foundInStored) {
            CodexVisibleAccount visible;
            visible.id = "live-system";
            visible.email = m_snapshot.liveSystemAccount->email;
            visible.workspaceLabel = m_snapshot.liveSystemAccount->workspaceLabel;
            visible.isLive = true;
            visible.isActive = (m_activeAccountID.isEmpty() || m_activeAccountID == "live-system");
            visible.storedAccountID = QString();
            visible.displayName = resolveDisplayName(*m_snapshot.liveSystemAccount);
            accounts.prepend(visible);
        }
    }

    return accounts;
}

QString ManagedCodexAccountService::activeAccountID() const
{
    if (m_activeAccountID.isEmpty()) {
        return "live-system";
    }
    return m_activeAccountID;
}

QString ManagedCodexAccountService::liveAccountID() const
{
    if (m_snapshot.liveSystemAccount.has_value()) {
        return "live-system";
    }
    return QString();
}

bool ManagedCodexAccountService::addAccount(const QString& email, const QString& homePath)
{
    if (m_isAuthenticating || m_isRemoving) {
        return false;
    }

    m_isAuthenticating = true;
    m_authenticatingAccountID = QString();
    emit authenticationStarted(QString());

    // Create managed account
    ManagedCodexAccount account = ManagedCodexAccount::create(email, homePath);
    
    // Try to load credentials from the home path
    QHash<QString, QString> scopedEnv = CodexHomeScope::scopedEnvironment(m_env, homePath);
    auto credentials = CodexOAuthCredentials::load(scopedEnv);
    
    if (credentials.has_value()) {
        account.providerAccountId = credentials->accountId;
        account.lastAuthenticatedAt = QDateTime::currentDateTime();
    }

    // Add to store
    m_store.addAccount(account);
    
    // Set as active
    m_activeAccountID = account.id;

    m_isAuthenticating = false;
    m_authenticatingAccountID = QString();
    
    refresh();
    
    emit authenticationFinished(account.id, true);
    emit accountsChanged();
    emit activeAccountChanged(account.id);
    
    return true;
}

bool ManagedCodexAccountService::removeAccount(const QString& accountID)
{
    if (m_isAuthenticating || m_isRemoving) {
        return false;
    }

    // Don't allow removing the live system account
    if (accountID == "live-system") {
        return false;
    }

    m_isRemoving = true;
    m_removingAccountID = accountID;
    emit removalStarted(accountID);

    // Remove from store
    m_store.removeAccount(accountID);

    // If removed account was active, switch to live system
    if (m_activeAccountID == accountID) {
        m_activeAccountID = "live-system";
        emit activeAccountChanged(m_activeAccountID);
    }

    m_isRemoving = false;
    m_removingAccountID = QString();

    refresh();

    emit removalFinished(accountID, true);
    emit accountsChanged();

    return true;
}

bool ManagedCodexAccountService::setActiveAccount(const QString& accountID)
{
    if (m_isAuthenticating || m_isRemoving) {
        return false;
    }

    if (m_activeAccountID == accountID) {
        return true;
    }

    m_activeAccountID = accountID;
    emit activeAccountChanged(accountID);
    
    return true;
}

bool ManagedCodexAccountService::reauthenticateAccount(const QString& accountID)
{
    if (m_isAuthenticating || m_isRemoving) {
        return false;
    }

    m_isAuthenticating = true;
    m_authenticatingAccountID = accountID;
    emit authenticationStarted(accountID);

    // Find the account
    auto account = m_store.account(accountID);
    if (!account.has_value()) {
        m_isAuthenticating = false;
        m_authenticatingAccountID = QString();
        emit authenticationFinished(accountID, false);
        return false;
    }

    // Try to load credentials
    QHash<QString, QString> scopedEnv = CodexHomeScope::scopedEnvironment(m_env, account->managedHomePath);
    auto credentials = CodexOAuthCredentials::load(scopedEnv);
    
    if (credentials.has_value()) {
        // Update account
        ManagedCodexAccount updated = *account;
        updated.providerAccountId = credentials->accountId;
        updated.lastAuthenticatedAt = QDateTime::currentDateTime();
        m_store.updateAccount(updated);
        
        m_isAuthenticating = false;
        m_authenticatingAccountID = QString();
        
        refresh();
        
        emit authenticationFinished(accountID, true);
        emit accountsChanged();
        return true;
    }

    m_isAuthenticating = false;
    m_authenticatingAccountID = QString();
    emit authenticationFinished(accountID, false);
    return false;
}

bool ManagedCodexAccountService::isAuthenticating() const
{
    return m_isAuthenticating;
}

bool ManagedCodexAccountService::isRemoving() const
{
    return m_isRemoving;
}

QString ManagedCodexAccountService::authenticatingAccountID() const
{
    return m_authenticatingAccountID;
}

QString ManagedCodexAccountService::removingAccountID() const
{
    return m_removingAccountID;
}

bool ManagedCodexAccountService::hasUnreadableStore() const
{
    return m_snapshot.hasUnreadableAddedAccountStore;
}

void ManagedCodexAccountService::refresh()
{
    CodexAccountReconciliation reconciliation(m_env);
    m_snapshot = reconciliation.loadSnapshot();
    
    // If no active account set, default to live system
    if (m_activeAccountID.isEmpty()) {
        m_activeAccountID = "live-system";
    }
    
    emit accountsChanged();
}

CodexAccountReconciliationSnapshot ManagedCodexAccountService::currentSnapshot() const
{
    return m_snapshot;
}

QString ManagedCodexAccountService::resolveDisplayName(const ManagedCodexAccount& account) const
{
    if (!account.workspaceLabel.isEmpty()) {
        return account.workspaceLabel + " (" + account.email + ")";
    }
    if (!account.email.isEmpty()) {
        return account.email;
    }
    return "Account " + account.id.left(8);
}

QString ManagedCodexAccountService::resolveDisplayName(const ObservedSystemCodexAccount& account) const
{
    if (!account.workspaceLabel.isEmpty()) {
        return account.workspaceLabel + " (" + account.email + ")";
    }
    if (!account.email.isEmpty()) {
        return account.email + " (System)";
    }
    return "System Account";
}
