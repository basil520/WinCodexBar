#pragma once

// NOTE: WidgetSnapshot is currently unused in WinCodexBar.
// It was ported from the macOS upstream CodexBar for potential
// future Windows Widgets (Win11) support. All references are
// currently dead code.

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
