#pragma once

#include "IProviderLoginFlow.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

/**
 * @brief Generic OAuth2 device flow implementation.
 *
 * Implements the OAuth2 device authorization grant flow:
 * 1. Request device code from authorization server
 * 2. Display user_code to user and instruct them to visit verification_uri
 * 3. Poll token endpoint until user completes authorization or code expires
 */
class OAuthDeviceFlow : public IProviderLoginFlow {
    Q_OBJECT
public:
    explicit OAuthDeviceFlow(const QString& providerId,
                             const QUrl& deviceAuthEndpoint,
                             const QUrl& tokenEndpoint,
                             const QString& clientId,
                             QObject* parent = nullptr);

    void start() override;
    void cancel() override;

    LoginFlowState state() const override { return m_loginState; }
    QString userMessage() const override { return m_userMessage; }
    QUrl actionUrl() const override { return m_actionUrl; }

private:
    void requestDeviceCode();
    void startPolling();
    void pollTokenEndpoint();
    void handleDeviceCodeResponse(QNetworkReply* reply);
    void handleTokenResponse(QNetworkReply* reply);

    QString m_providerId;
    QUrl m_deviceAuthEndpoint;
    QUrl m_tokenEndpoint;
    QString m_clientId;
    QNetworkAccessManager* m_netManager = nullptr;
    QTimer* m_pollTimer = nullptr;
    OAuthDeviceCode m_deviceCode;
    int m_pollAttempts = 0;
    static constexpr int MAX_POLL_ATTEMPTS = 60;
};
