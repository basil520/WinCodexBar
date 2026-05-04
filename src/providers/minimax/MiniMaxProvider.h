#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

class MiniMaxProvider : public IProvider {
    Q_OBJECT
public:
    explicit MiniMaxProvider(QObject* parent = nullptr);

    QString id() const override { return "minimax"; }
    QString displayName() const override { return "MiniMax"; }
    QString sessionLabel() const override { return "Prompts"; }
    QString weeklyLabel() const override { return "Interval"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"apiRegion", "API region", "picker", QVariant(QStringLiteral("global")),
             { {"global", "Global"}, {"cn", "China"} }},
            {"apiKey", "API key", "secret", QVariant(),
             {}, "com.codexbar.apikey.minimax", "MINIMAX_API_KEY",
             "sk-cp-...", "MiniMax Coding Plan API key", false, true}
        };
    }

    QString brandColor() const override { return "#EC4899"; }
    QString dashboardURL() const override { return "https://platform.minimax.io/user-center/payment/coding-plan"; }
    QVector<QString> supportedSourceModes() const override { return {"api"}; }
};

class MiniMaxAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit MiniMaxAPIStrategy(QObject* parent = nullptr);

    QString id() const override { return "minimax.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseResponse(const QJsonObject& json);

private:
    static QString resolveApiKey(const ProviderFetchContext& ctx);
    static QString resolveEndpoint(const ProviderFetchContext& ctx);
};
