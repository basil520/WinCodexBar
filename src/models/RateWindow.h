#pragma once

#include <QString>
#include <QDateTime>
#include <optional>
#include <algorithm>

struct RateWindow {
    double usedPercent = 0.0;
    std::optional<int> windowMinutes;
    std::optional<QDateTime> resetsAt;
    std::optional<QString> resetDescription;
    std::optional<double> nextRegenPercent;

    double remainingPercent() const { return std::max(0.0, 100.0 - usedPercent); }
    bool isValid() const { return usedPercent >= 0.0 && usedPercent <= 100.0; }
};

struct NamedRateWindow {
    QString id;
    QString title;
    RateWindow window;
};
