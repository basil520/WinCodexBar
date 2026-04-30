#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>
#include <optional>

struct CreditEvent {
    QString id;
    QString type;
    double amount = 0.0;
    QDateTime timestamp;
    QString description;
};

struct CreditsSnapshot {
    double remaining = 0.0;
    QVector<CreditEvent> events;
    QDateTime updatedAt;
};
