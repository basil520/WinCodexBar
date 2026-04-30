#pragma once

#include "ProviderIdentitySnapshot.h"
#include "RateWindow.h"

#include <QString>
#include <QDateTime>
#include <optional>

struct UsageSnapshot;

struct MiniMaxUsageSnapshot {
    std::optional<QString> planName;
    std::optional<int> availablePrompts;
    std::optional<int> currentPrompts;
    std::optional<int> remainingPrompts;
    std::optional<int> windowMinutes;
    std::optional<double> usedPercent;
    std::optional<QDateTime> resetsAt;
    QDateTime updatedAt;

    QString limitDescription() const;
    QString windowDescription() const;
    UsageSnapshot toUsageSnapshot() const;
};
