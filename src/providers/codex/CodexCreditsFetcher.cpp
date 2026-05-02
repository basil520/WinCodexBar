#include "CodexCreditsFetcher.h"
#include "CodexOAuthUsageFetcher.h"
#include "CodexStatusProbe.h"
#include "CodexRpcClient.h"
#include "../../models/CodexUsageResponse.h"
#include "../../network/NetworkManager.h"
#include "../../util/BinaryLocator.h"
#include "../../util/TextParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QDebug>

CodexCreditsFetcher::CodexCreditsFetcher(const QHash<QString, QString>& env)
    : m_env(env)
{
}

CodexCreditsFetcher::FetchResult CodexCreditsFetcher::fetchCreditsSync(int timeoutMs)
{
    if (timeoutMs <= 0) timeoutMs = m_timeoutMs;

    // 1. Try OAuth API first (lightweight, no process spawn)
    auto oauthResult = fetchViaOAuthSync(timeoutMs);
    if (oauthResult.success) {
        qDebug() << "[CodexCreditsFetcher] Credits via OAuth:" << oauthResult.credits->remaining;
        return oauthResult;
    }
    if (!oauthResult.errorMessage.isEmpty()) {
        qDebug() << "[CodexCreditsFetcher] OAuth failed:" << oauthResult.errorMessage;
    }

    // 2. Try RPC via codex CLI app-server
    auto rpcResult = fetchViaRPCSync(timeoutMs);
    if (rpcResult.success) {
        qDebug() << "[CodexCreditsFetcher] Credits via RPC:" << rpcResult.credits->remaining;
        return rpcResult;
    }
    if (!rpcResult.errorMessage.isEmpty()) {
        qDebug() << "[CodexCreditsFetcher] RPC failed:" << rpcResult.errorMessage;
    }

    // 3. Fallback to PTY via codex status
    auto ptyResult = fetchViaPTYSync(timeoutMs);
    if (ptyResult.success) {
        qDebug() << "[CodexCreditsFetcher] Credits via PTY:" << ptyResult.credits->remaining;
        return ptyResult;
    }
    if (!ptyResult.errorMessage.isEmpty()) {
        qDebug() << "[CodexCreditsFetcher] PTY failed:" << ptyResult.errorMessage;
    }

    // All failed: return the most specific error
    FetchResult result;
    result.success = false;
    if (!ptyResult.errorMessage.isEmpty()) {
        result.errorMessage = ptyResult.errorMessage;
    } else if (!rpcResult.errorMessage.isEmpty()) {
        result.errorMessage = rpcResult.errorMessage;
    } else {
        result.errorMessage = oauthResult.errorMessage;
    }
    return result;
}

// ============================================================================
// OAuth API path
// ============================================================================

CodexCreditsFetcher::FetchResult CodexCreditsFetcher::fetchViaOAuthSync(int timeoutMs)
{
    FetchResult result;
    auto credsOpt = CodexOAuthCredentials::load(m_env);
    if (!credsOpt.has_value()) {
        result.errorMessage = "No OAuth credentials available";
        return result;
    }

    const auto& creds = *credsOpt;
    auto fetchResult = CodexOAuthUsageFetcher::fetchUsage(
        creds.accessToken, creds.accountId, m_env);

    if (!fetchResult.success || !fetchResult.response.has_value()) {
        result.errorMessage = fetchResult.errorMessage;
        return result;
    }

    const auto& response = *fetchResult.response;
    if (!response.credits.has_value()) {
        result.errorMessage = "No credits data in OAuth response";
        return result;
    }

    double balance = response.credits->balance.value_or(0.0);
    CreditsSnapshot snap;
    snap.remaining = balance;
    snap.updatedAt = QDateTime::currentDateTime();
    result.credits = snap;
    result.success = true;
    return result;
}

// ============================================================================
// RPC path (codex CLI app-server)
// ============================================================================

CodexCreditsFetcher::FetchResult CodexCreditsFetcher::fetchViaRPCSync(int timeoutMs)
{
    FetchResult result;
    QString binary = BinaryLocator::resolve("codex");
    if (binary.isEmpty()) {
        result.errorMessage = "codex CLI not found in PATH";
        return result;
    }

    QStringList args;
    args << "-s" << "read-only" << "-a" << "untrusted" << "app-server" << "--listen" << "stdio://";

    int rpcTimeout = qMax(3000, timeoutMs);

    CodexRpcClient rpc(m_env);
    if (!rpc.start(binary, args, 5000)) {
        result.errorMessage = "Codex CLI RPC failed to start";
        return result;
    }

    auto initResult = rpc.initialize("wincodexbar", "0.1.0", rpcTimeout);
    if (!initResult.success) {
        rpc.shutdown();
        result.errorMessage = "RPC initialize failed: " + initResult.errorMessage;
        return result;
    }

    auto limitsResult = rpc.sendRequest("account/rateLimits/read", QJsonObject{}, rpcTimeout);
    if (!limitsResult.success) {
        rpc.shutdown();
        result.errorMessage = "RPC rateLimits/read failed: " + limitsResult.errorMessage;
        return result;
    }

    if (limitsResult.result.contains("error")) {
        rpc.shutdown();
        QJsonObject errObj = limitsResult.result.value("error").toObject();
        result.errorMessage = "RPC error: " + errObj.value("message").toString();
        return result;
    }

    rpc.shutdown();

    QJsonObject resultObj = limitsResult.result;
    QJsonObject byLimitId = resultObj.value("rateLimitsByLimitId").toObject();
    QJsonObject codex = byLimitId.value("codex").toObject();
    if (codex.isEmpty()) {
        for (auto it = byLimitId.constBegin(); it != byLimitId.constEnd(); ++it) {
            QJsonObject bucket = it.value().toObject();
            if (bucket.value("limitId").toString() == "codex") {
                codex = bucket;
                break;
            }
        }
    }
    if (codex.isEmpty()) {
        codex = resultObj.value("rateLimits").toObject();
    }

    QJsonObject creditsObj = codex.value("credits").toObject();
    if (creditsObj.isEmpty()) {
        result.errorMessage = "No credits in RPC response";
        return result;
    }

    double balance = parseBalanceValue(creditsObj.value("balance"));
    CreditsSnapshot snap;
    snap.remaining = balance;
    snap.updatedAt = QDateTime::currentDateTime();
    result.credits = snap;
    result.success = true;
    return result;
}

// ============================================================================
// PTY path (codex status TTY output)
// ============================================================================

CodexCreditsFetcher::FetchResult CodexCreditsFetcher::fetchViaPTYSync(int timeoutMs)
{
    FetchResult result;

    CodexStatusProbe probe;
    probe.setEnvironment(m_env);
    probe.setTimeout(timeoutMs);

    auto probeResult = probe.fetchWithRetry();
    if (!probeResult.success || !probeResult.snapshot.has_value()) {
        result.errorMessage = probeResult.errorMessage;
        return result;
    }

    const auto& snapshot = *probeResult.snapshot;
    if (!snapshot.credits.has_value()) {
        result.errorMessage = "No credits parsed from CLI status output";
        return result;
    }

    CreditsSnapshot snap;
    snap.remaining = *snapshot.credits;
    snap.updatedAt = QDateTime::currentDateTime();
    result.credits = snap;
    result.success = true;
    return result;
}

// ============================================================================
// Helpers
// ============================================================================

std::optional<double> CodexCreditsFetcher::parseCreditsFromJson(const QJsonObject& json)
{
    QJsonObject credits = json.value("credits").toObject();
    if (credits.isEmpty()) return std::nullopt;
    double balance = parseBalanceValue(credits.value("balance"));
    return balance;
}

double CodexCreditsFetcher::parseBalanceValue(const QJsonValue& value)
{
    if (value.isDouble()) return value.toDouble();
    bool ok = false;
    double parsed = value.toString().toDouble(&ok);
    return ok ? parsed : 0.0;
}
