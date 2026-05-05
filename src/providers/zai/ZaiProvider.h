#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/ZaiUsageSnapshot.h"

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <optional>

class ZaiProvider : public IProvider {
    Q_OBJECT
public:
    explicit ZaiProvider(QObject* parent = nullptr);

    QString id() const override { return "zai"; }
    QString displayName() const override { return "z.ai"; }
    QString sessionLabel() const override { return "Tokens"; }
    QString weeklyLabel() const override { return "MCP"; }
    QString opusLabel() const override { return "5-hour"; }
    bool supportsCredits() const override { return false; }
    QString cliName() const override { return "zai"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QString dashboardURL() const override { return "https://z.ai/manage-apikey/subscription"; }
    QString brandColor() const override { return "#E85A6A"; }
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"apiKey", "API key", "secret", QVariant(),
             {}, "com.codexbar.apikey.zai", "Z_AI_API_KEY", "sk-...", "Stored in Windows Credential Manager", false, true},
            {"apiRegion", "API region", "picker", QVariant(QStringLiteral("global")),
             { {"global", "Global"}, {"bigmodelCN", "BigModel CN"} }}
        };
    }
    QVector<QString> supportedSourceModes() const override { return {"api"}; }
    bool supportsMultipleAccounts() const override { return true; }
    QVector<QString> requiredCredentialTypes() const override { return {"apiKey"}; }
};

class ZaiAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit ZaiAPIStrategy(QObject* parent = nullptr);

    QString id() const override { return "zai.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;
};
