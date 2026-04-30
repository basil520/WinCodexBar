#include "PlanUtilizationHistory.h"
#include <algorithm>

QDateTime PlanUtilizationSeriesHistory::latestCapturedAt() const {
    if (entries.isEmpty()) return {};
    return entries.last().capturedAt;
}

QVector<PlanUtilizationSeriesHistory> PlanUtilizationHistoryBuckets::histories(const QString& accountKey) const {
    if (!accountKey.isEmpty()) {
        auto it = accounts.constFind(accountKey);
        if (it != accounts.constEnd()) return it.value();
        return {};
    }
    return unscoped;
}

void PlanUtilizationHistoryBuckets::setHistories(const QVector<PlanUtilizationSeriesHistory>& h, const QString& accountKey) {
    if (accountKey.isEmpty()) {
        unscoped = h;
    } else {
        accounts[accountKey] = h;
    }
}

bool PlanUtilizationHistoryBuckets::isEmpty() const {
    return unscoped.isEmpty() && accounts.isEmpty();
}
