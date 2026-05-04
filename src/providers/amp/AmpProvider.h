#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>

class AmpProvider : public IProvider {
    Q_OBJECT
public:
    explicit AmpProvider(QObject* parent = nullptr);

    QString id() const override { return "amp"; }
    QString displayName() const override { return "Amp"; }
    QString sessionLabel() const override { return "Usage"; }
    QString weeklyLabel() const override { return "Quota"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"cookieSource", "Cookie source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Automatic"}, {"manual", "Manual"} }},
            {"manualCookieHeader", "Manual cookie", "secret", QVariant(),
             {}, {}, {}, "session=...", "Paste cookie header", false, true}
        };
    }

    QString brandColor() const override { return "#D946EF"; }
    QString dashboardURL() const override { return "https://ampcode.com/settings"; }
    QVector<QString> supportedSourceModes() const override { return {"web"}; }
};

class AmpWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit AmpWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "amp.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseHTML(const QString& html);
private:
    static QString resolveCookieHeader(const ProviderFetchContext& ctx);
};
