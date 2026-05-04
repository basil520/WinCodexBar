#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

class AugmentProvider : public IProvider {
    Q_OBJECT
public:
    explicit AugmentProvider(QObject* parent = nullptr);

    QString id() const override { return "augment"; }
    QString displayName() const override { return "Augment"; }
    QString sessionLabel() const override { return "Credits"; }
    QString weeklyLabel() const override { return "Credits"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"cookieSource", "Cookie source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Automatic"}, {"manual", "Manual"} }},
            {"manualCookieHeader", "Manual cookie", "secret", QVariant(),
             {}, {}, {}, "_session=...", "Paste cookie header", false, true}
        };
    }

    QString brandColor() const override { return "#14B8A6"; }
    QString dashboardURL() const override { return "https://app.augmentcode.com"; }
    QVector<QString> supportedSourceModes() const override { return {"cli", "web"}; }
};

class AugmentCLIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit AugmentCLIStrategy(QObject* parent = nullptr);

    QString id() const override { return "augment.cli"; }
    int kind() const override { return ProviderFetchKind::CLI; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;
};

class AugmentWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit AugmentWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "augment.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseCreditsResponse(const QJsonObject& json);
private:
    static QString resolveCookieHeader(const ProviderFetchContext& ctx);
};
