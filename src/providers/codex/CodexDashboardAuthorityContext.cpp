#include "CodexDashboardAuthorityContext.h"
#include "../ProviderFetchContext.h"

#include <QSettings>

ObservedSystemCodexAccount CodexDashboardAuthorityContext::loadAuthBackedAccount(
    const ProviderFetchContext& ctx)
{
    CodexSystemAccountObserver observer;
    auto account = observer.loadSystemAccount(ctx.env);
    return account.value_or(ObservedSystemCodexAccount{});
}

QVector<CodexDashboardKnownOwnerCandidate> CodexDashboardAuthorityContext::loadKnownOwners(
    const ProviderFetchContext& ctx)
{
    QVector<CodexDashboardKnownOwnerCandidate> owners;

    QSettings settings("HKEY_CURRENT_USER\\Software\\CodexBar", QSettings::NativeFormat);
    settings.beginGroup("codex");
    QStringList knownOwnerEmails = settings.value("dashboardAuthorityKnownOwners").toStringList();
    settings.endGroup();

    for (const auto& email : knownOwnerEmails) {
        QString normalized = CodexIdentityResolver::normalizeEmail(email);
        if (!normalized.isEmpty()) {
            CodexDashboardKnownOwnerCandidate candidate;
            candidate.identity = CodexIdentity::emailOnly(normalized);
            candidate.normalizedEmail = normalized;
            owners.append(candidate);
        }
    }

    return owners;
}

CodexDashboardAuthorityInput CodexDashboardAuthorityContext::makeLiveWebInput(
    const QString& dashboardSignedInEmail,
    const ProviderFetchContext& ctx,
    const std::optional<QString>& routingTargetEmail)
{
    auto auth = loadAuthBackedAccount(ctx);
    auto knownOwners = loadKnownOwners(ctx);

    CodexDashboardOwnershipProofContext proof;
    proof.currentIdentity = auth.identity;
    proof.expectedScopedEmail = auth.email;
    proof.trustedCurrentUsageEmail = {};
    proof.dashboardSignedInEmail = dashboardSignedInEmail;
    proof.knownOwners = knownOwners;

    CodexDashboardRoutingHints routing;
    routing.targetEmail = CodexIdentityResolver::normalizeEmail(
        routingTargetEmail.value_or(QString{}));
    routing.lastKnownDashboardRoutingEmail = {};

    CodexDashboardAuthorityInput input;
    input.sourceKind = CodexDashboardSourceKind::LiveWeb;
    input.proof = proof;
    input.routing = routing;
    return input;
}

CodexDashboardAuthorityInput CodexDashboardAuthorityContext::makeCachedDashboardInput(
    const QString& dashboardSignedInEmail,
    const QString& cachedAccountEmail,
    const QString& usageAccountEmail,
    const QString& sourceLabel,
    const ProviderFetchContext& ctx)
{
    auto auth = loadAuthBackedAccount(ctx);
    auto knownOwners = loadKnownOwners(ctx);

    QString trustedUsageEmail;
    if (shouldTrustUsageEmail(sourceLabel)) {
        trustedUsageEmail = usageAccountEmail;
    }

    CodexDashboardOwnershipProofContext proof;
    proof.currentIdentity = auth.identity;
    proof.expectedScopedEmail = auth.email;
    proof.trustedCurrentUsageEmail = trustedUsageEmail;
    proof.dashboardSignedInEmail = dashboardSignedInEmail;
    proof.knownOwners = knownOwners;

    CodexDashboardRoutingHints routing;
    routing.targetEmail = auth.email;
    routing.lastKnownDashboardRoutingEmail = cachedAccountEmail;

    CodexDashboardAuthorityInput input;
    input.sourceKind = CodexDashboardSourceKind::CachedDashboard;
    input.proof = proof;
    input.routing = routing;
    return input;
}

std::optional<QString> CodexDashboardAuthorityContext::attachmentEmail(
    const CodexDashboardAuthorityInput& input)
{
    QString email = input.proof.expectedScopedEmail;
    if (email.isEmpty()) email = input.proof.trustedCurrentUsageEmail;
    if (email.isEmpty()) email = input.proof.dashboardSignedInEmail;

    QString normalized = CodexIdentityResolver::normalizeEmail(email);
    if (normalized.isEmpty()) return std::nullopt;
    return normalized;
}

bool CodexDashboardAuthorityContext::shouldTrustUsageEmail(const QString& sourceLabel)
{
    QString lower = sourceLabel.trimmed().toLower();
    return lower == "codex-cli" || lower == "oauth";
}
