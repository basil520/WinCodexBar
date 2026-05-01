#include "CodexAccountReconciliation.h"
#include "CodexHomeScope.h"
#include "../../models/CodexUsageResponse.h"

CodexIdentity CodexAccountReconciliationSnapshot::runtimeIdentity(const ManagedCodexAccount& account) const
{
    auto it = storedAccountRuntimeIdentities.find(account.id);
    if (it != storedAccountRuntimeIdentities.end()) {
        return it.value();
    }
    return CodexIdentityResolver::resolve(QString(), account.email);
}

QString CodexAccountReconciliationSnapshot::runtimeEmail(const ManagedCodexAccount& account) const
{
    auto it = storedAccountRuntimeEmails.find(account.id);
    if (it != storedAccountRuntimeEmails.end()) {
        return it.value();
    }
    return ManagedCodexAccount::normalizeEmail(account.email);
}

CodexIdentity CodexAccountReconciliationSnapshot::runtimeIdentity(
    const ObservedSystemCodexAccount& account) const
{
    return CodexIdentityMatcher::normalized(account.identity, account.email);
}

CodexAccountReconciliation::CodexAccountReconciliation(const QHash<QString, QString>& env)
    : m_env(env)
{
}

CodexAccountReconciliationSnapshot CodexAccountReconciliation::loadSnapshot()
{
    auto liveSystemAccount = loadLiveSystemAccount();

    auto accounts = m_store.loadAccounts();
    QHash<QString, CodexIdentity> runtimeIdentities;
    QHash<QString, QString> runtimeEmails;

    for (const auto& account : accounts) {
        QHash<QString, QString> scopedEnv = account.managedHomePath.isEmpty()
            ? m_env
            : CodexHomeScope::scopedEnvironment(m_env, account.managedHomePath);
        auto credentials = CodexOAuthCredentials::load(scopedEnv);
        QString email = normalizeEmail(account.email);
        if (email.isEmpty() && credentials.has_value() && !credentials->idToken.isEmpty()) {
            QJsonObject payload = parseJWTPayload(credentials->idToken);
            email = payload.value("email").toString().trimmed();
            if (email.isEmpty()) {
                email = payload.value("https://api.openai.com/profile")
                            .toObject()
                            .value("email")
                            .toString()
                            .trimmed();
            }
            email = normalizeEmail(email);
        }
        CodexIdentity identity = CodexIdentityResolver::resolve(QString(), email);
        runtimeIdentities[account.id] = identity;
        runtimeEmails[account.id] = email;
    }

    CodexAccountReconciliationSnapshot snapshot;
    snapshot.storedAccounts = accounts;
    snapshot.activeStoredAccount = std::nullopt;
    snapshot.liveSystemAccount = liveSystemAccount;
    snapshot.activeSource = CodexActiveSource::LiveSystem;
    snapshot.hasUnreadableAddedAccountStore = false;
    snapshot.storedAccountRuntimeIdentities = runtimeIdentities;
    snapshot.storedAccountRuntimeEmails = runtimeEmails;

    if (liveSystemAccount.has_value()) {
        auto liveIdentity = snapshot.runtimeIdentity(*liveSystemAccount);
        for (const auto& account : accounts) {
            auto accountIdentity = runtimeIdentities.value(account.id);
            if (CodexIdentityMatcher::matches(accountIdentity, liveIdentity)) {
                snapshot.matchingStoredAccountForLiveSystemAccount = account;
                break;
            }
        }
    }

    return snapshot;
}

CodexResolvedActiveSource CodexAccountReconciliation::resolveActiveSource(
    const CodexAccountReconciliationSnapshot& snapshot)
{
    CodexResolvedActiveSource result;
    result.persistedSource = snapshot.activeSource;
    result.resolvedSource = snapshot.activeSource;

    if (snapshot.activeSource == CodexActiveSource::ManagedAccount) {
        if (snapshot.activeStoredAccount.has_value()) {
            auto storedIdentity = snapshot.runtimeIdentity(*snapshot.activeStoredAccount);
            if (snapshot.liveSystemAccount.has_value()) {
                auto liveIdentity = snapshot.runtimeIdentity(*snapshot.liveSystemAccount);
                if (CodexIdentityMatcher::matches(storedIdentity, liveIdentity)) {
                    result.resolvedSource = CodexActiveSource::LiveSystem;
                }
            }
        } else if (snapshot.liveSystemAccount.has_value()) {
            result.resolvedSource = CodexActiveSource::LiveSystem;
        }
    }

    return result;
}

std::optional<ObservedSystemCodexAccount> CodexAccountReconciliation::loadLiveSystemAccount()
{
    return m_observer.loadSystemAccount(m_env);
}

QString CodexAccountReconciliation::normalizeEmail(const QString& email)
{
    return email.trimmed().toLower();
}
