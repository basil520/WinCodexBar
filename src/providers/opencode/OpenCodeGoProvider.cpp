#include "OpenCodeGoProvider.h"
#include "OpenCodeUtils.h"
#include "../../network/NetworkManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDateTime>
#include <QUrl>
#include <algorithm>

OpenCodeGoProvider::OpenCodeGoProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> OpenCodeGoProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new OpenCodeGoWebStrategy() };
}

QVector<ProviderSettingsDescriptor> OpenCodeGoProvider::settingsDescriptors() const {
    return {
        {"cookieSource", "Cookie source", "picker", "auto",
         {{"auto", "Automatic"}, {"manual", "Manual"}}, {}, {}, {}, "Browser or manual cookie", false, false},
        {"manualCookieHeader", "Manual cookie", "secret", QVariant(),
         {}, {}, "OPENCODE_COOKIE", "auth=...", "Paste auth cookie from opencode.ai", false, true},
        {"workspaceID", "Workspace ID", "text", QVariant(),
         {}, {}, "CODEXBAR_OPENCODE_WORKSPACE_ID", "wrk_...", "Optional: override workspace ID", false, false}
    };
}

OpenCodeGoWebStrategy::OpenCodeGoWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool OpenCodeGoWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !OpenCodeUtils::resolveCookieHeader(ctx).isEmpty();
}

QString OpenCodeGoWebStrategy::fetchUsagePage(const QString& workspaceID, const QString& cookieHeader, int timeoutMs) {
    QString urlStr = "https://opencode.ai/workspace/" + workspaceID + "/go";
    QUrl url(urlStr);
    
    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
    headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
    
    QString text = NetworkManager::instance().getStringSync(url, headers, timeoutMs, false);
    
    if (text.isEmpty() || OpenCodeUtils::looksSignedOut(text)) {
        return {};
    }
    
    // Quick validation: check if usage fields exist
    if (text.contains("rollingUsage") && text.contains("usagePercent")) {
        return text;
    }
    
    // Try to find any usage-related JSON in the page
    QRegularExpression usageRe(R"re((?:rolling|weekly|monthly)Usage[^}]*?usagePercent\s*:\s*(?:[0-9]+(?:\.[0-9]+)?))re");
    if (usageRe.match(text).hasMatch()) {
        return text;
    }
    
    return {};
}

double OpenCodeGoWebStrategy::normalizePercent(double value) {
    if (value <= 1.0 && value >= 0) {
        value *= 100.0;
    }
    return std::clamp(value, 0.0, 100.0);
}

std::optional<RateWindow> OpenCodeGoWebStrategy::parseWindow(const QJsonObject& obj) {
    if (obj.isEmpty()) return std::nullopt;
    
    QStringList percentKeys = {
        "usagePercent", "usedPercent", "percentUsed", "percent",
        "usage_percent", "used_percent", "utilization", "utilizationPercent",
        "utilization_percent", "usage"
    };
    
    QStringList resetInKeys = {
        "resetInSec", "resetInSeconds", "resetSeconds", "reset_sec",
        "reset_in_sec", "resetsInSec", "resetsInSeconds", "resetIn", "resetSec"
    };
    
    QStringList resetAtKeys = {
        "resetAt", "resetsAt", "reset_at", "resets_at",
        "nextReset", "next_reset", "renewAt", "renew_at"
    };
    
    QStringList usedKeys = {"used", "usage", "consumed", "count", "usedTokens"};
    QStringList limitKeys = {"limit", "total", "quota", "max", "cap", "tokenLimit"};
    
    double percent = -1;
    
    // Try direct percent keys
    for (const auto& key : percentKeys) {
        if (obj.contains(key)) {
            QJsonValue v = obj[key];
            if (v.isDouble()) {
                percent = v.toDouble();
                break;
            }
            if (v.isString()) {
                bool ok = false;
                double d = v.toString().toDouble(&ok);
                if (ok) {
                    percent = d;
                    break;
                }
            }
        }
    }
    
    // Try used/limit calculation
    if (percent < 0) {
        double used = -1, limit = -1;
        for (const auto& key : usedKeys) {
            if (obj.contains(key)) {
                QJsonValue v = obj[key];
                if (v.isDouble()) {
                    used = v.toDouble();
                    break;
                }
                if (v.isString()) {
                    bool ok = false;
                    double d = v.toString().toDouble(&ok);
                    if (ok) {
                        used = d;
                        break;
                    }
                }
            }
        }
        for (const auto& key : limitKeys) {
            if (obj.contains(key)) {
                QJsonValue v = obj[key];
                if (v.isDouble()) {
                    limit = v.toDouble();
                    break;
                }
                if (v.isString()) {
                    bool ok = false;
                    double d = v.toString().toDouble(&ok);
                    if (ok) {
                        limit = d;
                        break;
                    }
                }
            }
        }
        if (used >= 0 && limit > 0) {
            percent = (used / limit) * 100.0;
        }
    }
    
    if (percent < 0) return std::nullopt;
    
    percent = normalizePercent(percent);
    
    // Find reset
    int resetInSec = -1;
    for (const auto& key : resetInKeys) {
        if (obj.contains(key)) {
            QJsonValue v = obj[key];
            if (v.isDouble()) {
                resetInSec = static_cast<int>(v.toDouble());
                break;
            }
            if (v.isString()) {
                bool ok = false;
                int i = v.toString().toInt(&ok);
                if (ok) {
                    resetInSec = i;
                    break;
                }
            }
        }
    }
    
    // Try resetAt timestamp
    if (resetInSec < 0) {
        for (const auto& key : resetAtKeys) {
            if (obj.contains(key)) {
                QJsonValue v = obj[key];
                QDateTime resetAt;
                if (v.isString()) {
                    resetAt = QDateTime::fromString(v.toString(), Qt::ISODate);
                    if (!resetAt.isValid()) {
                        resetAt = QDateTime::fromString(v.toString(), Qt::ISODateWithMs);
                    }
                } else if (v.isDouble()) {
                    double ts = v.toDouble();
                    if (ts > 1'000'000'000'000) {
                        resetAt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(ts));
                    } else if (ts > 1'000'000'000) {
                        resetAt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts));
                    }
                }
                if (resetAt.isValid()) {
                    resetInSec = static_cast<int>(QDateTime::currentDateTime().secsTo(resetAt));
                    break;
                }
            }
        }
    }
    
    RateWindow rw;
    rw.usedPercent = percent;
    if (resetInSec > 0) {
        rw.windowMinutes = resetInSec / 60;
        rw.resetsAt = QDateTime::currentDateTime().addSecs(resetInSec);
    }
    return rw;
}

UsageSnapshot OpenCodeGoWebStrategy::parseUsage(const QString& text) {
    UsageSnapshot snapshot;
    snapshot.updatedAt = QDateTime::currentDateTime();
    
    // Try JSON parse first
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    
    if (err.error == QJsonParseError::NoError) {
        QJsonObject root = doc.object();
        
        // Look for nested data
        QJsonObject target = root;
        for (const auto& key : {"data", "result", "usage", "billing", "payload"}) {
            if (target.contains(key) && target[key].isObject()) {
                target = target[key].toObject();
            }
        }
        
        auto findWindow = [](const QJsonObject& obj, const QStringList& keys) -> QJsonObject {
            for (const auto& key : keys) {
                if (obj.contains(key) && obj[key].isObject()) {
                    return obj[key].toObject();
                }
            }
            return {};
        };
        
        QJsonObject rolling = findWindow(target, {"rollingUsage", "rolling", "rolling_usage", "rollingWindow", "rolling_window"});
        QJsonObject weekly = findWindow(target, {"weeklyUsage", "weekly", "weekly_usage", "weeklyWindow", "weekly_window"});
        QJsonObject monthly = findWindow(target, {"monthlyUsage", "monthly", "monthly_usage", "monthlyWindow", "monthly_window"});
        
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
                    
                    if ((key.contains("rolling") || key.contains("hour") || key.contains("5h") || key.contains("5-hour")) && rolling.isEmpty()) {
                        auto rw = parseWindow(sub);
                        if (rw.has_value()) rolling = sub;
                    }
                    if ((key.contains("weekly") || key.contains("week")) && weekly.isEmpty()) {
                        auto rw = parseWindow(sub);
                        if (rw.has_value()) weekly = sub;
                    }
                    if ((key.contains("monthly") || key.contains("month")) && monthly.isEmpty()) {
                        auto rw = parseWindow(sub);
                        if (rw.has_value()) monthly = sub;
                    }
                    search(it.value(), depth + 1);
                }
            };
            search(target, 0);
        }
        
        if (!rolling.isEmpty()) snapshot.primary = parseWindow(rolling);
        if (!weekly.isEmpty()) snapshot.secondary = parseWindow(weekly);
        if (!monthly.isEmpty()) snapshot.tertiary = parseWindow(monthly);
    }
    
    // Fallback: regex parsing
    if (!snapshot.primary.has_value() || !snapshot.secondary.has_value()) {
        QRegularExpression rollingRe(R"re(rollingUsage[^}]*?usagePercent\s*:\s*([0-9]+(?:\.[0-9]+)?))re");
        QRegularExpression weeklyRe(R"re(weeklyUsage[^}]*?usagePercent\s*:\s*([0-9]+(?:\.[0-9]+)?))re");
        QRegularExpression monthlyRe(R"re(monthlyUsage[^}]*?usagePercent\s*:\s*([0-9]+(?:\.[0-9]+)?))re");
        QRegularExpression rollingResetRe(R"re(rollingUsage[^}]*?resetInSec\s*:\s*([0-9]+))re");
        QRegularExpression weeklyResetRe(R"re(weeklyUsage[^}]*?resetInSec\s*:\s*([0-9]+))re");
        QRegularExpression monthlyResetRe(R"re(monthlyUsage[^}]*?resetInSec\s*:\s*([0-9]+))re");
        
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
            
            // Optional monthly
            QRegularExpressionMatch monthlyM = monthlyRe.match(text);
            if (monthlyM.hasMatch()) {
                bool ok3 = false;
                double monthlyPct = monthlyM.captured(1).toDouble(&ok3);
                if (ok3) {
                    RateWindow rw;
                    rw.usedPercent = monthlyPct;
                    QRegularExpressionMatch resetM = monthlyResetRe.match(text);
                    if (resetM.hasMatch()) {
                        int sec = resetM.captured(1).toInt();
                        rw.windowMinutes = sec / 60;
                        rw.resetsAt = QDateTime::currentDateTime().addSecs(sec);
                    }
                    snapshot.tertiary = rw;
                }
            }
        }
    }
    
    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::opencodego;
    snapshot.identity = identity;
    
    return snapshot;
}

ProviderFetchResult OpenCodeGoWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";
    
    QString cookieHeader = OpenCodeUtils::resolveCookieHeader(ctx);
    if (cookieHeader.isEmpty()) {
        result.success = false;
        result.errorMessage = "No OpenCode Go auth cookie found in browser cookies or manual input.";
        return result;
    }
    
    // Check for workspace ID override
    QString workspaceID;
    if (ctx.settings.values.contains("workspaceID")) {
        workspaceID = OpenCodeUtils::normalizeWorkspaceID(ctx.settings.values["workspaceID"].toString());
    }
    if (workspaceID.isEmpty() && ctx.env.contains("CODEXBAR_OPENCODE_WORKSPACE_ID")) {
        workspaceID = OpenCodeUtils::normalizeWorkspaceID(ctx.env["CODEXBAR_OPENCODE_WORKSPACE_ID"]);
    }
    
    if (workspaceID.isEmpty()) {
        workspaceID = OpenCodeUtils::fetchWorkspaceID(cookieHeader, ctx.networkTimeoutMs);
    }
    
    if (workspaceID.isEmpty()) {
        result.success = false;
        result.errorMessage = "Failed to fetch OpenCode workspace ID.";
        return result;
    }
    
    QString pageText = fetchUsagePage(workspaceID, cookieHeader, ctx.networkTimeoutMs);
    if (pageText.isEmpty()) {
        result.success = false;
        result.errorMessage = "OpenCode Go workspace page returned no usage data.";
        return result;
    }
    
    result.usage = parseUsage(pageText);
    result.success = result.usage.primary.has_value() || result.usage.secondary.has_value();
    if (!result.success) {
        result.errorMessage = "No usage data found in OpenCode Go response.";
    }
    return result;
}
