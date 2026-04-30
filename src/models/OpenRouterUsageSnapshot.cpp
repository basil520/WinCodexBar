#include "OpenRouterUsageSnapshot.h"
#include "UsageSnapshot.h"

#include <algorithm>

bool OpenRouterUsageSnapshot::isValid() const {
    return totalCredits >= 0;
}

bool OpenRouterUsageSnapshot::keyDataFetched() const {
    return keyLimit.has_value() || keyUsage.has_value();
}

bool OpenRouterUsageSnapshot::hasValidKeyQuota() const {
    return keyDataFetched() && keyLimit.value_or(0) > 0 && keyUsage.value_or(-1) >= 0;
}

OpenRouterKeyQuotaStatus OpenRouterUsageSnapshot::keyQuotaStatus() const {
    if (!keyDataFetched()) return OpenRouterKeyQuotaStatus::Unavailable;
    if (!keyLimit.has_value() || *keyLimit <= 0) return OpenRouterKeyQuotaStatus::NoLimitConfigured;
    if (keyUsage.value_or(-1) < 0) return OpenRouterKeyQuotaStatus::Unavailable;
    return OpenRouterKeyQuotaStatus::Available;
}

double OpenRouterUsageSnapshot::keyRemaining() const {
    return std::max(0.0, keyLimit.value_or(0) - keyUsage.value_or(0));
}

double OpenRouterUsageSnapshot::keyUsedPercent() const {
    if (keyLimit.value_or(0) <= 0) return 0.0;
    return std::min(100.0, (keyUsage.value_or(0) / *keyLimit) * 100.0);
}

UsageSnapshot OpenRouterUsageSnapshot::toUsageSnapshot() const {
    UsageSnapshot snap;

    if (hasValidKeyQuota()) {
        snap.primary = RateWindow{ keyUsedPercent(), std::nullopt, std::nullopt, std::nullopt, std::nullopt };
    } else if (isValid() && totalCredits > 0) {
        snap.primary = RateWindow{ usedPercent, std::nullopt, std::nullopt, std::nullopt, std::nullopt };
    }

    QString method = QString("Balance: $%1").arg(balance, 0, 'f', 2);
    snap.identity = ProviderIdentitySnapshot{
        UsageProvider::openrouter, std::nullopt, std::nullopt, method
    };

    snap.openRouterUsage = *this;
    snap.updatedAt = updatedAt;
    return snap;
}
