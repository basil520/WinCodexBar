#include "OpenCodeUtils.h"
#include "../../providers/shared/CookieImporter.h"
#include "../../network/NetworkManager.h"
#include "../../providers/ProviderFetchContext.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QUuid>

namespace OpenCodeUtils {

QString extractAuthCookie(const QString& raw) {
    QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) return {};

    QStringList result;
    for (const auto& pair : trimmed.split(';', Qt::SkipEmptyParts)) {
        QString p = pair.trimmed();
        int eq = p.indexOf('=');
        if (eq <= 0) continue;
        QString name = p.left(eq).trimmed();
        QString value = p.mid(eq + 1).trimmed();
        if (name == "auth" || name == "__Host-auth") {
            result.append(name + "=" + value);
        }
    }
    return result.join("; ");
}

QString resolveCookieHeader(const ProviderFetchContext& ctx) {
    // 1. Manual cookie header
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        QString filtered = extractAuthCookie(*ctx.manualCookieHeader);
        if (!filtered.isEmpty()) return filtered;
    }

    // 2. Environment variable
    for (const auto& key : {"OPENCODE_COOKIE", "OPENCODE_AUTH", "opencode_cookie"}) {
        if (ctx.env.contains(key)) {
            QString filtered = extractAuthCookie(ctx.env[key]);
            if (!filtered.isEmpty()) return filtered;
        }
    }

    // 3. Browser cookie import
    QStringList domains = {"opencode.ai", "app.opencode.ai"};
    for (auto browser : CookieImporter::importOrder()) {
        if (!CookieImporter::isBrowserInstalled(browser)) continue;
        QVector<QNetworkCookie> cookies = CookieImporter::importCookies(browser, domains);
        QStringList parts;
        for (const auto& cookie : cookies) {
            QString name = QString::fromUtf8(cookie.name());
            if (name == "auth" || name == "__Host-auth") {
                parts.append(name + "=" + QString::fromUtf8(cookie.value()));
            }
        }
        if (!parts.isEmpty()) return parts.join("; ");
    }

    return {};
}

QString normalizeWorkspaceID(const QString& raw) {
    QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) return {};
    
    if (trimmed.startsWith("wrk_") && trimmed.length() > 4) {
        return trimmed;
    }
    
    QUrl url(trimmed);
    if (url.isValid()) {
        QStringList parts = url.path().split('/', Qt::SkipEmptyParts);
        int idx = parts.indexOf("workspace");
        if (idx >= 0 && idx + 1 < parts.size()) {
            QString candidate = parts[idx + 1];
            if (candidate.startsWith("wrk_") && candidate.length() > 4) {
                return candidate;
            }
        }
    }
    
    QRegularExpression re("wrk_[A-Za-z0-9]+");
    QRegularExpressionMatch m = re.match(trimmed);
    if (m.hasMatch()) return m.captured(0);
    
    return {};
}

QString fetchWorkspaceID(const QString& cookieHeader, int timeoutMs) {
    static const QString workspacesServerID = "def39973159c7f0483d8793a822b8dbb10d067e12c65455fcb4608459ba0234f";

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["X-Server-Id"] = workspacesServerID;
    headers["X-Server-Instance"] = "server-fn:" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
    headers["Origin"] = "https://opencode.ai";
    headers["Referer"] = "https://opencode.ai";
    headers["Accept"] = "text/javascript, application/json;q=0.9, */*;q=0.8";

    QString url = serverRequestURL(workspacesServerID, {}, "GET");
    QString text = NetworkManager::instance().getStringSync(QUrl(url), headers, timeoutMs, false);

    if (text.isEmpty() || looksSignedOut(text)) return {};

    // Try regex first
    QRegularExpression re(R"re(id\s*:\s*"(wrk_[^"]+)")re");
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        if (m.hasMatch()) return m.captured(1);
    }

    // Helper to recursively find workspace IDs
    std::function<QString(const QJsonValue&)> findId = [&](const QJsonValue& v) -> QString {
        if (v.isString()) {
            QString s = v.toString();
            if (s.startsWith("wrk_")) return s;
        }
        if (v.isObject()) {
            QJsonObject o = v.toObject();
            for (auto it = o.begin(); it != o.end(); ++it) {
                QString found = findId(it.value());
                if (!found.isEmpty()) return found;
            }
        }
        if (v.isArray()) {
            for (const auto& item : v.toArray()) {
                QString found = findId(item);
                if (!found.isEmpty()) return found;
            }
        }
        return {};
    };

    // Try JSON
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (err.error == QJsonParseError::NoError) {
        QString found = findId(doc.object());
        if (!found.isEmpty()) return found;
    }

    // Fallback: POST
    QJsonObject body;
    body["args"] = QJsonArray();
    headers["Content-Type"] = "application/json";
    url = serverRequestURL(workspacesServerID, {}, "POST");
    QJsonObject json = NetworkManager::instance().postJsonSync(QUrl(url), body, headers, timeoutMs, false);
    if (!json.isEmpty()) {
        QString found = findId(json);
        if (!found.isEmpty()) return found;
    }

    return {};
}

bool looksSignedOut(const QString& text) {
    QString lower = text.toLower();
    return lower.contains("login") || lower.contains("sign in") ||
           lower.contains("auth/authorize") || lower.contains("not associated with an account") ||
           lower.contains("actor of type \"public\"");
}

QString serverRequestURL(const QString& serverID, const QStringList& args, const QString& method) {
    QUrl url("https://opencode.ai/_server");
    if (method.toUpper() == "GET") {
        QUrlQuery query;
        query.addQueryItem("id", serverID);
        if (!args.isEmpty()) {
            QJsonArray arr;
            for (const auto& arg : args) arr.append(arg);
            query.addQueryItem("args", QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
        }
        url.setQuery(query);
    }
    return url.toString();
}

} // namespace OpenCodeUtils
