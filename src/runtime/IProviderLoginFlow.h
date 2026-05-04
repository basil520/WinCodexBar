#pragma once

#include <QObject>
#include <QUrl>

enum class LoginFlowState {
    Idle,
    AwaitingUserAction,
    Polling,
    Completing,
    Success,
    Failed,
    Cancelled
};

struct OAuthDeviceCode {
    QString userCode;
    QString deviceCode;
    QUrl verificationUri;
    int expiresIn = 0;
    int interval = 5;
};

class IProviderLoginFlow : public QObject {
    Q_OBJECT
public:
    explicit IProviderLoginFlow(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IProviderLoginFlow() = default;

    virtual void start() = 0;
    virtual void cancel() = 0;

    virtual LoginFlowState state() const = 0;
    virtual QString userMessage() const = 0;
    virtual QUrl actionUrl() const = 0;

    virtual bool acceptsManualInput() const { return false; }
    virtual void submitManualInput(const QString& input) { Q_UNUSED(input) }

signals:
    void stateChanged(LoginFlowState newState);
    void userMessageChanged(const QString& message);
    void actionUrlChanged(const QUrl& url);
    void completedSuccessfully(const QString& accountId);
    void failed(const QString& errorMessage);

protected:
    void setLoginState(LoginFlowState state);
    void setUserMessage(const QString& msg);
    void setActionUrl(const QUrl& url);

    LoginFlowState m_loginState = LoginFlowState::Idle;
    QString m_userMessage;
    QUrl m_actionUrl;
};
