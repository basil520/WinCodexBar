#include "CodexDashboardCache.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QFileInfo>

void CodexDashboardCache::save(const CodexDashboardCacheEntry& entry)
{
    QJsonObject obj;
    obj["accountEmail"] = entry.accountEmail;
    obj["html"] = entry.html;
    obj["updatedAt"] = entry.updatedAt.toString(Qt::ISODateWithMs);

    QString path = defaultPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        file.close();
        QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
    }
}

std::optional<CodexDashboardCacheEntry> CodexDashboardCache::load(const QString& expectedEmail)
{
    QString path = defaultPath();
    QFile file(path);
    if (!file.exists()) return std::nullopt;
    if (!file.open(QIODevice::ReadOnly)) return std::nullopt;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return std::nullopt;

    QJsonObject obj = doc.object();
    QString cachedEmail = obj.value("accountEmail").toString().trimmed().toLower();
    QString reqEmail = expectedEmail.trimmed().toLower();

    if (cachedEmail.isEmpty() || reqEmail.isEmpty() || cachedEmail != reqEmail) {
        return std::nullopt;
    }

    CodexDashboardCacheEntry entry;
    entry.accountEmail = obj.value("accountEmail").toString();
    entry.html = obj.value("html").toString();
    entry.updatedAt = QDateTime::fromString(obj.value("updatedAt").toString(), Qt::ISODateWithMs);

    if (entry.html.isEmpty()) return std::nullopt;

    return entry;
}

void CodexDashboardCache::clear()
{
    QString path = defaultPath();
    QFile::remove(path);
}

QString CodexDashboardCache::defaultPath()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) base = QDir::homePath() + "/.codexbar";
    return base + "/codex-dashboard-cache.json";
}
