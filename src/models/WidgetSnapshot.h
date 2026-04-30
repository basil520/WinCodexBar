#pragma once

#include "RateWindow.h"
#include "ProviderIdentitySnapshot.h"

#include <QDateTime>
#include <QVector>
#include <optional>

struct DailyUsagePoint {
    QDateTime date;
    double value = 0.0;
};

struct WidgetSnapshot {
    struct ProviderEntry {
        UsageProvider provider;
        std::optional<RateWindow> primary;
        std::optional<RateWindow> secondary;
        std::optional<RateWindow> tertiary;
        std::optional<double> creditsRemaining;
        QVector<DailyUsagePoint> dailyUsage;
    };

    QVector<ProviderEntry> entries;
    QVector<UsageProvider> enabledProviders;
    QDateTime generatedAt;
};
