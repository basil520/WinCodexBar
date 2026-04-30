#pragma once

#include "RateWindow.h"
#include "ProviderIdentitySnapshot.h"
#include "ProviderCostSnapshot.h"
#include "CursorRequestUsage.h"

#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <optional>

struct UsageSnapshot;

struct CursorPlanBreakdown {
    std::optional<int> includedCents;
    std::optional<int> bonusCents;
    std::optional<int> totalCents;
};

struct CursorPlanUsage {
    std::optional<double> percentUsed;
    std::optional<int> usageCents;
    std::optional<int> limitCents;
    std::optional<CursorPlanBreakdown> breakdown;
};

struct CursorOnDemandUsage {
    std::optional<int> usedCents;
    std::optional<int> limitCents;
    std::optional<double> percentUsed;
};

struct CursorTeamUsage {
    std::optional<int> usedCents;
    std::optional<int> limitCents;
};

struct CursorIndividualUsage {
    std::optional<CursorPlanUsage> plan;
    std::optional<CursorOnDemandUsage> onDemand;
    std::optional<CursorTeamUsage> team;
    QString limitPeriod;
};

struct CursorUsageSummary {
    QVector<CursorIndividualUsage> items;
};

struct CursorUserInfo {
    QString email;
    QString name;
    QString sub;
};

struct CursorSubscriptionInfo {
    QString plan;
    QString status;
};

struct CursorUsageSnapshot {
    std::optional<double> planPercentUsed;
    std::optional<double> autoPercentUsed;
    std::optional<double> apiPercentUsed;
    std::optional<double> planUsedUSD;
    std::optional<double> planLimitUSD;
    std::optional<double> onDemandUsedUSD;
    std::optional<double> onDemandLimitUSD;
    std::optional<CursorRequestUsage> requestUsage;
    std::optional<QString> accountEmail;
    std::optional<QString> membershipType;
    std::optional<QString> billingCycle;
    QDateTime updatedAt;

    UsageSnapshot toUsageSnapshot() const;
};

CursorUsageSummary parseCursorUsageSummary(const QJsonObject& json);
CursorUserInfo parseCursorUserInfo(const QJsonObject& json);
CursorSubscriptionInfo parseCursorSubscription(const QJsonObject& json);
CursorUsageSnapshot buildCursorSnapshot(
    const CursorUsageSummary& summary,
    const CursorUserInfo& userInfo,
    const CursorSubscriptionInfo& subscription);
