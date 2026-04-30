#include "UsagePace.h"
#include "RateWindow.h"

#include <QDateTime>
#include <algorithm>

static double clamp(double v, double lo, double hi) {
    return std::max(lo, std::min(hi, v));
}

static UsagePace::Stage stageForDelta(double delta) {
    double absDelta = std::abs(delta);
    if (absDelta <= 2.0) return UsagePace::Stage::onTrack;
    if (absDelta <= 6.0) return delta >= 0 ? UsagePace::Stage::slightlyAhead : UsagePace::Stage::slightlyBehind;
    if (absDelta <= 12.0) return delta >= 0 ? UsagePace::Stage::ahead : UsagePace::Stage::behind;
    return delta >= 0 ? UsagePace::Stage::farAhead : UsagePace::Stage::farBehind;
}

std::optional<UsagePace> UsagePace::weekly(const RateWindow& window, const QDateTime& now, int defaultWindowMinutes) {
    if (!window.resetsAt.has_value()) return std::nullopt;
    int minutes = window.windowMinutes.value_or(defaultWindowMinutes);
    if (minutes <= 0) return std::nullopt;

    double duration = static_cast<double>(minutes) * 60.0;
    double timeUntilReset = static_cast<double>(now.secsTo(*window.resetsAt));
    if (timeUntilReset <= 0) return std::nullopt;
    if (timeUntilReset > duration) return std::nullopt;

    double elapsed = clamp(duration - timeUntilReset, 0.0, duration);
    double expected = clamp((elapsed / duration) * 100.0, 0.0, 100.0);
    double actual = clamp(window.usedPercent, 0.0, 100.0);

    if (elapsed == 0 && actual > 0) return std::nullopt;

    double delta = actual - expected;
    auto stage = stageForDelta(delta);

    std::optional<int64_t> etaSeconds;
    bool willLastToReset = false;

    if (elapsed > 0 && actual > 0) {
        double rate = actual / elapsed;
        if (rate > 0) {
            double remaining = std::max(0.0, 100.0 - actual);
            double candidate = remaining / rate;
            if (candidate >= timeUntilReset) {
                willLastToReset = true;
            } else {
                etaSeconds = static_cast<int64_t>(candidate);
            }
        }
    } else if (elapsed > 0 && actual == 0) {
        willLastToReset = true;
    }

    return UsagePace{
        stage,
        delta,
        expected,
        actual,
        etaSeconds,
        willLastToReset,
        std::nullopt
    };
}
