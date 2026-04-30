#pragma once

#include <QString>
#include <QDateTime>
#include <optional>

struct ProviderCostSnapshot {
    double used = 0.0;
    double limit = 0.0;
    QString currencyCode;
    std::optional<QString> period;
    std::optional<QDateTime> resetsAt;
    std::optional<double> nextRegenAmount;
    QDateTime updatedAt;
};
