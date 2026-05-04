#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

class PerplexityProvider : public IProvider {
    Q_OBJECT
public:
    explicit PerplexityProvider(QObject* parent = nullptr);

    QString id() const override { return "perplexity"; }
    QString displayName() const override { return "Perplexity"; }
    QString sessionLabel() const override { return "Recurring"; }
    QString weeklyLabel() const override { return "Promo"; }
    QString opusLabel() const override { return "Purchased"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"cookieSource", "Cookie source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Automatic"}, {"manual", "Manual"} }},
            {"manualCookieHeader", "Manual cookie", "secret", QVariant(),
             {}, {}, "PERPLEXITY_COOKIE",
             "authjs.session-token=...", "Paste cookie header", false, true}
        };
    }

    QString brandColor() const override { return "#22C55E"; }
    QString dashboardURL() const override { return "https://www.perplexity.ai/account/usage"; }
    QVector<QString> supportedSourceModes() const override { return {"web"}; }
};

class PerplexityWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit PerplexityWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "perplexity.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseResponse(const QJsonObject& json);
private:
    static QString resolveCookieHeader(const ProviderFetchContext& ctx);
};
