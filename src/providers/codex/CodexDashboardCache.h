#pragma once

#include <QString>
#include <QDateTime>
#include <optional>

struct CodexDashboardCacheEntry {
    QString accountEmail;
    QString html;
    QDateTime updatedAt;
};

class CodexDashboardCache {
public:
    static void save(const CodexDashboardCacheEntry& entry);
    static std::optional<CodexDashboardCacheEntry> load(const QString& expectedEmail);
    static void clear();
    static QString defaultPath();
};
