#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"
#include "../../models/UsageSnapshot.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

#include <optional>

struct WindsurfDevinSessionAuth {
    QString sessionToken;
    QString auth1Token;
    QString accountID;
    QString primaryOrgID;

    bool isValid() const;
};

struct WindsurfPlanStatus {
    QString planName;
    std::optional<QDateTime> planStart;
    std::optional<QDateTime> planEnd;
    std::optional<int> dailyQuotaRemainingPercent;
    std::optional<int> weeklyQuotaRemainingPercent;
    std::optional<qint64> dailyQuotaResetAtUnix;
    std::optional<qint64> weeklyQuotaResetAtUnix;
};

struct WindsurfCachedPlanInfo {
    QString planName;
    std::optional<qint64> startTimestamp;
    std::optional<qint64> endTimestamp;

    struct Usage {
        std::optional<int> messages;
        std::optional<int> usedMessages;
        std::optional<int> remainingMessages;
        std::optional<int> flowActions;
        std::optional<int> usedFlowActions;
        std::optional<int> remainingFlowActions;
        std::optional<int> flexCredits;
        std::optional<int> usedFlexCredits;
        std::optional<int> remainingFlexCredits;
    };

    struct QuotaUsage {
        std::optional<double> dailyRemainingPercent;
        std::optional<double> weeklyRemainingPercent;
        std::optional<qint64> dailyResetAtUnix;
        std::optional<qint64> weeklyResetAtUnix;
    };

    std::optional<Usage> usage;
    std::optional<QuotaUsage> quotaUsage;

    static std::optional<WindsurfCachedPlanInfo> fromJson(const QString& json, QString* errorMessage = nullptr);
    UsageSnapshot toUsageSnapshot() const;
};

class WindsurfPlanStatusProtoCodec {
public:
    struct Request {
        QString authToken;
        bool includeTopUpStatus = false;
    };

    static QByteArray encodeRequest(const QString& authToken, bool includeTopUpStatus);
    static std::optional<Request> decodeRequest(const QByteArray& data, QString* errorMessage = nullptr);
    static std::optional<WindsurfPlanStatus> decodeResponse(const QByteArray& data, QString* errorMessage = nullptr);
};

class WindsurfProvider : public IProvider {
    Q_OBJECT
public:
    explicit WindsurfProvider(QObject* parent = nullptr);

    QString id() const override { return "windsurf"; }
    QString displayName() const override { return "Windsurf"; }
    QString sessionLabel() const override { return "Daily"; }
    QString weeklyLabel() const override { return "Weekly"; }
    QString cliName() const override { return "windsurf"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;
    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override;

    QString brandColor() const override { return "#34E8BB"; }
    QString dashboardURL() const override { return "https://windsurf.com/subscription/usage"; }
    QVector<QString> supportedSourceModes() const override { return {"auto", "web", "cli"}; }
};

class WindsurfWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit WindsurfWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "windsurf.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static std::optional<WindsurfDevinSessionAuth> parseManualSessionInput(const QString& raw,
                                                                           QString* errorMessage = nullptr);
    static QHash<QString, QString> buildHeaders(const WindsurfDevinSessionAuth& auth);
    static UsageSnapshot planStatusToUsageSnapshot(const WindsurfPlanStatus& status);

private:
    static QString resolveManualSessionInput(const ProviderFetchContext& ctx);
    static QUrl endpointURL(const ProviderFetchContext& ctx);
};

class WindsurfLocalStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit WindsurfLocalStrategy(QObject* parent = nullptr);

    QString id() const override { return "windsurf.local"; }
    // Windsurf's upstream source mode names the local SQLite cache "cli".
    int kind() const override { return ProviderFetchKind::CLI; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

    static QStringList candidateStateDBPaths(const ProviderFetchContext& ctx);
    static ProviderFetchResult parseCachedPlanInfoJson(const QString& json);
    static ProviderFetchResult readDatabase(const QString& dbPath);

private:
    static QString decodeSQLiteValue(const QVariant& value);
};
