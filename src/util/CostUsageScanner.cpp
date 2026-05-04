#include "CostUsageScanner.h"
#include "CostUsageCache.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QUuid>
#include <algorithm>
#include <atomic>

static std::atomic<bool> g_shuttingDown{false};

void CostUsageScanner::setShuttingDown(bool shuttingDown)
{
    g_shuttingDown = shuttingDown;
}

CostUsageScanner::CostUsageScanner(CostUsageCache* cache) : m_cache(cache) {
    initPricing();
}

const QHash<QString, CostUsageScanner::Pricing>& CostUsageScanner::codexPricingMap() {
    static QHash<QString, Pricing> map;
    if (map.isEmpty()) {
        CostUsageScanner scanner;
        map = scanner.m_codexPricing;
    }
    return map;
}

const QHash<QString, CostUsageScanner::Pricing>& CostUsageScanner::claudePricingMap() {
    static QHash<QString, Pricing> map;
    if (map.isEmpty()) {
        CostUsageScanner scanner;
        map = scanner.m_claudePricing;
    }
    return map;
}

const QHash<QString, CostUsageScanner::Pricing>& CostUsageScanner::opencodeGoPricingMap() {
    static QHash<QString, Pricing> map;
    if (map.isEmpty()) {
        CostUsageScanner scanner;
        map = scanner.m_opencodeGoPricing;
    }
    return map;
}

void CostUsageScanner::initPricing() {
    // Codex models (per million tokens; cacheWritePerM=0 — Codex has no cache creation tokens)
    m_codexPricing["gpt-5"]                = { 1.25,  0.125,  0,  10.0 };
    m_codexPricing["gpt-5-codex"]          = { 1.25,  0.125,  0,  10.0 };
    m_codexPricing["gpt-5-mini"]           = { 0.25,  0.025,  0,   2.0 };
    m_codexPricing["gpt-5-nano"]           = { 0.05,  0.005,  0,   0.4 };
    m_codexPricing["gpt-5-pro"]            = { 15.0, 15.0,    0, 120.0 };
    m_codexPricing["gpt-5.1"]              = { 1.25,  0.125,  0,  10.0 };
    m_codexPricing["gpt-5.1-codex"]        = { 1.25,  0.125,  0,  10.0 };
    m_codexPricing["gpt-5.1-codex-max"]    = { 1.25,  0.125,  0,  10.0 };
    m_codexPricing["gpt-5.1-codex-mini"]   = { 0.25,  0.025,  0,   2.0 };
    m_codexPricing["gpt-5.2"]              = { 1.75,  0.175,  0,  14.0 };
    m_codexPricing["gpt-5.2-codex"]        = { 1.75,  0.175,  0,  14.0 };
    m_codexPricing["gpt-5.2-pro"]          = { 21.0, 21.0,    0, 168.0 };
    m_codexPricing["gpt-5.3-codex"]        = { 1.75,  0.175,  0,  14.0 };
    m_codexPricing["gpt-5.3-codex-spark"]  = { 0.0,   0.0,    0,   0.0 };
    m_codexPricing["gpt-5.4"]              = { 2.5,   0.25,   0,  15.0 };
    m_codexPricing["gpt-5.4-mini"]         = { 0.75,  0.075,  0,   4.5 };
    m_codexPricing["gpt-5.4-nano"]         = { 0.2,   0.02,   0,   1.25 };
    m_codexPricing["gpt-5.4-pro"]          = { 30.0, 30.0,     0, 180.0 };
    m_codexPricing["gpt-5.5"]              = { 5.0,   0.5,    0,  30.0 };
    m_codexPricing["gpt-5.5-pro"]          = { 30.0, 30.0,     0, 180.0 };

    // Claude models (per million tokens) — prices aligned with Mac CostUsagePricing.swift
    m_claudePricing["claude-haiku-4-5-20251001"] = { 1.0,  0.1,  1.25,   5.0 };
    m_claudePricing["claude-haiku-4-5"]          = { 1.0,  0.1,  1.25,   5.0 };
    m_claudePricing["claude-opus-4-5-20251101"]  = { 5.0,  0.5,  6.25,  25.0 };
    m_claudePricing["claude-opus-4-5"]           = { 5.0,  0.5,  6.25,  25.0 };
    m_claudePricing["claude-opus-4-6-20260205"]  = { 5.0,  0.5,  6.25,  25.0 };
    m_claudePricing["claude-opus-4-6"]           = { 5.0,  0.5,  6.25,  25.0 };
    m_claudePricing["claude-opus-4-7"]           = { 5.0,  0.5,  6.25,  25.0 };
    m_claudePricing["claude-opus-4-20250514"]    = { 15.0, 1.5, 18.75,  75.0 };
    m_claudePricing["claude-opus-4-1"]           = { 15.0, 1.5, 18.75,  75.0 };

    // Tiered Claude sonnet models (200k token threshold)
    auto tieredSonnet = Pricing{ 3.0, 0.3, 3.75, 15.0, 200000, 6.0, 22.5, 7.5, 0.6 };
    m_claudePricing["claude-sonnet-4-5"]          = tieredSonnet;
    m_claudePricing["claude-sonnet-4-6"]          = tieredSonnet;
    m_claudePricing["claude-sonnet-4-5-20250929"] = tieredSonnet;
    m_claudePricing["claude-sonnet-4-20250514"]   = tieredSonnet;

    // Legacy Claude 3.x models (kept for backward compatibility)
    m_claudePricing["claude-opus-4"]              = { 15.0, 1.5, 22.5,  75.0 };
    m_claudePricing["claude-3-5-sonnet-20241022"] = { 3.0,  0.3,  4.5,  15.0 };
    m_claudePricing["claude-3-5-haiku-20241022"]  = { 0.8,  0.08, 1.2,   4.0 };
    m_claudePricing["claude-haiku-3-5"]           = { 0.8,  0.08, 1.2,   4.0 };
    m_claudePricing["claude-3-opus-20240229"]     = { 15.0, 1.5, 22.5,  75.0 };
    m_claudePricing["claude-3-sonnet-20240229"]   = { 3.0,  0.3,  4.5,  15.0 };
    m_claudePricing["claude-3-haiku-20240307"]    = { 0.25, 0.03, 0.375, 1.25 };

    // OpenCode Go models (proxied from various providers)
    m_opencodeGoPricing["glm-5"]            = { 0.5, 0.05, 0.5, 2.0 };    // Zhipu GLM-5 approximate
    m_opencodeGoPricing["glm-5.1"]          = { 0.5, 0.05, 0.5, 2.0 };    // Zhipu GLM-5.1
    m_opencodeGoPricing["kimi-k2.5"]        = { 1.0, 0.1, 1.25, 5.0 };    // Kimi K2.5 approximate
    m_opencodeGoPricing["kimi-k2.6"]        = { 1.0, 0.1, 1.25, 5.0 };    // Kimi K2.6
    m_opencodeGoPricing["minimax"]           = { 0.1, 0.01, 0.1, 0.4 };    // MiniMax approximate

    // Kimi for coding models (used by kimi-for-coding provider)
    m_opencodeGoPricing["k2p6"]             = { 1.0, 0.1, 1.25, 5.0 };    // Kimi K2.6 for coding
}

QString CostUsageScanner::normalizeCodexModel(const QString& raw) {
    QString trimmed = raw.trimmed();
    if (trimmed.startsWith("openai/"))
        trimmed = trimmed.mid(7);

    if (CostUsageScanner::codexPricingMap().contains(trimmed))
        return trimmed;

    QRegularExpression dateSuffix("-\\d{4}-\\d{2}-\\d{2}$");
    auto match = dateSuffix.match(trimmed);
    if (match.hasMatch()) {
        QString base = trimmed.left(match.capturedStart());
        if (CostUsageScanner::codexPricingMap().contains(base))
            return base;
    }
    return trimmed;
}

QString CostUsageScanner::normalizeClaudeModel(const QString& raw) {
    QString trimmed = raw.trimmed();
    if (trimmed.startsWith("anthropic."))
        trimmed = trimmed.mid(10);

    int lastDot = trimmed.lastIndexOf('.');
    if (lastDot >= 0) {
        QString tail = trimmed.mid(lastDot + 1);
        if (tail.startsWith("claude-"))
            trimmed = tail;
    }

    QRegularExpression versionSuffix("-v\\d+:\\d+$");
    trimmed.remove(versionSuffix);

    if (CostUsageScanner::claudePricingMap().contains(trimmed))
        return trimmed;

    QRegularExpression dateSuffix("-\\d{8}$");
    auto match = dateSuffix.match(trimmed);
    if (match.hasMatch()) {
        QString base = trimmed.left(match.capturedStart());
        if (CostUsageScanner::claudePricingMap().contains(base))
            return base;
    }
    return trimmed;
}

double CostUsageScanner::tieredCost(int tokens, double basePerM, const std::optional<double>& abovePerM, const std::optional<int>& threshold) {
    if (!threshold.has_value() || !abovePerM.has_value())
        return tokens / 1e6 * basePerM;
    int below = std::min(tokens, *threshold);
    int over = std::max(tokens - *threshold, 0);
    return below / 1e6 * basePerM + over / 1e6 * *abovePerM;
}

double CostUsageScanner::costForClaudeModel(const Pricing& p, int inputTokens, int cacheReadTokens, int cacheCreationTokens, int outputTokens) {
    return tieredCost(std::max(0, inputTokens),        p.inputPerM,  p.inputPerMAboveThreshold,  p.thresholdTokens)
         + tieredCost(std::max(0, cacheReadTokens),     p.cacheReadPerM,  p.cacheReadPerMAboveThreshold,  p.thresholdTokens)
         + tieredCost(std::max(0, cacheCreationTokens), p.cacheWritePerM, p.cacheWritePerMAboveThreshold, p.thresholdTokens)
         + tieredCost(std::max(0, outputTokens),        p.outputPerM, p.outputPerMAboveThreshold, p.thresholdTokens);
}

double CostUsageScanner::costForCodexModel(const Pricing& p, int inputTokens, int cacheReadTokens, int outputTokens) {
    int nonCached = std::max(0, inputTokens - cacheReadTokens);
    int cached = std::max(0, std::min(cacheReadTokens, inputTokens));
    return nonCached / 1e6 * p.inputPerM
         + cached / 1e6 * p.cacheReadPerM
         + std::max(0, outputTokens) / 1e6 * p.outputPerM;
}

CostUsageScanner::Pricing CostUsageScanner::priceForModel(const QString& modelName) {
    // Check OpenCode Go pricing first (includes Kimi, MiniMax, and other proxied models)
    for (auto mit = CostUsageScanner::opencodeGoPricingMap().constBegin(); mit != CostUsageScanner::opencodeGoPricingMap().constEnd(); ++mit) {
        if (modelName.contains(mit.key(), Qt::CaseInsensitive)) return mit.value();
    }

    QString normCodex = normalizeCodexModel(modelName);
    auto cit = CostUsageScanner::codexPricingMap().constFind(normCodex);
    if (cit != CostUsageScanner::codexPricingMap().constEnd()) return cit.value();

    QString normClaude = normalizeClaudeModel(modelName);
    auto clt = CostUsageScanner::claudePricingMap().constFind(normClaude);
    if (clt != CostUsageScanner::claudePricingMap().constEnd()) return clt.value();

    for (auto mit = CostUsageScanner::codexPricingMap().constBegin(); mit != CostUsageScanner::codexPricingMap().constEnd(); ++mit) {
        if (modelName.contains(mit.key(), Qt::CaseInsensitive)) return mit.value();
    }
    for (auto mit = CostUsageScanner::claudePricingMap().constBegin(); mit != CostUsageScanner::claudePricingMap().constEnd(); ++mit) {
        if (modelName.contains(mit.key(), Qt::CaseInsensitive)) return mit.value();
    }

    if (modelName.contains("gpt", Qt::CaseInsensitive) || modelName.contains("codex", Qt::CaseInsensitive))
        return { 1.25, 0.125, 0, 10.0 };
    if (modelName.contains("opus", Qt::CaseInsensitive))
        return { 15.0, 1.5, 18.75, 75.0 };
    if (modelName.contains("sonnet", Qt::CaseInsensitive))
        return { 3.0, 0.3, 3.75, 15.0 };
    if (modelName.contains("haiku", Qt::CaseInsensitive))
        return { 1.0, 0.1, 1.25, 5.0 };
    return { 3.0, 0.3, 3.75, 15.0 };
}

QString CostUsageScanner::cacheDir() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
           + "/CodexBar/cost-usage";
}

QString CostUsageScanner::stripQuotes(const QString& s) {
    QString r = s;
    if (r.startsWith('"') && r.endsWith('"')) r = r.mid(1, r.length() - 2);
    return r;
}

std::optional<QString> CostUsageScanner::extractString(const QByteArray& line, const char* key, int startPos) {
    QByteArray search = "\"" + QByteArray(key) + "\"";
    int idx = line.indexOf(search, startPos);
    if (idx < 0) return std::nullopt;
    int colon = line.indexOf(':', idx + search.length());
    if (colon < 0) return std::nullopt;
    int start = colon + 1;
    while (start < line.size() && (line[start] == ' ' || line[start] == '"')) start++;
    if (start >= line.size()) return std::nullopt;
    if (line[start] == '"') {
        int end = line.indexOf('"', start + 1);
        if (end < 0) return std::nullopt;
        return QString::fromUtf8(line.mid(start + 1, end - start - 1));
    }
    int end = start;
    while (end < line.size() && (line[end] != ',' && line[end] != '}' && line[end] != '\n' && line[end] != '\r'))
        end++;
    return QString::fromUtf8(line.mid(start, end - start));
}

std::optional<int> CostUsageScanner::extractInt(const QByteArray& line, const char* key, int startPos) {
    auto s = extractString(line, key, startPos);
    if (!s.has_value()) return std::nullopt;
    bool ok = false;
    int v = s->toInt(&ok);
    return ok ? std::optional<int>(v) : std::nullopt;
}

static QString scanClaudeFiles(const QString& configDir, const QDate& since, const QDate& until) {
    Q_UNUSED(since)
    Q_UNUSED(until)
    if (!QDir(configDir).exists()) return {};
    return configDir;
}

static QDateTime parseJsonTimestamp(const QJsonValue& value) {
    if (value.isString()) {
        const QString text = value.toString();
        QDateTime dt = QDateTime::fromString(text, Qt::ISODateWithMs);
        if (!dt.isValid()) dt = QDateTime::fromString(text, Qt::ISODate);
        return dt;
    }

    if (value.isDouble()) {
        const qint64 raw = static_cast<qint64>(value.toDouble());
        if (raw > 0) {
            return QDateTime::fromMSecsSinceEpoch(raw > 1000000000000LL ? raw : raw * 1000);
        }
    }

    return {};
}

// ---------------------------------------------------------------------------
// Cache helpers
// ---------------------------------------------------------------------------
static void mergeFileIntoGlobal(
    QHash<QString, QHash<QString, CostUsageModelBreakdown>>& global,
    const QHash<QString, QHash<QString, CostUsageModelBreakdown>>& file)
{
    for (auto dit = file.constBegin(); dit != file.constEnd(); ++dit) {
        const QString& dateKey = dit.key();
        for (auto mit = dit.value().constBegin(); mit != dit.value().constEnd(); ++mit) {
            const CostUsageModelBreakdown& src = mit.value();
            CostUsageModelBreakdown& dst = global[dateKey][mit.key()];
            dst.modelName = src.modelName;
            dst.inputTokens += src.inputTokens;
            dst.cacheReadTokens += src.cacheReadTokens;
            dst.cacheCreationTokens += src.cacheCreationTokens;
            dst.outputTokens += src.outputTokens;
        }
    }
}

static QHash<QString, QVector<CostUsageModelBreakdown>> convertToCacheContributions(
    const QHash<QString, QHash<QString, CostUsageModelBreakdown>>& file)
{
    QHash<QString, QVector<CostUsageModelBreakdown>> result;
    for (auto dit = file.constBegin(); dit != file.constEnd(); ++dit) {
        QVector<CostUsageModelBreakdown> vec;
        for (auto mit = dit.value().constBegin(); mit != dit.value().constEnd(); ++mit) {
            vec.append(mit.value());
        }
        result[dit.key()] = vec;
    }
    return result;
}

static void mergeCachedIntoGlobal(
    QHash<QString, QHash<QString, CostUsageModelBreakdown>>& global,
    const QHash<QString, QVector<CostUsageModelBreakdown>>& cached)
{
    for (auto dit = cached.constBegin(); dit != cached.constEnd(); ++dit) {
        const QString& dateKey = dit.key();
        for (const auto& src : dit.value()) {
            CostUsageModelBreakdown& dst = global[dateKey][src.modelName];
            dst.modelName = src.modelName;
            dst.inputTokens += src.inputTokens;
            dst.cacheReadTokens += src.cacheReadTokens;
            dst.cacheCreationTokens += src.cacheCreationTokens;
            dst.outputTokens += src.outputTokens;
        }
    }
}

CostUsageSnapshot CostUsageScanner::scanClaude(const QString& configDir, const QDate& since, const QDate& until) {
    CostUsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    QStringList roots;
    if (!configDir.isEmpty()) {
        for (auto& r : configDir.split(',')) {
            QString trimmed = r.trimmed();
            if (!trimmed.isEmpty()) roots.append(trimmed);
        }
    }
    // P2: Support CLAUDE_CONFIG_DIR environment variable
    if (roots.isEmpty()) {
        QString claudeConfigDir = qEnvironmentVariable("CLAUDE_CONFIG_DIR").trimmed();
        if (!claudeConfigDir.isEmpty()) {
            for (auto& part : claudeConfigDir.split(',')) {
                QString trimmed = part.trimmed();
                if (trimmed.isEmpty()) continue;
                QDir d(trimmed);
                if (d.dirName() == "projects" || d.dirName() == "transcripts") {
                    roots.append(trimmed);
                } else {
                    roots.append(trimmed + "/projects");
                    roots.append(trimmed + "/transcripts");
                }
            }
        }
    }
    if (roots.isEmpty()) {
        QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        roots.append(home + "/.claude/projects");
        roots.append(home + "/.claude/transcripts");
        roots.append(home + "/.config/claude/projects");
        roots.append(home + "/.config/claude/transcripts");
    }

    QHash<QString, QHash<QString, CostUsageModelBreakdown>> dayModels;

    for (auto& root : roots) {
        QDir dir(root);
        if (!dir.exists()) continue;

        QStringList jsonlFiles;
        QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot | QDir::Readable;
        QDirIterator it(root, {"*.jsonl"}, filters, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            if (g_shuttingDown) break;
            jsonlFiles.append(it.next());
        }

        for (auto& path : jsonlFiles) {
            if (g_shuttingDown) break;
            QFileInfo info(path);

            // Cache hit: reuse parsed contributions
            if (m_cache && m_cache->isValid(path, info)) {
                mergeCachedIntoGlobal(dayModels, m_cache->contributions(path));
                continue;
            }

            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            struct ClaudeRow {
                QString dateKey, model;
                int input = 0, cacheRead = 0, cacheCreate = 0, output = 0;
                QString msgId, reqId, sessionId;
            };
            QHash<QString, ClaudeRow> keyedRows;
            QVector<ClaudeRow> unkeyedRows;
            // Per-file accumulator for cache
            QHash<QString, QHash<QString, CostUsageModelBreakdown>> fileDayModels;

        while (!file.atEnd()) {
            if (g_shuttingDown) break;
            QByteArray line = file.readLine().trimmed();
                if (line.isEmpty()) continue;
                if (!line.contains("\"type\":\"assistant\"") && !line.contains("\"type\": \"assistant\"")) continue;
                if (!line.contains("\"usage\"")) continue;

                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(line, &err);
                if (err.error != QJsonParseError::NoError) continue;

                QJsonObject obj = doc.object();
                QJsonObject msg = obj["message"].toObject();
                QJsonObject usage = msg["usage"].toObject();
                if (usage.isEmpty()) continue;

                QString model = msg["model"].toString();
                if (model.isEmpty()) model = obj["model"].toString();

                QString msgId = msg["id"].toString();
                QString reqId = obj["requestId"].toString();
                QString sessionId = obj["sessionId"].toString();
                if (sessionId.isEmpty()) sessionId = obj["session_id"].toString();

                QDateTime dt = parseJsonTimestamp(obj["timestamp"]);
                if (!dt.isValid()) continue;
                QString dateKey = dt.date().toString("yyyy-MM-dd");

                QDate d = dt.date();
                if (d < since || d > until) continue;

                ClaudeRow row;
                row.dateKey = dateKey;
                row.model = model;
                row.input = usage["input_tokens"].toInt(0);
                row.cacheRead = usage["cache_read_input_tokens"].toInt(0);
                row.cacheCreate = usage["cache_creation_input_tokens"].toInt(0);
                row.output = usage["output_tokens"].toInt(0);
                row.msgId = msgId;
                row.reqId = reqId;
                row.sessionId = sessionId;

                if (!msgId.isEmpty() && !reqId.isEmpty()) {
                    QString key = msgId + ":" + reqId;
                    keyedRows[key] = row;
                } else {
                    unkeyedRows.append(row);
                }
            }

            auto addRow = [&](const ClaudeRow& row) {
                auto& models = fileDayModels[row.dateKey];
                auto& mb = models[row.model];
                mb.modelName = row.model;
                mb.inputTokens += row.input;
                mb.cacheReadTokens += row.cacheRead;
                mb.cacheCreationTokens += row.cacheCreate;
                mb.outputTokens += row.output;
            };

            for (auto& row : keyedRows) addRow(row);
            for (auto& row : unkeyedRows) addRow(row);

            // Merge per-file result into global and cache it
            mergeFileIntoGlobal(dayModels, fileDayModels);
            if (m_cache) {
                m_cache->setContributions(path, "claude", convertToCacheContributions(fileDayModels));
            }
        }
    }

    QHash<QString, CostUsageDailyEntry> dayMap;
    for (auto dit = dayModels.constBegin(); dit != dayModels.constEnd(); ++dit) {
        CostUsageDailyEntry entry;
        entry.date = dit.key();
        for (auto mit = dit.value().constBegin(); mit != dit.value().constEnd(); ++mit) {
            auto mb = mit.value();
            Pricing p = priceForModel(mb.modelName);
            double cost = costForClaudeModel(p, mb.inputTokens, mb.cacheReadTokens, mb.cacheCreationTokens, mb.outputTokens);
            mb.costUSD = cost;
            entry.inputTokens += mb.inputTokens;
            entry.cacheReadTokens += mb.cacheReadTokens;
            entry.cacheCreationTokens += mb.cacheCreationTokens;
            entry.outputTokens += mb.outputTokens;
            entry.costUSD += cost;
            entry.models.append(mb);
        }
        dayMap[entry.date] = entry;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    int total30 = 0;
    double cost30 = 0;

    for (auto cit = dayMap.constBegin(); cit != dayMap.constEnd(); ++cit) {
        auto& entry = cit.value();
        snap.daily.append(entry);
        total30 += entry.totalTokens();
        cost30 += entry.costUSD;
        if (entry.date == today) {
            snap.sessionTokens = entry.totalTokens();
            snap.sessionCostUSD = entry.costUSD;
        }
    }

    std::sort(snap.daily.begin(), snap.daily.end(),
              [](const CostUsageDailyEntry& a, const CostUsageDailyEntry& b) {
                  return a.date < b.date;
              });

    snap.last30DaysTokens = total30;
    snap.last30DaysCostUSD = cost30;
    return snap;
}

CostUsageSnapshot CostUsageScanner::scanCodex(const QString& sessionsDir, const QDate& since, const QDate& until) {
    CostUsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    // P2: Support CODEX_HOME env var and archived_sessions
    QStringList roots;
    if (!sessionsDir.isEmpty()) {
        roots.append(sessionsDir);
    } else {
        QString codexHome = qEnvironmentVariable("CODEX_HOME").trimmed();
        QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString sessRoot = codexHome.isEmpty()
            ? (home + "/.codex/sessions")
            : (codexHome + "/sessions");
        roots.append(sessRoot);
        // Also scan archived_sessions sibling directory
        QFileInfo fi(sessRoot);
        QString archivedRoot = fi.absolutePath() + "/archived_sessions";
        if (QDir(archivedRoot).exists())
            roots.append(archivedRoot);
    }

    QHash<QString, QHash<QString, CostUsageModelBreakdown>> dayModels;

    QStringList jsonlFiles;
    for (const auto& root : roots) {
        if (!QDir(root).exists()) continue;
        QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot | QDir::Readable;
        QDirIterator it(root, {"*.jsonl"}, filters, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            if (g_shuttingDown) break;
            jsonlFiles.append(it.next());
        }
    }

    for (auto& path : jsonlFiles) {
        if (g_shuttingDown) break;
        QFileInfo info(path);

        // Cache hit: reuse parsed contributions
        if (m_cache && m_cache->isValid(path, info)) {
            mergeCachedIntoGlobal(dayModels, m_cache->contributions(path));
            continue;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        struct CodexTotals { int input = 0, cached = 0, output = 0; };
        CodexTotals prevTotals;
        QString currentModel;         // P0: track model from turn_context
        bool isForked = false;        // P1: fork session detection
        bool seenFirstTotal = false;  // P1: baseline for forked sessions
        QHash<QString, QHash<QString, CostUsageModelBreakdown>> fileDayModels;

        while (!file.atEnd()) {
            QByteArray line = file.readLine().trimmed();
            if (line.isEmpty()) continue;

            bool hasEventMsg = line.contains("\"type\":\"event_msg\"");
            bool hasTurnCtx  = line.contains("\"type\":\"turn_context\"");
            bool hasSesMeta  = line.contains("\"type\":\"session_meta\"");
            if (!hasEventMsg && !hasTurnCtx && !hasSesMeta) continue;
            if (hasEventMsg && !line.contains("\"token_count\"")) continue;

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(line, &err);
            if (err.error != QJsonParseError::NoError) continue;

            QJsonObject obj = doc.object();
            QString type = obj["type"].toString();

            // P1: Parse session_meta for fork detection
            if (type == "session_meta") {
                QJsonObject payload = obj["payload"].toObject();
                QString forkedFrom = payload["forked_from_id"].toString();
                if (forkedFrom.isEmpty()) forkedFrom = payload["forkedFromId"].toString();
                if (forkedFrom.isEmpty()) forkedFrom = payload["parent_session_id"].toString();
                if (forkedFrom.isEmpty()) forkedFrom = payload["parentSessionId"].toString();
                if (!forkedFrom.isEmpty())
                    isForked = true;
                continue;
            }

            // P0: Save turn_context model as fallback (was Q_UNUSED)
            if (type == "turn_context") {
                QJsonObject payload = obj["payload"].toObject();
                QString model = payload["model"].toString();
                if (model.isEmpty()) model = payload["info"].toObject()["model"].toString();
                if (!model.isEmpty())
                    currentModel = model;
                continue;
            }

            if (type == "event_msg") {
                QJsonObject payload = obj["payload"].toObject();
                if (payload["type"].toString() != "token_count") continue;
                QJsonObject info = payload["info"].toObject();

                // P0: Use currentModel from turn_context as fallback
                QString model = info["model"].toString();
                if (model.isEmpty()) model = info["model_name"].toString();
                if (model.isEmpty()) model = payload["model"].toString();
                if (model.isEmpty()) model = obj["model"].toString();
                if (model.isEmpty() && !currentModel.isEmpty()) model = currentModel;
                if (model.isEmpty()) model = "gpt-5";

                QDateTime dt = parseJsonTimestamp(obj["timestamp"]);
                if (!dt.isValid()) dt = parseJsonTimestamp(payload["timestamp"]);
                if (!dt.isValid()) continue;
                QString dateKey = dt.date().toString("yyyy-MM-dd");
                if (dt.date() < since || dt.date() > until) continue;

                QJsonObject totalUsage = info["total_token_usage"].toObject();
                QJsonObject lastUsage = info["last_token_usage"].toObject();

                if (!totalUsage.isEmpty()) {
                    int input = totalUsage["input_tokens"].toInt(0);
                    int cached = totalUsage["cached_input_tokens"].toInt(0);
                    int cacheRead = totalUsage["cache_read_input_tokens"].toInt(cached);
                    int output = totalUsage["output_tokens"].toInt(0);

                    // P1: For forked sessions, use first total as baseline to avoid
                    // double-counting inherited parent tokens
                    if (isForked && !seenFirstTotal) {
                        prevTotals = { input, cacheRead, output };
                        seenFirstTotal = true;
                        continue;
                    }
                    seenFirstTotal = true;

                    int dInput = qMax(0, input - prevTotals.input);
                    int dCached = qMax(0, cacheRead - prevTotals.cached);
                    int dOutput = qMax(0, output - prevTotals.output);
                    prevTotals = { input, cacheRead, output };

                    auto& mb = fileDayModels[dateKey][model];
                    mb.modelName = model;
                    mb.inputTokens += dInput;
                    mb.cacheReadTokens += dCached;
                    mb.outputTokens += dOutput;
                }
                // P2: Handle last_token_usage as incremental delta
                else if (!lastUsage.isEmpty()) {
                    int dInput = lastUsage["input_tokens"].toInt(0);
                    int dCached = lastUsage["cached_input_tokens"].toInt(0);
                    if (dCached == 0) dCached = lastUsage["cache_read_input_tokens"].toInt(0);
                    int dOutput = lastUsage["output_tokens"].toInt(0);

                    prevTotals.input += dInput;
                    prevTotals.cached += dCached;
                    prevTotals.output += dOutput;

                    auto& mb = fileDayModels[dateKey][model];
                    mb.modelName = model;
                    mb.inputTokens += dInput;
                    mb.cacheReadTokens += dCached;
                    mb.outputTokens += dOutput;
                }
            }
        }

        // Merge per-file result into global and cache it
        mergeFileIntoGlobal(dayModels, fileDayModels);
        if (m_cache) {
            m_cache->setContributions(path, "codex", convertToCacheContributions(fileDayModels));
        }
    }

    QHash<QString, CostUsageDailyEntry> dayMap;
    for (auto dit = dayModels.constBegin(); dit != dayModels.constEnd(); ++dit) {
        CostUsageDailyEntry entry;
        entry.date = dit.key();
        for (auto mit = dit.value().constBegin(); mit != dit.value().constEnd(); ++mit) {
            auto mb = mit.value();
            QString normModel = normalizeCodexModel(mb.modelName);
            Pricing p = priceForModel(normModel);
            double cost = costForCodexModel(p, mb.inputTokens, mb.cacheReadTokens, mb.outputTokens);
            mb.costUSD = cost;
            entry.inputTokens += mb.inputTokens;
            entry.cacheReadTokens += mb.cacheReadTokens;
            entry.outputTokens += mb.outputTokens;
            entry.costUSD += cost;
            entry.models.append(mb);
        }
        dayMap[entry.date] = entry;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    int total30 = 0;
    double cost30 = 0;
    for (auto cit = dayMap.constBegin(); cit != dayMap.constEnd(); ++cit) {
        auto& entry = cit.value();
        snap.daily.append(entry);
        total30 += entry.totalTokens();
        cost30 += entry.costUSD;
        if (entry.date == today) {
            snap.sessionTokens += entry.totalTokens();
            snap.sessionCostUSD += entry.costUSD;
        }
    }
    std::sort(snap.daily.begin(), snap.daily.end(),
              [](const CostUsageDailyEntry& a, const CostUsageDailyEntry& b) { return a.date < b.date; });
    snap.last30DaysTokens = total30;
    snap.last30DaysCostUSD = cost30;
    return snap;
}

// ---------------------------------------------------------------------------
// P1: Pi session scanner — ported from PiSessionCostScanner.swift
// Scans ~/.pi/agent/sessions/ JSONL files, categorises by provider.
// ---------------------------------------------------------------------------
CostUsageScanner::PiScanResult CostUsageScanner::scanPi(const QDate& since, const QDate& until) {
    PiScanResult result;
    result.codex.updatedAt = QDateTime::currentDateTime();
    result.claude.updatedAt = QDateTime::currentDateTime();

    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString piRoot = home + "/.pi/agent/sessions";
    if (!QDir(piRoot).exists()) return result;

    QStringList jsonlFiles;
    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot | QDir::Readable;
    QDirIterator it(piRoot, {"*.jsonl"}, filters, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (g_shuttingDown) break;
        jsonlFiles.append(it.next());
    }

    // provider-key → day → model → breakdown
    // provider-key is "codex" or "claude"
    QHash<QString, QHash<QString, QHash<QString, CostUsageModelBreakdown>>> providerDayModels;

    for (auto& path : jsonlFiles) {
        if (g_shuttingDown) break;
        // TODO: Pi files may contain both codex and claude data.
        // Per-file cache needs multi-provider support; skipping for now.
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        // Current model context from model_change events
        QString ctxProvider;  // mapped: "codex" or "claude"
        QString ctxModel;

        while (!file.atEnd()) {
            QByteArray line = file.readLine().trimmed();
            if (line.isEmpty()) continue;

            // Quick filter
            bool hasModelChange = line.contains("\"model_change\"");
            bool hasMessage = line.contains("\"message\"");
            if (!hasModelChange && !hasMessage) continue;

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(line, &err);
            if (err.error != QJsonParseError::NoError) continue;

            QJsonObject obj = doc.object();
            QString type = obj["type"].toString();

            // Handle model_change events to update provider/model context
            if (type == "model_change") {
                QString rawProvider = obj["provider"].toString().toLower().trimmed();
                QString rawModel = obj["modelId"].toString().trimmed();
                if (rawModel.isEmpty()) rawModel = obj["model"].toString().trimmed();

                // Map Pi provider names to our provider keys
                if (rawProvider == "openai-codex") {
                    ctxProvider = "codex";
                    ctxModel = rawModel.isEmpty() ? "gpt-5" : normalizeCodexModel(rawModel);
                } else if (rawProvider == "anthropic") {
                    ctxProvider = "claude";
                    ctxModel = rawModel.isEmpty() ? "claude-sonnet-4-5" : normalizeClaudeModel(rawModel);
                } else {
                    ctxProvider.clear();
                    ctxModel.clear();
                }
                continue;
            }

            // Handle message events with role=assistant
            if (type == "message") {
                QJsonObject msg = obj["message"].toObject();
                if (msg["role"].toString() != "assistant") continue;

                // Resolve provider/model from explicit fields or fallback to context
                QString explicitProvider = msg["provider"].toString().toLower().trimmed();
                if (explicitProvider.isEmpty()) explicitProvider = obj["provider"].toString().toLower().trimmed();
                QString explicitModel = msg["model"].toString().trimmed();
                if (explicitModel.isEmpty()) explicitModel = obj["model"].toString().trimmed();
                if (explicitModel.isEmpty()) explicitModel = msg["modelId"].toString().trimmed();

                QString mappedProvider;
                QString mappedModel;

                if (explicitProvider == "openai-codex") {
                    mappedProvider = "codex";
                    mappedModel = explicitModel.isEmpty() ? ctxModel : normalizeCodexModel(explicitModel);
                } else if (explicitProvider == "anthropic") {
                    mappedProvider = "claude";
                    mappedModel = explicitModel.isEmpty() ? ctxModel : normalizeClaudeModel(explicitModel);
                } else if (explicitProvider.isEmpty()) {
                    // Fall back to context
                    mappedProvider = ctxProvider;
                    if (!explicitModel.isEmpty() && mappedProvider == "codex")
                        mappedModel = normalizeCodexModel(explicitModel);
                    else if (!explicitModel.isEmpty() && mappedProvider == "claude")
                        mappedModel = normalizeClaudeModel(explicitModel);
                    else
                        mappedModel = ctxModel;
                } else {
                    continue; // Unknown provider, skip
                }

                if (mappedProvider.isEmpty() || mappedModel.isEmpty()) continue;

                // Extract timestamp and check date range
                QDateTime dt = parseJsonTimestamp(msg["timestamp"]);
                if (!dt.isValid()) dt = parseJsonTimestamp(obj["timestamp"]);
                if (!dt.isValid()) continue;
                if (dt.date() < since || dt.date() > until) continue;
                QString dateKey = dt.date().toString("yyyy-MM-dd");

                // Extract usage
                QJsonObject usage = msg["usage"].toObject();
                if (usage.isEmpty()) continue;

                auto readInt = [&](const QJsonObject& u, const QStringList& keys) -> int {
                    for (auto& k : keys) {
                        if (u.contains(k)) return u[k].toInt(0);
                    }
                    return 0;
                };

                int input = readInt(usage, {"input", "inputTokens", "input_tokens", "promptTokens", "prompt_tokens"});
                int cacheRead = readInt(usage, {"cacheRead", "cacheReadTokens", "cache_read", "cache_read_tokens",
                                                "cacheReadInputTokens", "cache_read_input_tokens"});
                int cacheWrite = readInt(usage, {"cacheWrite", "cacheWriteTokens", "cache_write", "cache_write_tokens",
                                                 "cacheCreationTokens", "cache_creation_tokens",
                                                 "cacheCreationInputTokens", "cache_creation_input_tokens"});
                int output = readInt(usage, {"output", "outputTokens", "output_tokens",
                                             "completionTokens", "completion_tokens"});

                if (input == 0 && cacheRead == 0 && cacheWrite == 0 && output == 0) continue;

                auto& mb = providerDayModels[mappedProvider][dateKey][mappedModel];
                mb.modelName = mappedModel;
                mb.inputTokens += input;
                mb.cacheReadTokens += cacheRead;
                mb.cacheCreationTokens += cacheWrite;
                mb.outputTokens += output;
            }
        }
    }

    // Build snapshots for each provider
    auto buildSnapshot = [&](const QString& providerKey, bool isClaude) -> CostUsageSnapshot {
        CostUsageSnapshot snap;
        snap.updatedAt = QDateTime::currentDateTime();

        auto provIt = providerDayModels.constFind(providerKey);
        if (provIt == providerDayModels.constEnd()) return snap;

        QHash<QString, CostUsageDailyEntry> dayMap;
        for (auto dit = provIt->constBegin(); dit != provIt->constEnd(); ++dit) {
            CostUsageDailyEntry entry;
            entry.date = dit.key();
            for (auto mit = dit->constBegin(); mit != dit->constEnd(); ++mit) {
                auto mb = mit.value();
                Pricing p = priceForModel(mb.modelName);
                double cost = isClaude
                    ? costForClaudeModel(p, mb.inputTokens, mb.cacheReadTokens, mb.cacheCreationTokens, mb.outputTokens)
                    : costForCodexModel(p, mb.inputTokens, mb.cacheReadTokens, mb.outputTokens);
                mb.costUSD = cost;
                entry.inputTokens += mb.inputTokens;
                entry.cacheReadTokens += mb.cacheReadTokens;
                entry.cacheCreationTokens += mb.cacheCreationTokens;
                entry.outputTokens += mb.outputTokens;
                entry.costUSD += cost;
                entry.models.append(mb);
            }
            dayMap[entry.date] = entry;
        }

        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        int total30 = 0;
        double cost30 = 0;
        for (auto cit = dayMap.constBegin(); cit != dayMap.constEnd(); ++cit) {
            auto& entry = cit.value();
            snap.daily.append(entry);
            total30 += entry.totalTokens();
            cost30 += entry.costUSD;
            if (entry.date == today) {
                snap.sessionTokens += entry.totalTokens();
                snap.sessionCostUSD += entry.costUSD;
            }
        }
        std::sort(snap.daily.begin(), snap.daily.end(),
                  [](const CostUsageDailyEntry& a, const CostUsageDailyEntry& b) { return a.date < b.date; });
        snap.last30DaysTokens = total30;
        snap.last30DaysCostUSD = cost30;
        return snap;
    };

    result.codex = buildSnapshot("codex", false);
    result.claude = buildSnapshot("claude", true);
    return result;
}

CostUsageSnapshot CostUsageScanner::scanOpenCodeGo(const QDate& since, const QDate& until) {
    Q_UNUSED(since)
    Q_UNUSED(until)
    // TODO: Implement OpenCode Go-specific scanning (not via opencode.db)
    return CostUsageSnapshot();
}

QHash<QString, CostUsageSnapshot> CostUsageScanner::scanOpenCodeDB(const QDate& since, const QDate& until) {
    QHash<QString, CostUsageSnapshot> result;

    // OpenCode stores session data in SQLite at ~/.local/share/opencode/opencode.db
    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString dbPath = home + "/.local/share/opencode/opencode.db";

    // Windows fallback: check AppData/Local or AppData/Roaming
    if (!QFile::exists(dbPath)) {
        QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        dbPath = localAppData + "/opencode/opencode.db";
    }

    if (!QFile::exists(dbPath)) {
        return result;
    }

    const QString connName = "opencode_db_scan_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase::removeDatabase(connName);
    }

    // Group by provider
    struct ProviderData {
        QHash<QString, QHash<QString, CostUsageModelBreakdown>> dayModels;
    };
    QHash<QString, ProviderData> providerData;

    // Scope the QSqlDatabase lifetime so removeDatabase never warns about "still in use".
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath);
        db.setConnectOptions("QSQLITE_OPEN_READONLY"); // avoid write-lock waits
        if (!db.open()) {
            // db destroyed at end of scope
            QSqlDatabase::removeDatabase(connName);
            return result;
        }

        // Speed up read-only queries
        {
            QSqlQuery pragma(db);
            pragma.exec("PRAGMA journal_mode = WAL");
            pragma.exec("PRAGMA synchronous = NORMAL");
            pragma.exec("PRAGMA temp_store = MEMORY");
            pragma.exec("PRAGMA mmap_size = 268435456");
        }

        if (g_shuttingDown) {
            db.close();
            // db destroyed at end of scope
            QSqlDatabase::removeDatabase(connName);
            return result;
        }

        // Query correlates assistant messages (with tokens) to the most recent user message (with model info)
        // within the same session. Groups by provider, date, and model.
        // Filters out baiduqianfancodingplan (discarded per user decision).
        QString sql = R"(
            WITH user_models AS (
                SELECT
                    session_id,
                    time_created,
                    json_extract(data, '$.model.providerID') AS provider,
                    json_extract(data, '$.model.modelID') AS model
                FROM message
                WHERE json_extract(data, '$.role') = 'user'
                  AND json_extract(data, '$.model') IS NOT NULL
            ),
            assistant_tokens AS (
                SELECT
                    session_id,
                    time_created,
                    json_extract(data, '$.tokens.input') AS input_tokens,
                    json_extract(data, '$.tokens.output') AS output_tokens,
                    json_extract(data, '$.tokens.cache.read') AS cache_read
                FROM message
                WHERE json_extract(data, '$.role') = 'assistant'
                  AND json_extract(data, '$.tokens') IS NOT NULL
            )
            SELECT
                um.provider AS provider_id,
                DATE(a.time_created / 1000, 'unixepoch') AS day,
                um.provider || '/' || um.model AS model_name,
                COUNT(*) AS message_count,
                SUM(a.input_tokens) AS input_tokens,
                SUM(a.output_tokens) AS output_tokens,
                SUM(a.cache_read) AS cache_read
            FROM assistant_tokens a
            LEFT JOIN user_models um
                ON a.session_id = um.session_id
                AND um.time_created = (
                    SELECT MAX(time_created)
                    FROM user_models
                    WHERE session_id = a.session_id
                      AND time_created <= a.time_created
                )
            WHERE um.provider IS NOT NULL
              AND um.provider != 'baiduqianfancodingplan'
            GROUP BY um.provider, day, um.model
            ORDER BY um.provider, day, input_tokens DESC
        )";

        {
            QSqlQuery query(db);
            query.setForwardOnly(true); // reduces memory overhead for large result sets
            query.prepare(sql);
            if (!query.exec() || g_shuttingDown) {
                // query destroyed at end of scope, then db destroyed, then removeDatabase is safe
                db.close();
                QSqlDatabase::removeDatabase(connName);
                return result;
            }

            while (query.next()) {
                if (g_shuttingDown) break;
                QString providerId = query.value("provider_id").toString();
                QString day = query.value("day").toString();
                QString model = query.value("model_name").toString();
                int inputTokens = query.value("input_tokens").toInt();
                int outputTokens = query.value("output_tokens").toInt();
                int cacheRead = query.value("cache_read").toInt();

                auto& mb = providerData[providerId].dayModels[day][model];
                mb.modelName = model;
                mb.inputTokens += qMax(0, inputTokens);
                mb.outputTokens += qMax(0, outputTokens);
                mb.cacheReadTokens += qMax(0, cacheRead);
            }
        } // QSqlQuery destroyed here

        db.close();
    } // QSqlDatabase destroyed here — now safe to removeDatabase

    QSqlDatabase::removeDatabase(connName);

    // Map raw provider IDs from opencode.db to standard CodexBar provider IDs
    QHash<QString, QString> providerIdMap;
    providerIdMap["opencode-go"] = "opencodego";
    providerIdMap["kimi-for-coding"] = "kimi";
    providerIdMap["kimi"] = "kimi";

    // Build snapshots for each provider found in the database
    for (auto pit = providerData.constBegin(); pit != providerData.constEnd(); ++pit) {
        if (g_shuttingDown) break;
        QString rawProviderId = pit.key();
        QString providerId = providerIdMap.value(rawProviderId, rawProviderId);
        CostUsageSnapshot snap;
        snap.updatedAt = QDateTime::currentDateTime();

        QHash<QString, CostUsageDailyEntry> dayMap;
        for (auto dit = pit->dayModels.constBegin(); dit != pit->dayModels.constEnd(); ++dit) {
            CostUsageDailyEntry entry;
            entry.date = dit.key();
            for (auto mit = dit.value().constBegin(); mit != dit.value().constEnd(); ++mit) {
                auto mb = mit.value();
                Pricing p = priceForModel(mb.modelName);
                double cost = costForCodexModel(p, mb.inputTokens, mb.cacheReadTokens, mb.outputTokens);
                mb.costUSD = cost;
                entry.inputTokens += mb.inputTokens;
                entry.cacheReadTokens += mb.cacheReadTokens;
                entry.outputTokens += mb.outputTokens;
                entry.costUSD += cost;
                entry.models.append(mb);
            }
            dayMap[entry.date] = entry;
        }

        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        int total30 = 0;
        double cost30 = 0;
        for (auto cit = dayMap.constBegin(); cit != dayMap.constEnd(); ++cit) {
            auto& entry = cit.value();
            snap.daily.append(entry);
            total30 += entry.totalTokens();
            cost30 += entry.costUSD;
            if (entry.date == today) {
                snap.sessionTokens += entry.totalTokens();
                snap.sessionCostUSD += entry.costUSD;
            }
        }
        std::sort(snap.daily.begin(), snap.daily.end(),
                  [](const CostUsageDailyEntry& a, const CostUsageDailyEntry& b) { return a.date < b.date; });
        snap.last30DaysTokens = total30;
        snap.last30DaysCostUSD = cost30;
        result[providerId] = snap;
    }

    return result;
}
