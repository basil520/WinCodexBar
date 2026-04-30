#include "OpenCodeProvider.h"
#include "OpenCodeUtils.h"
#include "../../network/NetworkManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDateTime>
#include <QUrlQuery>

OpenCodeProvider::OpenCodeProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> OpenCodeProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new OpenCodeWebStrategy() };
}

QVector<ProviderSettingsDescriptor> OpenCodeProvider::settingsDescriptors() const {
    return {
        {"cookieSource", "Cookie source", "picker", "auto",
         {{"auto", "Automatic"}, {"manual", "Manual"}}, {}, {}, {}, "Browser or manual cookie", false, false},
        {"manualCookieHeader", "Manual cookie", "secret", QVariant(),
         {}, {}, "OPENCODE_COOKIE", "auth=...", "Paste auth cookie from opencode.ai", false, true}
    };
}

OpenCodeWebStrategy::OpenCodeWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool OpenCodeWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !OpenCodeUtils::resolveCookieHeader(ctx).isEmpty();
}

QString OpenCodeWebStrategy::fetchSubscription(const QString& workspaceID, const QString& cookieHeader, int timeoutMs) {
    static const QString subscriptionServerID = "7abeebee372f304e050aaaf92be863f4a86490e382f8c79db68fd94040d691b4";

    QString referer = "https://opencode.ai/workspace/" + workspaceID + "/billing";

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["X-Server-Id"] = subscriptionServerID;
    headers["X-Server-Instance"] = "server-fn:" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
    headers["Origin"] = "https://opencode.ai";
    headers["Referer"] = referer;
    headers["Accept"] = "text/javascript, application/json;q=0.9, */*;q=0.8";

    QString url = OpenCodeUtils::serverRequestURL(subscriptionServerID, {workspaceID}, "GET");
    QString text = NetworkManager::instance().getStringSync(QUrl(url), headers, timeoutMs, false);

    if (text.isEmpty() || OpenCodeUtils::looksSignedOut(text)) return {};

    QString trimmed = text.trimmed();
    if (trimmed.compare("null", Qt::CaseInsensitive) == 0) return {};

    // Check if parseable
    UsageSnapshot snap = parseSubscription(text);
    if (snap.primary.has_value() || snap.secondary.has_value()) return text;

    // Fallback: POST
    QJsonObject body;
    QJsonArray args;
    args.append(workspaceID);
    body["args"] = args;

    headers["Content-Type"] = "application/json";
    url = OpenCodeUtils::serverRequestURL(subscriptionServerID, {workspaceID}, "POST");
    QJsonObject json = NetworkManager::instance().postJsonSync(QUrl(url), body, headers, timeoutMs, false);
    if (!json.isEmpty()) {
        QString jsonText = QString::fromUtf8(QJsonDocument(json).toJson());
        UsageSnapshot postSnap = parseSubscription(jsonText);
        if (postSnap.primary.has_value() || postSnap.secondary.has_value()) {
            return jsonText;
        }
    }

    return {};
}

UsageSnapshot OpenCodeWebStrategy::parseSubscription(const QString& text) {
    UsageSnapshot snapshot;
    snapshot.updatedAt = QDateTime::currentDateTime();

    // Try JSON first
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (err.error == QJsonParseError::NoError) {
        QJsonObject root = doc.object();

        // Look for nested data under common keys
        QJsonObject target = root;
        for (const auto& key : {"data", "result", "usage", "billing", "payload"}) {
            if (target.contains(key) && target[key].isObject()) {
                target = target[key].toObject();
            }
        }

        auto findWindow = [](const QJsonObject& obj, const QStringList& keys) -> QJsonObject {
            for (const auto& key : keys) {
                if (obj.contains(key) && obj[key].isObject()) return obj[key].toObject();
            }
            return {};
        };

        QJsonObject rolling = findWindow(target, {"rollingUsage", "rolling", "rolling_usage", "rollingWindow"});
        QJsonObject weekly = findWindow(target, {"weeklyUsage", "weekly", "weekly_usage", "weeklyWindow"});

        // If not found at top level, search recursively
        if (rolling.isEmpty() || weekly.isEmpty()) {
            std::function<void(const QJsonValue&, int)> search = [&](const QJsonValue& v, int depth) {
                if (depth > 3) return;
                if (!v.isObject()) return;
                QJsonObject o = v.toObject();
                for (auto it = o.begin(); it != o.end(); ++it) {
                    QString key = it.key().toLower();
                    if (!it.value().isObject()) continue;
                    QJsonObject sub = it.value().toObject();
                    if (key.contains("rolling") && rolling.isEmpty() && sub.contains("usagePercent")) {
                        rolling = sub;
                    }
                    if ((key.contains("weekly") || key.contains("week")) && weekly.isEmpty() && sub.contains("usagePercent")) {
                        weekly = sub;
                    }
                    search(it.value(), depth + 1);
                }
            };
            search(target, 0);
        }

        auto parseWindow = [](const QJsonObject& obj) -> std::optional<RateWindow> {
            if (obj.isEmpty()) return std::nullopt;

            auto getDouble = [](const QJsonObject& o, const QStringList& keys) -> std::optional<double> {
                for (const auto& k : keys) {
                    if (o.contains(k)) {
                        QJsonValue v = o[k];
                        if (v.isDouble()) return v.toDouble();
                        if (v.isString()) {
                            bool ok = false;
                            double d = v.toString().toDouble(&ok);
                            if (ok) return d;
                        }
                    }
                }
                return std::nullopt;
            };

            auto getInt = [](const QJsonObject& o, const QStringList& keys) -> std::optional<int> {
                for (const auto& k : keys) {
                    if (o.contains(k)) {
                        QJsonValue v = o[k];
                        if (v.isDouble()) return static_cast<int>(v.toDouble());
                        if (v.isString()) {
                            bool ok = false;
                            int i = v.toString().toInt(&ok);
                            if (ok) return i;
                        }
                    }
                }
                return std::nullopt;
            };

            std::optional<double> percent = getDouble(obj, {"usagePercent", "usedPercent", "percentUsed", "percent", "usage_percent", "used_percent", "utilization", "utilizationPercent", "utilization_percent", "usage"});
            if (!percent.has_value()) {
                auto used = getDouble(obj, {"used", "usage", "consumed", "count", "usedTokens"});
                auto limit = getDouble(obj, {"limit", "total", "quota", "max", "cap", "tokenLimit"});
                if (used.has_value() && limit.has_value() && *limit > 0) {
                    percent = (*used / *limit) * 100.0;
                }
            }

            if (!percent.has_value()) return std::nullopt;

            double p = *percent;
            if (p <= 1.0 && p >= 0) p *= 100.0;
            p = std::clamp(p, 0.0, 100.0);

            std::optional<int> resetInSec = getInt(obj, {"resetInSec", "resetInSeconds", "resetSeconds", "reset_sec", "reset_in_sec", "resetsInSec", "resetsInSeconds", "resetIn", "resetSec"});

            RateWindow rw;
            rw.usedPercent = p;
            if (resetInSec.has_value() && *resetInSec > 0) {
                rw.windowMinutes = *resetInSec / 60;
                rw.resetsAt = QDateTime::currentDateTime().addSecs(*resetInSec);
            }
            return rw;
        };

        if (!rolling.isEmpty()) snapshot.primary = parseWindow(rolling);
        if (!weekly.isEmpty()) snapshot.secondary = parseWindow(weekly);
    }

    // Fallback: regex from text
    if (!snapshot.primary.has_value() || !snapshot.secondary.has_value()) {
        QRegularExpression rollingRe(R"re(rollingUsage[^}]*?usagePercent\s*:\s*([0-9]+(?:\.[0-9]+)?))re");
        QRegularExpression weeklyRe(R"re(weeklyUsage[^}]*?usagePercent\s*:\s*([0-9]+(?:\.[0-9]+)?))re");
        QRegularExpression rollingResetRe(R"re(rollingUsage[^}]*?resetInSec\s*:\s*([0-9]+))re");
        QRegularExpression weeklyResetRe(R"re(weeklyUsage[^}]*?resetInSec\s*:\s*([0-9]+))re");

        QRegularExpressionMatch rollingM = rollingRe.match(text);
        QRegularExpressionMatch weeklyM = weeklyRe.match(text);

        if (rollingM.hasMatch() && weeklyM.hasMatch()) {
            bool ok1 = false, ok2 = false;
            double rollingPct = rollingM.captured(1).toDouble(&ok1);
            double weeklyPct = weeklyM.captured(1).toDouble(&ok2);

            if (ok1) {
                RateWindow rw;
                rw.usedPercent = rollingPct;
                QRegularExpressionMatch resetM = rollingResetRe.match(text);
                if (resetM.hasMatch()) {
                    int sec = resetM.captured(1).toInt();
                    rw.windowMinutes = sec / 60;
                    rw.resetsAt = QDateTime::currentDateTime().addSecs(sec);
                }
                snapshot.primary = rw;
            }

            if (ok2) {
                RateWindow rw;
                rw.usedPercent = weeklyPct;
                QRegularExpressionMatch resetM = weeklyResetRe.match(text);
                if (resetM.hasMatch()) {
                    int sec = resetM.captured(1).toInt();
                    rw.windowMinutes = sec / 60;
                    rw.resetsAt = QDateTime::currentDateTime().addSecs(sec);
                }
                snapshot.secondary = rw;
            }
        }
    }

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::opencode;
    snapshot.identity = identity;

    return snapshot;
}

ProviderFetchResult OpenCodeWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QString cookieHeader = OpenCodeUtils::resolveCookieHeader(ctx);
    if (cookieHeader.isEmpty()) {
        result.success = false;
        result.errorMessage = "No OpenCode auth cookie found in browser cookies or manual input.";
        return result;
    }

    QString workspaceID = OpenCodeUtils::fetchWorkspaceID(cookieHeader, ctx.networkTimeoutMs);
    if (workspaceID.isEmpty()) {
        result.success = false;
        result.errorMessage = "Failed to fetch OpenCode workspace ID.";
        return result;
    }

    QString subscriptionText = fetchSubscription(workspaceID, cookieHeader, ctx.networkTimeoutMs);
    if (subscriptionText.isEmpty()) {
        result.success = false;
        result.errorMessage = "OpenCode workspace has no subscription usage data. This usually means the workspace does not have a paid plan or quota is not enabled.";
        return result;
    }

    result.usage = parseSubscription(subscriptionText);
    result.success = result.usage.primary.has_value() || result.usage.secondary.has_value();
    if (!result.success) {
        result.errorMessage = "No usage data in OpenCode response.";
    }
    return result;
}
