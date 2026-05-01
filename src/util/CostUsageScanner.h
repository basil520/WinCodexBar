#pragma once

#include <QString>
#include <QDateTime>
#include <QHash>
#include <QVector>
#include <QRegularExpression>
#include <optional>
#include "../models/CostUsageReport.h"

class CostUsageScanner {
public:
    static void setShuttingDown(bool shuttingDown);

    explicit CostUsageScanner();

    CostUsageSnapshot scanClaude(const QString& configDir, const QDate& since, const QDate& until);
    CostUsageSnapshot scanCodex(const QString& sessionsDir, const QDate& since, const QDate& until);

    struct PiScanResult {
        CostUsageSnapshot codex;
        CostUsageSnapshot claude;
    };
    PiScanResult scanPi(const QDate& since, const QDate& until);

    // Scan opencode.db and return per-provider snapshots (provider_id -> snapshot)
    QHash<QString, CostUsageSnapshot> scanOpenCodeDB(const QDate& since, const QDate& until);
    CostUsageSnapshot scanOpenCodeGo(const QDate& since, const QDate& until);

    QString cacheDir();

    struct Pricing {
        double inputPerM = 0;
        double cacheReadPerM = 0;
        double cacheWritePerM = 0;
        double outputPerM = 0;
        std::optional<int> thresholdTokens;
        std::optional<double> inputPerMAboveThreshold;
        std::optional<double> outputPerMAboveThreshold;
        std::optional<double> cacheWritePerMAboveThreshold;
        std::optional<double> cacheReadPerMAboveThreshold;
    };

    static Pricing priceForModel(const QString& modelName);

    static double tieredCost(int tokens, double basePerM, const std::optional<double>& abovePerM, const std::optional<int>& threshold);
    static double costForClaudeModel(const Pricing& p, int inputTokens, int cacheReadTokens, int cacheCreationTokens, int outputTokens);
    static double costForCodexModel(const Pricing& p, int inputTokens, int cacheReadTokens, int outputTokens);

    static QString normalizeCodexModel(const QString& raw);
    static QString normalizeClaudeModel(const QString& raw);

    static const QHash<QString, Pricing>& codexPricingMap();
    static const QHash<QString, Pricing>& claudePricingMap();
    static const QHash<QString, Pricing>& opencodeGoPricingMap();

private:
    static QString stripQuotes(const QString& s);
    static std::optional<QString> extractString(const QByteArray& line, const char* key, int startPos = 0);
    static std::optional<int> extractInt(const QByteArray& line, const char* key, int startPos = 0);

    QHash<QString, Pricing> m_codexPricing;
    QHash<QString, Pricing> m_claudePricing;
    QHash<QString, Pricing> m_opencodeGoPricing;
    void initPricing();
};
