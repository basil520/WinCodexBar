#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/RateWindow.h"
#include "../../models/ProviderIdentitySnapshot.h"
#include <QObject>

class KiroProvider : public IProvider {
    Q_OBJECT
public:
    explicit KiroProvider(QObject* parent = nullptr);
    QString id() const override { return "kiro"; }
    QString displayName() const override { return "Kiro"; }
    QString sessionLabel() const override { return "Credits"; }
    QString weeklyLabel() const override { return "Bonus"; }
    QString brandColor() const override { return "#FF9900"; }
    QString statusLinkURL() const override { return "https://health.aws.amazon.com/health/status"; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QVector<QString> supportedSourceModes() const override { return {"cli"}; }
};

class KiroCLIStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit KiroCLIStrategy(QObject* parent = nullptr);
    QString id() const override { return "kiro.cli"; }
    int kind() const override { return ProviderFetchKind::CLI; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override { return false; }
};
