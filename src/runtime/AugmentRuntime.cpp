#include "AugmentRuntime.h"
#include "../providers/ProviderPipeline.h"
#include "../providers/ProviderRegistry.h"
#include "../account/TokenAccountStore.h"
#include <QDebug>

AugmentRuntime::AugmentRuntime(QObject* parent)
    : IProviderRuntime(parent)
{
}

AugmentRuntime::~AugmentRuntime()
{
    if (m_keepaliveTimer) {
        m_keepaliveTimer->stop();
        m_keepaliveTimer->deleteLater();
    }
}

void AugmentRuntime::start()
{
    setState(RuntimeState::Running);

    // Setup keepalive timer
    if (!m_keepaliveTimer) {
        m_keepaliveTimer = new QTimer(this);
        connect(m_keepaliveTimer, &QTimer::timeout, this, &AugmentRuntime::performKeepalive);
    }
    m_keepaliveTimer->start(m_keepaliveIntervalHours * 60 * 60 * 1000);
}

void AugmentRuntime::stop()
{
    if (m_keepaliveTimer) {
        m_keepaliveTimer->stop();
    }
    setState(RuntimeState::Stopped);
}

void AugmentRuntime::pause()
{
    if (m_keepaliveTimer) {
        m_keepaliveTimer->stop();
    }
    if (m_state == RuntimeState::Running) {
        setState(RuntimeState::Paused);
    }
}

void AugmentRuntime::resume()
{
    if (m_state == RuntimeState::Paused) {
        setState(RuntimeState::Running);
        if (m_keepaliveTimer) {
            m_keepaliveTimer->start(m_keepaliveIntervalHours * 60 * 60 * 1000);
        }
    }
}

ProviderFetchResult AugmentRuntime::fetch(const ProviderFetchContext& ctx)
{
    IProvider* provider = ProviderRegistry::instance().provider("augment");
    if (!provider) {
        recordFailure("Augment provider not found");
        emit fetchFailed(m_lastError);
        ProviderFetchResult r;
        r.success = false;
        r.errorMessage = m_lastError;
        return r;
    }

    if (m_state != RuntimeState::Running) {
        recordFailure("Runtime is not running");
        emit fetchFailed(m_lastError);
        ProviderFetchResult r;
        r.success = false;
        r.errorMessage = m_lastError;
        return r;
    }

    ProviderPipeline pipeline;
    ProviderFetchResult result = pipeline.executeProvider(provider, ctx);

    if (result.success) {
        recordSuccess();
        m_keepaliveFailures = 0;
        emit fetchCompleted(result);
    } else {
        recordFailure(result.errorMessage.isEmpty() ? "Augment fetch failed" : result.errorMessage);
        emit fetchFailed(m_lastError);
    }
    return result;
}

void AugmentRuntime::setKeepaliveIntervalHours(int hours)
{
    m_keepaliveIntervalHours = qMax(1, hours);
    if (m_keepaliveTimer && m_keepaliveTimer->isActive()) {
        m_keepaliveTimer->setInterval(m_keepaliveIntervalHours * 60 * 60 * 1000);
    }
}

void AugmentRuntime::performKeepalive()
{
    if (m_state != RuntimeState::Running) return;

    // TODO: Implement actual keepalive ping
    // This should make a lightweight API call to Augment to refresh the session.
    // For now, we simulate success.
    qDebug() << "AugmentRuntime: performing keepalive";

    if (refreshSessionCookie()) {
        m_keepaliveFailures = 0;
        m_sessionExpiry = QDateTime::currentDateTime().addDays(7);
    } else {
        m_keepaliveFailures++;
        if (m_keepaliveFailures >= MAX_KEEPALIVE_FAILURES) {
            setState(RuntimeState::Error);
            m_lastError = "Augment session keepalive failed repeatedly";
        }
    }
}

bool AugmentRuntime::refreshSessionCookie()
{
    // TODO: Implement actual session refresh.
    // This would typically involve:
    // 1. Checking if the current cookie is near expiry
    // 2. Making a headless browser request or API call to refresh
    // 3. Storing the new cookie in TokenAccountStore

    auto accountId = TokenAccountStore::instance()->defaultAccountId("augment");
    if (accountId.isEmpty()) return false;

    auto accOpt = TokenAccountStore::instance()->account(accountId);
    if (!accOpt.has_value()) return false;

    // Placeholder: assume cookie is valid
    return true;
}
