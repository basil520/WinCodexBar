#pragma once

#include "../models/UsagePace.h"

#include <QString>
#include <optional>

struct UsagePaceWeeklyDetail {
    QString leftLabel;
    QString rightLabel;
    double expectedUsedPercent = 0.0;
    UsagePace::Stage stage = UsagePace::Stage::onTrack;
};

class UsagePaceText {
public:
    static QString weeklySummary(const UsagePace& pace);
    static UsagePaceWeeklyDetail weeklyDetail(const UsagePace& pace);

private:
    static QString detailLeftLabel(const UsagePace& pace);
    static QString detailRightLabel(const UsagePace& pace);
    static QString durationText(int64_t seconds);
    static int roundedRiskPercent(double probability);
};
