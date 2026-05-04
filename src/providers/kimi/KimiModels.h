#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <optional>

/**
 * @brief Structured data models for Kimi provider API responses.
 *
 * Mirrors the upstream Kimi Swift data models.
 */

struct KimiUsageDetail {
    std::optional<double> limit;
    std::optional<double> used;
    std::optional<double> remaining;
    QString resetTime; // ISO-8601 string

    static KimiUsageDetail fromJson(const QJsonObject& obj) {
        KimiUsageDetail detail;
        // Try multiple field names for flexibility
        detail.limit = parseKimiNumber(obj, {"limit", "total", "quota", "max"});
        detail.used = parseKimiNumber(obj, {"used", "consumed", "count", "requests"});
        detail.remaining = parseKimiNumber(obj, {"remaining", "left", "available"});
        detail.resetTime = obj.value("resetTime").toString();
        return detail;
    }

private:
    static std::optional<double> parseKimiNumber(const QJsonObject& obj, const std::vector<QString>& keys) {
        for (const auto& key : keys) {
            if (obj.contains(key)) {
                QJsonValue v = obj.value(key);
                if (v.isString()) {
                    bool ok = false;
                    double d = v.toString().toDouble(&ok);
                    if (ok) return d;
                } else if (v.isDouble()) {
                    return v.toDouble();
                }
            }
        }
        return std::nullopt;
    }
};

struct KimiRateLimit {
    int windowDuration = 0; // in minutes
    QString windowTimeUnit;
    KimiUsageDetail detail;

    static KimiRateLimit fromJson(const QJsonObject& obj) {
        KimiRateLimit limit;
        QJsonObject window = obj.value("window").toObject();
        limit.windowDuration = window.value("duration").toInt();
        limit.windowTimeUnit = window.value("timeUnit").toString();
        limit.detail = KimiUsageDetail::fromJson(obj.value("detail").toObject());
        return limit;
    }
};

struct KimiCodingUsage {
    QString scope;
    KimiUsageDetail detail;
    QVector<KimiRateLimit> limits;

    static std::optional<KimiCodingUsage> fromJson(const QJsonObject& obj) {
        if (obj.value("scope").toString() != "FEATURE_CODING")
            return std::nullopt;

        KimiCodingUsage usage;
        usage.scope = obj.value("scope").toString();
        usage.detail = KimiUsageDetail::fromJson(obj.value("detail").toObject());

        QJsonArray limitsArr = obj.value("limits").toArray();
        for (const auto& val : limitsArr) {
            usage.limits.append(KimiRateLimit::fromJson(val.toObject()));
        }
        return usage;
    }
};

/**
 * @brief Parse Kimi microsecond-precision ISO-8601 timestamp.
 *
 * Kimi returns microsecond precision (e.g. 2026-05-07T02:43:45.585111Z)
 * Qt::ISODate handles up to milliseconds; strip extra digits.
 */
inline QDateTime parseKimiResetTime(const QString& iso) {
    if (iso.isEmpty()) return {};

    QString simplified = iso;
    int dotIdx = simplified.lastIndexOf('.');
    int zIdx = simplified.lastIndexOf('Z');
    if (dotIdx > 0 && zIdx > dotIdx) {
        QString ms = simplified.mid(dotIdx + 1, zIdx - dotIdx - 1);
        if (ms.length() > 3) {
            ms = ms.left(3);
            simplified = simplified.left(dotIdx + 1) + ms + "Z";
        }
    }
    QDateTime dt = QDateTime::fromString(simplified, Qt::ISODate);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(simplified, Qt::ISODateWithMs);
    }
    return dt;
}
