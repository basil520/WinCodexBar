#include "IProviderLoginFlow.h"

void IProviderLoginFlow::setLoginState(LoginFlowState state)
{
    if (m_loginState != state) {
        m_loginState = state;
        emit stateChanged(state);
    }
}

void IProviderLoginFlow::setUserMessage(const QString& msg)
{
    if (m_userMessage != msg) {
        m_userMessage = msg;
        emit userMessageChanged(msg);
    }
}

void IProviderLoginFlow::setActionUrl(const QUrl& url)
{
    if (m_actionUrl != url) {
        m_actionUrl = url;
        emit actionUrlChanged(url);
    }
}
