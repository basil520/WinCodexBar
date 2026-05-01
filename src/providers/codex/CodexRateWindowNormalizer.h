#pragma once

#include "../../models/RateWindow.h"

#include <optional>

class CodexRateWindowNormalizer {
public:
    struct Result {
        std::optional<RateWindow> primary;
        std::optional<RateWindow> secondary;
    };

    static Result normalize(const std::optional<RateWindow>& primary,
                           const std::optional<RateWindow>& secondary);

private:
    enum class WindowRole {
        Session,
        Weekly,
        Unknown
    };

    static WindowRole role(const RateWindow& window);
};
