#include "IProviderRuntime.h"
#include <cmath>

int IProviderRuntime::currentBackoffMs() const
{
    if (m_consecutiveFailures <= 0) return 0;
    int backoff = BASE_BACKOFF_MS * static_cast<int>(std::pow(2, m_consecutiveFailures - 1));
    return qMin(backoff, MAX_BACKOFF_MS);
}

void IProviderRuntime::setState(RuntimeState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

void IProviderRuntime::recordFailure(const QString& error)
{
    m_consecutiveFailures++;
    m_lastError = error;
}

void IProviderRuntime::recordSuccess()
{
    m_consecutiveFailures = 0;
    m_lastError.clear();
    m_lastFetchTime = QDateTime::currentDateTime();
}
