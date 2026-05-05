#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QUrl>
#include <optional>

struct CodebuffUsagePayload {
    std::optional<double> used;
    std::optional<double> total;
    std::optional<double> remaining;
    std::optional<QDateTime> nextQuotaReset;
    std::optional<bool> autoTopUpEnabled;
};

struct CodebuffSubscriptionPayload {
    std::optional<QString> status;
    std::optional<QString> tier;
    std::optional<QDateTime> billingPeriodEnd;
    std::optional<double> weeklyUsed;
    std::optional<double> weeklyLimit;
    std::optional<QDateTime> weeklyResetsAt;
    std::optional<QString> email;
};

class CodebuffProvider : public IProvider {
    Q_OBJECT
public:
    explicit CodebuffProvider(QObject* parent = nullptr);

    QString id() const override { return "codebuff"; }
    QString displayName() const override { return "Codebuff"; }
    QString sessionLabel() const override { return "Credits"; }
    QString weeklyLabel() const override { return "Weekly"; }
    bool supportsCredits() const override { return true; }
    bool defaultEnabled() const override { return false; }
    QString cliName() const override { return "codebuff"; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"apiKey", "API key", "secret", QVariant(),
             {}, "com.codexbar.apikey.codebuff", "CODEBUFF_API_KEY",
             "cb-...", "Stored in Windows Credential Manager; or run codebuff login.", false, true}
        };
    }

    QString brandColor() const override { return "#44FF00"; }
    QString dashboardURL() const override { return "https://www.codebuff.com/usage"; }
    QVector<QString> supportedSourceModes() const override { return {"api"}; }
    bool supportsMultipleAccounts() const override { return true; }
    QVector<QString> requiredCredentialTypes() const override { return {"apiKey"}; }
};

class CodebuffAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit CodebuffAPIStrategy(QObject* parent = nullptr);

    QString id() const override { return "codebuff.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static CodebuffUsagePayload parseUsagePayloadForTesting(const QJsonObject& json);
    static CodebuffSubscriptionPayload parseSubscriptionPayloadForTesting(const QJsonObject& json);
    static UsageSnapshot makeSnapshotForTesting(const CodebuffUsagePayload& usage,
                                                const std::optional<CodebuffSubscriptionPayload>& subscription);
    static QString authFileTokenForTesting(const QString& homePath);

private:
    enum class TokenSource {
        Unknown,
        Environment,
        Settings,
        CredentialStore,
        TokenAccount,
        AuthFile,
    };

    struct TokenResolution {
        QString token;
        TokenSource source = TokenSource::Unknown;
    };

    static std::optional<TokenResolution> resolveToken(const ProviderFetchContext& ctx);
    static QString authFileToken(const QString& homePath);
    static QUrl apiBaseUrl(const ProviderFetchContext& ctx);
    static ProviderFetchResult errorResult(const QString& message);
};
