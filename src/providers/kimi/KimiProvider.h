#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../../models/UsageSnapshot.h"
#include <QObject>
#include <optional>

class KimiProvider : public IProvider {
    Q_OBJECT
public:
    explicit KimiProvider(QObject* parent = nullptr);
    QString id() const override { return "kimi"; }
    QString displayName() const override { return "Kimi"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    bool defaultEnabled() const override { return false; }
    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override;
    QVector<QString> supportedSourceModes() const override { return {"web"}; }
    QString statusPageURL() const override { return "https://status.moonshot.cn"; }
    QString dashboardURL() const override { return "https://www.kimi.com/code/console"; }
    QString brandColor() const override { return "#FE603C"; }
};

class KimiWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit KimiWebStrategy(QObject* parent = nullptr);
    QString id() const override { return "kimi.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override { return false; }

private:
    static std::optional<QString> resolveAuthToken(const ProviderFetchContext& ctx);
    static std::optional<QString> extractKimiAuthToken(const QString& raw);
    static QJsonObject decodeJWTPayload(const QString& jwt);
    static UsageSnapshot parseKimiResponse(const QJsonObject& json);
};
