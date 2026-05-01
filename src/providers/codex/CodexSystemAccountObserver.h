#pragma once

#include "CodexIdentity.h"

#include <QString>
#include <QDateTime>
#include <QHash>
#include <optional>

struct ObservedSystemCodexAccount {
    QString email;
    QString workspaceLabel;
    QString workspaceAccountId;
    QString codexHomePath;
    QDateTime observedAt;
    CodexIdentity identity;

    bool isValid() const { return !email.isEmpty(); }
};

class CodexSystemAccountObserver {
public:
    CodexSystemAccountObserver();

    std::optional<ObservedSystemCodexAccount> loadSystemAccount(
        const QHash<QString, QString>& env) const;

private:
    struct AuthBackedAccount {
        QString email;
        CodexIdentity identity;
    };

    AuthBackedAccount loadAuthBackedCodexAccount(const QHash<QString, QString>& env) const;
    static QString normalizeEmail(const QString& email);
};
