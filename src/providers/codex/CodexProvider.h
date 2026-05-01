#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/CodexUsageResponse.h"
#include "CodexAccountReconciliation.h"
#include "CodexReconciledState.h"

#include <QObject>
#include <QString>
#include <QFuture>
#include <QJsonObject>

class CodexProvider : public IProvider {
    Q_OBJECT
public:
    explicit CodexProvider(QObject* parent = nullptr);

    QString id() const override { return "codex"; }
    QString displayName() const override { return "Codex"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    QString opusLabel() const override { return "Credits"; }
    bool supportsCredits() const override { return true; }
    QString cliName() const override { return "codex"; }
    bool defaultEnabled() const override { return true; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QString dashboardURL() const override { return "https://chatgpt.com/codex"; }
    QString statusPageURL() const override { return "https://status.openai.com"; }
    QString brandColor() const override { return "#49A3B0"; }

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"sourceMode", "Data source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Auto"}, {"oauth", "OAuth"}, {"cli", "CLI"}, {"web", "Web Dashboard"} }},
            {"manualCookieHeader", "Manual cookie header", "secret", QVariant()}
        };
    }
    QVector<QString> supportedSourceModes() const override { return {"auto", "oauth", "cli", "web"}; }

};

class CodexOAuthStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit CodexOAuthStrategy(QObject* parent = nullptr);

    QString id() const override { return "codex.oauth"; }
    int kind() const override { return ProviderFetchKind::OAuth; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static QString resolveAccountEmail(const CodexOAuthCredentials& creds);
    static std::optional<CodexOAuthCredentials> attemptTokenRefresh(
        const CodexOAuthCredentials& creds, const QHash<QString, QString>& env);
};

class CodexAppServerStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit CodexAppServerStrategy(QObject* parent = nullptr);

    QString id() const override { return "codex.cli.rpc"; }
    int kind() const override { return ProviderFetchKind::CLI; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult mapRpcResult(const QJsonObject& rateLimitsResult,
                                            const QJsonObject& accountResult = {});
    static ProviderFetchResult mapRpcError(const QJsonObject& errorObject);
};

class CodexCLIPtyStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit CodexCLIPtyStrategy(QObject* parent = nullptr);

    QString id() const override { return "codex.cli.pty"; }
    int kind() const override { return ProviderFetchKind::CLI; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;
};

class CodexWebDashboardStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit CodexWebDashboardStrategy(QObject* parent = nullptr);

    QString id() const override { return "codex.web"; }
    int kind() const override { return ProviderFetchKind::WebDashboard; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static UsageSnapshot parseDashboardHTML(const QString& html);
};
