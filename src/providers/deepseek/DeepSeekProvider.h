#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

class DeepSeekProvider : public IProvider {
    Q_OBJECT
public:
    explicit DeepSeekProvider(QObject* parent = nullptr);

    QString id() const override { return "deepseek"; }
    QString displayName() const override { return "DeepSeek"; }
    QString sessionLabel() const override { return "Balance"; }
    QString weeklyLabel() const override { return "Balance"; }
    bool supportsCredits() const override { return true; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"apiKey", "API key", "secret", QVariant(),
             {}, "com.codexbar.apikey.deepseek", "DEEPSEEK_API_KEY",
             "sk-...", "Stored in Windows Credential Manager", false, true}
        };
    }

    QString brandColor() const override { return "#4D6BFE"; }
    QString dashboardURL() const override { return "https://platform.deepseek.com/usage"; }
    QVector<QString> supportedSourceModes() const override { return {"api"}; }
};

class DeepSeekAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit DeepSeekAPIStrategy(QObject* parent = nullptr);

    QString id() const override { return "deepseek.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseResponse(const QJsonObject& json);
private:
    static QString resolveApiKey(const ProviderFetchContext& ctx);
};
