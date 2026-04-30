#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/RateWindow.h"
#include <QObject>

class KimiK2Provider : public IProvider {
    Q_OBJECT
public:
    explicit KimiK2Provider(QObject* parent = nullptr);
    QString id() const override { return "kimik2"; }
    QString displayName() const override { return "Kimi K2"; }
    QString sessionLabel() const override { return "Credits"; }
    QString weeklyLabel() const override { return ""; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return { {"apiKey", "API key", "secret", QVariant(),
                   {}, "com.codexbar.apikey.kimik2", "KIMI_K2_API_KEY",
                   "sk-...", "Stored in Windows Credential Manager", false, true} };
    }
    QString brandColor() const override { return "#4C00FF"; }
    QVector<QString> supportedSourceModes() const override { return {"api"}; }
};

class KimiK2APIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit KimiK2APIStrategy(QObject* parent = nullptr);
    QString id() const override { return "kimik2.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override { return false; }
};
