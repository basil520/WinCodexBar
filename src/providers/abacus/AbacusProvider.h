#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

class AbacusProvider : public IProvider {
    Q_OBJECT
public:
    explicit AbacusProvider(QObject* parent = nullptr);

    QString id() const override { return "abacus"; }
    QString displayName() const override { return "Abacus AI"; }
    QString sessionLabel() const override { return "Credits"; }
    QString weeklyLabel() const override { return "Credits"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"cookieSource", "Cookie source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Automatic"}, {"manual", "Manual"} }},
            {"manualCookieHeader", "Manual cookie", "secret", QVariant(),
             {}, {}, {}, "sessionid=...", "Paste cookie header", false, true}
        };
    }

    QString brandColor() const override { return "#6366F1"; }
    QString dashboardURL() const override { return "https://apps.abacus.ai"; }
    QVector<QString> supportedSourceModes() const override { return {"web"}; }
};

class AbacusWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit AbacusWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "abacus.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static QString resolveCookieHeader(const ProviderFetchContext& ctx);
};
