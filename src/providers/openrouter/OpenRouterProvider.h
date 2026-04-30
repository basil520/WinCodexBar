#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/OpenRouterUsageSnapshot.h"

#include <QObject>
#include <QString>
#include <QFuture>

class OpenRouterProvider : public IProvider {
    Q_OBJECT
public:
    explicit OpenRouterProvider(QObject* parent = nullptr);
    QString id() const override { return "openrouter"; }
    QString displayName() const override { return "OpenRouter"; }
    QString sessionLabel() const override { return "Credits"; }
    QString weeklyLabel() const override { return "Quota"; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QString dashboardURL() const override { return "https://openrouter.ai/settings/credits"; }
    QString brandColor() const override { return "#6467F2"; }
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return { {"apiKey", "API key", "secret", QVariant(),
                   {}, "com.codexbar.apikey.openrouter", "OPENROUTER_API_KEY",
                   "sk-or-...", "Stored in Windows Credential Manager", false, true} };
    }
    QVector<QString> supportedSourceModes() const override { return {"api"}; }
};

class OpenRouterAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit OpenRouterAPIStrategy(QObject* parent = nullptr);
    QString id() const override { return "openrouter.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;
};
