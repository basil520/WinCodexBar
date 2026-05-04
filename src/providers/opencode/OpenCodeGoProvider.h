#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include <QObject>
#include <optional>

class OpenCodeGoProvider : public IProvider {
    Q_OBJECT
public:
    explicit OpenCodeGoProvider(QObject* parent = nullptr);
    QString id() const override { return "opencodego"; }
    QString displayName() const override { return "OpenCode Go"; }
    QString sessionLabel() const override { return "5-hour"; }
    QString weeklyLabel() const override { return "Weekly"; }
    QString opusLabel() const override { return "Monthly"; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override;
    QVector<QString> supportedSourceModes() const override { return {"web"}; }
    QString statusPageURL() const override { return "https://status.opencode.ai"; }
    QString dashboardURL() const override { return "https://opencode.ai"; }
    QString brandColor() const override { return "#3B82F6"; }
};

class OpenCodeGoWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit OpenCodeGoWebStrategy(QObject* parent = nullptr);
    QString id() const override { return "opencodego.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override { return false; }

    static UsageSnapshot parseUsage(const QString& text);
    static std::optional<RateWindow> parseWindow(const QJsonObject& obj);
    static double normalizePercent(double value);

private:
    static QString fetchUsagePage(const QString& workspaceID, const QString& cookieHeader, int timeoutMs);
};
