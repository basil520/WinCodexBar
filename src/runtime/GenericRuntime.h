#pragma once

#include "IProviderRuntime.h"
#include "../providers/IProvider.h"

/**
 * @brief Default runtime for providers that don't need special lifecycle behavior.
 *
 * GenericRuntime simply wraps the existing ProviderPipeline and executes
 * the provider's static strategies. This ensures all providers work with
 * the runtime framework without requiring per-provider runtime implementations.
 */
class GenericRuntime : public IProviderRuntime {
    Q_OBJECT
public:
    explicit GenericRuntime(IProvider* provider, QObject* parent = nullptr);

    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;

    RuntimeState state() const override { return m_state; }
    QString lastError() const override { return m_lastError; }
    QDateTime lastFetchTime() const override { return m_lastFetchTime; }

    ProviderFetchResult fetch(const ProviderFetchContext& ctx) override;

private:
    IProvider* m_provider = nullptr;
};
