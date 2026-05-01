#pragma once

#include "CodexIdentity.h"

#include <QString>
#include <QSet>
#include <QVector>
#include <optional>

enum class CodexDashboardSourceKind {
    LiveWeb,
    CachedDashboard
};

enum class CodexDashboardDisposition {
    Attach,
    DisplayOnly,
    FailClosed
};

enum class CodexDashboardAllowedEffect {
    UsageBackfill,
    CreditsAttachment,
    RefreshGuardSeed,
    HistoricalBackfill,
    CachedDashboardReuse
};

enum class CodexDashboardCleanup {
    DashboardSnapshot,
    DashboardDerivedUsage,
    DashboardDerivedCredits,
    DashboardRefreshGuardSeed,
    DashboardCache
};

struct CodexDashboardKnownOwnerCandidate {
    CodexIdentity identity;
    QString normalizedEmail;

    bool operator==(const CodexDashboardKnownOwnerCandidate& other) const;
    bool operator!=(const CodexDashboardKnownOwnerCandidate& other) const;
};

uint qHash(const CodexDashboardKnownOwnerCandidate& candidate, uint seed = 0);

struct CodexDashboardOwnershipProofContext {
    CodexIdentity currentIdentity;
    QString expectedScopedEmail;
    QString trustedCurrentUsageEmail;
    QString dashboardSignedInEmail;
    QVector<CodexDashboardKnownOwnerCandidate> knownOwners;
};

struct CodexDashboardRoutingHints {
    QString targetEmail;
    QString lastKnownDashboardRoutingEmail;
};

struct CodexDashboardAuthorityInput {
    CodexDashboardSourceKind sourceKind;
    CodexDashboardOwnershipProofContext proof;
    CodexDashboardRoutingHints routing;
};

enum class CodexDashboardDecisionReason {
    ExactProviderAccountMatch,
    TrustedEmailMatchNoCompetingOwner,
    TrustedContinuityNoCompetingOwner,
    WrongEmail,
    SameEmailAmbiguity,
    UnresolvedWithoutTrustedEvidence,
    ProviderAccountMissingScopedEmail,
    ProviderAccountLacksExactOwnershipProof,
    MissingDashboardSignedInEmail
};

struct CodexDashboardDecisionReasonDetail {
    CodexDashboardDecisionReason reason;
    QString expectedEmail;
    QString actualEmail;
    QString ambiguousEmail;
};

struct CodexDashboardAuthorityDecision {
    CodexDashboardDisposition disposition;
    CodexDashboardDecisionReason reason;
    CodexDashboardDecisionReasonDetail reasonDetail;
    QSet<CodexDashboardAllowedEffect> allowedEffects;
    QSet<CodexDashboardCleanup> cleanup;
};

class CodexDashboardAuthority {
public:
    static CodexDashboardAuthorityDecision evaluate(const CodexDashboardAuthorityInput& input);

private:
    static CodexIdentity normalizeIdentity(const CodexIdentity& identity);
    static QSet<CodexDashboardKnownOwnerCandidate> normalizeKnownOwners(
        const QVector<CodexDashboardKnownOwnerCandidate>& candidates);
    static int knownOwnerCount(const QString& email,
                               const QSet<CodexDashboardKnownOwnerCandidate>& candidates);
    static CodexDashboardAuthorityDecision makeDecision(
        CodexDashboardDisposition disposition,
        CodexDashboardDecisionReason reason,
        CodexDashboardSourceKind sourceKind,
        const QString& expectedEmail = {},
        const QString& actualEmail = {},
        const QString& ambiguousEmail = {});
    static QSet<CodexDashboardAllowedEffect> allowedEffects(
        CodexDashboardDisposition disposition,
        CodexDashboardSourceKind sourceKind);
};
