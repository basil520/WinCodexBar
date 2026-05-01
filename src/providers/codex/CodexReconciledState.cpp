#include "CodexReconciledState.h"
#include "../../models/CodexUsageResponse.h"

#include <QJsonObject>

CodexReconciledState::CodexReconciledState()
    : updatedAt(QDateTime::currentDateTime())
{
}

std::optional<CodexReconciledState> CodexReconciledState::fromCLI(
    const std::optional<RateWindow>& primary,
    const std::optional<RateWindow>& secondary,
    const std::optional<ProviderIdentitySnapshot>& identity,
    const QDateTime& updatedAt)
{
    return make(primary, secondary, identity, updatedAt);
}

std::optional<CodexReconciledState> CodexReconciledState::fromOAuth(
    const CodexUsageResponse& response,
    const CodexOAuthCredentials& credentials,
    const QDateTime& updatedAt)
{
    auto primary = makeWindow(
        response.primaryWindow->usedPercent,
        response.primaryWindow->resetAt,
        response.primaryWindow->limitWindowSeconds);
    auto secondary = makeWindow(
        response.secondaryWindow->usedPercent,
        response.secondaryWindow->resetAt,
        response.secondaryWindow->limitWindowSeconds);

    auto identity = oauthIdentity(response, credentials);
    return make(primary, secondary, identity, updatedAt);
}

UsageSnapshot CodexReconciledState::toUsageSnapshot() const
{
    UsageSnapshot snap;
    snap.primary = session;
    snap.secondary = weekly;
    snap.tertiary = std::nullopt;
    snap.updatedAt = updatedAt;
    snap.identity = identity;
    return snap;
}

ProviderIdentitySnapshot CodexReconciledState::oauthIdentity(
    const CodexUsageResponse& response,
    const CodexOAuthCredentials& credentials)
{
    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::codex;
    identity.accountEmail = resolveAccountEmail(credentials);
    identity.loginMethod = resolvePlan(response, credentials);
    return identity;
}

std::optional<CodexReconciledState> CodexReconciledState::make(
    const std::optional<RateWindow>& primary,
    const std::optional<RateWindow>& secondary,
    const std::optional<ProviderIdentitySnapshot>& identity,
    const QDateTime& updatedAt)
{
    auto normalized = CodexRateWindowNormalizer::normalize(primary, secondary);
    if (!normalized.primary.has_value() && !normalized.secondary.has_value()) {
        return std::nullopt;
    }

    CodexReconciledState state;
    state.session = normalized.primary;
    state.weekly = normalized.secondary;
    state.identity = identity;
    state.updatedAt = updatedAt;
    return state;
}

std::optional<RateWindow> CodexReconciledState::makeWindow(
    int usedPercent, int resetAt, int limitWindowSeconds)
{
    if (usedPercent <= 0 && resetAt <= 0 && limitWindowSeconds <= 0) {
        return std::nullopt;
    }

    RateWindow window;
    window.usedPercent = std::clamp(static_cast<double>(usedPercent), 0.0, 100.0);
    if (limitWindowSeconds > 0) {
        window.windowMinutes = limitWindowSeconds / 60;
    }
    if (resetAt > 0) {
        window.resetsAt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(resetAt));
        qint64 secs = QDateTime::currentDateTime().secsTo(*window.resetsAt);
        if (secs <= 0) {
            window.resetDescription = "now";
        } else {
            int mins = static_cast<int>(secs / 60);
            if (mins < 60) {
                window.resetDescription = QString("%1m").arg(mins);
            } else {
                int hrs = mins / 60;
                int remMin = mins % 60;
                if (hrs < 24) {
                    window.resetDescription = QString("%1h %2m").arg(hrs).arg(remMin);
                } else {
                    int days = hrs / 24;
                    int remHrs = hrs % 24;
                    window.resetDescription = QString("%1d %2h").arg(days).arg(remHrs);
                }
            }
        }
    }
    return window;
}

QString CodexReconciledState::resolveAccountEmail(const CodexOAuthCredentials& credentials)
{
    if (credentials.idToken.isEmpty()) return {};
    QJsonObject payload = parseJWTPayload(credentials.idToken);
    if (payload.isEmpty()) return {};
    QJsonObject profile = payload.value("https://api.openai.com/profile").toObject();
    QString email = payload.value("email").toString().trimmed();
    if (email.isEmpty()) email = profile.value("email").toString().trimmed();
    return email;
}

QString CodexReconciledState::resolvePlan(
    const CodexUsageResponse& response,
    const CodexOAuthCredentials& credentials)
{
    static const char* planNames[] = {
        "guest", "free", "go", "plus", "pro", "free_workspace",
        "team", "business", "education", "quorum", "k12", "enterprise", "edu", "unknown"
    };

    if (response.planType.has_value()) {
        int idx = static_cast<int>(*response.planType);
        if (idx >= 0 && idx < 14) return planNames[idx];
    }

    if (credentials.idToken.isEmpty()) return {};
    QJsonObject payload = parseJWTPayload(credentials.idToken);
    if (payload.isEmpty()) return {};

    QJsonObject auth = payload.value("https://api.openai.com/auth").toObject();
    QString plan = auth.value("chatgpt_plan_type").toString().trimmed();
    if (plan.isEmpty()) plan = payload.value("chatgpt_plan_type").toString().trimmed();
    return plan;
}
