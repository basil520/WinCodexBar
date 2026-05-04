#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"
#include "../../models/MiniMaxUsageSnapshot.h"

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
    QString weeklyLabel() const override { return "Window"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"apiRegion", "API region", "picker", QVariant(QStringLiteral("global")),
             { {"global", "Global"}, {"cn", "China"} }},
            {"apiKey", "API key", "secret", QVariant(),
             {}, "com.codexbar.apikey.minimax", "MINIMAX_API_KEY",
             "sk-cp-...", "MiniMax Coding Plan API key", false, true},
            {"cookieSource", "Cookie source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Automatic"}, {"manual", "Manual"} }},
            {"manualCookieHeader", "Manual cookie", "secret", QVariant(),
             {}, {}, "MINIMAX_COOKIE", "HERTZ-SESSION=...", "Paste MiniMax cookie header", false, true}
        };
    }

    QString brandColor() const override { return "#EC4899"; }
    QString dashboardURL() const override {
        return "https://platform.minimax.io/user-center/payment/coding-plan?cycle_type=3";
    }
    QVector<QString> supportedSourceModes() const override { return {"auto", "web", "api"}; }
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
    static ProviderFetchResult fetchOnce(const QString& apiKey, const QString& baseUrl, const ProviderFetchContext& ctx);
};

class MiniMaxWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit MiniMaxWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "minimax.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static QString resolveCookieHeader(const ProviderFetchContext& ctx);
    static ProviderFetchResult fetchWithCookie(const QString& cookieHeader, const ProviderFetchContext& ctx);
};
