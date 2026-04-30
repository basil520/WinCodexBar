#pragma once

#include <algorithm>

struct CursorRequestUsage {
    int used = 0;
    int limit = 0;

    double usedPercent() const {
        if (limit <= 0) return 0.0;
        return (static_cast<double>(used) / static_cast<double>(limit)) * 100.0;
    }
    double remainingPercent() const {
        return std::max(0.0, 100.0 - usedPercent());
    }
};
