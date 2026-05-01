#pragma once

#include "CodexAccountReconciliation.h"
#include "CodexDashboardAuthority.h"

#include <QString>
#include <QVector>

enum class CodexUsageDataSource {
    Auto,
    OAuth,
    CLI
};

struct CodexProviderSettingsBuilderInput {
    CodexUsageDataSource usageDataSource;
    QString cookieSource;
    QString manualCookieHeader;
    CodexAccountReconciliationSnapshot reconciliationSnapshot;
    CodexResolvedActiveSource resolvedActiveSource;
};

struct CodexProviderSettings {
    CodexUsageDataSource usageDataSource;
    QString cookieSource;
    QString manualCookieHeader;
    bool managedAccountStoreUnreadable;
    bool managedAccountTargetUnavailable;
    QVector<CodexDashboardKnownOwnerCandidate> dashboardAuthorityKnownOwners;
};

class CodexProviderSettingsBuilder {
public:
    static CodexProviderSettings make(const CodexProviderSettingsBuilderInput& input);
};

class CodexKnownOwnerCatalog {
public:
    static QVector<CodexDashboardKnownOwnerCandidate> candidates(
        const CodexAccountReconciliationSnapshot& snapshot);
};
