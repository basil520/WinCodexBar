#pragma once

#include "ProviderIdentitySnapshot.h"
#include "RateWindow.h"

#include <QString>
#include <QDateTime>
#include <optional>

struct UsageSnapshot;

enum class OpenRouterKeyQuotaStatus : int {
    Available = 0,
    NoLimitConfigured = 1,
    Unavailable = 2,
};

struct OpenRouterRateLimit {
    int requests = 0;
    QString interval;
};

struct OpenRouterUsageSnapshot {
    double totalCredits = 0.0;
    double totalUsage = 0.0;
    double balance = 0.0;
    double usedPercent = 0.0;
    std::optional<double> keyLimit;
    std::optional<double> keyUsage;
    std::optional<OpenRouterRateLimit> rateLimit;
    QDateTime updatedAt;

    bool isValid() const;
    bool keyDataFetched() const;
    bool hasValidKeyQuota() const;
    OpenRouterKeyQuotaStatus keyQuotaStatus() const;
    double keyRemaining() const;
    double keyUsedPercent() const;

    UsageSnapshot toUsageSnapshot() const;
};
