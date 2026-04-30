#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include <QObject>
#include <optional>

/**
 * DeepSeek Provider - API type (pure token billing)
 * 
 * No monthly limit, purely token-based billing.
 * Token usage data comes from OpenCode DB scan.
 * This provider is display-only (no API fetching for now).
 */
class DeepSeekProvider : public IProvider {
    Q_OBJECT
public:
    explicit DeepSeekProvider(QObject* parent = nullptr);
    QString id() const override { return "deepseek"; }
    QString displayName() const override { return "DeepSeek"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    QString opusLabel() const override { return QString(); }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override;
    QVector<QString> supportedSourceModes() const override { return {"local"}; }
    QString statusPageURL() const override { return QString(); }
    QString dashboardURL() const override { return "https://platform.deepseek.com"; }
    QString brandColor() const override { return "#1E3A8A"; }
    bool supportsCredits() const override { return false; }
};

/**
 * DeepSeek Token Strategy
 * 
 * Returns empty result since DeepSeek has no API to fetch limits.
 * Token usage is populated by UsageStore from OpenCode DB scan.
 */
class DeepSeekTokenStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit DeepSeekTokenStrategy(QObject* parent = nullptr);
    QString id() const override { return "deepseek.local"; }
    int kind() const override { return ProviderFetchKind::LocalProbe; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override { return false; }
};
