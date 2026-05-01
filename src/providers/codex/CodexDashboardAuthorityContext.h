#pragma once

#include "CodexDashboardAuthority.h"
#include "CodexSystemAccountObserver.h"

#include <QString>
#include <QVector>
#include <optional>

struct ProviderFetchContext;

class CodexDashboardAuthorityContext {
public:
    static CodexDashboardAuthorityInput makeLiveWebInput(
        const QString& dashboardSignedInEmail,
        const ProviderFetchContext& ctx,
        const std::optional<QString>& routingTargetEmail);

    static CodexDashboardAuthorityInput makeCachedDashboardInput(
        const QString& dashboardSignedInEmail,
        const QString& cachedAccountEmail,
        const QString& usageAccountEmail,
        const QString& sourceLabel,
        const ProviderFetchContext& ctx);

    static std::optional<QString> attachmentEmail(const CodexDashboardAuthorityInput& input);

    static bool shouldTrustUsageEmail(const QString& sourceLabel);

private:
    static ObservedSystemCodexAccount loadAuthBackedAccount(const ProviderFetchContext& ctx);
    static QVector<CodexDashboardKnownOwnerCandidate> loadKnownOwners(const ProviderFetchContext& ctx);
};
