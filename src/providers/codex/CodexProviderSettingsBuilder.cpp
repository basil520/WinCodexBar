#include "CodexProviderSettingsBuilder.h"

CodexProviderSettings CodexProviderSettingsBuilder::make(const CodexProviderSettingsBuilderInput& input)
{
    auto snapshot = input.reconciliationSnapshot;
    auto persistedSource = input.resolvedActiveSource.persistedSource;
    bool managedSourceSelected = (persistedSource == CodexActiveSource::ManagedAccount);

    CodexProviderSettings settings;
    settings.usageDataSource = input.usageDataSource;
    settings.cookieSource = input.cookieSource;
    settings.manualCookieHeader = input.manualCookieHeader;
    settings.managedAccountStoreUnreadable = managedSourceSelected && snapshot.hasUnreadableAddedAccountStore;
    settings.managedAccountTargetUnavailable = managedSourceSelected
        && !snapshot.hasUnreadableAddedAccountStore
        && !snapshot.activeStoredAccount.has_value();
    settings.dashboardAuthorityKnownOwners = CodexKnownOwnerCatalog::candidates(snapshot);

    return settings;
}

QVector<CodexDashboardKnownOwnerCandidate> CodexKnownOwnerCatalog::candidates(
    const CodexAccountReconciliationSnapshot& snapshot)
{
    QVector<CodexDashboardKnownOwnerCandidate> result;

    for (const auto& account : snapshot.storedAccounts) {
        CodexDashboardKnownOwnerCandidate candidate;
        candidate.identity = snapshot.runtimeIdentity(account);
        candidate.normalizedEmail = CodexIdentityResolver::normalizeEmail(snapshot.runtimeEmail(account));
        result.append(candidate);
    }

    if (snapshot.liveSystemAccount.has_value()) {
        CodexDashboardKnownOwnerCandidate candidate;
        candidate.identity = snapshot.runtimeIdentity(*snapshot.liveSystemAccount);
        candidate.normalizedEmail = CodexIdentityResolver::normalizeEmail(snapshot.liveSystemAccount->email);
        result.append(candidate);
    }

    return result;
}
