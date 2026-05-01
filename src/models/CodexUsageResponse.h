#pragma once

#include "RateWindow.h"
#include "ProviderIdentitySnapshot.h"

#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <optional>

struct UsageSnapshot;

enum class CodexPlanType : int {
    Guest = 0,
    Free,
    Go,
    Plus,
    Pro,
    FreeWorkspace,
    Team,
    Business,
    Education,
    Quorum,
    K12,
    Enterprise,
    Edu,
    Unknown
};

struct CodexWindowSnapshot {
    int usedPercent = 0;
    int resetAt = 0;
    int limitWindowSeconds = 0;

    std::optional<int> windowMinutes() const;
    std::optional<QDateTime> resetsAtTime() const;
    std::optional<QString> resetDescription() const;
};

struct CodexCreditDetails {
    bool hasCredits = false;
    bool unlimited = false;
    std::optional<double> balance;
};

struct CodexUsageResponse {
    std::optional<CodexPlanType> planType;
    std::optional<CodexWindowSnapshot> primaryWindow;
    std::optional<CodexWindowSnapshot> secondaryWindow;
    std::optional<CodexCreditDetails> credits;

    static CodexUsageResponse fromJson(const QJsonObject& json);
    UsageSnapshot toUsageSnapshot(const QString& accountEmail = {},
                                   const QString& planOverride = {}) const;
};

void normalizeCodexWindows(std::optional<CodexWindowSnapshot>& primary,
                           std::optional<CodexWindowSnapshot>& secondary);

CodexPlanType codexPlanTypeFromString(const QString& raw);
QString codexPlanTypeDisplayName(const QString& raw);

struct CodexOAuthCredentials {
    QString accessToken;
    QString refreshToken;
    QString idToken;
    QString accountId;
    std::optional<QDateTime> lastRefresh;

    bool needsRefresh() const;

    static std::optional<CodexOAuthCredentials> load(const QHash<QString, QString>& env);
    static std::optional<CodexOAuthCredentials> parse(const QJsonObject& json);
    bool save(const QHash<QString, QString>& env) const;
    static QString resolveAuthFilePath(const QHash<QString, QString>& env);
};

QJsonObject parseJWTPayload(const QString& jwt);
