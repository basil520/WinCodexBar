#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/RateWindow.h"
#include "../../models/ProviderIdentitySnapshot.h"
#include <QObject>

class KiloProvider : public IProvider {
    Q_OBJECT
public:
    explicit KiloProvider(QObject* parent = nullptr);
    QString id() const override { return "kilo"; }
    QString displayName() const override { return "Kilo"; }
    QString sessionLabel() const override { return "Credits"; }
    QString weeklyLabel() const override { return "Pass"; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QString dashboardURL() const override { return "https://app.kilo.ai"; }
    QString brandColor() const override { return "#F27027"; }
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return { {"apiKey", "API key", "secret", QVariant(),
                   {}, "com.codexbar.apikey.kilo", "KILO_API_KEY",
                   "sk-...", "Stored in Windows Credential Manager", false, true} };
    }
    QVector<QString> supportedSourceModes() const override { return {"api"}; }
};

class KiloAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit KiloAPIStrategy(QObject* parent = nullptr);
    QString id() const override { return "kilo.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override { return false; }
};
