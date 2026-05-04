#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

class WarpProvider : public IProvider {
    Q_OBJECT
public:
    explicit WarpProvider(QObject* parent = nullptr);

    QString id() const override { return "warp"; }
    QString displayName() const override { return "Warp"; }
    QString sessionLabel() const override { return "Requests"; }
    QString weeklyLabel() const override { return "Bonus"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"apiKey", "API key", "secret", QVariant(),
             {}, "com.codexbar.apikey.warp", "WARP_API_KEY",
             "wk_...", "Stored in Windows Credential Manager", false, true}
        };
    }

    QString brandColor() const override { return "#00BCD4"; }
    QString dashboardURL() const override { return "https://app.warp.dev"; }
    QVector<QString> supportedSourceModes() const override { return {"api"}; }
};

class WarpAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit WarpAPIStrategy(QObject* parent = nullptr);

    QString id() const override { return "warp.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseResponse(const QJsonObject& json);
private:
    static QString resolveApiKey(const ProviderFetchContext& ctx);
};
