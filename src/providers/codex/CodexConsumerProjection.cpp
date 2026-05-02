#include "CodexConsumerProjection.h"

CodexConsumerProjection::Result CodexConsumerProjection::make(Surface surface, const Context& context) {
    Result result;

    bool allowsLiveAdjuncts = (surface != Surface::OverrideCard);
    result.dashboardVisibility = resolveDashboardVisibility(surface, context);

    auto windowsByLane = rateWindowsByLane(context.snapshot);
    result.visibleRateLanes = visibleRateLanes(windowsByLane, context.snapshot);
    result.planUtilizationLanes = planUtilizationLanes(windowsByLane);

    if (allowsLiveAdjuncts && context.credits) {
        CreditsProjection proj;
        proj.snapshot = *context.credits;
        proj.userFacingError = CodexUIErrorMapper::userFacingMessage(context.rawCreditsError);
        result.credits = proj;
    }

    result.userFacingErrors.usage = CodexUIErrorMapper::userFacingMessage(context.rawUsageError);
    if (allowsLiveAdjuncts) {
        result.userFacingErrors.credits = CodexUIErrorMapper::userFacingMessage(context.rawCreditsError);
        result.userFacingErrors.dashboard = CodexUIErrorMapper::userFacingMessage(QString());
    }

    if (surface == Surface::LiveCard &&
        result.dashboardVisibility == DashboardVisibility::Attached) {
        result.supplementalMetrics.append(SupplementalMetric::CodeReview);
    }

    result.canShowBuyCredits = (surface == Surface::LiveCard);
    // TODO: hasUsageBreakdown and hasCreditsHistory require the dashboard fetcher
    // (OpenAIDashboardFetcher) to parse usage breakdown and credits history from the
    // Codex web dashboard HTML. Currently the fetcher does not extract these sections.
    result.hasUsageBreakdown = false;
    result.hasCreditsHistory = false;

    std::optional<double> creditsRemaining;
    if (result.credits && result.credits->remaining().has_value()) {
        creditsRemaining = result.credits->remaining();
    }
    result.menuBarFallback = resolveMenuBarFallback(creditsRemaining, windowsByLane);

    return result;
}

std::optional<RateWindow> CodexConsumerProjection::rateWindow(const Result& result, RateLane lane) {
    for (const auto& pl : result.planUtilizationLanes) {
        if (pl.role == lane) {
            return pl.window;
        }
    }
    return std::nullopt;
}

std::optional<double> CodexConsumerProjection::remainingPercent(const Result& result, SupplementalMetric metric) {
    Q_UNUSED(result);
    Q_UNUSED(metric);
    return std::nullopt;
}

bool CodexConsumerProjection::hasExhaustedRateLane(const Result& result) {
    for (const auto& lane : result.visibleRateLanes) {
        auto window = rateWindow(result, lane);
        if (window && window->remainingPercent() <= 0) {
            return true;
        }
    }
    return false;
}

CodexConsumerProjection::DashboardVisibility CodexConsumerProjection::resolveDashboardVisibility(
    Surface surface, const Context& context) {
    if (surface == Surface::OverrideCard) return DashboardVisibility::Hidden;
    if (context.dashboardRequiresLogin) return DashboardVisibility::Hidden;
    return DashboardVisibility::Hidden;
}

QHash<CodexConsumerProjection::RateLane, RateWindow> CodexConsumerProjection::rateWindowsByLane(
    const UsageSnapshot& snapshot) {
    QHash<RateLane, RateWindow> result;

    auto primary = classifyRateWindow(snapshot.primary, true);
    if (primary) {
        result[primary->first] = primary->second;
    }

    auto secondary = classifyRateWindow(snapshot.secondary, false);
    if (secondary) {
        result[secondary->first] = secondary->second;
    }

    return result;
}

QVector<CodexConsumerProjection::RateLane> CodexConsumerProjection::visibleRateLanes(
    const QHash<RateLane, RateWindow>& windows, const UsageSnapshot& snapshot) {
    Q_UNUSED(snapshot);
    QVector<RateLane> result;

    if (windows.contains(RateLane::Session)) {
        result.append(RateLane::Session);
    }
    if (windows.contains(RateLane::Weekly)) {
        result.append(RateLane::Weekly);
    }

    return result;
}

QVector<CodexConsumerProjection::PlanUtilizationLane> CodexConsumerProjection::planUtilizationLanes(
    const QHash<RateLane, RateWindow>& windows) {
    QVector<PlanUtilizationLane> result;

    if (windows.contains(RateLane::Session)) {
        result.append({RateLane::Session, windows[RateLane::Session]});
    }
    if (windows.contains(RateLane::Weekly)) {
        result.append({RateLane::Weekly, windows[RateLane::Weekly]});
    }

    return result;
}

std::optional<std::pair<CodexConsumerProjection::RateLane, RateWindow>> CodexConsumerProjection::classifyRateWindow(
    const std::optional<RateWindow>& window, bool isPrimary) {
    if (!window) return std::nullopt;

    RateLane lane;
    if (window->windowMinutes.has_value()) {
        if (window->windowMinutes.value() == 300) {
            lane = RateLane::Session;
        } else if (window->windowMinutes.value() == 10080) {
            lane = RateLane::Weekly;
        } else {
            lane = isPrimary ? RateLane::Session : RateLane::Weekly;
        }
    } else {
        lane = isPrimary ? RateLane::Session : RateLane::Weekly;
    }

    return std::make_pair(lane, *window);
}

CodexConsumerProjection::MenuBarFallback CodexConsumerProjection::resolveMenuBarFallback(
    const std::optional<double>& creditsRemaining,
    const QHash<RateLane, RateWindow>& windows) {
    if (!creditsRemaining.has_value() || creditsRemaining.value() <= 0) {
        return MenuBarFallback::None;
    }

    bool hasExhaustedLane = false;
    for (auto it = windows.constBegin(); it != windows.constEnd(); ++it) {
        if (it.value().remainingPercent() <= 0) {
            hasExhaustedLane = true;
            break;
        }
    }

    bool hasNoRateWindows = windows.isEmpty();

    return (hasExhaustedLane || hasNoRateWindows) ? MenuBarFallback::CreditsBalance : MenuBarFallback::None;
}
