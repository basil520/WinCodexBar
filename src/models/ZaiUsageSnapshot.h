#pragma once

#include "RateWindow.h"
#include "ProviderIdentitySnapshot.h"

#include <QString>
#include <QDateTime>
#include <QVector>
#include <optional>

struct UsageSnapshot;

enum class ZaiLimitType : int {
    Unknown = 0,
    TimeLimit = 1,
    TokensLimit = 2,
};

enum class ZaiLimitUnit : int {
    Unknown = 0,
    Days = 1,
    Hours = 3,
    Minutes = 5,
    Weeks = 6,
};

struct ZaiUsageDetail {
    QString modelCode;
    int usage = 0;
};

struct ZaiLimitEntry {
    ZaiLimitType type = ZaiLimitType::Unknown;
    ZaiLimitUnit unit = ZaiLimitUnit::Unknown;
    int number = 0;
    std::optional<int> usage;
    std::optional<int> currentValue;
    std::optional<int> remaining;
    double percentage = 0.0;
    QVector<ZaiUsageDetail> usageDetails;
    std::optional<QDateTime> nextResetTime;

    double usedPercent() const;
    std::optional<int> windowMinutes() const;
    QString windowDescription() const;
    QString windowLabel() const;
};

struct ZaiUsageSnapshot {
    std::optional<ZaiLimitEntry> tokenLimit;
    std::optional<ZaiLimitEntry> sessionTokenLimit;
    std::optional<ZaiLimitEntry> timeLimit;
    std::optional<QString> planName;
    QDateTime updatedAt;

    bool isValid() const;
    UsageSnapshot toUsageSnapshot() const;
};
