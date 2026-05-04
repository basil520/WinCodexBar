#include "CostUsageCache.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

static constexpr int CACHE_SCHEMA_VERSION = 1;

CostUsageCache& CostUsageCache::instance() {
    static CostUsageCache inst;
    return inst;
}

CostUsageCache::CostUsageCache(QObject* parent) : QObject(parent) {}

QString CostUsageCache::cacheFilePath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
                  + "/CodexBar";
    QDir().mkpath(dir);
    return dir + "/cost-usage-index.json";
}

QString CostUsageCache::canonicalPath(const QString& raw) {
    QString canon = QFileInfo(raw).canonicalFilePath();
    if (canon.isEmpty()) {
        // File may not exist yet (e.g. test paths); fall back to absolute path
        canon = QFileInfo(raw).absoluteFilePath();
    }
    return canon;
}

bool CostUsageCache::load() {
    QMutexLocker lock(&m_mutex);
    if (m_loaded) return true;

    QString path = cacheFilePath();
    QFile file(path);
    if (!file.exists()) {
        m_loaded = true;
        return true;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "CostUsageCache: cannot open" << path;
        m_loaded = true;
        return false;
    }

    QByteArray data = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "CostUsageCache: corrupt JSON, clearing.";
        m_entries.clear();
        m_loaded = true;
        m_dirty = false;
        return false;
    }

    QJsonObject root = doc.object();
    int schema = root["schemaVersion"].toInt(0);
    if (schema != CACHE_SCHEMA_VERSION) {
        qWarning() << "CostUsageCache: schema" << schema << "!=" << CACHE_SCHEMA_VERSION << ", clearing.";
        m_entries.clear();
        m_loaded = true;
        m_dirty = false;
        return false;
    }

    QJsonObject entriesObj = root["entries"].toObject();
    for (auto it = entriesObj.begin(); it != entriesObj.end(); ++it) {
        QJsonObject e = it.value().toObject();
        CostUsageFileEntry entry;
        entry.mtime = static_cast<qint64>(e["mtime"].toDouble());
        entry.size  = static_cast<qint64>(e["size"].toDouble());
        entry.provider = e["provider"].toString();

        QJsonObject dailyObj = e["daily"].toObject();
        for (auto dit = dailyObj.begin(); dit != dailyObj.end(); ++dit) {
            QString dateKey = dit.key();
            QJsonArray arr = dit.value().toArray();
            QVector<CostUsageModelBreakdown> vec;
            vec.reserve(arr.size());
            for (const auto& v : arr) {
                QJsonObject m = v.toObject();
                CostUsageModelBreakdown mb;
                mb.modelName = m["model"].toString();
                mb.inputTokens = m["input"].toInt(0);
                mb.cacheReadTokens = m["cacheRead"].toInt(0);
                mb.cacheCreationTokens = m["cacheCreate"].toInt(0);
                mb.outputTokens = m["output"].toInt(0);
                mb.costUSD = m["cost"].toDouble(0.0);
                vec.append(mb);
            }
            entry.dailyContributions[dateKey] = vec;
        }
        m_entries[it.key()] = entry;
    }

    m_loaded = true;
    m_dirty = false;
    qDebug() << "CostUsageCache: loaded" << m_entries.size() << "entries.";
    return true;
}

bool CostUsageCache::save() {
    QMutexLocker lock(&m_mutex);
    if (!m_dirty) return true;

    QString path = cacheFilePath();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "CostUsageCache: cannot write" << path;
        return false;
    }

    QJsonObject root;
    root["schemaVersion"] = CACHE_SCHEMA_VERSION;
    root["createdAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonObject entriesObj;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        const CostUsageFileEntry& entry = it.value();
        QJsonObject e;
        e["mtime"] = static_cast<double>(entry.mtime);
        e["size"]  = static_cast<double>(entry.size);
        e["provider"] = entry.provider;

        QJsonObject dailyObj;
        for (auto dit = entry.dailyContributions.constBegin();
             dit != entry.dailyContributions.constEnd(); ++dit) {
            QJsonArray arr;
            for (const auto& mb : dit.value()) {
                QJsonObject m;
                m["model"] = mb.modelName;
                m["input"] = mb.inputTokens;
                m["cacheRead"] = mb.cacheReadTokens;
                m["cacheCreate"] = mb.cacheCreationTokens;
                m["output"] = mb.outputTokens;
                m["cost"] = mb.costUSD;
                arr.append(m);
            }
            dailyObj[dit.key()] = arr;
        }
        e["daily"] = dailyObj;
        entriesObj[it.key()] = e;
    }
    root["entries"] = entriesObj;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Compact));
    m_dirty = false;
    qDebug() << "CostUsageCache: saved" << m_entries.size() << "entries.";
    return true;
}

bool CostUsageCache::isValid(const QString& path, const QFileInfo& info) const {
    QMutexLocker lock(&m_mutex);
    QString canon = canonicalPath(path);
    auto it = m_entries.find(canon);
    if (it == m_entries.end()) return false;
    return it.value().isValid(info);
}

QHash<QString, QVector<CostUsageModelBreakdown>> CostUsageCache::contributions(const QString& path) const {
    QMutexLocker lock(&m_mutex);
    QString canon = canonicalPath(path);
    auto it = m_entries.find(canon);
    if (it == m_entries.end()) return {};
    return it.value().dailyContributions;
}

void CostUsageCache::setContributions(const QString& path, const QString& provider,
                                      const QHash<QString, QVector<CostUsageModelBreakdown>>& contributions) {
    QMutexLocker lock(&m_mutex);
    QString canon = canonicalPath(path);
    CostUsageFileEntry entry;
    QFileInfo info(path);
    entry.mtime = info.lastModified().toSecsSinceEpoch();
    entry.size  = info.size();
    entry.provider = provider;
    entry.dailyContributions = contributions;
    m_entries[canon] = entry;
    m_dirty = true;
}

void CostUsageCache::removeEntry(const QString& path) {
    QMutexLocker lock(&m_mutex);
    QString canon = canonicalPath(path);
    if (m_entries.remove(canon) > 0) m_dirty = true;
}

void CostUsageCache::clear() {
    QMutexLocker lock(&m_mutex);
    m_entries.clear();
    m_dirty = true;
}

int CostUsageCache::entryCount() const {
    QMutexLocker lock(&m_mutex);
    return m_entries.size();
}
