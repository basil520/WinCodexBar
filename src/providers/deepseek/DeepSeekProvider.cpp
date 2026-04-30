#include "DeepSeekProvider.h"

DeepSeekProvider::DeepSeekProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> DeepSeekProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new DeepSeekTokenStrategy() };
}

QVector<ProviderSettingsDescriptor> DeepSeekProvider::settingsDescriptors() const {
    return {};
}

DeepSeekTokenStrategy::DeepSeekTokenStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool DeepSeekTokenStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    // Always available - token data comes from OpenCode DB scan
    return true;
}

ProviderFetchResult DeepSeekTokenStrategy::fetchSync(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    // Return empty result - actual token data is populated by UsageStore
    // from the OpenCode DB scan in refreshCostUsage()
    ProviderFetchResult result;
    result.success = true;
    return result;
}
