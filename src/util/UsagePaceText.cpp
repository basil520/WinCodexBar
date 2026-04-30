#include "UsagePaceText.h"

#include <QCoreApplication>

#include <cmath>
#include <algorithm>

QString UsagePaceText::weeklySummary(const UsagePace& pace) {
    auto detail = weeklyDetail(pace);
    if (!detail.rightLabel.isEmpty()) {
        return QCoreApplication::translate("UsagePaceText", "Pace: %1 · %2")
            .arg(detail.leftLabel, detail.rightLabel);
    }
    return QCoreApplication::translate("UsagePaceText", "Pace: %1").arg(detail.leftLabel);
}

UsagePaceWeeklyDetail UsagePaceText::weeklyDetail(const UsagePace& pace) {
    return UsagePaceWeeklyDetail{
        detailLeftLabel(pace),
        detailRightLabel(pace),
        pace.expectedUsedPercent,
        pace.stage
    };
}

QString UsagePaceText::detailLeftLabel(const UsagePace& pace) {
    int deltaValue = static_cast<int>(std::round(std::abs(pace.deltaPercent)));
    switch (pace.stage) {
    case UsagePace::Stage::onTrack:
        return QCoreApplication::translate("UsagePaceText", "On pace");
    case UsagePace::Stage::slightlyAhead:
    case UsagePace::Stage::ahead:
    case UsagePace::Stage::farAhead:
        return QCoreApplication::translate("UsagePaceText", "%1% in deficit").arg(deltaValue);
    case UsagePace::Stage::slightlyBehind:
    case UsagePace::Stage::behind:
    case UsagePace::Stage::farBehind:
        return QCoreApplication::translate("UsagePaceText", "%1% in reserve").arg(deltaValue);
    }
    return QCoreApplication::translate("UsagePaceText", "On pace");
}

QString UsagePaceText::detailRightLabel(const UsagePace& pace) {
    QString etaLabel;

    if (pace.willLastToReset) {
        etaLabel = QCoreApplication::translate("UsagePaceText", "Lasts until reset");
    } else if (pace.etaSeconds.has_value()) {
        QString etaText = durationText(*pace.etaSeconds);
        if (etaText == QCoreApplication::translate("UsagePaceText", "now")) {
            etaLabel = QCoreApplication::translate("UsagePaceText", "Runs out now");
        } else {
            etaLabel = QCoreApplication::translate("UsagePaceText", "Runs out in %1").arg(etaText);
        }
    }

    if (pace.runOutProbability.has_value()) {
        int risk = roundedRiskPercent(*pace.runOutProbability);
        QString riskLabel = QCoreApplication::translate("UsagePaceText", "≈ %1% run-out risk").arg(risk);
        if (!etaLabel.isEmpty()) {
            return QCoreApplication::translate("UsagePaceText", "%1 · %2").arg(etaLabel, riskLabel);
        }
        return riskLabel;
    }

    return etaLabel;
}

QString UsagePaceText::durationText(int64_t seconds) {
    if (seconds <= 0) return QCoreApplication::translate("UsagePaceText", "now");

    int hours = static_cast<int>(seconds / 3600);
    int mins = static_cast<int>((seconds % 3600) / 60);

    if (hours >= 1 && mins >= 1) {
        return QCoreApplication::translate("UsagePaceText", "%1h %2m").arg(hours).arg(mins);
    }
    if (hours >= 1) {
        return QCoreApplication::translate("UsagePaceText", "%1h").arg(hours);
    }
    if (mins > 0) {
        return QCoreApplication::translate("UsagePaceText", "%1m").arg(mins);
    }
    return QCoreApplication::translate("UsagePaceText", "<1m");
}

int UsagePaceText::roundedRiskPercent(double probability) {
    double percent = std::clamp(probability, 0.0, 1.0) * 100.0;
    double rounded = std::round(percent / 5.0) * 5.0;
    return static_cast<int>(rounded);
}
