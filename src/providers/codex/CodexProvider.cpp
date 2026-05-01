#include "CodexProvider.h"
#include "CodexHomeScope.h"
#include "CodexDashboardAuthorityContext.h"
#include "../../network/NetworkManager.h"
#include "../../util/BinaryLocator.h"
#include "../../util/TextParser.h"
#include "../../models/CodexUsageResponse.h"
#include "../shared/ConPTYSession.h"
#include "../shared/CookieImporter.h"
#include "../shared/BrowserDetection.h"

#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QThread>
#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QJsonValue>
#include <QVariant>

CodexProvider::CodexProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> CodexProvider::createStrategies(const ProviderFetchContext& ctx) {
    QVector<IFetchStrategy*> strategies;
    
    QString mode = ctx.settings.get("sourceMode", "auto").toString();
    
    if (mode == "auto" || mode == "oauth") {
        strategies.append(new CodexOAuthStrategy());
    }
    if (mode == "auto" || mode == "cli") {
        strategies.append(new CodexAppServerStrategy());
        strategies.append(new CodexCLIPtyStrategy());
    }
    if (mode == "auto" || mode == "web") {
        strategies.append(new CodexWebDashboardStrategy());
    }
    
    return strategies;
}

CodexOAuthStrategy::CodexOAuthStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool CodexOAuthStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return CodexOAuthCredentials::load(ctx.env).has_value();
}

bool CodexOAuthStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

QString CodexOAuthStrategy::resolveAccountEmail(const CodexOAuthCredentials& creds) {
    if (creds.idToken.isEmpty()) return {};
    QJsonObject payload = parseJWTPayload(creds.idToken);
    if (payload.isEmpty()) return {};
    QJsonObject profile = payload.value("https://api.openai.com/profile").toObject();
    QString email = payload.value("email").toString().trimmed();
    if (email.isEmpty()) email = profile.value("email").toString().trimmed();
    return email;
}

std::optional<CodexOAuthCredentials> CodexOAuthStrategy::attemptTokenRefresh(
    const CodexOAuthCredentials& creds, const QHash<QString, QString>& env)
{
    Q_UNUSED(env)
    if (creds.refreshToken.isEmpty()) return std::nullopt;

    QJsonObject body;
    body["client_id"] = "app_EMoamEEZ73f0CkXaXp7hrann";
    body["grant_type"] = "refresh_token";
    body["refresh_token"] = creds.refreshToken;
    body["scope"] = "openid profile email";

    QHash<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Accept"] = "application/json";

    QJsonObject resp = NetworkManager::instance().postJsonSync(
        QUrl("https://auth.openai.com/oauth/token"), body, headers);

    if (resp.isEmpty()) return std::nullopt;

    QString newAccessToken = resp.value("access_token").toString();
    QString newRefreshToken = resp.value("refresh_token").toString();
    QString newIdToken = resp.value("id_token").toString();

    if (newAccessToken.isEmpty()) return std::nullopt;

    CodexOAuthCredentials refreshed;
    refreshed.accessToken = newAccessToken;
    refreshed.refreshToken = newRefreshToken.isEmpty() ? creds.refreshToken : newRefreshToken;
    refreshed.idToken = newIdToken.isEmpty() ? creds.idToken : newIdToken;
    refreshed.accountId = creds.accountId;
    refreshed.lastRefresh = QDateTime::currentDateTime();

    refreshed.save(env);

    return refreshed;
}

ProviderFetchResult CodexOAuthStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "oauth";

    qDebug() << "[CodexOAuth] Starting OAuth fetch...";
    auto credsOpt = CodexOAuthCredentials::load(ctx.env);
    if (!credsOpt.has_value()) {
        result.success = false;
        result.errorMessage = "Codex auth.json not found. Run `codex` to log in.";
        qDebug() << "[CodexOAuth] auth.json not found";
        return result;
    }

    CodexOAuthCredentials creds = *credsOpt;
    qDebug() << "[CodexOAuth] Loaded credentials, accountId:" << creds.accountId;

    if (creds.needsRefresh()) {
        auto refreshed = attemptTokenRefresh(creds, ctx.env);
        if (refreshed.has_value()) {
            creds = *refreshed;
        }
    }

    QString baseURL = "https://chatgpt.com/backend-api";
    QString usagePath = "/wham/usage";

    QString codexHome = ctx.env.value("CODEX_HOME").trimmed();
    QString home = ctx.env.value("USERPROFILE", ctx.env.value("HOME", QDir::homePath()));
    QString root = codexHome.isEmpty() ? home + "/.codex" : codexHome;
    QString configPath = root + "/config.toml";

    QFile configFile(configPath);
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray content = configFile.readAll();
        for (const auto& rawLine : content.split('\n')) {
            QByteArray line = rawLine.split('#')[0].trimmed();
            if (line.isEmpty()) continue;
            int eqIdx = line.indexOf('=');
            if (eqIdx < 0) continue;
            QString key = line.left(eqIdx).trimmed();
            QString value = line.mid(eqIdx + 1).trimmed();
            if (key != "chatgpt_base_url") continue;
            if (value.startsWith('"') && value.endsWith('"'))
                value = value.mid(1, value.length() - 2);
            else if (value.startsWith('\'') && value.endsWith('\''))
                value = value.mid(1, value.length() - 2);
            value = value.trimmed();
            if (!value.isEmpty()) {
                while (value.endsWith('/')) value.chop(1);
                if (value.startsWith("https://chatgpt.com") ||
                    value.startsWith("https://chat.openai.com")) {
                    if (!value.contains("/backend-api")) value += "/backend-api";
                }
                baseURL = value;
                usagePath = baseURL.contains("/backend-api") ? "/wham/usage" : "/api/codex/usage";
            }
            break;
        }
    }

    QString fullURL = baseURL + usagePath;
    qDebug() << "[CodexOAuth] Fetching URL:" << fullURL;

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + creds.accessToken;
    headers["Accept"] = "application/json";
    headers["User-Agent"] = "CodexBar";

    if (!creds.accountId.isEmpty()) {
        headers["ChatGPT-Account-Id"] = creds.accountId;
    }

    QJsonObject json = NetworkManager::instance().getJsonSync(QUrl(fullURL), headers, ctx.networkTimeoutMs);
    qDebug() << "[CodexOAuth] Response received, isEmpty:" << json.isEmpty();
    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "empty or invalid response from Codex API";
        return result;
    }

    if (json.contains("error")) {
        QString errorMsg = json.value("error").toObject().value("message").toString();
        if (errorMsg.isEmpty()) errorMsg = json.value("detail").toString();
        if (errorMsg.isEmpty()) errorMsg = "API error";
        result.success = false;
        result.errorMessage = errorMsg;
        qDebug() << "[CodexOAuth] API error:" << errorMsg;
        return result;
    }

    CodexUsageResponse usageResp = CodexUsageResponse::fromJson(json);
    qDebug() << "[CodexOAuth] Parsed response, primaryWindow:" << usageResp.primaryWindow.has_value()
             << "secondaryWindow:" << usageResp.secondaryWindow.has_value();
    if (!usageResp.primaryWindow.has_value() && !usageResp.secondaryWindow.has_value()) {
        result.success = false;
        result.errorMessage = "no rate limit data in response";
        return result;
    }

    QString email = resolveAccountEmail(creds);
    result.usage = usageResp.toUsageSnapshot(email);
    result.success = true;
    qDebug() << "[CodexOAuth] Success!";
    return result;
}

// ============================================================================
// CodexAppServerStrategy
// ============================================================================

namespace {

ProviderFetchResult makeCodexRpcFailure(const QString& message)
{
    ProviderFetchResult result;
    result.strategyID = "codex.cli.rpc";
    result.strategyKind = ProviderFetchKind::CLI;
    result.sourceLabel = "cli-rpc";
    result.success = false;
    result.errorMessage = message;
    return result;
}

QString codexResetDescription(const QDateTime& dt)
{
    if (!dt.isValid()) return {};
    qint64 secs = QDateTime::currentDateTime().secsTo(dt);
    if (secs <= 0) return "now";
    int mins = static_cast<int>(secs / 60);
    if (mins < 60) return QString("%1m").arg(mins);
    int hrs = mins / 60;
    int remMin = mins % 60;
    if (hrs < 24) return QString("%1h %2m").arg(hrs).arg(remMin);
    int days = hrs / 24;
    int remHrs = hrs % 24;
    return QString("%1d %2h").arg(days).arg(remHrs);
}

std::optional<RateWindow> mapCodexRpcWindow(const QJsonObject& obj)
{
    if (obj.isEmpty() || !obj.contains("usedPercent")) return std::nullopt;
    RateWindow window;
    window.usedPercent = std::clamp(obj.value("usedPercent").toDouble(), 0.0, 100.0);
    if (!obj.value("windowDurationMins").isNull() && !obj.value("windowDurationMins").isUndefined()) {
        window.windowMinutes = obj.value("windowDurationMins").toVariant().toInt();
    }
    if (!obj.value("resetsAt").isNull() && !obj.value("resetsAt").isUndefined()) {
        qint64 resetSecs = obj.value("resetsAt").toVariant().toLongLong();
        if (resetSecs > 0) {
            QDateTime resetAt = QDateTime::fromSecsSinceEpoch(resetSecs);
            window.resetsAt = resetAt;
            window.resetDescription = codexResetDescription(resetAt);
        }
    }
    return window;
}

double codexBalanceValue(const QJsonValue& value)
{
    if (value.isDouble()) return value.toDouble();
    bool ok = false;
    double parsed = value.toString().toDouble(&ok);
    return ok ? parsed : 0.0;
}

void applyCodexCredits(ProviderFetchResult& result, const QJsonObject& credits)
{
    if (credits.isEmpty()) return;
    if (!credits.value("hasCredits").toBool(false)) return;
    if (credits.value("unlimited").toBool(false)) return;

    double balance = codexBalanceValue(credits.value("balance"));

    CreditsSnapshot snapshot;
    snapshot.remaining = balance;
    snapshot.updatedAt = QDateTime::currentDateTime();
    result.credits = snapshot;

    ProviderCostSnapshot cost;
    cost.used = balance;
    cost.limit = 0.0;
    cost.currencyCode = "USD";
    cost.period = "Credits";
    cost.updatedAt = snapshot.updatedAt;
    result.usage.providerCost = cost;
}

QJsonObject selectCodexRateLimitSnapshot(const QJsonObject& rateLimitsResult)
{
    QJsonObject byLimitId = rateLimitsResult.value("rateLimitsByLimitId").toObject();
    QJsonObject codex = byLimitId.value("codex").toObject();
    if (!codex.isEmpty()) return codex;

    for (auto it = byLimitId.constBegin(); it != byLimitId.constEnd(); ++it) {
        QJsonObject bucket = it.value().toObject();
        if (bucket.value("limitId").toString() == "codex") return bucket;
    }

    return rateLimitsResult.value("rateLimits").toObject();
}

void applyCodexAccount(UsageSnapshot& usage, const QJsonObject& accountResult)
{
    QJsonObject account = accountResult.value("account").toObject();
    if (account.value("type").toString().compare("chatgpt", Qt::CaseInsensitive) != 0) {
        return;
    }

    QString email = account.value("email").toString().trimmed();
    QString plan = account.value("planType").toString().trimmed();
    if (email.isEmpty() && plan.isEmpty()) return;

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::codex;
    if (!email.isEmpty()) identity.accountEmail = email;
    if (!plan.isEmpty()) identity.loginMethod = plan;
    usage.identity = identity;
}

QString extractJsonObjectAfterMarker(const QString& text, const QString& marker)
{
    int markerIndex = text.indexOf(marker);
    if (markerIndex < 0) return {};
    int start = text.indexOf('{', markerIndex + marker.length());
    if (start < 0) return {};

    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (int i = start; i < text.length(); ++i) {
        QChar ch = text.at(i);
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                inString = false;
            }
            continue;
        }
        if (ch == '"') {
            inString = true;
        } else if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) return text.mid(start, i - start + 1);
        }
    }
    return {};
}

QString quoteWindowsCommandArg(QString arg)
{
    arg.replace("\"", "\\\"");
    if (arg.contains(QRegularExpression("[\\s&()\\[\\]{}^=;!'+,`~]"))) {
        return "\"" + arg + "\"";
    }
    return arg;
}

void configureCodexRpcProcess(QProcess& process,
                              const QString& binary,
                              const QStringList& codexArgs,
                              const ProviderFetchContext& ctx)
{
    QProcessEnvironment env;
    if (ctx.env.isEmpty()) {
        env = QProcessEnvironment::systemEnvironment();
    } else {
        for (auto it = ctx.env.constBegin(); it != ctx.env.constEnd(); ++it) {
            env.insert(it.key(), it.value());
        }
    }
    process.setProcessEnvironment(env);
    process.setProcessChannelMode(QProcess::SeparateChannels);

#ifdef Q_OS_WIN
    if (binary.endsWith(".cmd", Qt::CaseInsensitive) ||
        binary.endsWith(".bat", Qt::CaseInsensitive)) {
        QString command = quoteWindowsCommandArg(QDir::toNativeSeparators(binary));
        for (const auto& arg : codexArgs) {
            command += " " + quoteWindowsCommandArg(arg);
        }
        QString comspec = env.value("ComSpec", "cmd.exe");
        process.setProgram(comspec);
        process.setArguments({"/d", "/s", "/c", command});
        return;
    }
#endif

    process.setProgram(binary);
    process.setArguments(codexArgs);
}

bool writeCodexRpcMessage(QProcess& process, const QJsonObject& payload, QString* error)
{
    QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    data.append('\n');
    qint64 written = process.write(data);
    if (written != data.size() || !process.waitForBytesWritten(2000)) {
        if (error) *error = "failed to write JSON-RPC request";
        return false;
    }
    return true;
}

bool readCodexRpcResponse(QProcess& process,
                          int requestId,
                          int timeoutMs,
                          QJsonObject* response,
                          QString* error)
{
    QElapsedTimer timer;
    timer.start();
    QByteArray buffer;
    QByteArray stderrBuffer;

    auto consumeLine = [&](const QByteArray& rawLine) -> std::optional<QJsonObject> {
        QByteArray line = rawLine.trimmed();
        if (line.isEmpty()) return std::nullopt;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qDebug() << "[CodexRPC] Ignoring non-JSON stdout line:" << QString::fromUtf8(line.left(300));
            return std::nullopt;
        }

        QJsonObject obj = doc.object();
        if (!obj.contains("id")) {
            qDebug() << "[CodexRPC] Ignoring notification:" << obj.value("method").toString();
            return std::nullopt;
        }

        if (obj.value("id").toInt(-1) != requestId) {
            qDebug() << "[CodexRPC] Ignoring response for id:" << obj.value("id");
            return std::nullopt;
        }
        return obj;
    };

    while (timer.elapsed() < timeoutMs) {
        buffer.append(process.readAllStandardOutput());
        stderrBuffer.append(process.readAllStandardError());

        int newline = -1;
        while ((newline = buffer.indexOf('\n')) >= 0) {
            QByteArray line = buffer.left(newline);
            buffer.remove(0, newline + 1);
            auto obj = consumeLine(line);
            if (obj.has_value()) {
                if (response) *response = *obj;
                return true;
            }
        }

        if (process.state() == QProcess::NotRunning) {
            break;
        }

        int remaining = static_cast<int>(timeoutMs - timer.elapsed());
        process.waitForReadyRead(qBound(50, remaining, 250));
    }

    if (!buffer.trimmed().isEmpty()) {
        auto obj = consumeLine(buffer);
        if (obj.has_value()) {
            if (response) *response = *obj;
            return true;
        }
    }

    stderrBuffer.append(process.readAllStandardError());
    QString stderrText = QString::fromUtf8(stderrBuffer).trimmed();
    if (stderrText.length() > 500) stderrText = stderrText.left(500) + "...";
    if (error) {
        *error = stderrText.isEmpty()
            ? QString("timed out waiting for JSON-RPC response")
            : QString("timed out waiting for JSON-RPC response: %1").arg(stderrText);
    }
    return false;
}

bool sendCodexRpcRequest(QProcess& process,
                         int id,
                         const QString& method,
                         const QJsonValue& params,
                         int timeoutMs,
                         QJsonObject* response,
                         QString* error)
{
    QJsonObject payload;
    payload["id"] = id;
    payload["method"] = method;
    payload["params"] = params;
    if (!writeCodexRpcMessage(process, payload, error)) return false;
    return readCodexRpcResponse(process, id, timeoutMs, response, error);
}

} // namespace

CodexAppServerStrategy::CodexAppServerStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool CodexAppServerStrategy::isAvailable(const ProviderFetchContext&) const {
    return BinaryLocator::isAvailable("codex");
}

bool CodexAppServerStrategy::shouldFallback(const ProviderFetchResult& result,
                                            const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult CodexAppServerStrategy::mapRpcResult(const QJsonObject& rateLimitsResult,
                                                         const QJsonObject& accountResult)
{
    ProviderFetchResult result;
    result.strategyID = "codex.cli.rpc";
    result.strategyKind = ProviderFetchKind::CLI;
    result.sourceLabel = "cli-rpc";

    QJsonObject snapshot = selectCodexRateLimitSnapshot(rateLimitsResult);
    if (snapshot.isEmpty()) {
        result.success = false;
        result.errorMessage = "Codex CLI RPC returned no rate limit data.";
        return result;
    }

    UsageSnapshot usage;
    usage.updatedAt = QDateTime::currentDateTime();
    usage.primary = mapCodexRpcWindow(snapshot.value("primary").toObject());
    usage.secondary = mapCodexRpcWindow(snapshot.value("secondary").toObject());
    applyCodexAccount(usage, accountResult);

    result.usage = usage;
    applyCodexCredits(result, snapshot.value("credits").toObject());

    if (!result.usage.primary.has_value() && !result.usage.secondary.has_value()) {
        result.success = false;
        result.errorMessage = "Codex CLI RPC returned no rate limit windows.";
        return result;
    }

    result.success = true;
    return result;
}

ProviderFetchResult CodexAppServerStrategy::mapRpcError(const QJsonObject& errorObject)
{
    QString message = errorObject.value("message").toString();
    QString bodyJson = extractJsonObjectAfterMarker(message, "body=");
    if (!bodyJson.isEmpty()) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(bodyJson.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject body = doc.object();
            CodexUsageResponse response = CodexUsageResponse::fromJson(body);
            QString email = body.value("email").toString().trimmed();
            QString plan = body.value("plan_type").toString().trimmed();
            UsageSnapshot snapshot = response.toUsageSnapshot(email, plan);
            if (snapshot.primary.has_value() || snapshot.secondary.has_value()) {
                ProviderFetchResult result;
                result.strategyID = "codex.cli.rpc";
                result.strategyKind = ProviderFetchKind::CLI;
                result.sourceLabel = "cli-rpc";
                result.usage = snapshot;
                result.success = true;
                if (response.credits.has_value() && response.credits->hasCredits && !response.credits->unlimited) {
                    CreditsSnapshot credits;
                    credits.remaining = response.credits->balance.value_or(0.0);
                    credits.updatedAt = QDateTime::currentDateTime();
                    result.credits = credits;
                }
                return result;
            }
        }
    }

    QString shortMessage = "Codex CLI RPC failed; falling back to /status.";
    if (!message.trimmed().isEmpty()) {
        qDebug() << "[CodexRPC] JSON-RPC error:" << message.left(1000);
    }
    return makeCodexRpcFailure(shortMessage);
}

ProviderFetchResult CodexAppServerStrategy::fetchSync(const ProviderFetchContext& ctx) {
    QString binary = BinaryLocator::resolve("codex");
    qDebug() << "[CodexRPC] Binary resolve result:" << binary;
    if (binary.isEmpty()) {
        return makeCodexRpcFailure("codex CLI not found in PATH. Install via `npm i -g @openai/codex`");
    }

    QStringList args;
    args << "-s" << "read-only" << "-a" << "untrusted" << "app-server" << "--listen" << "stdio://";

    QProcess process;
    configureCodexRpcProcess(process, binary, args, ctx);
    qDebug() << "[CodexRPC] Starting app-server:" << process.program() << process.arguments().join(' ');
    process.start();
    if (!process.waitForStarted(5000)) {
        return makeCodexRpcFailure("Codex CLI RPC failed to start; falling back to /status.");
    }

    auto stopProcess = [&]() {
        process.closeWriteChannel();
        if (process.state() != QProcess::NotRunning) {
            process.terminate();
            if (!process.waitForFinished(2000)) {
                process.kill();
                process.waitForFinished(1000);
            }
        }
    };

    int timeoutMs = qMax(3000, ctx.networkTimeoutMs);
    QString error;
    QJsonObject initializeResponse;
    QJsonObject initParams{
        {"clientInfo", QJsonObject{
            {"name", "wincodexbar"},
            {"title", "WinCodexBar"},
            {"version", "0.1.0"}
        }}
    };
    if (!sendCodexRpcRequest(process, 1, "initialize", initParams, timeoutMs, &initializeResponse, &error)) {
        qDebug() << "[CodexRPC] initialize failed:" << error;
        stopProcess();
        return makeCodexRpcFailure("Codex CLI RPC failed during initialize; falling back to /status.");
    }

    QString notifyError;
    if (!writeCodexRpcMessage(process, QJsonObject{{"method", "initialized"}, {"params", QJsonObject{}}}, &notifyError)) {
        qDebug() << "[CodexRPC] initialized notification failed:" << notifyError;
    }

    QJsonObject limitsResponse;
    if (!sendCodexRpcRequest(process, 2, "account/rateLimits/read", QJsonObject{},
                             timeoutMs, &limitsResponse, &error)) {
        qDebug() << "[CodexRPC] account/rateLimits/read failed:" << error;
        stopProcess();
        return makeCodexRpcFailure("Codex CLI RPC failed while reading rate limits; falling back to /status.");
    }

    if (limitsResponse.contains("error")) {
        stopProcess();
        return mapRpcError(limitsResponse.value("error").toObject());
    }

    QJsonObject accountResult;
    QJsonObject accountResponse;
    if (sendCodexRpcRequest(process, 3, "account/read", QJsonObject{{"refreshToken", false}},
                            timeoutMs, &accountResponse, &error)) {
        if (!accountResponse.contains("error")) {
            accountResult = accountResponse.value("result").toObject();
        } else {
            qDebug() << "[CodexRPC] account/read error:" << accountResponse.value("error").toObject();
        }
    } else {
        qDebug() << "[CodexRPC] account/read failed:" << error;
    }

    stopProcess();

    ProviderFetchResult result = mapRpcResult(limitsResponse.value("result").toObject(), accountResult);
    if (!result.success) {
        qDebug() << "[CodexRPC] Mapping failed:" << result.errorMessage;
        result.errorMessage = "Codex CLI RPC returned unusable rate limit data; falling back to /status.";
    }
    return result;
}

// ============================================================================
// CodexCLIPtyStrategy
// ============================================================================

CodexCLIPtyStrategy::CodexCLIPtyStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool CodexCLIPtyStrategy::isAvailable(const ProviderFetchContext&) const {
    return BinaryLocator::isAvailable("codex");
}

bool CodexCLIPtyStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

static UsageSnapshot parseCodexCLIOutput(const QString& raw) {
    QString text = TextParser::stripAnsiEscapes(raw);
    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    // Credits: "Credits:\s*([0-9][0-9.,]*)" (matching original CodexBar)
    static QRegularExpression creditsRe("Credits:\\s*([0-9][0-9.,]*)");
    auto creditsMatch = creditsRe.match(text);
    if (creditsMatch.hasMatch()) {
        QString creditsStr = creditsMatch.captured(1);
        creditsStr.remove(',');
        bool ok;
        double credits = creditsStr.toDouble(&ok);
        if (ok && credits >= 0) {
            // Credits are stored in ProviderFetchResult, not UsageSnapshot
            // The caller should handle this
        }
    }

    // 5h limit line: "5h limit[^\n]*" (matching original CodexBar)
    static QRegularExpression fiveHRe("5h limit[^\\n]*");
    QString fiveLine;
    auto fiveMatch = fiveHRe.match(text);
    if (fiveMatch.hasMatch()) {
        fiveLine = fiveMatch.captured(0);
    }

    // Weekly limit line: "Weekly limit[^\n]*" (matching original CodexBar)
    static QRegularExpression weeklyRe("Weekly limit[^\\n]*");
    QString weekLine;
    auto weekMatch = weeklyRe.match(text);
    if (weekMatch.hasMatch()) {
        weekLine = weekMatch.captured(0);
    }

    // Parse percentage from line: "(\d+)%\s*(?:remaining|left)"
    auto parsePercentLeft = [](const QString& line) -> std::optional<double> {
        if (line.isEmpty()) return std::nullopt;
        static QRegularExpression pctRe("(\\d+(?:\\.\\d+)?)\\s*%");
        auto m = pctRe.match(line);
        if (m.hasMatch()) {
            bool ok;
            double pct = m.captured(1).toDouble(&ok);
            if (ok) return std::clamp(pct, 0.0, 100.0);
        }
        return std::nullopt;
    };

    // Parse reset string from line: "resets?\s+(.+)"
    auto parseResetString = [](const QString& line) -> QString {
        if (line.isEmpty()) return QString();
        static QRegularExpression resetRe("resets?\\s+(.+)");
        auto m = resetRe.match(line);
        if (m.hasMatch()) {
            QString reset = m.captured(1).trimmed();
            // Remove trailing parentheses
            if (reset.endsWith(')')) reset.chop(1);
            if (reset.startsWith('(')) reset = reset.mid(1);
            return reset.trimmed();
        }
        return QString();
    };

    auto fivePct = parsePercentLeft(fiveLine);
    auto weekPct = parsePercentLeft(weekLine);
    QString fiveReset = parseResetString(fiveLine);
    QString weekReset = parseResetString(weekLine);

    if (fivePct.has_value()) {
        snap.primary = RateWindow{
            *fivePct,
            std::nullopt,
            std::nullopt,
            fiveReset,
            std::nullopt
        };
    }

    if (weekPct.has_value()) {
        snap.secondary = RateWindow{
            *weekPct,
            std::nullopt,
            std::nullopt,
            weekReset,
            std::nullopt
        };
    }

    return snap;
}

ProviderFetchResult CodexCLIPtyStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = "codex.cli.pty";
    result.strategyKind = ProviderFetchKind::CLI;
    result.sourceLabel = "cli-pty";

    QString binary = BinaryLocator::resolve("codex");
    qDebug() << "[CodexCLI] Binary resolve result:" << binary;
    if (binary.isEmpty()) {
        result.errorMessage = "codex CLI not found in PATH. Install via `npm i -g @openai/codex`";
        return result;
    }

    // codex CLI requires a real terminal (PTY). QProcess won't work because
    // codex checks isTTY() and fails with "stdin is not a terminal".
    // We must use ConPTY on Windows.
    if (!ConPTYSession::isConPtyAvailable()) {
        result.errorMessage = "ConPTY is not available on this Windows version (requires Windows 10 1809+). codex CLI needs a terminal.";
        return result;
    }

    // Build the command to run inside ConPTY.
    // BinaryLocator now prefers .cmd over .ps1, so binary should be codex.cmd
    // which can be executed directly in ConPTY (cmd environment).
    // Build the command to run inside ConPTY.
    // We run codex with no args, then send "/status" via stdin.
    QStringList args;
    args << "--no-alt-screen";

    QProcessEnvironment env;
    if (ctx.env.isEmpty()) {
        env = QProcessEnvironment::systemEnvironment();
    } else {
        for (auto it = ctx.env.constBegin(); it != ctx.env.constEnd(); ++it) {
            env.insert(it.key(), it.value());
        }
    }

    ConPTYSession session;
    qDebug() << "[CodexCLI] Starting ConPTY session:" << binary << args.join(' ');
    if (!session.start(binary, args, env, 200, 60)) {
        result.errorMessage = "Failed to start ConPTY session for codex CLI";
        return result;
    }

    // Wait for codex to fully initialize (MCP server boot, UI draw, etc.)
    // Then send /status command via PTY stdin.
    QThread::msleep(2000);

    if (!session.isRunning()) {
        result.errorMessage = "codex CLI exited before we could send /status";
        return result;
    }

    session.write(QByteArray("\x15/status\r\n", 10));
    qDebug() << "[CodexCLI] Sent /status command";

    // Give codex time to process /status and print output
    QThread::msleep(1500);

    QByteArray accumulatedOutput;
    QDateTime deadline = QDateTime::currentDateTimeUtc().addSecs(15);

    while (QDateTime::currentDateTimeUtc() < deadline) {
        QByteArray chunk = session.readOutput(500);
        if (!chunk.isEmpty()) {
            accumulatedOutput.append(chunk);
        }

        if (!session.isRunning()) {
            qDebug() << "[CodexCLI] Session ended";
            break;
        }

        QThread::msleep(100);
    }

    if (session.isRunning()) {
        session.terminate();
    }

    // Read any remaining output
    QByteArray remaining = session.readOutput(1000);
    accumulatedOutput.append(remaining);
    
    qDebug() << "[CodexCLI] Total output length:" << accumulatedOutput.length();

    if (accumulatedOutput.isEmpty()) {
        result.errorMessage = "empty CLI output";
        return result;
    }

    // ConPTY outputs UTF-8, not local 8-bit encoding
    QString combined = QString::fromUtf8(accumulatedOutput);

    // Log full raw output for debugging (first 2000 chars)
    qDebug() << "[CodexCLI] Raw output preview:" << combined.left(2000);

    // Check for "stdin is not a terminal" - this should not happen with ConPTY,
    // but if it does, something is wrong with the PTY setup.
    QString lower = combined.toLower();
    if (lower.contains("stdin is not a terminal")) {
        result.errorMessage = "codex CLI still reports 'stdin is not a terminal' even with ConPTY. Please report this issue.";
        return result;
    }

    // Check for update prompt
    if (lower.contains("update available") && lower.contains("codex")) {
        result.errorMessage = "Codex CLI update needed. Run `npm i -g @openai/codex` to continue.";
        return result;
    }

    // Check for "data not available yet"
    if (lower.contains("data not available")) {
        result.errorMessage = "Codex data not available yet; will retry shortly.";
        return result;
    }

    UsageSnapshot snap = parseCodexCLIOutput(combined);

    if (!snap.primary.has_value() && !snap.secondary.has_value()) {
        QString preview = TextParser::stripAnsiEscapes(combined).simplified();
        if (preview.length() > 300) preview = preview.left(300) + "...";
        qDebug() << "[CodexCLI] Sanitized unparsed output preview:" << preview;
        result.errorMessage = "Could not parse codex CLI status output. The CLI returned interactive screen output instead of usage data.";
        return result;
    }

    result.usage = snap;
    result.success = true;
    return result;
}

// ============================================================================
// CodexWebDashboardStrategy
// ============================================================================

CodexWebDashboardStrategy::CodexWebDashboardStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString CodexWebDashboardStrategy::importBrowserCookie() {
    QStringList domains = {"chatgpt.com", "chat.openai.com"};
    auto browsers = BrowserDetection::installedBrowsers();

    for (auto browser : browsers) {
        auto cookies = CookieImporter::importCookies(browser, domains);
        if (cookies.isEmpty()) continue;

        QString cookieHeader;
        for (const auto& cookie : cookies) {
            if (!cookieHeader.isEmpty()) cookieHeader += "; ";
            cookieHeader += cookie.name() + "=" + cookie.value();
        }
        if (!cookieHeader.isEmpty()) {
            qDebug() << "[CodexWeb] Imported cookies from browser" << static_cast<int>(browser)
                     << "length:" << cookieHeader.length();
            return cookieHeader;
        }
    }
    return {};
}

bool CodexWebDashboardStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return true;
}

bool CodexWebDashboardStrategy::shouldFallback(const ProviderFetchResult& result,
                                                const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

UsageSnapshot CodexWebDashboardStrategy::parseDashboardHTML(const QString& html) {
    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    // Strip HTML tags to get text content (similar to WebView text extraction)
    QString text = html;
    // Simple HTML tag removal
    static QRegularExpression tagRe("<[^>]+>");
    text = text.replace(tagRe, " ");
    // Decode common HTML entities
    text = text.replace("&nbsp;", " ");
    text = text.replace("&lt;", "<");
    text = text.replace("&gt;", ">");
    text = text.replace("&amp;", "&");
    text = text.replace("&quot;", "\"");
    // Normalize whitespace
    static QRegularExpression wsRe("\\s+");
    text = text.replace(wsRe, " ");
    text = text.trimmed();

    // Try JSON embedded in script tags first
    static QRegularExpression jsonDataRe(
        "window\.__INITIAL_STATE__\s*=\s*(\\{.*?\\});",
        QRegularExpression::DotMatchesEverythingOption
    );
    
    auto match = jsonDataRe.match(html);
    if (match.hasMatch()) {
        QString jsonStr = match.captured(1);
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError) {
            QJsonObject root = doc.object();
            QJsonObject usage = root.value("usage").toObject();
            if (!usage.isEmpty()) {
                QJsonObject session = usage.value("session").toObject();
                QJsonObject weekly = usage.value("weekly").toObject();

                if (!session.isEmpty()) {
                    double pct = session.value("percentage").toDouble();
                    if (pct > 0) {
                        snap.primary = RateWindow{
                            std::clamp(pct, 0.0, 100.0),
                            std::nullopt, std::nullopt, QString(), std::nullopt
                        };
                    }
                }
                if (!weekly.isEmpty()) {
                    double pct = weekly.value("percentage").toDouble();
                    if (pct > 0) {
                        snap.secondary = RateWindow{
                            std::clamp(pct, 0.0, 100.0),
                            std::nullopt, std::nullopt, QString(), std::nullopt
                        };
                    }
                }
            }
        }
    }

    // Fallback: parse from visible text (matching original CodexBar's text-based parsing)
    if (!snap.primary.has_value()) {
        // Look for patterns like "Session: 80%" or "5h limit: 20% remaining"
        static QRegularExpression sessionPctRe(
            "(?:Session|5h limit)[^\\d]*?(\\d+(?:\\.\\d+)?)\\s*%",
            QRegularExpression::CaseInsensitiveOption
        );
        auto m = sessionPctRe.match(text);
        if (m.hasMatch()) {
            double pct = m.captured(1).toDouble();
            snap.primary = RateWindow{
                std::clamp(pct, 0.0, 100.0),
                std::nullopt, std::nullopt, QString(), std::nullopt
            };
        }
    }

    if (!snap.secondary.has_value()) {
        static QRegularExpression weeklyPctRe(
            "(?:Weekly|7d limit)[^\\d]*?(\\d+(?:\\.\\d+)?)\\s*%",
            QRegularExpression::CaseInsensitiveOption
        );
        auto m = weeklyPctRe.match(text);
        if (m.hasMatch()) {
            double pct = m.captured(1).toDouble();
            snap.secondary = RateWindow{
                std::clamp(pct, 0.0, 100.0),
                std::nullopt, std::nullopt, QString(), std::nullopt
            };
        }
    }

    return snap;
}

QString CodexWebDashboardStrategy::parseSignedInEmail(const QString& html) {
    static QRegularExpression emailRe(
        "\"email\"\\s*:\\s*\"([^\"]+@[^\"]+)\"",
        QRegularExpression::CaseInsensitiveOption);
    auto match = emailRe.match(html);
    if (match.hasMatch()) {
        return CodexIdentityResolver::normalizeEmail(match.captured(1));
    }

    static QRegularExpression userRe(
        "\"user\"\\s*:\\s*\\{[^}]*\"email\"\\s*:\\s*\"([^\"]+@[^\"]+)\"",
        QRegularExpression::CaseInsensitiveOption);
    match = userRe.match(html);
    if (match.hasMatch()) {
        return CodexIdentityResolver::normalizeEmail(match.captured(1));
    }

    return {};
}

ProviderFetchResult CodexWebDashboardStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = "codex.web";
    result.strategyKind = ProviderFetchKind::WebDashboard;
    result.sourceLabel = "web";

    QString cookieHeader;
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        cookieHeader = *ctx.manualCookieHeader;
    } else {
        cookieHeader = ctx.settings.get("manualCookieHeader").toString();
    }

    if (cookieHeader.isEmpty()) {
        cookieHeader = importBrowserCookie();
    }

    qDebug() << "[CodexWeb] Cookie header length:" << cookieHeader.length();
    if (cookieHeader.isEmpty()) {
        result.success = false;
        result.errorMessage = "No cookie header available (browser import failed and no manual cookie configured)";
        return result;
    }

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
    headers["Accept-Language"] = "en-US,en;q=0.9";

    qDebug() << "[CodexWeb] Fetching dashboard URL...";
    QString html = NetworkManager::instance().getStringSync(
        QUrl("https://chatgpt.com/codex/settings/usage"),
        headers,
        ctx.networkTimeoutMs
    );

    qDebug() << "[CodexWeb] Response length:" << html.length();
    if (html.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty response from web dashboard (HTTP request failed or returned empty)";
        return result;
    }

    UsageSnapshot snap = parseDashboardHTML(html);
    if (!snap.primary.has_value() && !snap.secondary.has_value()) {
        result.success = false;
        result.errorMessage = "Could not parse usage data from web dashboard. Response preview: "
                            + html.left(300).replace('\n', ' ');
        return result;
    }

    QString signedInEmail = parseSignedInEmail(html);
    qDebug() << "[CodexWeb] Signed-in email:" << signedInEmail;

    auto authInput = CodexDashboardAuthorityContext::makeLiveWebInput(
        signedInEmail, ctx, std::nullopt);
    auto decision = CodexDashboardAuthority::evaluate(authInput);

    qDebug() << "[CodexWeb] Authority disposition:" << static_cast<int>(decision.disposition)
             << "reason:" << static_cast<int>(decision.reason);

    if (decision.disposition == CodexDashboardDisposition::FailClosed) {
        result.success = false;
        QString detail;
        switch (decision.reasonDetail.reason) {
        case CodexDashboardDecisionReason::WrongEmail:
            detail = QString("expected %1, got %2")
                .arg(decision.reasonDetail.expectedEmail, decision.reasonDetail.actualEmail);
            break;
        case CodexDashboardDecisionReason::MissingDashboardSignedInEmail:
            detail = "dashboard did not expose signed-in email";
            break;
        default:
            detail = "policy rejected";
            break;
        }
        result.errorMessage = "Web dashboard authority rejected: " + detail;
        return result;
    }

    auto attachEmail = CodexDashboardAuthorityContext::attachmentEmail(authInput);
    if (attachEmail.has_value()) {
        snap.identity = ProviderIdentitySnapshot();
        snap.identity->providerID = UsageProvider::codex;
        snap.identity->accountEmail = *attachEmail;
    }

    result.usage = snap;
    result.success = true;
    return result;
}
