#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/ClaudeUsageSnapshot.h"

#include <QObject>
#include <QString>
#include <QFuture>
#include <QNetworkCookie>

class ClaudeProvider : public IProvider {
    Q_OBJECT
public:
    explicit ClaudeProvider(QObject* parent = nullptr);

    QString id() const override { return "claude"; }
    QString displayName() const override { return "Claude"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    QString opusLabel() const override { return "Opus"; }
    bool supportsCredits() const override { return true; }
    QString cliName() const override { return "claude"; }
    bool defaultEnabled() const override { return true; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QString dashboardURL() const override { return "https://claude.ai"; }
    QString statusPageURL() const override { return "https://status.anthropic.com"; }
    QString brandColor() const override { return "#CC7C5E"; }

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"sourceMode", "Data source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Auto"}, {"oauth", "OAuth"}, {"web", "Web"} }},
            {"manualCookieHeader", "Manual cookie header", "secret", QVariant(),
             {}, "com.codexbar.cookie.claude", {}, "sessionKey=...", "Stored in Windows Credential Manager", true, true}
        };
    }
    QVector<QString> supportedSourceModes() const override { return {"auto", "oauth", "web"}; }
};

class ClaudeOAuthStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit ClaudeOAuthStrategy(QObject* parent = nullptr);

    QString id() const override { return "claude.oauth"; }
    int kind() const override { return ProviderFetchKind::OAuth; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;
};

class ClaudeWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit ClaudeWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "claude.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static std::optional<QString> extractSessionKey(const QVector<QNetworkCookie>& cookies);
    static QString fetchOrgId(const QString& sessionKey, int timeoutMs);
    static ClaudeUsageSnapshot fetchUsageData(const QString& orgId, const QString& sessionKey, int timeoutMs);
    static std::optional<ProviderCostSnapshot> fetchOverageCost(const QString& orgId, const QString& sessionKey, int timeoutMs);
    static std::optional<QString> fetchAccountEmail(const QString& sessionKey, int timeoutMs);
};
