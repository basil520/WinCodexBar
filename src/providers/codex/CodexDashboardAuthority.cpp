#include "CodexDashboardAuthority.h"

bool CodexDashboardKnownOwnerCandidate::operator==(const CodexDashboardKnownOwnerCandidate& other) const
{
    return identity == other.identity && normalizedEmail == other.normalizedEmail;
}

bool CodexDashboardKnownOwnerCandidate::operator!=(const CodexDashboardKnownOwnerCandidate& other) const
{
    return !(*this == other);
}

uint qHash(const CodexDashboardKnownOwnerCandidate& candidate, uint seed)
{
    switch (candidate.identity.type()) {
    case CodexIdentityType::ProviderAccount:
        seed = qHash("providerAccount", seed);
        seed = qHash(candidate.identity.accountId(), seed);
        break;
    case CodexIdentityType::EmailOnly:
        seed = qHash("emailOnly", seed);
        seed = qHash(candidate.identity.email(), seed);
        break;
    case CodexIdentityType::Unresolved:
        seed = qHash("unresolved", seed);
        break;
    }
    seed = qHash(candidate.normalizedEmail, seed);
    return seed;
}

CodexDashboardAuthorityDecision CodexDashboardAuthority::evaluate(const CodexDashboardAuthorityInput& input)
{
    auto proof = input.proof;
    auto currentIdentity = normalizeIdentity(proof.currentIdentity);
    auto expectedScopedEmail = CodexIdentityResolver::normalizeEmail(proof.expectedScopedEmail);
    auto trustedCurrentUsageEmail = CodexIdentityResolver::normalizeEmail(proof.trustedCurrentUsageEmail);
    auto dashboardSignedInEmail = CodexIdentityResolver::normalizeEmail(proof.dashboardSignedInEmail);
    auto knownOwners = normalizeKnownOwners(proof.knownOwners);

    if (dashboardSignedInEmail.isEmpty()) {
        return makeDecision(CodexDashboardDisposition::FailClosed,
                           CodexDashboardDecisionReason::MissingDashboardSignedInEmail,
                           input.sourceKind);
    }

    if (!expectedScopedEmail.isEmpty() && dashboardSignedInEmail != expectedScopedEmail) {
        return makeDecision(CodexDashboardDisposition::FailClosed,
                           CodexDashboardDecisionReason::WrongEmail,
                           input.sourceKind);
    }

    switch (currentIdentity.type()) {
    case CodexIdentityType::ProviderAccount: {
        bool exactMatch = false;
        for (const auto& candidate : knownOwners) {
            if (candidate.identity == CodexIdentity::providerAccount(currentIdentity.accountId()) &&
                candidate.normalizedEmail == dashboardSignedInEmail) {
                exactMatch = true;
                break;
            }
        }
        if (exactMatch) {
            return makeDecision(CodexDashboardDisposition::Attach,
                               CodexDashboardDecisionReason::ExactProviderAccountMatch,
                               input.sourceKind);
        }
        if (expectedScopedEmail.isEmpty()) {
            return makeDecision(CodexDashboardDisposition::FailClosed,
                               CodexDashboardDecisionReason::ProviderAccountMissingScopedEmail,
                               input.sourceKind);
        }
        if (knownOwnerCount(dashboardSignedInEmail, knownOwners) > 1) {
            return makeDecision(CodexDashboardDisposition::DisplayOnly,
                               CodexDashboardDecisionReason::SameEmailAmbiguity,
                               input.sourceKind);
        }
        return makeDecision(CodexDashboardDisposition::FailClosed,
                           CodexDashboardDecisionReason::ProviderAccountLacksExactOwnershipProof,
                           input.sourceKind);
    }

    case CodexIdentityType::EmailOnly: {
        QString normalizedEmail = currentIdentity.email();
        if (dashboardSignedInEmail != normalizedEmail) {
            return makeDecision(CodexDashboardDisposition::FailClosed,
                               CodexDashboardDecisionReason::WrongEmail,
                               input.sourceKind);
        }
        if (knownOwnerCount(normalizedEmail, knownOwners) > 1) {
            return makeDecision(CodexDashboardDisposition::DisplayOnly,
                               CodexDashboardDecisionReason::SameEmailAmbiguity,
                               input.sourceKind);
        }
        return makeDecision(CodexDashboardDisposition::Attach,
                           CodexDashboardDecisionReason::TrustedEmailMatchNoCompetingOwner,
                           input.sourceKind);
    }

    case CodexIdentityType::Unresolved: {
        if (trustedCurrentUsageEmail.isEmpty()) {
            return makeDecision(CodexDashboardDisposition::FailClosed,
                               CodexDashboardDecisionReason::UnresolvedWithoutTrustedEvidence,
                               input.sourceKind);
        }
        if (dashboardSignedInEmail != trustedCurrentUsageEmail) {
            return makeDecision(CodexDashboardDisposition::FailClosed,
                               CodexDashboardDecisionReason::WrongEmail,
                               input.sourceKind);
        }
        if (knownOwnerCount(trustedCurrentUsageEmail, knownOwners) > 1) {
            return makeDecision(CodexDashboardDisposition::DisplayOnly,
                               CodexDashboardDecisionReason::SameEmailAmbiguity,
                               input.sourceKind);
        }
        return makeDecision(CodexDashboardDisposition::Attach,
                           CodexDashboardDecisionReason::TrustedContinuityNoCompetingOwner,
                           input.sourceKind);
    }
    }

    return makeDecision(CodexDashboardDisposition::FailClosed,
                       CodexDashboardDecisionReason::UnresolvedWithoutTrustedEvidence,
                       input.sourceKind);
}

CodexIdentity CodexDashboardAuthority::normalizeIdentity(const CodexIdentity& identity)
{
    switch (identity.type()) {
    case CodexIdentityType::ProviderAccount: {
        QString normalizedId = CodexIdentityResolver::normalizeAccountId(identity.accountId());
        if (!normalizedId.isEmpty()) {
            return CodexIdentity::providerAccount(normalizedId);
        }
        return CodexIdentity::unresolved();
    }
    case CodexIdentityType::EmailOnly: {
        QString normalizedEmail = CodexIdentityResolver::normalizeEmail(identity.email());
        if (!normalizedEmail.isEmpty()) {
            return CodexIdentity::emailOnly(normalizedEmail);
        }
        return CodexIdentity::unresolved();
    }
    case CodexIdentityType::Unresolved:
        return CodexIdentity::unresolved();
    }
    return CodexIdentity::unresolved();
}

QSet<CodexDashboardKnownOwnerCandidate> CodexDashboardAuthority::normalizeKnownOwners(
    const QVector<CodexDashboardKnownOwnerCandidate>& candidates)
{
    QSet<CodexDashboardKnownOwnerCandidate> result;
    for (const auto& candidate : candidates) {
        CodexDashboardKnownOwnerCandidate normalized;
        normalized.identity = normalizeIdentity(candidate.identity);
        normalized.normalizedEmail = CodexIdentityResolver::normalizeEmail(candidate.normalizedEmail);
        result.insert(normalized);
    }
    return result;
}

int CodexDashboardAuthority::knownOwnerCount(
    const QString& email,
    const QSet<CodexDashboardKnownOwnerCandidate>& candidates)
{
    int count = 0;
    for (const auto& candidate : candidates) {
        if (candidate.normalizedEmail == email) {
            count++;
        }
    }
    return count;
}

CodexDashboardAuthorityDecision CodexDashboardAuthority::makeDecision(
    CodexDashboardDisposition disposition,
    CodexDashboardDecisionReason reason,
    CodexDashboardSourceKind sourceKind)
{
    CodexDashboardAuthorityDecision decision;
    decision.disposition = disposition;
    decision.reason = reason;
    decision.allowedEffects = allowedEffects(disposition, sourceKind);
    if (disposition != CodexDashboardDisposition::Attach) {
        decision.cleanup = QSet<CodexDashboardCleanup>({
            CodexDashboardCleanup::DashboardSnapshot,
            CodexDashboardCleanup::DashboardDerivedUsage,
            CodexDashboardCleanup::DashboardDerivedCredits,
            CodexDashboardCleanup::DashboardRefreshGuardSeed,
            CodexDashboardCleanup::DashboardCache
        });
    }
    return decision;
}

QSet<CodexDashboardAllowedEffect> CodexDashboardAuthority::allowedEffects(
    CodexDashboardDisposition disposition,
    CodexDashboardSourceKind sourceKind)
{
    if (disposition != CodexDashboardDisposition::Attach) {
        return {};
    }

    switch (sourceKind) {
    case CodexDashboardSourceKind::LiveWeb:
        return {
            CodexDashboardAllowedEffect::UsageBackfill,
            CodexDashboardAllowedEffect::CreditsAttachment,
            CodexDashboardAllowedEffect::RefreshGuardSeed,
            CodexDashboardAllowedEffect::HistoricalBackfill
        };
    case CodexDashboardSourceKind::CachedDashboard:
        return {CodexDashboardAllowedEffect::CachedDashboardReuse};
    }

    return {};
}
