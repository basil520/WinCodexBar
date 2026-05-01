#pragma once

#include "CodexIdentity.h"
#include "ManagedCodexAccount.h"
#include "CodexSystemAccountObserver.h"

#include <QVector>
#include <QHash>
#include <optional>

enum class CodexActiveSource {
    LiveSystem,
    ManagedAccount
};

struct CodexResolvedActiveSource {
    CodexActiveSource persistedSource;
    CodexActiveSource resolvedSource;

    bool requiresPersistenceCorrection() const { return persistedSource != resolvedSource; }
};

struct CodexAccountReconciliationSnapshot {
    QVector<ManagedCodexAccount> storedAccounts;
    std::optional<ManagedCodexAccount> activeStoredAccount;
    std::optional<ObservedSystemCodexAccount> liveSystemAccount;
    std::optional<ManagedCodexAccount> matchingStoredAccountForLiveSystemAccount;
    CodexActiveSource activeSource;
    bool hasUnreadableAddedAccountStore;
    QHash<QString, CodexIdentity> storedAccountRuntimeIdentities;
    QHash<QString, QString> storedAccountRuntimeEmails;

    CodexIdentity runtimeIdentity(const ManagedCodexAccount& account) const;
    QString runtimeEmail(const ManagedCodexAccount& account) const;
    CodexIdentity runtimeIdentity(const ObservedSystemCodexAccount& account) const;
};

class CodexAccountReconciliation {
public:
    CodexAccountReconciliation(const QHash<QString, QString>& env);

    CodexAccountReconciliationSnapshot loadSnapshot();
    CodexResolvedActiveSource resolveActiveSource(const CodexAccountReconciliationSnapshot& snapshot);

private:
    QHash<QString, QString> m_env;
    ManagedCodexAccountStore m_store;
    CodexSystemAccountObserver m_observer;

    std::optional<ObservedSystemCodexAccount> loadLiveSystemAccount();
    static QString normalizeEmail(const QString& email);
};
