#include "CLICostCommand.h"
#include "../util/CostUsageScanner.h"
#include "../util/CostUsageCache.h"
#include "../models/CostUsageReport.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

int CLICostCommand::execute(const CLICostOptions& opts, CLIRenderer& out)
{
    QDate since = opts.since;
    QDate until = opts.until;
    if (!since.isValid()) {
        since = QDate::currentDate().addDays(-30);
    }
    if (!until.isValid()) {
        until = QDate::currentDate();
    }

    CostUsageCache& cache = CostUsageCache::instance();
    if (!opts.forceRefresh) {
        cache.load();
    } else {
        cache.clear();
    }

    CostUsageScanner scanner(&cache);

    // Scan Claude and Codex default locations
    QString claudeDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString codexDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions";

    CostUsageSnapshot claudeSnap = scanner.scanClaude(claudeDir, since, until);
    CostUsageSnapshot codexSnap = scanner.scanCodex(codexDir, since, until);

    QJsonArray providers;
    double totalCost = 0.0;
    int totalTokens = 0;

    auto processSnapshot = [&](const QString& providerId, const CostUsageSnapshot& snap) {
        if (opts.providerId != "all" && providerId != opts.providerId) return;
        if (snap.daily.isEmpty() && snap.errorMessage.isEmpty() && snap.last30DaysCostUSD <= 0) return;

        double cost = snap.last30DaysCostUSD;
        int tokens = snap.last30DaysTokens;
        totalCost += cost;
        totalTokens += tokens;

        QJsonArray dailyArray;
        for (const auto& day : snap.daily) {
            QDate d = QDate::fromString(day.date, Qt::ISODate);
            if (!d.isValid() || d < since || d > until) continue;
            QJsonObject dayObj;
            dayObj["date"] = day.date;
            dayObj["costUSD"] = day.costUSD;
            dayObj["tokens"] = day.totalTokens();
            dailyArray.append(dayObj);
        }

        QJsonObject pObj;
        pObj["providerId"] = providerId;
        pObj["totalCostUSD"] = cost;
        pObj["totalTokens"] = tokens;
        pObj["daily"] = dailyArray;
        providers.append(pObj);

        if (!out.isJsonMode()) {
            out.heading(providerId);
            out.line(QString("  Total: $%1 (%2 tokens)")
                     .arg(cost, 0, 'f', 2)
                     .arg(tokens));
            if (!dailyArray.isEmpty()) {
                out.line(QString("  Daily avg: $%1")
                         .arg(cost / dailyArray.size(), 0, 'f', 2));
            }
            if (!snap.errorMessage.isEmpty()) {
                out.error(QString("  Error: %1").arg(snap.errorMessage));
            }
            out.blank();
        }
    };

    processSnapshot("claude", claudeSnap);
    processSnapshot("codex", codexSnap);

    if (out.isJsonMode()) {
        QJsonObject root;
        root["providers"] = providers;
        root["totalCostUSD"] = totalCost;
        root["totalTokens"] = totalTokens;
        root["since"] = since.toString(Qt::ISODate);
        root["until"] = until.toString(Qt::ISODate);
        root["generatedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        out.setRootObject(root);
    } else {
        out.heading("Summary");
        out.line(QString("Period: %1 to %2")
                 .arg(since.toString(Qt::ISODate))
                 .arg(until.toString(Qt::ISODate)));
        out.line(QString("Combined: $%1 (%2 tokens)")
                 .arg(totalCost, 0, 'f', 2)
                 .arg(totalTokens));
    }
    out.flush();

    cache.save();
    return 0;
}
