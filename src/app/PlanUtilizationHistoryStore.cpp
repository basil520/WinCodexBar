#include "PlanUtilizationHistoryStore.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDateTime>
#include <algorithm>

PlanUtilizationHistoryStore::PlanUtilizationHistoryStore(QObject* parent)
    : QObject(parent)
{
    m_saveCoalesceTimer.setSingleShot(true);
    m_saveCoalesceTimer.setInterval(2000);
    connect(&m_saveCoalesceTimer, &QTimer::timeout, this, [this]() {
        for (auto it = m_pendingSave.constBegin(); it != m_pendingSave.constEnd(); ++it) {
            if (it.value()) saveToDisk(it.key());
        }
        m_pendingSave.clear();
    });
}

QString PlanUtilizationHistoryStore::filePath(const QString& providerId) const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/history";
    QDir().mkpath(dir);
    return dir + "/" + providerId + ".json";
}

void PlanUtilizationHistoryStore::loadFromDisk(const QString& providerId) {
    if (m_buckets.contains(providerId)) return;

    QFile file(filePath(providerId));
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return;

    QJsonObject root = doc.object();
    int version = root.value("version").toInt(0);
    if (version != 1) return;

    PlanUtilizationHistoryBuckets buckets;
    buckets.preferredAccountKey = root.value("preferredAccountKey").toString();

    auto parseSeries = [](const QJsonArray& arr) -> QVector<PlanUtilizationSeriesHistory> {
        QVector<PlanUtilizationSeriesHistory> result;
        for (const auto& v : arr) {
            QJsonObject obj = v.toObject();
            PlanUtilizationSeriesHistory sh;
            sh.name = {obj.value("name").toString()};
            sh.windowMinutes = obj.value("windowMinutes").toInt(0);
            QJsonArray entriesArr = obj.value("entries").toArray();
            for (const auto& ev : entriesArr) {
                QJsonObject eo = ev.toObject();
                PlanUtilizationHistoryEntry e;
                e.capturedAt = QDateTime::fromString(eo.value("capturedAt").toString(), Qt::ISODateWithMs);
                e.usedPercent = eo.value("usedPercent").toDouble(0);
                if (eo.contains("resetsAt") && !eo.value("resetsAt").isNull()) {
                    e.resetsAt = QDateTime::fromString(eo.value("resetsAt").toString(), Qt::ISODateWithMs);
                }
                sh.entries.append(e);
            }
            result.append(sh);
        }
        return result;
    };

    buckets.unscoped = parseSeries(root.value("unscoped").toArray());

    QJsonObject accountsObj = root.value("accounts").toObject();
    for (auto it = accountsObj.constBegin(); it != accountsObj.constEnd(); ++it) {
        buckets.accounts[it.key()] = parseSeries(it.value().toArray());
    }

    m_buckets[providerId] = buckets;
}

void PlanUtilizationHistoryStore::saveToDisk(const QString& providerId) {
    auto it = m_buckets.constFind(providerId);
    if (it == m_buckets.constEnd()) return;

    const auto& buckets = it.value();

    auto serializeSeries = [](const QVector<PlanUtilizationSeriesHistory>& series) -> QJsonArray {
        QJsonArray arr;
        for (const auto& sh : series) {
            QJsonObject obj;
            obj["name"] = sh.name.rawValue;
            obj["windowMinutes"] = sh.windowMinutes;
            QJsonArray entriesArr;
            for (const auto& e : sh.entries) {
                QJsonObject eo;
                eo["capturedAt"] = e.capturedAt.toString(Qt::ISODateWithMs);
                eo["usedPercent"] = e.usedPercent;
                if (e.resetsAt.has_value()) {
                    eo["resetsAt"] = e.resetsAt->toString(Qt::ISODateWithMs);
                } else {
                    eo["resetsAt"] = QJsonValue::Null;
                }
                entriesArr.append(eo);
            }
            obj["entries"] = entriesArr;
            arr.append(obj);
        }
        return arr;
    };

    QJsonObject root;
    root["version"] = 1;
    root["preferredAccountKey"] = buckets.preferredAccountKey;
    root["unscoped"] = serializeSeries(buckets.unscoped);

    QJsonObject accountsObj;
    for (auto ait = buckets.accounts.constBegin(); ait != buckets.accounts.constEnd(); ++ait) {
        accountsObj[ait.key()] = serializeSeries(ait.value());
    }
    root["accounts"] = accountsObj;

    QSaveFile file(filePath(providerId));
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.commit();
}

void PlanUtilizationHistoryStore::enqueueSave(const QString& providerId) {
    m_pendingSave[providerId] = true;
    if (!m_saveCoalesceTimer.isActive()) m_saveCoalesceTimer.start();
}

PlanUtilizationHistoryBuckets PlanUtilizationHistoryStore::buckets(const QString& providerId) const {
    auto it = m_buckets.constFind(providerId);
    if (it != m_buckets.constEnd()) return it.value();
    return {};
}

void PlanUtilizationHistoryStore::recordSample(const QString& providerId, const UsageSnapshot& snapshot, const QString& accountKey) {
    loadFromDisk(providerId);
    auto& buckets = m_buckets[providerId];

    QDateTime now = QDateTime::currentDateTime();

    auto addOrUpdateSeries = [&](PlanUtilizationSeriesName name, int windowMinutes, double usedPercent, std::optional<QDateTime> resetsAt) {
        auto& seriesList = accountKey.isEmpty() ? buckets.unscoped : buckets.accounts[accountKey];

        PlanUtilizationSeriesHistory* target = nullptr;
        for (auto& sh : seriesList) {
            if (sh.name == name && sh.windowMinutes == windowMinutes) {
                target = &sh;
                break;
            }
        }
        if (!target) {
            seriesList.append(PlanUtilizationSeriesHistory{name, windowMinutes, {}});
            target = &seriesList.last();
        }

        if (target->entries.size() > 0) {
            qint64 secsSinceLast = target->entries.last().capturedAt.secsTo(now);
            if (secsSinceLast < minSampleIntervalSeconds) return;
        }

        PlanUtilizationHistoryEntry entry;
        entry.capturedAt = now;
        entry.usedPercent = usedPercent;
        entry.resetsAt = resetsAt;
        target->entries.append(entry);

        if (target->entries.size() > maxSamples) {
            target->entries = target->entries.mid(target->entries.size() - maxSamples);
        }
    };

    if (snapshot.primary.has_value()) {
        PlanUtilizationSeriesName name = PlanUtilizationSeriesName::session();
        int windowMin = snapshot.primary->windowMinutes.value_or(300);
        if (windowMin == 10080) name = PlanUtilizationSeriesName::weekly();
        addOrUpdateSeries(name, windowMin, snapshot.primary->usedPercent, snapshot.primary->resetsAt);
    }
    if (snapshot.secondary.has_value()) {
        PlanUtilizationSeriesName name = PlanUtilizationSeriesName::weekly();
        int windowMin = snapshot.secondary->windowMinutes.value_or(10080);
        addOrUpdateSeries(name, windowMin, snapshot.secondary->usedPercent, snapshot.secondary->resetsAt);
    }
    if (snapshot.tertiary.has_value()) {
        addOrUpdateSeries(PlanUtilizationSeriesName::opus(),
                          snapshot.tertiary->windowMinutes.value_or(10080),
                          snapshot.tertiary->usedPercent,
                          snapshot.tertiary->resetsAt);
    }

    if (!accountKey.isEmpty()) {
        buckets.preferredAccountKey = accountKey;
    }

    enqueueSave(providerId);
}

QVariantList PlanUtilizationHistoryStore::chartData(const QString& providerId, const QString& seriesName, const QString& accountKey) const {
    auto it = m_buckets.constFind(providerId);
    if (it == m_buckets.constEnd()) return {};

    const auto& buckets = it.value();
    auto seriesList = buckets.histories(accountKey);

    const PlanUtilizationSeriesHistory* target = nullptr;
    for (const auto& sh : seriesList) {
        if (sh.name.rawValue == seriesName) {
            target = &sh;
            break;
        }
    }
    if (!target) return {};

    int maxPoints = 30;
    int start = std::max(0, static_cast<int>(target->entries.size() - maxPoints));
    QVariantList result;
    for (int i = start; i < target->entries.size(); i++) {
        const auto& e = target->entries[i];
        QVariantMap point;
        point["capturedAt"] = e.capturedAt.toMSecsSinceEpoch();
        point["usedPercent"] = e.usedPercent;
        point["dateLabel"] = e.capturedAt.date().toString("MMM d");
        result.append(point);
    }
    return result;
}
