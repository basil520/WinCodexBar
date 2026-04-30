#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>
#include <optional>

struct CostUsageModelBreakdown {
    QString modelName;
    int inputTokens = 0;
    int cacheReadTokens = 0;
    int cacheCreationTokens = 0;
    int outputTokens = 0;
    int totalTokens() const { return inputTokens + cacheReadTokens + cacheCreationTokens + outputTokens; }
    double costUSD = 0.0;
};

struct CostUsageDailyEntry {
    QString date;
    int inputTokens = 0;
    int cacheReadTokens = 0;
    int cacheCreationTokens = 0;
    int outputTokens = 0;
    int totalTokens() const { return inputTokens + cacheReadTokens + cacheCreationTokens + outputTokens; }
    double costUSD = 0.0;
    QVector<CostUsageModelBreakdown> models;
};

struct CostUsageSnapshot {
    int sessionTokens = 0;
    double sessionCostUSD = 0.0;
    int last30DaysTokens = 0;
    double last30DaysCostUSD = 0.0;
    QVector<CostUsageDailyEntry> daily;
    QDateTime updatedAt;
    QString errorMessage;
};

struct ProviderCostUsageSnapshot {
    QString providerId;
    CostUsageSnapshot snapshot;
    QVector<CostUsageModelBreakdown> modelSummary;
};
