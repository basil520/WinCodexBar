#pragma once

#include "ManagedCodexAccount.h"
#include "CodexAccountReconciliation.h"
#include "CodexSystemAccountObserver.h"
#include "CodexOpenAIWorkspaceResolver.h"
#include "CodexOpenAIWorkspaceIdentityCache.h"

#include <QObject>
#include <QString>
#include <QVector>
#include <QHash>
#include <QFuture>
#include <optional>

struct CodexVisibleAccount {
    QString id;
    QString displayName;
    QString email;
    QString workspaceLabel;
    bool isLive;
    bool isActive;
    QString storedAccountID;
};

class ManagedCodexAccountService : public QObject {
    Q_OBJECT

public:
    explicit ManagedCodexAccountService(const QHash<QString, QString>& env, QObject* parent = nullptr);

    // Account operations
    QVector<CodexVisibleAccount> visibleAccounts() const;
    QString activeAccountID() const;
    QString liveAccountID() const;

    // Account management
    bool addAccount(const QString& email, const QString& homePath);
    bool removeAccount(const QString& accountID);
    bool setActiveAccount(const QString& accountID);
    bool reauthenticateAccount(const QString& accountID);

    // State
    bool isAuthenticating() const;
    bool isRemoving() const;
    QString authenticatingAccountID() const;
    QString removingAccountID() const;
    bool hasUnreadableStore() const;

    // Reconciliation
    void refresh();
    CodexAccountReconciliationSnapshot currentSnapshot() const;

signals:
    void accountsChanged();
    void activeAccountChanged(const QString& accountID);
    void authenticationStarted(const QString& accountID);
    void authenticationFinished(const QString& accountID, bool success);
    void removalStarted(const QString& accountID);
    void removalFinished(const QString& accountID, bool success);

private:
    QHash<QString, QString> m_env;
    ManagedCodexAccountStore m_store;
    CodexSystemAccountObserver m_observer;
    CodexOpenAIWorkspaceIdentityCache m_workspaceCache;
    CodexAccountReconciliationSnapshot m_snapshot;
    QString m_activeAccountID;
    bool m_isAuthenticating;
    bool m_isRemoving;
    QString m_authenticatingAccountID;
    QString m_removingAccountID;

    void updateVisibleAccounts();
    QString resolveDisplayName(const ManagedCodexAccount& account) const;
    QString resolveDisplayName(const ObservedSystemCodexAccount& account) const;
};
