#pragma once

#include <QHash>
#include <QString>
#include <QVector>
#include <memory>

#include "IProvider.h"
#include "../models/ProviderDescriptor.h"

class ProviderRegistry {
public:
    static ProviderRegistry& instance();

    void registerProvider(IProvider* provider);
    void registerDescriptor(const ProviderDescriptor& descriptor);

    IProvider* provider(const QString& id) const;
    QVector<IProvider*> allProviders() const;
    QVector<QString> providerIDs() const;
    std::optional<ProviderDescriptor> descriptor(const QString& id) const;

    bool isProviderEnabled(const QString& id) const;
    void setProviderEnabled(const QString& id, bool enabled);
    QVector<QString> enabledProviderIDs() const;

private:
    ProviderRegistry() = default;

    QHash<QString, IProvider*> m_providers;
    QHash<QString, ProviderDescriptor> m_descriptors;
    QHash<QString, bool> m_enabled;
};
