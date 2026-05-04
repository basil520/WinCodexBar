#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <optional>

class GeminiProvider : public IProvider {
    Q_OBJECT
public:
    explicit GeminiProvider(QObject* parent = nullptr);

    QString id() const override { return "gemini"; }
    QString displayName() const override { return "Gemini"; }
    QString sessionLabel() const override { return "Pro"; }
    QString weeklyLabel() const override { return "Flash"; }
    QString opusLabel() const override { return "Flash Lite"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"apiKey", "API key (optional)", "secret", QVariant(),
             {}, {}, "GEMINI_API_KEY", "AIza...", "Google API key for fallback", false, true}
        };
    }

    QString brandColor() const override { return "#8860D0"; }
    QString dashboardURL() const override { return "https://aistudio.google.com/usage"; }
    QVector<QString> supportedSourceModes() const override { return {"oauth", "api"}; }
};

struct GeminiCredentials {
    QString clientId;
    QString clientSecret;
    QString accessToken;
    QString refreshToken;
};

class GeminiOAuthStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit GeminiOAuthStrategy(QObject* parent = nullptr);

    QString id() const override { return "gemini.oauth"; }
    int kind() const override { return ProviderFetchKind::OAuth; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseResponse(const QJsonObject& json);
private:
    static std::optional<GeminiCredentials> loadCredentials();
    static std::optional<QString> refreshAccessToken(const GeminiCredentials& creds);
};

class GeminiAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit GeminiAPIStrategy(QObject* parent = nullptr);

    QString id() const override { return "gemini.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;
};
