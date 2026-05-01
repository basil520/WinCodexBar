#include "CodexOAuthUsageFetcher.h"
#include "../../network/NetworkManager.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

const QString CodexOAuthUsageFetcher::DefaultChatGPTBaseURL = "https://chatgpt.com/backend-api/";
const QString CodexOAuthUsageFetcher::ChatGPTUsagePath = "/wham/usage";
const QString CodexOAuthUsageFetcher::CodexUsagePath = "/api/codex/usage";

CodexOAuthUsageFetcher::FetchResult CodexOAuthUsageFetcher::fetchUsage(
    const QString& accessToken,
    const QString& accountId,
    const QHash<QString, QString>& env)
{
    FetchResult result;
    result.success = false;
    result.error = FetchError::NetworkError;
    result.statusCode = 0;

    QUrl url = resolveUsageURL(env);

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + accessToken;
    headers["Accept"] = "application/json";
    headers["User-Agent"] = "CodexBar";

    if (!accountId.isEmpty()) {
        headers["ChatGPT-Account-Id"] = accountId;
    }

    NetworkManager& nm = NetworkManager::instance();
    QJsonObject json = nm.getJsonSync(url, headers, 30000);

    if (json.isEmpty()) {
        result.errorMessage = "Empty or invalid response from Codex API";
        return result;
    }

    if (json.contains("error")) {
        QJsonObject errorObj = json.value("error").toObject();
        QString errorMsg = errorObj.value("message").toString();
        if (errorMsg.isEmpty()) errorMsg = json.value("detail").toString();
        if (errorMsg.isEmpty()) errorMsg = "API error";

        result.errorMessage = errorMsg;
        result.error = FetchError::ServerError;
        return result;
    }

    CodexUsageResponse response = CodexUsageResponse::fromJson(json);
    if (!response.primaryWindow.has_value() && !response.secondaryWindow.has_value()) {
        result.errorMessage = "No rate limit data in response";
        result.error = FetchError::InvalidResponse;
        return result;
    }

    result.success = true;
    result.response = response;
    return result;
}

QUrl CodexOAuthUsageFetcher::resolveUsageURL(const QHash<QString, QString>& env)
{
    QString baseURL = resolveChatGPTBaseURL(env);
    QString normalized = normalizeChatGPTBaseURL(baseURL);
    QString path = normalized.contains("/backend-api") ? ChatGPTUsagePath : CodexUsagePath;
    QString full = normalized + path;
    return QUrl(full);
}

QString CodexOAuthUsageFetcher::resolveChatGPTBaseURL(const QHash<QString, QString>& env)
{
    QString configContents = loadConfigContents(env);
    if (!configContents.isEmpty()) {
        QString parsed = parseChatGPTBaseURL(configContents);
        if (!parsed.isEmpty()) return parsed;
    }
    return DefaultChatGPTBaseURL;
}

QString CodexOAuthUsageFetcher::normalizeChatGPTBaseURL(const QString& value)
{
    QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) return DefaultChatGPTBaseURL;
    while (trimmed.endsWith('/')) trimmed.chop(1);
    if ((trimmed.startsWith("https://chatgpt.com") || trimmed.startsWith("https://chat.openai.com"))
        && !trimmed.contains("/backend-api")) {
        trimmed += "/backend-api";
    }
    return trimmed;
}

QString CodexOAuthUsageFetcher::parseChatGPTBaseURL(const QString& contents)
{
    QStringList lines = contents.split('\n');
    for (const QString& rawLine : lines) {
        QString line = rawLine.split('#').first().trimmed();
        if (line.isEmpty()) continue;
        int eqIdx = line.indexOf('=');
        if (eqIdx < 0) continue;
        QString key = line.left(eqIdx).trimmed();
        QString value = line.mid(eqIdx + 1).trimmed();
        if (key != "chatgpt_base_url") continue;
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        } else if (value.startsWith('\'') && value.endsWith('\'')) {
            value = value.mid(1, value.length() - 2);
        }
        return value.trimmed();
    }
    return {};
}

QString CodexOAuthUsageFetcher::loadConfigContents(const QHash<QString, QString>& env)
{
    QString home = env.value("USERPROFILE", env.value("HOME", QDir::homePath()));
    QString codexHome = env.value("CODEX_HOME").trimmed();
    QString root = codexHome.isEmpty() ? home + "/.codex" : codexHome;
    QString configPath = root + "/config.toml";

    QFile file(configPath);
    if (!file.exists()) return {};
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(file.readAll());
}
