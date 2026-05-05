#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/RateWindow.h"
#include "../../models/ProviderIdentitySnapshot.h"
#include <QObject>

class MistralProvider : public IProvider {
    Q_OBJECT
public:
    explicit MistralProvider(QObject* parent = nullptr);
    QString id() const override { return "mistral"; }
    QString displayName() const override { return "Mistral"; }
    QString sessionLabel() const override { return "Monthly"; }
    QString weeklyLabel() const override { return ""; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QString dashboardURL() const override { return "https://console.mistral.ai"; }
    QString brandColor() const override { return "#FF500F"; }
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return { {"manualCookieHeader", "Manual cookie header", "secret", QVariant(),
                   {}, "com.codexbar.cookie.mistral", {}, "cookie=value; ...",
                   "Stored in Windows Credential Manager", true, true} };
    }
    QVector<QString> supportedSourceModes() const override { return {"web"}; }
    QString statusLinkURL() const override { return "https://status.mistral.ai"; }
};

class MistralWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit MistralWebStrategy(QObject* parent = nullptr);
    QString id() const override { return "mistral.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override { return false; }
};
