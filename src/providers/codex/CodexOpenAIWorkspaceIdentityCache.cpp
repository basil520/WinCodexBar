#include "CodexOpenAIWorkspaceIdentityCache.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>

CodexOpenAIWorkspaceIdentityCache::CodexOpenAIWorkspaceIdentityCache()
    : m_fileURL(defaultURL())
{
}

QString CodexOpenAIWorkspaceIdentityCache::workspaceLabel(const QString& workspaceAccountID) const
{
    QString normalizedID = CodexOpenAIWorkspaceIdentity::normalizeWorkspaceAccountID(workspaceAccountID);
    if (normalizedID.isEmpty()) return QString();

    auto payload = loadPayload();
    return payload.labelsByWorkspaceAccountID.value(normalizedID);
}

void CodexOpenAIWorkspaceIdentityCache::store(const CodexOpenAIWorkspaceIdentity& identity)
{
    QString normalizedLabel = CodexOpenAIWorkspaceIdentity::normalizeWorkspaceLabel(identity.workspaceLabel);
    if (normalizedLabel.isEmpty()) return;

    auto payload = loadPayload();
    payload.labelsByWorkspaceAccountID[identity.workspaceAccountID] = normalizedLabel;
    savePayload(payload);
}

QString CodexOpenAIWorkspaceIdentityCache::defaultURL()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) base = QDir::homePath() + "/.codexbar";
    return base + "/codex-openai-workspaces.json";
}

CodexOpenAIWorkspaceIdentityCache::Payload CodexOpenAIWorkspaceIdentityCache::loadPayload() const
{
    Payload payload;
    payload.version = currentVersion();

    QFile file(m_fileURL);
    if (!file.exists()) return payload;
    if (!file.open(QIODevice::ReadOnly)) return payload;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return payload;

    QJsonObject obj = doc.object();
    if (obj.value("version").toInt() != currentVersion()) return payload;

    QJsonObject labels = obj.value("labelsByWorkspaceAccountID").toObject();
    for (auto it = labels.begin(); it != labels.end(); ++it) {
        payload.labelsByWorkspaceAccountID[it.key()] = it.value().toString();
    }

    return payload;
}

void CodexOpenAIWorkspaceIdentityCache::savePayload(const Payload& payload)
{
    QJsonObject labels;
    for (auto it = payload.labelsByWorkspaceAccountID.begin(); it != payload.labelsByWorkspaceAccountID.end(); ++it) {
        labels[it.key()] = it.value();
    }

    QJsonObject obj;
    obj["version"] = payload.version;
    obj["labelsByWorkspaceAccountID"] = labels;

    QDir().mkpath(QFileInfo(m_fileURL).absolutePath());
    QFile file(m_fileURL);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}

int CodexOpenAIWorkspaceIdentityCache::currentVersion()
{
    return 1;
}
