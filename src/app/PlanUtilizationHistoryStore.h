#pragma once

#include "../models/PlanUtilizationHistory.h"
#include "../models/UsageSnapshot.h"

#include <QObject>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QJsonDocument>

class PlanUtilizationHistoryStore : public QObject {
    Q_OBJECT
public:
    explicit PlanUtilizationHistoryStore(QObject* parent = nullptr);

    void recordSample(const QString& providerId, const UsageSnapshot& snapshot, const QString& accountKey = {});
    PlanUtilizationHistoryBuckets buckets(const QString& providerId) const;
    QVariantList chartData(const QString& providerId, const QString& seriesName, const QString& accountKey = {}) const;
    void stopSaveTimer();

    static constexpr int maxSamples = 17520;
    static constexpr int minSampleIntervalSeconds = 3600;

private:
    void loadFromDisk(const QString& providerId);
    void saveToDisk(const QString& providerId);
    QJsonDocument serializeProvider(const QString& providerId) const;
    QString filePath(const QString& providerId) const;
    void enqueueSave(const QString& providerId);

    QHash<QString, PlanUtilizationHistoryBuckets> m_buckets;
    QHash<QString, bool> m_pendingSave;
    QTimer m_saveCoalesceTimer;
};
