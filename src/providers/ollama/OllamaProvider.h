#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/RateWindow.h"
#include "../../models/ProviderIdentitySnapshot.h"
#include <QObject>

class OllamaProvider : public IProvider {
    Q_OBJECT
public:
    explicit OllamaProvider(QObject* parent = nullptr);
    QString id() const override { return "ollama"; }
    QString displayName() const override { return "Ollama"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QString dashboardURL() const override { return "https://ollama.com/settings"; }
    QString brandColor() const override { return "#888888"; }
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return { {"manualCookieHeader", "Manual cookie header", "secret", QVariant(),
                   {}, "com.codexbar.cookie.ollama", {}, "cookie=value; ...",
                   "Stored in Windows Credential Manager", true, true} };
    }
    QVector<QString> supportedSourceModes() const override { return {"web"}; }
};

class OllamaWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit OllamaWebStrategy(QObject* parent = nullptr);
    QString id() const override { return "ollama.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override { return false; }
};
