#include "CodexTokenRefresher.h"
#include "../../network/NetworkManager.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

const QString CodexTokenRefresher::RefreshEndpoint = "https://auth.openai.com/oauth/token";
const QString CodexTokenRefresher::ClientID = "app_EMoamEEZ73f0CkXaXp7hrann";

CodexTokenRefresher::RefreshResult CodexTokenRefresher::refresh(const CodexOAuthCredentials& credentials)
{
    RefreshResult result;
    result.success = false;
    result.error = RefreshError::NetworkError;

    if (credentials.refreshToken.isEmpty()) {
        result.success = true;
        result.credentials = credentials;
        return result;
    }

    QJsonObject body;
    body["client_id"] = ClientID;
    body["grant_type"] = "refresh_token";
    body["refresh_token"] = credentials.refreshToken;
    body["scope"] = "openid profile email";

    QHash<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Accept"] = "application/json";

    QJsonObject resp = NetworkManager::instance().postJsonSync(
        QUrl(RefreshEndpoint), body, headers, 30000);

    if (resp.isEmpty()) {
        result.errorMessage = "Empty response from token refresh endpoint";
        return result;
    }

    if (resp.contains("error")) {
        QJsonObject errorObj = resp.value("error").toObject();
        QString errorCode = errorObj.value("code").toString();
        if (errorCode.isEmpty()) errorCode = errorObj.value("error").toString();
        if (errorCode.isEmpty()) errorCode = resp.value("code").toString();

        result.error = classifyError(errorCode);
        switch (result.error) {
        case RefreshError::Expired:
            result.errorMessage = "Refresh token expired. Please run `codex` to log in again.";
            break;
        case RefreshError::Revoked:
            result.errorMessage = "Refresh token was revoked. Please run `codex` to log in again.";
            break;
        case RefreshError::Reused:
            result.errorMessage = "Refresh token was already used. Please run `codex` to log in again.";
            break;
        default:
            result.errorMessage = "Token refresh failed: " + errorCode;
            break;
        }
        return result;
    }

    QString newAccessToken = resp.value("access_token").toString();
    QString newRefreshToken = resp.value("refresh_token").toString();
    QString newIdToken = resp.value("id_token").toString();

    if (newAccessToken.isEmpty()) {
        result.errorMessage = "Invalid refresh response: missing access_token";
        result.error = RefreshError::InvalidResponse;
        return result;
    }

    CodexOAuthCredentials refreshed;
    refreshed.accessToken = newAccessToken;
    refreshed.refreshToken = newRefreshToken.isEmpty() ? credentials.refreshToken : newRefreshToken;
    refreshed.idToken = newIdToken.isEmpty() ? credentials.idToken : newIdToken;
    refreshed.accountId = credentials.accountId;
    refreshed.lastRefresh = QDateTime::currentDateTime();

    result.success = true;
    result.credentials = refreshed;
    return result;
}

QString CodexTokenRefresher::extractErrorCode(const QJsonObject& json)
{
    QJsonObject error = json.value("error").toObject();
    QString code = error.value("code").toString();
    if (!code.isEmpty()) return code;

    QString errorStr = json.value("error").toString();
    if (!errorStr.isEmpty()) return errorStr;

    return json.value("code").toString();
}

CodexTokenRefresher::RefreshError CodexTokenRefresher::classifyError(const QString& errorCode)
{
    QString lower = errorCode.toLower();
    if (lower == "refresh_token_expired") return RefreshError::Expired;
    if (lower == "refresh_token_reused") return RefreshError::Reused;
    if (lower == "refresh_token_invalidated") return RefreshError::Revoked;
    return RefreshError::Expired;
}
