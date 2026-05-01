#include "CodexRateWindowNormalizer.h"

CodexRateWindowNormalizer::Result CodexRateWindowNormalizer::normalize(
    const std::optional<RateWindow>& primary,
    const std::optional<RateWindow>& secondary)
{
    if (primary.has_value() && secondary.has_value()) {
        auto primaryRole = role(primary.value());
        auto secondaryRole = role(secondary.value());

        if ((primaryRole == WindowRole::Session && secondaryRole == WindowRole::Weekly) ||
            (primaryRole == WindowRole::Session && secondaryRole == WindowRole::Unknown) ||
            (primaryRole == WindowRole::Unknown && secondaryRole == WindowRole::Weekly)) {
            return {primary, secondary};
        }
        if ((primaryRole == WindowRole::Weekly && secondaryRole == WindowRole::Session) ||
            (primaryRole == WindowRole::Weekly && secondaryRole == WindowRole::Unknown)) {
            return {secondary, primary};
        }
        return {primary, secondary};
    }

    if (primary.has_value()) {
        auto primaryRole = role(primary.value());
        if (primaryRole == WindowRole::Weekly) {
            return {std::nullopt, primary};
        }
        return {primary, std::nullopt};
    }

    if (secondary.has_value()) {
        auto secondaryRole = role(secondary.value());
        if (secondaryRole == WindowRole::Session) {
            return {secondary, std::nullopt};
        }
        return {std::nullopt, secondary};
    }

    return {std::nullopt, std::nullopt};
}

CodexRateWindowNormalizer::WindowRole CodexRateWindowNormalizer::role(const RateWindow& window)
{
    if (window.windowMinutes.has_value()) {
        int minutes = window.windowMinutes.value();
        if (minutes == 300) { // 5 hours = 300 minutes
            return WindowRole::Session;
        }
        if (minutes == 10080) { // 7 days = 10080 minutes
            return WindowRole::Weekly;
        }
    }
    return WindowRole::Unknown;
}
