#include "CursorUsageSummary.h"
#include "UsageSnapshot.h"

#include <QJsonArray>
#include <cmath>

CursorUsageSummary parseCursorUsageSummary(const QJsonObject& json) {
    CursorUsageSummary summary;
    QJsonArray items = json.value("items").toArray();
    for (const auto& itemVal : items) {
        QJsonObject item = itemVal.toObject();
        CursorIndividualUsage ciu;
        ciu.limitPeriod = item.value("limitPeriod").toString();

        QJsonObject plan = item.value("plan").toObject();
        if (!plan.isEmpty()) {
            CursorPlanUsage pu;
            if (plan.contains("percentUsed")) pu.percentUsed = plan.value("percentUsed").toDouble();
            if (plan.contains("usageCents")) pu.usageCents = plan.value("usageCents").toInt();
            if (plan.contains("limitCents")) pu.limitCents = plan.value("limitCents").toInt();

            QJsonObject bd = plan.value("breakdown").toObject();
            if (!bd.isEmpty()) {
                CursorPlanBreakdown breakdown;
                if (bd.contains("includedCents")) breakdown.includedCents = bd.value("includedCents").toInt();
                if (bd.contains("bonusCents")) breakdown.bonusCents = bd.value("bonusCents").toInt();
                if (bd.contains("totalCents")) breakdown.totalCents = bd.value("totalCents").toInt();
                pu.breakdown = breakdown;
            }
            ciu.plan = pu;
        }

        QJsonObject onDemand = item.value("onDemand").toObject();
        if (!onDemand.isEmpty()) {
            CursorOnDemandUsage odu;
            if (onDemand.contains("usedCents")) odu.usedCents = onDemand.value("usedCents").toInt();
            if (onDemand.contains("limitCents")) odu.limitCents = onDemand.value("limitCents").toInt();
            if (onDemand.contains("percentUsed")) odu.percentUsed = onDemand.value("percentUsed").toDouble();
            ciu.onDemand = odu;
        }

        QJsonObject team = item.value("team").toObject();
        if (!team.isEmpty()) {
            CursorTeamUsage tu;
            if (team.contains("usedCents")) tu.usedCents = team.value("usedCents").toInt();
            if (team.contains("limitCents")) tu.limitCents = team.value("limitCents").toInt();
            ciu.team = tu;
        }

        summary.items.append(ciu);
    }
    return summary;
}

CursorUserInfo parseCursorUserInfo(const QJsonObject& json) {
    CursorUserInfo info;
    info.email = json.value("email").toString().trimmed();
    info.name = json.value("name").toString().trimmed();
    info.sub = json.value("sub").toString().trimmed();
    return info;
}

CursorSubscriptionInfo parseCursorSubscription(const QJsonObject& json) {
    CursorSubscriptionInfo info;
    info.plan = json.value("plan").toString().trimmed();
    if (info.plan.isEmpty()) info.plan = json.value("stripePlan").toString().trimmed();
    info.status = json.value("status").toString().trimmed();
    return info;
}

CursorUsageSnapshot buildCursorSnapshot(
    const CursorUsageSummary& summary,
    const CursorUserInfo& userInfo,
    const CursorSubscriptionInfo& subscription)
{
    CursorUsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    if (!userInfo.email.isEmpty()) snap.accountEmail = userInfo.email;
    if (!subscription.plan.isEmpty()) snap.membershipType = subscription.plan;

    if (summary.items.isEmpty()) return snap;

    const auto& first = summary.items[0];

    if (first.plan.has_value()) {
        if (first.plan->percentUsed.has_value()) {
            snap.planPercentUsed = *first.plan->percentUsed;
        } else if (first.plan->usageCents.has_value() && first.plan->limitCents.has_value()
                   && *first.plan->limitCents > 0) {
            snap.planPercentUsed = (static_cast<double>(*first.plan->usageCents) /
                                    static_cast<double>(*first.plan->limitCents)) * 100.0;
        }

        if (first.plan->usageCents.has_value()) {
            snap.planUsedUSD = static_cast<double>(*first.plan->usageCents) / 100.0;
        }
        if (first.plan->limitCents.has_value()) {
            snap.planLimitUSD = static_cast<double>(*first.plan->limitCents) / 100.0;
        }
    }

    if (first.onDemand.has_value()) {
        if (first.onDemand->usedCents.has_value()) {
            snap.onDemandUsedUSD = static_cast<double>(*first.onDemand->usedCents) / 100.0;
        }
        if (first.onDemand->limitCents.has_value()) {
            snap.onDemandLimitUSD = static_cast<double>(*first.onDemand->limitCents) / 100.0;
        }
    }

    if (summary.items.size() >= 2) {
        const auto& second = summary.items[1];
        if (second.plan.has_value() && second.plan->percentUsed.has_value()) {
            snap.apiPercentUsed = *second.plan->percentUsed;
        }
    }

    if (!first.limitPeriod.isEmpty()) {
        snap.billingCycle = first.limitPeriod;
    }

    return snap;
}

UsageSnapshot CursorUsageSnapshot::toUsageSnapshot() const {
    UsageSnapshot snap;

    if (planPercentUsed.has_value()) {
        RateWindow rw;
        rw.usedPercent = *planPercentUsed;
        snap.primary = rw;
    }

    if (apiPercentUsed.has_value()) {
        RateWindow rw;
        rw.usedPercent = *apiPercentUsed;
        snap.tertiary = rw;
    }

    if (onDemandUsedUSD.has_value()) {
        ProviderCostSnapshot cost;
        cost.used = *onDemandUsedUSD;
        cost.limit = onDemandLimitUSD.value_or(0.0);
        cost.currencyCode = "USD";
        cost.period = billingCycle.value_or("Monthly");
        cost.updatedAt = updatedAt;
        snap.providerCost = cost;
    }

    if (accountEmail.has_value() || membershipType.has_value()) {
        ProviderIdentitySnapshot identity;
        identity.providerID = UsageProvider::cursor;
        identity.accountEmail = accountEmail;
        identity.loginMethod = membershipType;
        snap.identity = identity;
    }

    snap.updatedAt = updatedAt.isValid() ? updatedAt : QDateTime::currentDateTime();
    return snap;
}
