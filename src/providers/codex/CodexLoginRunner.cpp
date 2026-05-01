#include "CodexLoginRunner.h"
#include "../../util/BinaryLocator.h"
#include "../../util/TextParser.h"

#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVector>

CodexLoginRunner::CodexLoginRunner(QObject* parent)
    : QObject(parent)
{
}

CodexLoginRunner::~CodexLoginRunner()
{
    cancel();
}

void CodexLoginRunner::start(const QString& homePath, int timeoutMs)
{
    if (m_process) {
        qWarning() << "CodexLoginRunner: already running";
        return;
    }

    m_cancelled = false;
    m_output.clear();
    m_promptEmitted = false;
    m_homePath = homePath;

    // 1. Find codex binary
    QString codexPath = BinaryLocator::resolve("codex");
    if (codexPath.isEmpty()) {
        qWarning() << "CodexLoginRunner: codex binary not found";
        Result result;
        result.outcome = Result::MissingBinary;
        result.output = QStringLiteral("codex binary not found");
        result.exitCode = -1;
        emit finished(result);
        return;
    }

    qDebug() << "CodexLoginRunner: using codex at" << codexPath;

    // 2. Prepare environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!homePath.isEmpty()) {
        env.insert("CODEX_HOME", homePath);
        qDebug() << "CodexLoginRunner: CODEX_HOME set to" << homePath;
    }

    // 3. Create and start process
    emit progressUpdate(QStringLiteral("Starting codex login..."));

    m_process = new QProcess(this);
    m_process->setProcessEnvironment(env);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CodexLoginRunner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &CodexLoginRunner::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &CodexLoginRunner::onReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &CodexLoginRunner::onReadyRead);

    m_process->start(codexPath, {"login", "--device-auth"});

    if (!m_process->waitForStarted(5000)) {
        QString error = m_process->errorString();
        qWarning() << "CodexLoginRunner: failed to start:" << error;
        delete m_process;
        m_process = nullptr;
        Result result;
        result.outcome = Result::LaunchFailed;
        result.output = error;
        result.exitCode = -1;
        emit finished(result);
        return;
    }

    // 4. Start timeout timer
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &CodexLoginRunner::onTimeout);
    m_timeoutTimer->start(timeoutMs);

    qDebug() << "CodexLoginRunner: process started, waiting for completion...";
}

void CodexLoginRunner::cancel()
{
    m_cancelled = true;

    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        delete m_timeoutTimer;
        m_timeoutTimer = nullptr;
    }

    if (m_process) {
        qDebug() << "CodexLoginRunner: cancelling process";
        disconnect(m_process, nullptr, this, nullptr);
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
        delete m_process;
        m_process = nullptr;
    }
}

bool CodexLoginRunner::isRunning() const
{
    return m_process != nullptr;
}

std::optional<CodexLoginRunner::DeviceAuthPrompt>
CodexLoginRunner::parseDeviceAuthPrompt(const QString& output)
{
    QString text = TextParser::stripAnsiEscapes(output);

    static const QRegularExpression urlRe(
        R"((https?://[^\s\]\)>'"`]+))",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch urlMatch = urlRe.match(text);
    if (!urlMatch.hasMatch()) {
        qDebug() << "[CodexLoginRunner] parseDeviceAuthPrompt: no URL found in output:" << text.left(200);
        return std::nullopt;
    }

    QString url = urlMatch.captured(1).trimmed();
    while (!url.isEmpty() && QStringLiteral(".,;:").contains(url.back())) {
        url.chop(1);
    }

    static const QVector<QRegularExpression> codePatterns{
        QRegularExpression(
            R"(\buser\s+code(?:\s+is)?\s*[:=]?\s*([A-Z0-9]{4}(?:[-\s][A-Z0-9]{4})+|[A-Z0-9][A-Z0-9-]{5,}))",
            QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(
            R"(\bcode(?:\s+is)?\s*[:=]\s*([A-Z0-9]{4}(?:[-\s][A-Z0-9]{4})+|[A-Z0-9][A-Z0-9-]{5,}))",
            QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(
            R"(\benter\s+(?:the\s+)?code\s+([A-Z0-9]{4}(?:[-\s][A-Z0-9]{4})+|[A-Z0-9][A-Z0-9-]{5,}))",
            QRegularExpression::CaseInsensitiveOption)
    };

    QString userCode;
    for (const auto& pattern : codePatterns) {
        QRegularExpressionMatch match = pattern.match(text);
        if (match.hasMatch()) {
            userCode = match.captured(1).trimmed().toUpper();
            break;
        }
    }

    if (userCode.isEmpty()) {
        qDebug() << "[CodexLoginRunner] parseDeviceAuthPrompt: URL found but no user code matched."
                 << "URL:" << url << "Text preview:" << text.left(200);
        return std::nullopt;
    }

    DeviceAuthPrompt prompt;
    prompt.verificationUri = url;
    prompt.userCode = userCode.simplified().replace(' ', '-');
    return prompt;
}

void CodexLoginRunner::onReadyRead()
{
    if (!m_process) return;

    QByteArray data = m_process->readAllStandardOutput();
    data.append(m_process->readAllStandardError());
    if (data.isEmpty()) return;

    m_output += QString::fromUtf8(data);

    if (!m_promptEmitted) {
        auto prompt = parseDeviceAuthPrompt(m_output);
        if (prompt.has_value()) {
            m_promptEmitted = true;
            emit progressUpdate(QStringLiteral("Waiting for Codex device authorization..."));
            emit deviceAuthPromptReady(prompt->verificationUri, prompt->userCode);
        } else {
            qDebug() << "[CodexLoginRunner] parseDeviceAuthPrompt returned nullopt. Accumulated output size:"
                     << m_output.size() << "bytes";
        }
    }
}

void CodexLoginRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_cancelled) return;

    qDebug() << "CodexLoginRunner: process finished with exit code" << exitCode;

    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        delete m_timeoutTimer;
        m_timeoutTimer = nullptr;
    }

    Result result = collectResult(exitCode, exitStatus);

    delete m_process;
    m_process = nullptr;

    emit finished(result);
}

void CodexLoginRunner::onProcessError(QProcess::ProcessError error)
{
    if (m_cancelled) return;

    qWarning() << "CodexLoginRunner: process error:" << error;

    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        delete m_timeoutTimer;
        m_timeoutTimer = nullptr;
    }

    Result result;
    result.outcome = Result::Failed;
    QString processError = m_process ? m_process->errorString() : QStringLiteral("Unknown error");
    result.output = processError;
    if (!m_output.isEmpty())
        result.output += QStringLiteral(" | Accumulated output: ") + m_output;
    result.exitCode = -1;

    delete m_process;
    m_process = nullptr;

    emit finished(result);
}

void CodexLoginRunner::onTimeout()
{
    if (m_cancelled) return;

    qWarning() << "CodexLoginRunner: timed out";

    if (m_process) {
        disconnect(m_process, nullptr, this, nullptr);
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }

    Result result;
    result.outcome = Result::TimedOut;
    result.output = QStringLiteral("Login timed out");
    result.exitCode = -1;

    emit finished(result);
}

CodexLoginRunner::Result CodexLoginRunner::collectResult(int exitCode, QProcess::ExitStatus exitStatus)
{
    Result result;
    result.exitCode = exitCode;

    onReadyRead();
    result.output = m_output;

    qDebug() << "CodexLoginRunner: exit code =" << exitCode;
    if (!result.output.isEmpty()) {
        qDebug() << "CodexLoginRunner: output:" << result.output.left(500);
    }

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        result.outcome = Result::Success;
        emit progressUpdate(QStringLiteral("Login successful"));
    } else {
        qDebug() << "[CodexLoginRunner] Login process failed. Exit code:" << exitCode
                 << "Output preview:" << result.output.left(500);
        result.outcome = Result::Failed;
        emit progressUpdate(QStringLiteral("Login failed"));
    }

    return result;
}
