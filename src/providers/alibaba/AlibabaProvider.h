#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/RateWindow.h"
#include "../../models/ProviderIdentitySnapshot.h"
#include <QObject>

class AlibabaProvider : public IProvider {
    Q_OBJECT
public:
    explicit AlibabaProvider(QObject* parent = nullptr);

    QString id() const override { return "alibaba"; }
    QString displayName() const override { return "Alibaba"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QString dashboardURL() const override { return "https://www.alibabacloud.com"; }
    QString statusPageURL() const override { return "https://status.alibabacloud.com"; }
    QString statusLinkURL() const override { return "https://status.aliyun.com"; }
    QString brandColor() const override { return "#F97316"; }

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override;
    QVector<QString> supportedSourceModes() const override { return {"auto", "web", "api"}; }
    bool supportsMultipleAccounts() const override { return true; }
    QVector<QString> requiredCredentialTypes() const override { return {"apiKey"}; }
};

// Web Cookie strategy
class AlibabaWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit AlibabaWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "alibaba.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static QString buildCookieHeader();
};

// API Key strategy
class AlibabaAPIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit AlibabaAPIStrategy(QObject* parent = nullptr);

    QString id() const override { return "alibaba.api"; }
    int kind() const override { return ProviderFetchKind::APIToken; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static QString resolveAPIKey(const ProviderFetchContext& ctx);
    static QString resolveRegion(const ProviderFetchContext& ctx);
};
