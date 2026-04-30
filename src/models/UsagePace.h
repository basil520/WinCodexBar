#pragma once

#include <QDateTime>
#include <optional>
#include <cstdint>

struct RateWindow;

struct UsagePace {
    enum class Stage {
        onTrack,
        slightlyAhead,
        ahead,
        farAhead,
        slightlyBehind,
        behind,
        farBehind
    };

    Stage stage = Stage::onTrack;
    double deltaPercent = 0.0;
    double expectedUsedPercent = 0.0;
    double actualUsedPercent = 0.0;
    std::optional<int64_t> etaSeconds;
    bool willLastToReset = true;
    std::optional<double> runOutProbability;

    static std::optional<UsagePace> weekly(const RateWindow& window, const QDateTime& now = QDateTime::currentDateTime(), int defaultWindowMinutes = 10080);
};
