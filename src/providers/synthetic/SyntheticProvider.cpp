#include "SyntheticProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

SyntheticProvider::SyntheticProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> SyntheticProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new SyntheticAPIStrategy(this) };
}

SyntheticAPIStrategy::SyntheticAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString SyntheticAPIStrategy::resolveApiKey(const ProviderFetchContext& ctx) {
    if (ctx.env.contains("SYNTHETIC_API_KEY")) return ctx.env["SYNTHETIC_API_KEY"];
    auto cred = ProviderCredentialStore::read("com.codexbar.apikey.synthetic");
    if (cred.has_value()) return QString::fromUtf8(cred.value());
    return {};
}

bool SyntheticAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveApiKey(ctx).isEmpty();
}

bool SyntheticAPIStrategy::shouldFallback(const ProviderFetchResult& result,
                                           const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

static double extractPercent(const QJsonObject& obj) {
    static const QStringList keys = {"percentUsed", "usedPercent", "usagePercent", "used_percent", "percent"};
    for (const auto& key : keys) {
        if (obj.contains(key)) {
            QJsonValue v = obj[key];
            double val = -1;
            if (v.isDouble()) val = v.toDouble();
            else if (v.isString()) val = v.toString().toDouble();
            if (val >= 0) {
                return (val <= 1.0) ? val * 100.0 : val;
            }
        }
    }
    return -1;
}

static double extractLimit(const QJsonObject& obj) {
    static const QStringList keys = {"limit", "max", "total", "capacity", "allowance"};
    for (const auto& key : keys) {
        if (obj.contains(key)) {
            QJsonValue v = obj[key];
            double val = 0;
            if (v.isDouble()) val = v.toDouble();
            else if (v.isString()) val = v.toString().toDouble();
            if (val > 0) return val;
        }
    }
    return -1;
}

static double extractUsed(const QJsonObject& obj) {
    static const QStringList keys = {"used", "usage", "consumed", "spent"};
    for (const auto& key : keys) {
        if (obj.contains(key)) {
            QJsonValue v = obj[key];
            double val = -1;
            if (v.isDouble()) val = v.toDouble();
            else if (v.isString()) val = v.toString().toDouble();
            if (val >= 0) return val;
        }
    }
    return -1;
}

enum class SlotLane { Session, Weekly, Tertiary };

static std::optional<SlotLane> classifySlotKey(const QString& key) {
    QString lower = key.toLower();
    if (lower.contains("rollingfivehour") || lower.contains("fivehour") || lower.contains("5hour"))
        return SlotLane::Session;
    if (lower.contains("weekly") || lower.contains("weeklimit"))
        return SlotLane::Weekly;
    if (lower.contains("search") && lower.contains("hourly"))
        return SlotLane::Tertiary;
    return std::nullopt;
}

ProviderFetchResult SyntheticAPIStrategy::parseResponse(const QJsonObject& json) {
    ProviderFetchResult result;
    result.strategyID = "synthetic.api";
    result.strategyKind = ProviderFetchKind::APIToken;
    result.sourceLabel = "api";

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty or invalid response from Synthetic API";
        return result;
    }

    struct SlotMatch { SlotLane lane; double usedPercent; int windowMinutes; };
    QVector<SlotMatch> matches;

    std::function<void(const QJsonObject&, int)> scan = [&](const QJsonObject& obj, int depth) {
        if (depth > 5) return;
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QString key = it.key();
            auto lane = classifySlotKey(key);
            if (lane.has_value() && it.value().isObject()) {
                // Parent key matches a slot — extract percent from child object
                QJsonObject child = it.value().toObject();
                double pct = extractPercent(child);
                if (pct < 0) {
                    double limit = extractLimit(child);
                    double used = extractUsed(child);
                    if (limit > 0 && used >= 0) pct = (used / limit) * 100.0;
                }
                if (pct >= 0) {
                    int mins = 0;
                    if (*lane == SlotLane::Session) mins = 300;
                    else if (*lane == SlotLane::Weekly) mins = 10080;
                    matches.append({*lane, pct, mins});
                }
            }
            if (it.value().isObject()) {
                scan(it.value().toObject(), depth + 1);
            } else {
                if (lane.has_value()) {
                    double pct = extractPercent(obj);
                    if (pct < 0) {
                        double limit = extractLimit(obj);
                        double used = extractUsed(obj);
                        if (limit > 0 && used >= 0) pct = (used / limit) * 100.0;
                    }
                    if (pct >= 0) {
                        int mins = 0;
                        if (*lane == SlotLane::Session) mins = 300;
                        else if (*lane == SlotLane::Weekly) mins = 10080;
                        matches.append({*lane, pct, mins});
                    }
                }
            }
        }
    };
    scan(json, 0);

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::synthetic;
    snap.identity = identity;

    bool hasSession = false, hasWeekly = false;
    for (const auto& m : matches) {
        RateWindow rw;
        rw.usedPercent = m.usedPercent;
        rw.windowMinutes = m.windowMinutes;
        if (m.lane == SlotLane::Session && !hasSession) {
            snap.primary = rw;
            hasSession = true;
        } else if (m.lane == SlotLane::Weekly && !hasWeekly) {
            snap.secondary = rw;
            hasWeekly = true;
        } else if (m.lane == SlotLane::Tertiary && !snap.tertiary.has_value()) {
            snap.tertiary = rw;
        }
    }

    result.usage = snap;
    result.success = true;
    return result;
}

ProviderFetchResult SyntheticAPIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

    QString apiKey = resolveApiKey(ctx);
    if (apiKey.isEmpty()) {
        result.success = false;
        result.errorMessage = "Synthetic API key not configured.";
        return result;
    }

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://api.synthetic.new/v2/quotas"), headers, ctx.networkTimeoutMs);

    return parseResponse(json);
}
