#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/RateWindow.h"
#include "../../models/ProviderIdentitySnapshot.h"

#include <QObject>
#include <QString>
#include <QFuture>
#include <QJsonObject>
#include <QAtomicInt>

class CopilotProvider : public IProvider {
    Q_OBJECT
public:
    explicit CopilotProvider(QObject* parent = nullptr);
    QString id() const override { return "copilot"; }
    QString displayName() const override { return "Copilot"; }
    QString sessionLabel() const override { return "Premium"; }
    QString weeklyLabel() const override { return "Chat"; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QString dashboardURL() const override { return "https://github.com/settings/copilot"; }
    QString statusPageURL() const override { return "https://www.githubstatus.com"; }
    QString brandColor() const override { return "#A855F7"; }
    QVector<QString> supportedSourceModes() const override { return {"auto", "oauth"}; }
};

class CopilotOAuthStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit CopilotOAuthStrategy(QObject* parent = nullptr);
    QString id() const override { return "copilot.oauth"; }
    int kind() const override { return ProviderFetchKind::OAuth; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    std::optional<QString> performDeviceFlow(bool allowInteractiveAuth);
    QString m_cachedToken;
    static QAtomicInt s_deviceFlowInProgress;
};
