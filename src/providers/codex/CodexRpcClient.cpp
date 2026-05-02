#include "CodexRpcClient.h"

#include <QJsonDocument>
#include <QElapsedTimer>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QDir>
#include <QDebug>

CodexRpcClient::CodexRpcClient(const QHash<QString, QString>& env)
    : m_env(env)
{
}

CodexRpcClient::~CodexRpcClient()
{
    shutdown();
}

QString CodexRpcClient::quoteWindowsCommandArg(QString arg)
{
    arg.replace("\"", "\\\"");
    if (arg.contains(QRegularExpression("[\\s&()\\[\\]{}^=;!'+,`~]"))) {
        return "\"" + arg + "\"";
    }
    return arg;
}

void CodexRpcClient::configureProcess(QProcess& process, const QString& binary, const QStringList& args, const QHash<QString, QString>& env)
{
    QProcessEnvironment penv;
    if (env.isEmpty()) {
        penv = QProcessEnvironment::systemEnvironment();
    } else {
        for (auto it = env.constBegin(); it != env.constEnd(); ++it) {
            penv.insert(it.key(), it.value());
        }
    }
    process.setProcessEnvironment(penv);
    process.setProcessChannelMode(QProcess::SeparateChannels);

#ifdef Q_OS_WIN
    if (binary.endsWith(".cmd", Qt::CaseInsensitive) ||
        binary.endsWith(".bat", Qt::CaseInsensitive)) {
        QString command = quoteWindowsCommandArg(QDir::toNativeSeparators(binary));
        for (const auto& a : args) {
            command += " " + quoteWindowsCommandArg(a);
        }
        QString comspec = penv.value("ComSpec", "cmd.exe");
        process.setProgram(comspec);
        process.setArguments({"/d", "/s", "/c", command});
        return;
    }
#endif
    process.setProgram(binary);
    process.setArguments(args);
}

bool CodexRpcClient::start(const QString& binary, const QStringList& args, int startTimeoutMs)
{
    shutdown();
    m_process = new QProcess();
    configureProcess(*m_process, binary, args, m_env);
    m_process->start();
    if (!m_process->waitForStarted(startTimeoutMs)) {
        delete m_process;
        m_process = nullptr;
        return false;
    }
    return true;
}

void CodexRpcClient::shutdown()
{
    if (!m_process) return;
    m_process->closeWriteChannel();
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(2000)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
    delete m_process;
    m_process = nullptr;
}

bool CodexRpcClient::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

bool CodexRpcClient::writeRpcMessage(QProcess& process, const QJsonObject& payload, QString* error)
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

std::optional<QJsonObject> CodexRpcClient::readRpcResponse(QProcess& process, int requestId, int timeoutMs, QString* error)
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
            return std::nullopt;
        }
        QJsonObject obj = doc.object();
        if (!obj.contains("id")) return std::nullopt;
        if (obj.value("id").toInt(-1) != requestId) return std::nullopt;
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
            if (obj.has_value()) return obj;
        }
        if (process.state() == QProcess::NotRunning) break;
        int remaining = static_cast<int>(timeoutMs - timer.elapsed());
        process.waitForReadyRead(qBound(50, remaining, 250));
    }

    if (!buffer.trimmed().isEmpty()) {
        auto obj = consumeLine(buffer);
        if (obj.has_value()) return obj;
    }

    stderrBuffer.append(process.readAllStandardError());
    QString stderrText = QString::fromUtf8(stderrBuffer).trimmed();
    if (stderrText.length() > 500) stderrText = stderrText.left(500) + "...";
    if (error) {
        *error = stderrText.isEmpty()
            ? QString("timed out waiting for JSON-RPC response")
            : QString("timed out waiting for JSON-RPC response: %1").arg(stderrText);
    }
    return std::nullopt;
}

CodexRpcClient::RpcResult CodexRpcClient::initialize(const QString& clientName, const QString& clientVersion, int timeoutMs)
{
    RpcResult result;
    if (!m_process) {
        result.errorMessage = "RPC client not started";
        return result;
    }

    QJsonObject initParams{
        {"clientInfo", QJsonObject{
            {"name", clientName},
            {"title", "WinCodexBar"},
            {"version", clientVersion}
        }}
    };

    int id = m_nextRequestId++;
    QJsonObject payload;
    payload["id"] = id;
    payload["method"] = "initialize";
    payload["params"] = initParams;

    QString writeError;
    if (!writeRpcMessage(*m_process, payload, &writeError)) {
        result.errorMessage = writeError;
        return result;
    }

    QString readError;
    auto resp = readRpcResponse(*m_process, id, timeoutMs, &readError);
    if (!resp.has_value()) {
        result.errorMessage = readError;
        return result;
    }

    if (resp->contains("error")) {
        result.errorMessage = resp->value("error").toObject().value("message").toString("initialize failed");
        return result;
    }

    // Send initialized notification
    QJsonObject notifyPayload;
    notifyPayload["method"] = "initialized";
    notifyPayload["params"] = QJsonObject{};
    writeRpcMessage(*m_process, notifyPayload, nullptr);

    result.success = true;
    result.result = resp->value("result").toObject();
    return result;
}

bool CodexRpcClient::sendNotification(const QString& method, const QJsonValue& params)
{
    if (!m_process) return false;
    QJsonObject payload;
    payload["method"] = method;
    payload["params"] = params;
    return writeRpcMessage(*m_process, payload, nullptr);
}

CodexRpcClient::RpcResult CodexRpcClient::sendRequest(const QString& method, const QJsonValue& params, int timeoutMs)
{
    RpcResult result;
    if (!m_process) {
        result.errorMessage = "RPC client not started";
        return result;
    }

    int id = m_nextRequestId++;
    QJsonObject payload;
    payload["id"] = id;
    payload["method"] = method;
    payload["params"] = params;

    QString writeError;
    if (!writeRpcMessage(*m_process, payload, &writeError)) {
        result.errorMessage = writeError;
        return result;
    }

    QString readError;
    auto resp = readRpcResponse(*m_process, id, timeoutMs, &readError);
    if (!resp.has_value()) {
        result.errorMessage = readError;
        return result;
    }

    if (resp->contains("error")) {
        result.errorMessage = resp->value("error").toObject().value("message").toString(method + " failed");
        return result;
    }

    result.success = true;
    result.result = resp->value("result").toObject();
    return result;
}
