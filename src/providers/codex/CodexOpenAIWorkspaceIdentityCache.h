#pragma once

#include "CodexOpenAIWorkspaceResolver.h"

#include <QString>
#include <QHash>
#include <QFile>

class CodexOpenAIWorkspaceIdentityCache {
public:
    CodexOpenAIWorkspaceIdentityCache();

    QString workspaceLabel(const QString& workspaceAccountID) const;
    void store(const CodexOpenAIWorkspaceIdentity& identity);

    static QString defaultURL();

private:
    struct Payload {
        int version;
        QHash<QString, QString> labelsByWorkspaceAccountID;
    };

    QString m_fileURL;

    Payload loadPayload() const;
    void savePayload(const Payload& payload);
    static int currentVersion();
};
