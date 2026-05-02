#pragma once

#include <QString>
#include <QDateTime>

struct ProviderFetchAttempt {
    QString strategyID;
    QString strategyKindLabel;
    bool available = false;
    bool success = false;
    QString errorMessage;
    QDateTime attemptedAt;

    QVariantMap toMap() const {
        QVariantMap m;
        m["strategyID"] = strategyID;
        m["strategyKindLabel"] = strategyKindLabel;
        m["available"] = available;
        m["success"] = success;
        m["errorMessage"] = errorMessage;
        m["attemptedAt"] = attemptedAt.toString(Qt::ISODate);
        return m;
    }
};
