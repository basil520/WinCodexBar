#pragma once

#include "RateWindow.h"
#include "ProviderIdentitySnapshot.h"
#include "ProviderCostSnapshot.h"

#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <optional>

struct UsageSnapshot;

struct ClaudeOAuthWindow {
    std::optional<double> utilization;
    std::optional<QString> resetsAt;

    std::optional<QDateTime> resetsAtTime() const;
};

struct ClaudeExtraUsage {
    bool isEnabled = false;
    std::optional<double> monthlyLimit;
    std::optional<double> usedCredits;
    std::optional<double> utilization;
    std::optional<QString> currency;
};

struct ClaudeUsageSnapshot {
    ClaudeOAuthWindow fiveHour;
    ClaudeOAuthWindow sevenDay;
    std::optional<ClaudeOAuthWindow> sevenDayOpus;
    std::optional<ClaudeOAuthWindow> sevenDaySonnet;
    std::optional<ClaudeOAuthWindow> sevenDayDesign;
    std::optional<ClaudeOAuthWindow> sevenDayRoutines;
    std::optional<ClaudeOAuthWindow> sevenDayOAuthApps;
    std::optional<ClaudeOAuthWindow> iguanaNecktie;
    std::optional<ClaudeExtraUsage> extraUsage;

    std::optional<QString> accountEmail;
    std::optional<QString> loginMethod;
    std::optional<QString> accountOrganization;
    QDateTime updatedAt;

    bool isValid() const;
    UsageSnapshot toUsageSnapshot() const;

    static ClaudeUsageSnapshot fromOAuthJson(const QJsonObject& json);
    static ClaudeUsageSnapshot fromWebJson(const QJsonObject& json);
};

struct ClaudeOAuthCredentials {
    QString accessToken;
    QString refreshToken;
    std::optional<QDateTime> expiresAt;
    QVector<QString> scopes;
    std::optional<QString> rateLimitTier;

    bool isExpired() const;

    static std::optional<ClaudeOAuthCredentials> load(const QHash<QString, QString>& env);
    static std::optional<ClaudeOAuthCredentials> parse(const QJsonObject& json);
};

enum class ClaudePlan {
    Max,
    Pro,
    Team,
    Enterprise,
    Ultra,
    Unknown
};

QString claudePlanDisplayName(ClaudePlan plan);
ClaudePlan claudePlanFromRateLimitTier(const QString& tier);
