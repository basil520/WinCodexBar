#include "ZaiUsageSnapshot.h"
#include "UsageSnapshot.h"

#include <cmath>

double ZaiLimitEntry::usedPercent() const {
    if (usage.has_value() && remaining.has_value() && currentValue.has_value()) {
        int total = *usage + *remaining;
        if (total > 0) return (static_cast<double>(*currentValue) / total) * 100.0;
    }
    return percentage;
}

std::optional<int> ZaiLimitEntry::windowMinutes() const {
    switch (unit) {
    case ZaiLimitUnit::Days:    return number * 1440;
    case ZaiLimitUnit::Hours:   return number * 60;
    case ZaiLimitUnit::Minutes: return number;
    case ZaiLimitUnit::Weeks:   return number * 10080;
    default: return std::nullopt;
    }
}

QString ZaiLimitEntry::windowDescription() const {
    switch (unit) {
    case ZaiLimitUnit::Days:    return QString("%1 day%2").arg(number).arg(number != 1 ? "s" : "");
    case ZaiLimitUnit::Hours:   return QString("%1 hour%2").arg(number).arg(number != 1 ? "s" : "");
    case ZaiLimitUnit::Minutes: return QString("%1 minute%2").arg(number).arg(number != 1 ? "s" : "");
    case ZaiLimitUnit::Weeks:   return QString("%1 week%2").arg(number).arg(number != 1 ? "s" : "");
    default: return {};
    }
}

QString ZaiLimitEntry::windowLabel() const {
    QString desc = windowDescription();
    return desc.isEmpty() ? QString() : desc + " window";
}

bool ZaiUsageSnapshot::isValid() const {
    return tokenLimit.has_value() || timeLimit.has_value();
}

UsageSnapshot ZaiUsageSnapshot::toUsageSnapshot() const {
    UsageSnapshot snap;

    if (tokenLimit.has_value()) {
        snap.primary = RateWindow{
            tokenLimit->usedPercent(),
            tokenLimit->windowMinutes(),
            tokenLimit->nextResetTime,
            tokenLimit->windowLabel(),
            std::nullopt
        };
        if (timeLimit.has_value()) {
            snap.secondary = RateWindow{
                timeLimit->usedPercent(),
                timeLimit->windowMinutes(),
                timeLimit->nextResetTime,
                timeLimit->windowLabel(),
                std::nullopt
            };
        }
    } else if (timeLimit.has_value()) {
        snap.primary = RateWindow{
            timeLimit->usedPercent(),
            timeLimit->windowMinutes(),
            timeLimit->nextResetTime,
            timeLimit->windowLabel(),
            std::nullopt
        };
    }

    if (sessionTokenLimit.has_value()) {
        snap.tertiary = RateWindow{
            sessionTokenLimit->usedPercent(),
            sessionTokenLimit->windowMinutes(),
            sessionTokenLimit->nextResetTime,
            sessionTokenLimit->windowLabel(),
            std::nullopt
        };
    }

    if (planName.has_value()) {
        snap.identity = ProviderIdentitySnapshot{
            UsageProvider::zai, std::nullopt, std::nullopt, *planName
        };
    }

    snap.zaiUsage = *this;
    snap.updatedAt = updatedAt;
    return snap;
}
