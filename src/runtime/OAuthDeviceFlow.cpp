#include "OAuthDeviceFlow.h"
#include "../account/TokenAccountStore.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QDebug>

OAuthDeviceFlow::OAuthDeviceFlow(const QString& providerId,
                                 const QUrl& deviceAuthEndpoint,
                                 const QUrl& tokenEndpoint,
                                 const QString& clientId,
                                 QObject* parent)
    : IProviderLoginFlow(parent)
    , m_providerId(providerId)
    , m_deviceAuthEndpoint(deviceAuthEndpoint)
    , m_tokenEndpoint(tokenEndpoint)
    , m_clientId(clientId)
{
    m_netManager = new QNetworkAccessManager(this);
    m_pollTimer = new QTimer(this);
    m_pollTimer->setSingleShot(true);
    connect(m_pollTimer, &QTimer::timeout, this, &OAuthDeviceFlow::pollTokenEndpoint);
}

void OAuthDeviceFlow::start()
{
    setLoginState(LoginFlowState::AwaitingUserAction);
    setUserMessage("Starting OAuth device flow...");
    requestDeviceCode();
}

void OAuthDeviceFlow::cancel()
{
    m_pollTimer->stop();
    setLoginState(LoginFlowState::Cancelled);
    emit failed("User cancelled");
}

void OAuthDeviceFlow::requestDeviceCode()
{
    QNetworkRequest request(m_deviceAuthEndpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("client_id", m_clientId);
    params.addQueryItem("scope", "read"); // Adjust scope as needed

    QNetworkReply* reply = m_netManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleDeviceCodeResponse(reply);
    });
}

void OAuthDeviceFlow::handleDeviceCodeResponse(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        setLoginState(LoginFlowState::Failed);
        emit failed(QString("Device code request failed: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        setLoginState(LoginFlowState::Failed);
        emit failed("Invalid device code response");
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("error")) {
        setLoginState(LoginFlowState::Failed);
        emit failed(QString("Device code error: %1").arg(obj.value("error").toString()));
        return;
    }

    m_deviceCode.deviceCode = obj.value("device_code").toString();
    m_deviceCode.userCode = obj.value("user_code").toString();
    m_deviceCode.verificationUri = QUrl(obj.value("verification_uri").toString());
    m_deviceCode.expiresIn = obj.value("expires_in").toInt(600);
    m_deviceCode.interval = obj.value("interval").toInt(5);

    setLoginState(LoginFlowState::AwaitingUserAction);
    setUserMessage(QString("Enter code %1 at %2")
                   .arg(m_deviceCode.userCode)
                   .arg(m_deviceCode.verificationUri.toString()));
    setActionUrl(m_deviceCode.verificationUri);

    m_pollAttempts = 0;
    startPolling();
}

void OAuthDeviceFlow::startPolling()
{
    // Wait for the specified interval before first poll
    m_pollTimer->start(m_deviceCode.interval * 1000);
}

void OAuthDeviceFlow::pollTokenEndpoint()
{
    if (m_loginState == LoginFlowState::Cancelled) return;

    m_pollAttempts++;
    if (m_pollAttempts > MAX_POLL_ATTEMPTS) {
        setLoginState(LoginFlowState::Failed);
        emit failed("Device code expired");
        return;
    }

    setLoginState(LoginFlowState::Polling);

    QNetworkRequest request(m_tokenEndpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:device_code");
    params.addQueryItem("device_code", m_deviceCode.deviceCode);
    params.addQueryItem("client_id", m_clientId);

    QNetworkReply* reply = m_netManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleTokenResponse(reply);
    });
}

void OAuthDeviceFlow::handleTokenResponse(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        // Check if it's just "authorization_pending"
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QString error = doc.object().value("error").toString();
            if (error == "authorization_pending" || error == "slow_down") {
                // Continue polling
                int interval = m_deviceCode.interval * 1000;
                if (error == "slow_down") {
                    interval *= 2;
                }
                m_pollTimer->start(interval);
                return;
            }
        }
        setLoginState(LoginFlowState::Failed);
        emit failed(QString("Token request failed: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        setLoginState(LoginFlowState::Failed);
        emit failed("Invalid token response");
        return;
    }

    QJsonObject obj = doc.object();
    QString accessToken = obj.value("access_token").toString();
    QString refreshToken = obj.value("refresh_token").toString();
    int expiresIn = obj.value("expires_in").toInt(3600);

    if (accessToken.isEmpty()) {
        setLoginState(LoginFlowState::Failed);
        emit failed("No access token in response");
        return;
    }

    // Store credentials
    auto accountId = TokenAccountStore::instance()->defaultAccountId(m_providerId);
    if (!accountId.isEmpty()) {
        auto accOpt = TokenAccountStore::instance()->account(accountId);
        if (accOpt.has_value()) {
            TokenAccount acc = accOpt.value();
            OAuthCredentials oauth;
            oauth.accessToken = SecureString(accessToken.toUtf8());
            if (!refreshToken.isEmpty()) {
                oauth.refreshToken = SecureString(refreshToken.toUtf8());
            }
            oauth.expiresAt = QDateTime::currentDateTimeUtc().addSecs(expiresIn);
            oauth.tokenType = obj.value("token_type").toString("Bearer");
            acc.credentials.oauth = std::move(oauth);
            TokenAccountStore::instance()->updateAccount(accountId, acc);
        }
    }

    setLoginState(LoginFlowState::Success);
    emit completedSuccessfully(accountId);
}
