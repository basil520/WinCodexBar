#pragma once

#include "../../models/UsageSnapshot.h"
#include "../../models/CreditsSnapshot.h"
#include "CodexUIErrorMapper.h"

#include <QString>
#include <QDateTime>
#include <optional>
#include <QHash>

class CodexConsumerProjection {
public:
    enum class Surface {
        LiveCard,
        OverrideCard,
        MenuBar
    };

    enum class RateLane {
        Session,
        Weekly
    };

    enum class SupplementalMetric {
        CodeReview
    };

    enum class DashboardVisibility {
        Hidden,
        DisplayOnly,
        Attached
    };

    enum class MenuBarFallback {
        None,
        CreditsBalance
    };

    struct PlanUtilizationLane {
        RateLane role;
        RateWindow window;
    };

    struct CreditsProjection {
        std::optional<CreditsSnapshot> snapshot;
        QString userFacingError;

        std::optional<double> remaining() const {
            return snapshot ? std::make_optional(snapshot->remaining) : std::nullopt;
        }
    };

    struct UserFacingErrors {
        QString usage;
        QString credits;
        QString dashboard;
    };

    struct Context {
        UsageSnapshot snapshot;
        QString rawUsageError;
        const CreditsSnapshot* credits = nullptr;
        QString rawCreditsError;
        bool dashboardRequiresLogin = false;
        QDateTime now;
    };

    struct Result {
        QVector<RateLane> visibleRateLanes;
        QVector<SupplementalMetric> supplementalMetrics;
        QVector<PlanUtilizationLane> planUtilizationLanes;
        DashboardVisibility dashboardVisibility = DashboardVisibility::Hidden;
        std::optional<CreditsProjection> credits;
        MenuBarFallback menuBarFallback = MenuBarFallback::None;
        UserFacingErrors userFacingErrors;
        bool canShowBuyCredits = false;
        bool hasUsageBreakdown = false;
        bool hasCreditsHistory = false;
    };

    static Result make(Surface surface, const Context& context);

    static std::optional<RateWindow> rateWindow(const Result& result, RateLane lane);
    static std::optional<double> remainingPercent(const Result& result, SupplementalMetric metric);
    static bool hasExhaustedRateLane(const Result& result);

private:
    static DashboardVisibility resolveDashboardVisibility(Surface surface, const Context& context);
    static QHash<RateLane, RateWindow> rateWindowsByLane(const UsageSnapshot& snapshot);
    static QVector<RateLane> visibleRateLanes(const QHash<RateLane, RateWindow>& windows, const UsageSnapshot& snapshot);
    static QVector<PlanUtilizationLane> planUtilizationLanes(const QHash<RateLane, RateWindow>& windows);
    static std::optional<std::pair<RateLane, RateWindow>> classifyRateWindow(const std::optional<RateWindow>& window, bool isPrimary);
    static MenuBarFallback resolveMenuBarFallback(const std::optional<double>& creditsRemaining, const QHash<RateLane, RateWindow>& windows);
};
