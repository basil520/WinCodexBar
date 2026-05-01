#pragma once

#include "../../models/RateWindow.h"
#include "../../models/ProviderIdentitySnapshot.h"
#include "../../models/UsageSnapshot.h"
#include "CodexIdentity.h"
#include "CodexRateWindowNormalizer.h"

#include <QDateTime>
#include <optional>

class CodexUsageResponse;
class CodexOAuthCredentials;

class CodexReconciledState {
public:
    std::optional<RateWindow> session;
    std::optional<RateWindow> weekly;
    std::optional<ProviderIdentitySnapshot> identity;
    QDateTime updatedAt;

    CodexReconciledState();

    static std::optional<CodexReconciledState> fromCLI(
        const std::optional<RateWindow>& primary,
        const std::optional<RateWindow>& secondary,
        const std::optional<ProviderIdentitySnapshot>& identity,
        const QDateTime& updatedAt = QDateTime::currentDateTime());

    static std::optional<CodexReconciledState> fromOAuth(
        const CodexUsageResponse& response,
        const CodexOAuthCredentials& credentials,
        const QDateTime& updatedAt = QDateTime::currentDateTime());

    UsageSnapshot toUsageSnapshot() const;

    static ProviderIdentitySnapshot oauthIdentity(
        const CodexUsageResponse& response,
        const CodexOAuthCredentials& credentials);

private:
    static std::optional<CodexReconciledState> make(
        const std::optional<RateWindow>& primary,
        const std::optional<RateWindow>& secondary,
        const std::optional<ProviderIdentitySnapshot>& identity,
        const QDateTime& updatedAt);

    static std::optional<RateWindow> makeWindow(int usedPercent, int resetAt, int limitWindowSeconds);

    static QString resolveAccountEmail(const CodexOAuthCredentials& credentials);
    static QString resolvePlan(const CodexUsageResponse& response, const CodexOAuthCredentials& credentials);
};
