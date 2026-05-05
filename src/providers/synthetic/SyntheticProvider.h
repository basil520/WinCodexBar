#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

class SyntheticProvider : public IProvider {
    Q_OBJECT
public:
    explicit SyntheticProvider(QObject* parent = nullptr);

    QString id() const override { return "synthetic"; }
    QString displayName() const override { return "Synthetic"; }
    QString sessionLabel() const override { return "5-hour"; }
    QString weeklyLabel() const override { return "Weekly"; }
    QString opusLabel() const override { return "Search"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"apiKey", "API key", "secret", QVariant(),
             {}, "com.codexbar.apikey.synthetic", "SYNTHETIC_API_KEY",
             "sk-...", "Synthetic API key", false, true}
        };
    }

    QString brandColor() const override { return "#6366F1"; }
    QVector<QString> supportedSourceModes() const override { return {"api"}; }
    bool supportsMultipleAccounts() const override { return true; }
    QVector<QString> requiredCredentialTypes() const override { return {"apiKey"}; }
};

class SyntheticAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit SyntheticAPIStrategy(QObject* parent = nullptr);

    QString id() const override { return "synthetic.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseResponse(const QJsonObject& json);
private:
    static QString resolveApiKey(const ProviderFetchContext& ctx);
};
