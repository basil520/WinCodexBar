#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include <QObject>
#include <optional>

class OpenCodeProvider : public IProvider {
    Q_OBJECT
public:
    explicit OpenCodeProvider(QObject* parent = nullptr);
    QString id() const override { return "opencode"; }
    QString displayName() const override { return "OpenCode"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override;
    QVector<QString> supportedSourceModes() const override { return {"web"}; }
    QString statusPageURL() const override { return "https://status.opencode.ai"; }
    QString dashboardURL() const override { return "https://opencode.ai"; }
    QString brandColor() const override { return "#3B82F6"; }
};

class OpenCodeWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit OpenCodeWebStrategy(QObject* parent = nullptr);
    QString id() const override { return "opencode.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override { return false; }

private:
    static QString fetchSubscription(const QString& workspaceID, const QString& cookieHeader, int timeoutMs);
    static UsageSnapshot parseSubscription(const QString& text);
};
