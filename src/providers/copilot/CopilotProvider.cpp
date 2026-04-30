#include "CopilotProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QUrlQuery>
#include <QThread>
#include <algorithm>

CopilotProvider::CopilotProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> CopilotProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new CopilotOAuthStrategy(const_cast<CopilotProvider*>(this)) };
}

CopilotOAuthStrategy::CopilotOAuthStrategy(QObject* parent) : IFetchStrategy(parent) {}

QAtomicInt CopilotOAuthStrategy::s_deviceFlowInProgress(0);

bool CopilotOAuthStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return ProviderCredentialStore::exists("com.codexbar.oauth.copilot") || ctx.allowInteractiveAuth;
}

bool CopilotOAuthStrategy::shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const {
    return false;
}

std::optional<QString> CopilotOAuthStrategy::performDeviceFlow(bool allowInteractiveAuth) {
    auto cached = ProviderCredentialStore::read("com.codexbar.oauth.copilot");
    if (cached.has_value()) {
        return QString::fromUtf8(cached.value());
    }
    if (!allowInteractiveAuth) {
        return std::nullopt;
    }

    if (s_deviceFlowInProgress.testAndSetAcquire(0, 1) == false) {
        return std::nullopt;
    }

    struct Guard {
        ~Guard() { CopilotOAuthStrategy::s_deviceFlowInProgress.storeRelease(0); }
    } guard;

    QByteArray formData = QUrlQuery().toString(QUrl::FullyEncoded).toUtf8();
    Q_UNUSED(formData)

    QUrlQuery deviceBody;
    deviceBody.addQueryItem("client_id", "Iv1.b507a08c87ecfe98");
    deviceBody.addQueryItem("scope", "read:user");

    QJsonObject deviceResp = NetworkManager::instance().postFormSync(
        QUrl("https://github.com/login/device/code"),
        deviceBody.toString(QUrl::FullyEncoded).toUtf8(),
        { {"Accept", "application/json"} }
    );

    QString deviceCode = deviceResp.value("device_code").toString();
    int interval = deviceResp.value("interval").toInt(5);

    QString accessToken;
    for (int i = 0; i < 60; i++) {
        QThread::msleep((interval + 1) * 1000);

        QUrlQuery tokenBody;
        tokenBody.addQueryItem("client_id", "Iv1.b507a08c87ecfe98");
        tokenBody.addQueryItem("device_code", deviceCode);
        tokenBody.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:device_code");

        QJsonObject tokenResp = NetworkManager::instance().postFormSync(
            QUrl("https://github.com/login/oauth/access_token"),
            tokenBody.toString(QUrl::FullyEncoded).toUtf8(),
            { {"Accept", "application/json"} }
        );

        QString errorType = tokenResp.value("error").toString();
        if (errorType == "authorization_pending") continue;
        if (errorType == "slow_down") { interval += 5; continue; }
        if (!errorType.isEmpty()) break;

        accessToken = tokenResp.value("access_token").toString();
        if (!accessToken.isEmpty()) {
            ProviderCredentialStore::write("com.codexbar.oauth.copilot", "", accessToken.toUtf8());
            return accessToken;
        }
        break;
    }
    return std::nullopt;
}

static UsageSnapshot parseCopilotResponse(const QJsonObject& json) {
    UsageSnapshot snapshot;
    snapshot.updatedAt = QDateTime::currentDateTime();

    QJsonObject quotaSnapshots = json.value("quota_snapshots").toObject();

    auto makeRateFromQuota = [&](const QString& key) -> std::optional<RateWindow> {
        QJsonObject quota = quotaSnapshots.value(key).toObject();
        if (quota.isEmpty()) return std::nullopt;
        double entitlement = quota.value("entitlement").toDouble(0);
        double remaining = quota.value("remaining").toDouble(0);
        double percentRemaining = quota.value("percent_remaining").toDouble(-1);
        if (entitlement == 0 && remaining == 0 && percentRemaining <= 0) return std::nullopt;
        if (percentRemaining < 0 && entitlement > 0) {
            percentRemaining = (remaining / entitlement) * 100.0;
        }
        RateWindow rw;
        rw.usedPercent = std::clamp(100.0 - std::clamp(percentRemaining, 0.0, 100.0), 0.0, 100.0);
        return rw;
    };

    auto premium = makeRateFromQuota("premium_interactions");
    auto chat = makeRateFromQuota("chat");

    if (premium.has_value()) {
        snapshot.primary = premium;
        if (chat.has_value()) snapshot.secondary = chat;
    } else if (chat.has_value()) {
        snapshot.primary = chat;
    }

    QString plan = json.value("copilot_plan").toString();
    if (!plan.isEmpty()) {
        ProviderIdentitySnapshot identity;
        identity.providerID = UsageProvider::copilot;
        identity.loginMethod = plan;
        snapshot.identity = identity;
    }

    return snapshot;
}

ProviderFetchResult CopilotOAuthStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "oauth";

    auto token = performDeviceFlow(ctx.allowInteractiveAuth);
    if (!token.has_value()) {
        result.success = false;
        result.errorMessage = "Copilot OAuth token not found";
        return result;
    }

    QHash<QString, QString> headers;
    headers["Authorization"] = "token " + token.value();
    headers["Accept"] = "application/json";
    headers["Editor-Version"] = "vscode/1.96.2";
    headers["Editor-Plugin-Version"] = "copilot-chat/0.26.7";
    headers["User-Agent"] = "GitHubCopilotChat/0.26.7";
    headers["X-Github-Api-Version"] = "2025-04-01";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://api.github.com/copilot_internal/user"), headers, ctx.networkTimeoutMs);

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "empty copilot response";
        return result;
    }

    result.usage = parseCopilotResponse(json);
    result.success = true;
    return result;
}
