#include "GenericRuntime.h"
#include "../providers/ProviderPipeline.h"

GenericRuntime::GenericRuntime(IProvider* provider, QObject* parent)
    : IProviderRuntime(parent)
    , m_provider(provider)
{
}

void GenericRuntime::start()
{
    setState(RuntimeState::Running);
}

void GenericRuntime::stop()
{
    setState(RuntimeState::Stopped);
}

void GenericRuntime::pause()
{
    if (m_state == RuntimeState::Running) {
        setState(RuntimeState::Paused);
    }
}

void GenericRuntime::resume()
{
    if (m_state == RuntimeState::Paused) {
        setState(RuntimeState::Running);
    }
}

ProviderFetchResult GenericRuntime::fetch(const ProviderFetchContext& ctx)
{
    if (!m_provider) {
        recordFailure("No provider assigned to runtime");
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
    ProviderFetchResult result = pipeline.executeProvider(m_provider, ctx);

    if (result.success) {
        recordSuccess();
        emit fetchCompleted(result);
    } else {
        recordFailure(result.errorMessage.isEmpty() ? "Fetch failed" : result.errorMessage);
        emit fetchFailed(m_lastError);
    }
    return result;
}
