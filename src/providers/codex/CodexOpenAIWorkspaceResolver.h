#pragma once

#include "../../models/CodexUsageResponse.h"

#include <QString>
#include <QHash>
#include <optional>

struct CodexOpenAIWorkspaceIdentity {
    QString workspaceAccountID;
    QString workspaceLabel;

    bool operator==(const CodexOpenAIWorkspaceIdentity& other) const;
    bool operator!=(const CodexOpenAIWorkspaceIdentity& other) const;

    static QString normalizeWorkspaceAccountID(const QString& value);
    static QString normalizeWorkspaceLabel(const QString& value);
};

class CodexOpenAIWorkspaceResolver {
public:
    static std::optional<CodexOpenAIWorkspaceIdentity> resolve(
        const CodexOAuthCredentials& credentials,
        const QHash<QString, QString>& env);

    static QString normalizeWorkspaceAccountID(const QString& value);

private:
    static const QString AccountsURL;

    struct AccountItem {
        QString id;
        QString name;
    };

    static QString resolveWorkspaceLabel(const AccountItem& account);
};
