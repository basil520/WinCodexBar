#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMutex>

class AntigravityProvider : public IProvider {
    Q_OBJECT
public:
    explicit AntigravityProvider(QObject* parent = nullptr);

    QString id() const override { return "antigravity"; }
    QString displayName() const override { return "Antigravity"; }
    QString sessionLabel() const override { return "Claude"; }
    QString weeklyLabel() const override { return "Gemini Pro"; }
    QString opusLabel() const override { return "Gemini Flash"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QString brandColor() const override { return "#10B981"; }
    QString statusLinkURL() const override { return "https://www.google.com/appsstatus/dashboard/products/npdyhgECDJ6tB66MxXyo/history"; }
    QString statusWorkspaceProductID() const override { return "npdyhgECDJ6tB66MxXyo"; }
    QVector<QString> supportedSourceModes() const override { return {"auto"}; }
};

class AntigravityLocalStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit AntigravityLocalStrategy(QObject* parent = nullptr);

    QString id() const override { return "antigravity.local"; }
    int kind() const override { return ProviderFetchKind::LocalProbe; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static bool findProcess(int& port, QString& csrfToken);
    static ProviderFetchResult queryStatus(int port, const QString& csrfToken);

    // Cache isAvailable() result to avoid blocking the thread pool with repeated PowerShell calls.
    struct AvailabilityCache {
        bool available = false;
        int port = 0;
        QString csrfToken;
        QDateTime cachedAt;
    };
    static AvailabilityCache s_availCache;
    static QMutex s_availCacheMutex;
};
