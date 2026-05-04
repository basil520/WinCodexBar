#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>

class JetBrainsProvider : public IProvider {
    Q_OBJECT
public:
    explicit JetBrainsProvider(QObject* parent = nullptr);

    QString id() const override { return "jetbrains"; }
    QString displayName() const override { return "JetBrains AI"; }
    QString sessionLabel() const override { return "Usage"; }
    QString weeklyLabel() const override { return "Quota"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QString brandColor() const override { return "#F000F0"; }
    QVector<QString> supportedSourceModes() const override { return {"auto"}; }
};

class JetBrainsLocalStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit JetBrainsLocalStrategy(QObject* parent = nullptr);

    QString id() const override { return "jetbrains.local"; }
    int kind() const override { return ProviderFetchKind::LocalProbe; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static ProviderFetchResult parseXml(const QString& xmlContent);
private:
    static QString findQuotaXml();
};
