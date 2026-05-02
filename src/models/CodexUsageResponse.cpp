#include "CodexUsageResponse.h"
#include "UsageSnapshot.h"

#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <cmath>

std::optional<int> CodexWindowSnapshot::windowMinutes() const {
    if (limitWindowSeconds <= 0) return std::nullopt;
    return limitWindowSeconds / 60;
}

std::optional<QDateTime> CodexWindowSnapshot::resetsAtTime() const {
    if (resetAt <= 0) return std::nullopt;
    return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(resetAt));
}

std::optional<QString> CodexWindowSnapshot::resetDescription() const {
    auto dt = resetsAtTime();
    if (!dt.has_value()) return std::nullopt;
    qint64 secs = dt->toSecsSinceEpoch() - QDateTime::currentDateTime().toSecsSinceEpoch();
    if (secs <= 0) return QString("now");
    int mins = static_cast<int>(secs / 60);
    if (mins < 60) return QString("%1m").arg(mins);
    int hrs = mins / 60;
    int remMin = mins % 60;
    if (hrs < 24) return QString("%1h %2m").arg(hrs).arg(remMin);
    int days = hrs / 24;
    int remHrs = hrs % 24;
    return QString("%1d %2h").arg(days).arg(remHrs);
}

CodexPlanType codexPlanTypeFromString(const QString& raw) {
    QString lower = raw.trimmed().toLower();
    if (lower == "guest") return CodexPlanType::Guest;
    if (lower == "free") return CodexPlanType::Free;
    if (lower == "go") return CodexPlanType::Go;
    if (lower == "plus") return CodexPlanType::Plus;
    if (lower == "pro") return CodexPlanType::Pro;
    if (lower == "free_workspace") return CodexPlanType::FreeWorkspace;
    if (lower == "team") return CodexPlanType::Team;
    if (lower == "business") return CodexPlanType::Business;
    if (lower == "education") return CodexPlanType::Education;
    if (lower == "quorum") return CodexPlanType::Quorum;
    if (lower == "k12") return CodexPlanType::K12;
    if (lower == "enterprise") return CodexPlanType::Enterprise;
    if (lower == "edu") return CodexPlanType::Edu;
    return CodexPlanType::Unknown;
}

QString codexPlanTypeDisplayName(const QString& raw) {
    QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) return {};
    QStringList parts = trimmed.split(QRegularExpression("[_\\-\\s]"), Qt::SkipEmptyParts);
    QStringList result;
    for (const auto& part : parts) {
        if (part.isEmpty()) continue;
        QString capped = part[0].toUpper() + part.mid(1).toLower();
        result.append(capped);
    }
    return result.join(" ");
}

enum class CodexWindowRole { Session, Weekly, Unknown };

static CodexWindowRole roleForWindow(const CodexWindowSnapshot& window) {
    int mins = window.limitWindowSeconds / 60;
    if (mins == 300) return CodexWindowRole::Session;
    if (mins == 10080) return CodexWindowRole::Weekly;
    return CodexWindowRole::Unknown;
}

void normalizeCodexWindows(std::optional<CodexWindowSnapshot>& primary,
                           std::optional<CodexWindowSnapshot>& secondary) {
    if (primary.has_value() && secondary.has_value()) {
        auto pRole = roleForWindow(*primary);
        auto sRole = roleForWindow(*secondary);
        if (pRole == CodexWindowRole::Weekly &&
            (sRole == CodexWindowRole::Session || sRole == CodexWindowRole::Unknown)) {
            std::swap(primary, secondary);
        }
    } else if (primary.has_value() && !secondary.has_value()) {
        if (roleForWindow(*primary) == CodexWindowRole::Weekly) {
            secondary = primary;
            primary = std::nullopt;
        }
    } else if (!primary.has_value() && secondary.has_value()) {
        auto sRole = roleForWindow(*secondary);
        if (sRole == CodexWindowRole::Session || sRole == CodexWindowRole::Unknown) {
            primary = secondary;
            secondary = std::nullopt;
        }
    }
}

CodexUsageResponse CodexUsageResponse::fromJson(const QJsonObject& json) {
    CodexUsageResponse resp;

    QString planStr = json.value("plan_type").toString();
    if (!planStr.isEmpty()) {
        resp.planType = codexPlanTypeFromString(planStr);
    }

    QJsonObject rateLimit = json.value("rate_limit").toObject();
    if (!rateLimit.isEmpty()) {
        QJsonObject pw = rateLimit.value("primary_window").toObject();
        if (!pw.isEmpty()) {
            CodexWindowSnapshot ws;
            ws.usedPercent = pw.value("used_percent").toInt(0);
            ws.resetAt = pw.value("reset_at").toInt(0);
            ws.limitWindowSeconds = pw.value("limit_window_seconds").toInt(0);
            resp.primaryWindow = ws;
        }
        QJsonObject sw = rateLimit.value("secondary_window").toObject();
        if (!sw.isEmpty()) {
            CodexWindowSnapshot ws;
            ws.usedPercent = sw.value("used_percent").toInt(0);
            ws.resetAt = sw.value("reset_at").toInt(0);
            ws.limitWindowSeconds = sw.value("limit_window_seconds").toInt(0);
            resp.secondaryWindow = ws;
        }
    }

    QJsonObject credits = json.value("credits").toObject();
    if (!credits.isEmpty()) {
        CodexCreditDetails cd;
        cd.hasCredits = credits.value("has_credits").toBool(false);
        cd.unlimited = credits.value("unlimited").toBool(false);
        if (credits.contains("balance")) {
            auto balVal = credits.value("balance");
            if (balVal.isDouble()) {
                cd.balance = balVal.toDouble();
            } else {
                bool ok = false;
                double d = balVal.toString().toDouble(&ok);
                if (ok) cd.balance = d;
            }
        }
        resp.credits = cd;
    }

    return resp;
}

UsageSnapshot CodexUsageResponse::toUsageSnapshot(const QString& accountEmail,
                                                   const QString& planOverride) const {
    UsageSnapshot snap;

    auto primary = primaryWindow;
    auto secondary = secondaryWindow;
    normalizeCodexWindows(primary, secondary);

    if (primary.has_value()) {
        RateWindow rw;
        rw.usedPercent = primary->usedPercent;
        rw.windowMinutes = primary->windowMinutes();
        rw.resetsAt = primary->resetsAtTime();
        rw.resetDescription = primary->resetDescription();
        snap.primary = rw;
    }

    if (secondary.has_value()) {
        RateWindow rw;
        rw.usedPercent = secondary->usedPercent;
        rw.windowMinutes = secondary->windowMinutes();
        rw.resetsAt = secondary->resetsAtTime();
        rw.resetDescription = secondary->resetDescription();
        snap.secondary = rw;
    }

    static const char* planNames[] = {
        "guest", "free", "go", "plus", "pro", "free_workspace",
        "team", "business", "education", "quorum", "k12", "enterprise", "edu", "unknown"
    };

    QString planName = planOverride;
    if (planName.isEmpty() && planType.has_value()) {
        int idx = static_cast<int>(*planType);
        if (idx >= 0 && idx < 14) planName = planNames[idx];
    }

    if (!accountEmail.isEmpty() || !planName.isEmpty() || planType.has_value()) {
        ProviderIdentitySnapshot identity;
        identity.providerID = UsageProvider::codex;
        if (!accountEmail.isEmpty()) identity.accountEmail = accountEmail;
        if (!planName.isEmpty()) identity.loginMethod = planName;
        snap.identity = identity;
    }

    if (credits.has_value() && credits->hasCredits && !credits->unlimited) {
        ProviderCostSnapshot cost;
        cost.used = credits->balance.value_or(0.0);
        cost.limit = 0.0;
        cost.currencyCode = "USD";
        cost.period = "Credits";
        cost.updatedAt = QDateTime::currentDateTime();
        snap.providerCost = cost;
    }

    snap.updatedAt = QDateTime::currentDateTime();
    return snap;
}

bool CodexOAuthCredentials::needsRefresh() const {
    if (!lastRefresh.has_value()) return true;
    qint64 eightDaysSecs = 8 * 24 * 60 * 60;
    return lastRefresh->secsTo(QDateTime::currentDateTime()) > eightDaysSecs;
}

std::optional<CodexOAuthCredentials> CodexOAuthCredentials::load(const QHash<QString, QString>& env) {
    QString home = env.value("USERPROFILE", env.value("HOME", QDir::homePath()));
    QString codexHome = env.value("CODEX_HOME").trimmed();
    QString root = codexHome.isEmpty() ? home + "/.codex" : codexHome;
    QString authPath = root + "/auth.json";

    QFile file(authPath);
    if (!file.exists()) return std::nullopt;
    if (!file.open(QIODevice::ReadOnly)) return std::nullopt;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return std::nullopt;

    return parse(doc.object());
}

std::optional<CodexOAuthCredentials> CodexOAuthCredentials::parse(const QJsonObject& json) {
    QString apiKey = json.value("OPENAI_API_KEY").toString().trimmed();
    if (!apiKey.isEmpty()) {
        return CodexOAuthCredentials{apiKey, {}, {}, {}, {}};
    }

    QJsonObject tokens = json.value("tokens").toObject();
    if (tokens.isEmpty()) return std::nullopt;

    QString accessToken = tokens.value("access_token").toString().trimmed();
    if (accessToken.isEmpty()) accessToken = tokens.value("accessToken").toString().trimmed();
    if (accessToken.isEmpty()) return std::nullopt;

    QString refreshToken = tokens.value("refresh_token").toString().trimmed();
    if (refreshToken.isEmpty()) refreshToken = tokens.value("refreshToken").toString().trimmed();

    QString idToken = tokens.value("id_token").toString().trimmed();
    if (idToken.isEmpty()) idToken = tokens.value("idToken").toString().trimmed();

    QString accountId = tokens.value("account_id").toString().trimmed();
    if (accountId.isEmpty()) accountId = tokens.value("accountId").toString().trimmed();

    std::optional<QDateTime> lastRefresh;
    QString lrStr = json.value("last_refresh").toString().trimmed();
    if (!lrStr.isEmpty()) {
        lastRefresh = QDateTime::fromString(lrStr, Qt::ISODateWithMs);
        if (!lastRefresh.has_value() || !lastRefresh->isValid()) {
            lastRefresh = QDateTime::fromString(lrStr, Qt::ISODate);
        }
    }

    return CodexOAuthCredentials{accessToken, refreshToken, idToken, accountId, lastRefresh};
}

QString CodexOAuthCredentials::resolveAuthFilePath(const QHash<QString, QString>& env) {
    QString home = env.value("USERPROFILE", env.value("HOME", QDir::homePath()));
    QString codexHome = env.value("CODEX_HOME").trimmed();
    QString root = codexHome.isEmpty() ? home + "/.codex" : codexHome;
    return root + "/auth.json";
}

bool CodexOAuthCredentials::save(const QHash<QString, QString>& env) const {
    QString authPath = resolveAuthFilePath(env);

    QJsonObject existingJson;
    QFile readFile(authPath);
    if (readFile.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(readFile.readAll(), &err);
        if (err.error == QJsonParseError::NoError) existingJson = doc.object();
        readFile.close();
    }

    QJsonObject tokens;
    tokens["access_token"] = accessToken;
    tokens["refresh_token"] = refreshToken;
    if (!idToken.isEmpty()) tokens["id_token"] = idToken;
    if (!accountId.isEmpty()) tokens["account_id"] = accountId;
    existingJson["tokens"] = tokens;
    existingJson["last_refresh"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    QDir().mkpath(QFileInfo(authPath).absolutePath());
    QFile writeFile(authPath);
    if (!writeFile.open(QIODevice::WriteOnly)) return false;
    writeFile.write(QJsonDocument(existingJson).toJson(QJsonDocument::Compact));
    writeFile.close();
    QFile::setPermissions(authPath, QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

QJsonObject parseJWTPayload(const QString& jwt) {
    QStringList parts = jwt.split('.');
    if (parts.size() != 3) return {};
    QByteArray payload = QByteArray::fromBase64(
        parts[1].toUtf8(),
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError) return {};
    return doc.object();
}
