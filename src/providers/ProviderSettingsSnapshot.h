#pragma once

#include <QString>
#include <QVariant>
#include <QHash>

struct ProviderSettingsSnapshot {
    QHash<QString, QVariant> values;

    QVariant get(const QString& key, const QVariant& defaultValue = {}) const {
        return values.value(key, defaultValue);
    }

    void set(const QString& key, const QVariant& value) {
        values[key] = value;
    }
};
