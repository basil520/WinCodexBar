#include "CodexProvider.h"
#include "../../network/NetworkManager.h"
#include "../../util/BinaryLocator.h"
#include "../../models/CodexUsageResponse.h"

#include <QJsonDocument>
#include <QFile>
#include <QDir>

CodexProvider::CodexProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> CodexProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new CodexOAuthStrategy() };
}

CodexOAuthStrategy::CodexOAuthStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool CodexOAuthStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return CodexOAuthCredentials::load(ctx.env).has_value();
}

bool CodexOAuthStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

QString CodexOAuthStrategy::resolveAccountEmail(const CodexOAuthCredentials& creds) {
    if (creds.idToken.isEmpty()) return {};
    QJsonObject payload = parseJWTPayload(creds.idToken);
    if (payload.isEmpty()) return {};
    QJsonObject profile = payload.value("https://api.openai.com/profile").toObject();
    QString email = payload.value("email").toString().trimmed();
    if (email.isEmpty()) email = profile.value("email").toString().trimmed();
    return email;
}

std::optional<CodexOAuthCredentials> CodexOAuthStrategy::attemptTokenRefresh(
    const CodexOAuthCredentials& creds, const QHash<QString, QString>& env)
{
    Q_UNUSED(env)
    if (creds.refreshToken.isEmpty()) return std::nullopt;

    QJsonObject body;
    body["client_id"] = "app_EMoamEEZ73f0CkXaXp7hrann";
    body["grant_type"] = "refresh_token";
    body["refresh_token"] = creds.refreshToken;
    body["scope"] = "openid profile email";

    QHash<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Accept"] = "application/json";

    QJsonObject resp = NetworkManager::instance().postJsonSync(
        QUrl("https://auth.openai.com/oauth/token"), body, headers);

    if (resp.isEmpty()) return std::nullopt;

    QString newAccessToken = resp.value("access_token").toString();
    QString newRefreshToken = resp.value("refresh_token").toString();
    QString newIdToken = resp.value("id_token").toString();

    if (newAccessToken.isEmpty()) return std::nullopt;

    CodexOAuthCredentials refreshed;
    refreshed.accessToken = newAccessToken;
    refreshed.refreshToken = newRefreshToken.isEmpty() ? creds.refreshToken : newRefreshToken;
    refreshed.idToken = newIdToken.isEmpty() ? creds.idToken : newIdToken;
    refreshed.accountId = creds.accountId;
    refreshed.lastRefresh = QDateTime::currentDateTime();

    QString home = env.value("USERPROFILE", env.value("HOME", QDir::homePath()));
    QString codexHome = env.value("CODEX_HOME").trimmed();
    QString root = codexHome.isEmpty() ? home + "/.codex" : codexHome;
    QString authPath = root + "/auth.json";

    QFile file(authPath);
    QJsonObject existingJson;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        if (err.error == QJsonParseError::NoError) existingJson = doc.object();
        file.close();
    }

    QJsonObject tokens;
    tokens["access_token"] = refreshed.accessToken;
    tokens["refresh_token"] = refreshed.refreshToken;
    if (!refreshed.idToken.isEmpty()) tokens["id_token"] = refreshed.idToken;
    if (!refreshed.accountId.isEmpty()) tokens["account_id"] = refreshed.accountId;
    existingJson["tokens"] = tokens;
    existingJson["last_refresh"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(existingJson).toJson(QJsonDocument::Compact));
    }

    return refreshed;
}

ProviderFetchResult CodexOAuthStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "oauth";

    auto credsOpt = CodexOAuthCredentials::load(ctx.env);
    if (!credsOpt.has_value()) {
        result.success = false;
        result.errorMessage = "Codex auth.json not found. Run `codex` to log in.";
        return result;
    }

    CodexOAuthCredentials creds = *credsOpt;

    if (creds.needsRefresh()) {
        auto refreshed = attemptTokenRefresh(creds, ctx.env);
        if (refreshed.has_value()) {
            creds = *refreshed;
        }
    }

    QString baseURL = "https://chatgpt.com/backend-api";
    QString usagePath = "/wham/usage";

    QString codexHome = ctx.env.value("CODEX_HOME").trimmed();
    QString home = ctx.env.value("USERPROFILE", ctx.env.value("HOME", QDir::homePath()));
    QString root = codexHome.isEmpty() ? home + "/.codex" : codexHome;
    QString configPath = root + "/config.toml";

    QFile configFile(configPath);
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray content = configFile.readAll();
        for (const auto& rawLine : content.split('\n')) {
            QByteArray line = rawLine.split('#')[0].trimmed();
            if (line.isEmpty()) continue;
            int eqIdx = line.indexOf('=');
            if (eqIdx < 0) continue;
            QString key = line.left(eqIdx).trimmed();
            QString value = line.mid(eqIdx + 1).trimmed();
            if (key != "chatgpt_base_url") continue;
            if (value.startsWith('"') && value.endsWith('"'))
                value = value.mid(1, value.length() - 2);
            else if (value.startsWith('\'') && value.endsWith('\''))
                value = value.mid(1, value.length() - 2);
            value = value.trimmed();
            if (!value.isEmpty()) {
                while (value.endsWith('/')) value.chop(1);
                if (value.startsWith("https://chatgpt.com") ||
                    value.startsWith("https://chat.openai.com")) {
                    if (!value.contains("/backend-api")) value += "/backend-api";
                }
                baseURL = value;
                usagePath = baseURL.contains("/backend-api") ? "/wham/usage" : "/api/codex/usage";
            }
            break;
        }
    }

    QString fullURL = baseURL + usagePath;

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + creds.accessToken;
    headers["Accept"] = "application/json";
    headers["User-Agent"] = "CodexBar";

    if (!creds.accountId.isEmpty()) {
        headers["ChatGPT-Account-Id"] = creds.accountId;
    }

    QJsonObject json = NetworkManager::instance().getJsonSync(QUrl(fullURL), headers, ctx.networkTimeoutMs);
    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "empty or invalid response from Codex API";
        return result;
    }

    if (json.contains("error")) {
        QString errorMsg = json.value("error").toObject().value("message").toString();
        if (errorMsg.isEmpty()) errorMsg = json.value("detail").toString();
        if (errorMsg.isEmpty()) errorMsg = "API error";
        result.success = false;
        result.errorMessage = errorMsg;
        return result;
    }

    CodexUsageResponse usageResp = CodexUsageResponse::fromJson(json);
    if (!usageResp.primaryWindow.has_value() && !usageResp.secondaryWindow.has_value()) {
        result.success = false;
        result.errorMessage = "no rate limit data in response";
        return result;
    }

    QString email = resolveAccountEmail(creds);
    result.usage = usageResp.toUsageSnapshot(email);
    result.success = true;
    return result;
}
