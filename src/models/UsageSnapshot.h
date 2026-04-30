#pragma once

#include "RateWindow.h"
#include "ProviderCostSnapshot.h"
#include "CreditsSnapshot.h"
#include "ProviderIdentitySnapshot.h"
#include "ZaiUsageSnapshot.h"
#include "OpenRouterUsageSnapshot.h"
#include "MiniMaxUsageSnapshot.h"
#include "CursorRequestUsage.h"

#include <QDateTime>
#include <QVector>
#include <optional>

struct UsageSnapshot {
    std::optional<RateWindow> primary;
    std::optional<RateWindow> secondary;
    std::optional<RateWindow> tertiary;
    QVector<NamedRateWindow> extraRateWindows;
    std::optional<ProviderCostSnapshot> providerCost;
    std::optional<ProviderIdentitySnapshot> identity;
    std::optional<ZaiUsageSnapshot> zaiUsage;
    std::optional<OpenRouterUsageSnapshot> openRouterUsage;
    std::optional<MiniMaxUsageSnapshot> minimaxUsage;
    std::optional<CursorRequestUsage> cursorRequests;
    QDateTime updatedAt;
};
