#include "ClaudeUsageSnapshot.h"
#include "UsageSnapshot.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <cmath>

std::optional<QDateTime> ClaudeOAuthWindow::resetsAtTime() const {
    if (!resetsAt.has_value() || resetsAt->isEmpty()) return std::nullopt;
    QDateTime dt = QDateTime::fromString(*resetsAt, Qt::ISODateWithMs);
    if (dt.isValid()) return dt;
    dt = QDateTime::fromString(*resetsAt, Qt::ISODate);
    if (dt.isValid()) return dt;
    return std::nullopt;
}

bool ClaudeUsageSnapshot::isValid() const {
    return fiveHour.utilization.has_value() || sevenDay.utilization.has_value();
}

UsageSnapshot ClaudeUsageSnapshot::toUsageSnapshot() const {
    UsageSnapshot snap;

    if (fiveHour.utilization.has_value()) {
        RateWindow rw;
        rw.usedPercent = *fiveHour.utilization;
        rw.windowMinutes = 300;
        rw.resetsAt = fiveHour.resetsAtTime();
        snap.primary = rw;
    }

    if (sevenDay.utilization.has_value()) {
        RateWindow rw;
        rw.usedPercent = *sevenDay.utilization;
        rw.windowMinutes = 10080;
        rw.resetsAt = sevenDay.resetsAtTime();
        snap.secondary = rw;
    }

    std::optional<double> opusPercent;
    if (sevenDaySonnet.has_value() && sevenDaySonnet->utilization.has_value()) {
        opusPercent = *sevenDaySonnet->utilization;
    } else if (sevenDayOpus.has_value() && sevenDayOpus->utilization.has_value()) {
        opusPercent = *sevenDayOpus->utilization;
    }
    if (opusPercent.has_value()) {
        RateWindow rw;
        rw.usedPercent = *opusPercent;
        rw.windowMinutes = 10080;
        if (sevenDaySonnet.has_value()) rw.resetsAt = sevenDaySonnet->resetsAtTime();
        else if (sevenDayOpus.has_value()) rw.resetsAt = sevenDayOpus->resetsAtTime();
        snap.tertiary = rw;
    }

    if (sevenDayDesign.has_value() && sevenDayDesign->utilization.has_value()) {
        NamedRateWindow nrw;
        nrw.id = "claude-design";
        nrw.title = "Designs";
        nrw.window.usedPercent = *sevenDayDesign->utilization;
        nrw.window.windowMinutes = 10080;
        nrw.window.resetsAt = sevenDayDesign->resetsAtTime();
        snap.extraRateWindows.append(nrw);
    }

    if (sevenDayRoutines.has_value() && sevenDayRoutines->utilization.has_value()) {
        NamedRateWindow nrw;
        nrw.id = "claude-routines";
        nrw.title = "Routines";
        nrw.window.usedPercent = *sevenDayRoutines->utilization;
        nrw.window.windowMinutes = 10080;
        nrw.window.resetsAt = sevenDayRoutines->resetsAtTime();
        snap.extraRateWindows.append(nrw);
    }

    if (sevenDayOAuthApps.has_value() && sevenDayOAuthApps->utilization.has_value()) {
        NamedRateWindow nrw;
        nrw.id = "claude-oauth-apps";
        nrw.title = "OAuth Apps";
        nrw.window.usedPercent = *sevenDayOAuthApps->utilization;
        nrw.window.windowMinutes = 10080;
        nrw.window.resetsAt = sevenDayOAuthApps->resetsAtTime();
        snap.extraRateWindows.append(nrw);
    }

    if (iguanaNecktie.has_value() && iguanaNecktie->utilization.has_value()) {
        NamedRateWindow nrw;
        nrw.id = "claude-iguana-necktie";
        nrw.title = "Iguana Necktie";
        nrw.window.usedPercent = *iguanaNecktie->utilization;
        nrw.window.resetsAt = iguanaNecktie->resetsAtTime();
        snap.extraRateWindows.append(nrw);
    }

    if (extraUsage.has_value() && extraUsage->isEnabled) {
        ProviderCostSnapshot cost;
        cost.used = extraUsage->usedCredits.value_or(0.0) / 100.0;
        cost.limit = extraUsage->monthlyLimit.value_or(0.0) / 100.0;
        cost.currencyCode = extraUsage->currency.value_or("USD");
        cost.period = "Monthly";
        cost.updatedAt = QDateTime::currentDateTime();
        snap.providerCost = cost;
    }

    if (accountEmail.has_value() || loginMethod.has_value() || accountOrganization.has_value()) {
        ProviderIdentitySnapshot identity;
        identity.providerID = UsageProvider::claude;
        identity.accountEmail = accountEmail;
        identity.loginMethod = loginMethod;
        identity.accountOrganization = accountOrganization;
        snap.identity = identity;
    }

    snap.updatedAt = updatedAt.isValid() ? updatedAt : QDateTime::currentDateTime();
    return snap;
}

static std::optional<ClaudeOAuthWindow> decodeWindow(const QJsonObject& json, const QString& key) {
    if (!json.contains(key)) return std::nullopt;
    QJsonObject win = json.value(key).toObject();
    if (win.isEmpty()) return std::nullopt;
    ClaudeOAuthWindow w;
    if (win.contains("utilization")) {
        auto val = win.value("utilization");
        if (val.isDouble()) w.utilization = val.toDouble();
        else {
            bool ok = false;
            double d = val.toString().toDouble(&ok);
            if (ok) w.utilization = d;
        }
    }
    if (win.contains("resets_at")) {
        w.resetsAt = win.value("resets_at").toString();
    }
    return w;
}

static std::optional<ClaudeOAuthWindow> decodeWindowMulti(
    const QJsonObject& json, const QVector<QString>& keys) {
    for (const auto& key : keys) {
        auto w = decodeWindow(json, key);
        if (w.has_value()) return w;
    }
    return std::nullopt;
}

ClaudeUsageSnapshot ClaudeUsageSnapshot::fromOAuthJson(const QJsonObject& json) {
    ClaudeUsageSnapshot snap;
    snap.fiveHour = decodeWindow(json, "five_hour").value_or(ClaudeOAuthWindow{});
    snap.sevenDay = decodeWindow(json, "seven_day").value_or(ClaudeOAuthWindow{});
    snap.sevenDayOpus = decodeWindow(json, "seven_day_opus");
    snap.sevenDaySonnet = decodeWindow(json, "seven_day_sonnet");

    snap.sevenDayDesign = decodeWindowMulti(json, {
        "seven_day_design", "seven_day_claude_design", "claude_design",
        "design", "seven_day_omelette", "omelette", "omelette_promotional"
    });
    snap.sevenDayRoutines = decodeWindowMulti(json, {
        "seven_day_routines", "seven_day_claude_routines", "claude_routines",
        "routines", "routine", "seven_day_cowork", "cowork"
    });
    snap.sevenDayOAuthApps = decodeWindow(json, "seven_day_oauth_apps");
    snap.iguanaNecktie = decodeWindow(json, "iguana_necktie");

    QJsonObject extra = json.value("extra_usage").toObject();
    if (!extra.isEmpty()) {
        ClaudeExtraUsage eu;
        eu.isEnabled = extra.value("is_enabled").toBool(false);
        if (extra.contains("monthly_limit")) eu.monthlyLimit = extra.value("monthly_limit").toDouble();
        if (extra.contains("used_credits")) eu.usedCredits = extra.value("used_credits").toDouble();
        if (extra.contains("utilization")) eu.utilization = extra.value("utilization").toDouble();
        if (extra.contains("currency")) eu.currency = extra.value("currency").toString();
        snap.extraUsage = eu;
    }

    snap.updatedAt = QDateTime::currentDateTime();
    return snap;
}

ClaudeUsageSnapshot ClaudeUsageSnapshot::fromWebJson(const QJsonObject& json) {
    ClaudeUsageSnapshot snap;
    snap.fiveHour = decodeWindow(json, "five_hour").value_or(ClaudeOAuthWindow{});
    snap.sevenDay = decodeWindow(json, "seven_day").value_or(ClaudeOAuthWindow{});

    auto opusWin = decodeWindow(json, "seven_day_sonnet");
    if (!opusWin.has_value()) opusWin = decodeWindow(json, "seven_day_opus");
    snap.sevenDaySonnet = opusWin;

    snap.sevenDayDesign = decodeWindowMulti(json, {
        "seven_day_design", "seven_day_claude_design", "claude_design",
        "design", "seven_day_omelette", "omelette"
    });
    snap.sevenDayRoutines = decodeWindowMulti(json, {
        "seven_day_routines", "seven_day_claude_routines", "claude_routines",
        "routines", "routine", "seven_day_cowork", "cowork"
    });

    snap.updatedAt = QDateTime::currentDateTime();
    return snap;
}

bool ClaudeOAuthCredentials::isExpired() const {
    if (!expiresAt.has_value()) return true;
    return QDateTime::currentDateTime() >= *expiresAt;
}

std::optional<ClaudeOAuthCredentials> ClaudeOAuthCredentials::load(const QHash<QString, QString>& env) {
    if (env.contains("CODEXBAR_CLAUDE_OAUTH_TOKEN")) {
        QString token = env.value("CODEXBAR_CLAUDE_OAUTH_TOKEN").trimmed();
        if (!token.isEmpty()) {
            return ClaudeOAuthCredentials{token, {}, {}, {}, {}};
        }
    }

    QString home = env.value("USERPROFILE", env.value("HOME", QDir::homePath()));
    QString credPath = home + "/.claude/.credentials.json";

    QFile file(credPath);
    if (!file.exists()) return std::nullopt;
    if (!file.open(QIODevice::ReadOnly)) return std::nullopt;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return std::nullopt;

    return parse(doc.object());
}

std::optional<ClaudeOAuthCredentials> ClaudeOAuthCredentials::parse(const QJsonObject& json) {
    QJsonObject oauth = json.value("claudeAiOauth").toObject();
    if (oauth.isEmpty()) return std::nullopt;

    QString accessToken = oauth.value("accessToken").toString().trimmed();
    if (accessToken.isEmpty()) return std::nullopt;

    ClaudeOAuthCredentials creds;
    creds.accessToken = accessToken;
    creds.refreshToken = oauth.value("refreshToken").toString().trimmed();

    if (oauth.contains("expiresAt")) {
        double millis = oauth.value("expiresAt").toDouble(0);
        if (millis > 0) {
            creds.expiresAt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(millis));
        }
    }

    QJsonArray scopesArr = oauth.value("scopes").toArray();
    for (const auto& s : scopesArr) {
        creds.scopes.append(s.toString());
    }

    if (oauth.contains("rateLimitTier")) {
        creds.rateLimitTier = oauth.value("rateLimitTier").toString().trimmed();
    }

    return creds;
}

QString claudePlanDisplayName(ClaudePlan plan) {
    switch (plan) {
    case ClaudePlan::Max: return "Max";
    case ClaudePlan::Pro: return "Pro";
    case ClaudePlan::Team: return "Team";
    case ClaudePlan::Enterprise: return "Enterprise";
    case ClaudePlan::Ultra: return "Ultra";
    default: return "Unknown";
    }
}

ClaudePlan claudePlanFromRateLimitTier(const QString& tier) {
    QString lower = tier.trimmed().toLower();
    if (lower == "max" || lower == "rate_limit_tier_max") return ClaudePlan::Max;
    if (lower == "pro" || lower == "rate_limit_tier_pro") return ClaudePlan::Pro;
    if (lower == "team" || lower == "rate_limit_tier_team") return ClaudePlan::Team;
    if (lower == "enterprise" || lower == "rate_limit_tier_enterprise") return ClaudePlan::Enterprise;
    if (lower == "ultra" || lower == "rate_limit_tier_ultra") return ClaudePlan::Ultra;
    return ClaudePlan::Unknown;
}
