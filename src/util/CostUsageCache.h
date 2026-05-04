#pragma once

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include <optional>
#include "../models/CostUsageReport.h"

/**
 * @brief Per-file cache entry for CostUsageScanner.
 *
 * Stores the parsed contribution of a single JSONL file to daily cost usage.
 * When a file's mtime+size are unchanged, the parser can skip re-reading it
 * and reuse the cached daily contributions directly.
 */
struct CostUsageFileEntry {
    qint64 mtime = 0;               // QFileInfo::lastModified().toSecsSinceEpoch()
    qint64 size = 0;                // QFileInfo::size()
    QString provider;               // "codex", "claude", or "pi"
    // dateKey (yyyy-MM-dd) → list of model breakdowns from this file
    QHash<QString, QVector<CostUsageModelBreakdown>> dailyContributions;

    bool isValid(const QFileInfo& info) const {
        return mtime == info.lastModified().toSecsSinceEpoch() && size == info.size();
    }
};

/**
 * @brief Disk-backed incremental cache for CostUsageScanner.
 *
 * Eliminates redundant JSONL re-parsing by remembering each file's
 * mtime, size, and parsed daily contributions.  When a file has not
 * changed, the scanner reuses the cached contributions instead of
 * opening and parsing the file again.
 *
 * Thread-safety: all public methods are protected by an internal mutex.
 */
class CostUsageCache : public QObject {
    Q_OBJECT
public:
    static CostUsageCache& instance();

    /** Load index from %LOCALAPPDATA%/CodexBar/cache/cost-usage-index.json */
    bool load();
    /** Save index to disk (no-op if nothing changed since last load/save). */
    bool save();

    /** Return true if the cached entry for @p path matches @p info. */
    bool isValid(const QString& path, const QFileInfo& info) const;

    /** Retrieve cached daily contributions for @p path (empty if none). */
    QHash<QString, QVector<CostUsageModelBreakdown>> contributions(const QString& path) const;

    /** Store daily contributions for @p path after a successful parse. */
    void setContributions(const QString& path, const QString& provider,
                          const QHash<QString, QVector<CostUsageModelBreakdown>>& contributions);

    /** Remove a file entry (e.g. file deleted). */
    void removeEntry(const QString& path);

    /** Clear all entries and mark dirty. */
    void clear();

    /** Number of cached file entries. */
    int entryCount() const;

private:
    explicit CostUsageCache(QObject* parent = nullptr);

    QString cacheFilePath() const;
    static QString canonicalPath(const QString& raw);

    mutable QMutex m_mutex;
    QHash<QString, CostUsageFileEntry> m_entries; // canonical path → entry
    bool m_dirty = false;
    bool m_loaded = false;
};
