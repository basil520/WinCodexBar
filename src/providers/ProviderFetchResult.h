#pragma once

#include "../models/UsageSnapshot.h"
#include "../models/CreditsSnapshot.h"
#include "ProviderFetchKind.h"

#include <QString>
#include <optional>

struct ProviderFetchResult {
    UsageSnapshot usage;
    std::optional<CreditsSnapshot> credits;
    QString sourceLabel;
    QString strategyID;
    int strategyKind = ProviderFetchKind::CLI;
    bool success = false;
    QString errorMessage;
};
