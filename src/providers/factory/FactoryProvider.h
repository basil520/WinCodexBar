#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

class FactoryProvider : public IProvider {
    Q_OBJECT
public:
    explicit FactoryProvider(QObject* parent = nullptr);

    QString id() const override { return "factory"; }
    QString displayName() const override { return "Factory"; }
    QString sessionLabel() const override { return "Standard"; }
    QString weeklyLabel() const override { return "Premium"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"cookieSource", "Cookie source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Automatic"}, {"manual", "Manual"} }},
            {"manualCookieHeader", "Manual cookie", "secret", QVariant(),
             {}, {}, {}, "wos-session=...", "Paste cookie header", false, true}
        };
    }

    QString brandColor() const override { return "#84CC16"; }
    QString dashboardURL() const override { return "https://app.factory.ai"; }
    QVector<QString> supportedSourceModes() const override { return {"web"}; }
};

class FactoryWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit FactoryWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "factory.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseResponse(const QJsonObject& json);
private:
    static QString resolveCookieHeader(const ProviderFetchContext& ctx);
};
