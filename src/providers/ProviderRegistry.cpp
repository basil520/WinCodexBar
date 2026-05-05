#include "ProviderRegistry.h"

ProviderRegistry& ProviderRegistry::instance() {
    static ProviderRegistry reg;
    return reg;
}

void ProviderRegistry::registerProvider(IProvider* provider) {
    if (!provider) return;
    m_providers[provider->id()] = provider;

    ProviderDescriptor descriptor;
    descriptor.id = provider->id();
    descriptor.metadata.displayName = provider->displayName();
    descriptor.metadata.sessionLabel = provider->sessionLabel();
    descriptor.metadata.weeklyLabel = provider->weeklyLabel();
    QString opus = provider->opusLabel();
    if (!opus.isEmpty()) descriptor.metadata.opusLabel = opus;
    descriptor.metadata.supportsCredits = provider->supportsCredits();
    descriptor.metadata.cliName = provider->cliName();
    descriptor.metadata.statusPageURL = provider->statusPageURL();
    descriptor.metadata.statusLinkURL = provider->statusLinkURL();
    descriptor.metadata.statusWorkspaceProductID = provider->statusWorkspaceProductID();
    descriptor.metadata.dashboardURL = provider->dashboardURL();
    descriptor.metadata.subscriptionDashboardURL = provider->subscriptionDashboardURL();
    descriptor.fetchPlan.allowedSourceModes = provider->supportedSourceModes();
    descriptor.fetchPlan.defaultSourceMode = descriptor.fetchPlan.allowedSourceModes.contains(QStringLiteral("auto"))
        ? QStringLiteral("auto")
        : (descriptor.fetchPlan.allowedSourceModes.isEmpty()
               ? QStringLiteral("auto")
               : descriptor.fetchPlan.allowedSourceModes.first());
    descriptor.cli.name = provider->cliName();
    descriptor.tokenAccounts.supportsMultipleAccounts = provider->supportsMultipleAccounts();
    descriptor.tokenAccounts.requiredCredentialTypes = provider->requiredCredentialTypes();
    registerDescriptor(descriptor);
}

void ProviderRegistry::registerDescriptor(const ProviderDescriptor& descriptor) {
    if (descriptor.id.isEmpty()) return;
    m_descriptors[descriptor.id] = descriptor;
}

IProvider* ProviderRegistry::provider(const QString& id) const {
    return m_providers.value(id, nullptr);
}

QVector<IProvider*> ProviderRegistry::allProviders() const {
    QVector<IProvider*> result;
    for (auto* p : m_providers) {
        result.append(p);
    }
    return result;
}

QVector<QString> ProviderRegistry::providerIDs() const {
    return QVector<QString>(m_providers.keyBegin(), m_providers.keyEnd());
}

std::optional<ProviderDescriptor> ProviderRegistry::descriptor(const QString& id) const {
    auto it = m_descriptors.constFind(id);
    if (it != m_descriptors.constEnd()) {
        return *it;
    }
    return std::nullopt;
}

bool ProviderRegistry::isProviderEnabled(const QString& id) const {
    return m_enabled.value(id, false);
}

void ProviderRegistry::setProviderEnabled(const QString& id, bool enabled) {
    m_enabled[id] = enabled;
}

QVector<QString> ProviderRegistry::enabledProviderIDs() const {
    QVector<QString> result;
    for (auto it = m_enabled.constBegin(); it != m_enabled.constEnd(); ++it) {
        if (it.value()) {
            result.append(it.key());
        }
    }
    return result;
}
