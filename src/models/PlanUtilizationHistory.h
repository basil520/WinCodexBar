#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <optional>

struct PlanUtilizationSeriesName {
    QString rawValue;
    static PlanUtilizationSeriesName session() { return {"session"}; }
    static PlanUtilizationSeriesName weekly() { return {"weekly"}; }
    static PlanUtilizationSeriesName opus() { return {"opus"}; }
    bool operator==(const PlanUtilizationSeriesName& o) const { return rawValue == o.rawValue; }
};

struct PlanUtilizationHistoryEntry {
    QDateTime capturedAt;
    double usedPercent = 0.0;
    std::optional<QDateTime> resetsAt;
};

struct PlanUtilizationSeriesHistory {
    PlanUtilizationSeriesName name;
    int windowMinutes = 0;
    QVector<PlanUtilizationHistoryEntry> entries;
    QDateTime latestCapturedAt() const;
};

struct PlanUtilizationHistoryBuckets {
    QString preferredAccountKey;
    QVector<PlanUtilizationSeriesHistory> unscoped;
    QMap<QString, QVector<PlanUtilizationSeriesHistory>> accounts;

    QVector<PlanUtilizationSeriesHistory> histories(const QString& accountKey) const;
    void setHistories(const QVector<PlanUtilizationSeriesHistory>& h, const QString& accountKey);
    bool isEmpty() const;
};
