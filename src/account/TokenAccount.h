#pragma once

#include "TokenAccountCredentials.h"
#include "../providers/ProviderSourceMode.h"

#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include <QUuid>

enum class AccountVisibility {
    Visible,   // Included in fetch, shown in UI
    Hidden,    // Excluded from fetch, hidden in UI but preserved
    Archived   // Excluded from fetch, shown only in archived list
};

inline QString visibilityToString(AccountVisibility v) {
    switch (v) {
    case AccountVisibility::Visible: return "visible";
    case AccountVisibility::Hidden: return "hidden";
    case AccountVisibility::Archived: return "archived";
    }
    return "visible";
}

inline AccountVisibility visibilityFromString(const QString& s) {
    if (s == "hidden") return AccountVisibility::Hidden;
    if (s == "archived") return AccountVisibility::Archived;
    return AccountVisibility::Visible;
}

struct TokenAccount {
    QString accountId;
    QString providerId;
    QString displayName;
    ProviderSourceMode sourceMode = ProviderSourceMode::Auto;
    AccountVisibility visibility = AccountVisibility::Visible;
    TokenAccountCredentials credentials;
    QDateTime createdAt;
    QDateTime lastUsedAt;

    bool isValid() const {
        return !accountId.isEmpty() && !providerId.isEmpty() && !displayName.isEmpty();
    }

    bool isActive() const {
        return visibility == AccountVisibility::Visible;
    }

    QVariantMap toVariantMap() const {
        QVariantMap m;
        m["accountId"] = accountId;
        m["providerId"] = providerId;
        m["displayName"] = displayName;
        m["sourceMode"] = sourceModeToString(sourceMode);
        m["visibility"] = visibilityToString(visibility);
        m["credentials"] = credentials.toVariantMap();
        m["createdAt"] = createdAt.toString(Qt::ISODate);
        m["lastUsedAt"] = lastUsedAt.toString(Qt::ISODate);
        return m;
    }

    static TokenAccount fromVariantMap(const QVariantMap& map) {
        TokenAccount acc;
        acc.accountId = map.value("accountId").toString();
        acc.providerId = map.value("providerId").toString();
        acc.displayName = map.value("displayName").toString();
        acc.sourceMode = sourceModeFromString(map.value("sourceMode").toString());
        acc.visibility = visibilityFromString(map.value("visibility").toString());
        acc.createdAt = QDateTime::fromString(map.value("createdAt").toString(), Qt::ISODate);
        acc.lastUsedAt = QDateTime::fromString(map.value("lastUsedAt").toString(), Qt::ISODate);
        // Credentials are loaded separately from secure storage
        return acc;
    }

    static QString generateAccountId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
};
