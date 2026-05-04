#include "CodexPersistentCLISession.h"
#include "../shared/ConPTYSession.h"

#include <QThread>
#include <QDateTime>
#include <QDebug>
#include <QProcessEnvironment>

CodexPersistentCLISession& CodexPersistentCLISession::instance()
{
    static CodexPersistentCLISession session;
    return session;
}

CodexPersistentCLISession::CodexPersistentCLISession() {}

CodexPersistentCLISession::~CodexPersistentCLISession()
{
    cleanup();
}

bool CodexPersistentCLISession::ensureStarted(
    const QString& binary,
    int cols,
    int rows,
    const QHash<QString, QString>& env)
{
    if (m_started && m_session && m_session->isRunning() &&
        m_binaryPath == binary && m_cols == cols && m_rows == rows && m_env == env) {
        return true;
    }

    cleanup();

    m_session = new ConPTYSession();
    m_binaryPath = binary;
    m_cols = cols;
    m_rows = rows;
    m_env = env;

    QStringList args;
    args << "--no-alt-screen";

    QProcessEnvironment processEnv;
    for (auto it = env.constBegin(); it != env.constEnd(); ++it) {
        processEnv.insert(it.key(), it.value());
    }

    qDebug() << "[CodexPersistentCLI] Starting session:" << binary << args.join(' ');
    if (!m_session->start(binary, args, processEnv, cols, rows)) {
        qDebug() << "[CodexPersistentCLI] Failed to start session";
        cleanup();
        return false;
    }

    m_started = true;
    return true;
}

void CodexPersistentCLISession::cleanup()
{
    if (m_session) {
        if (m_session->isRunning()) {
            m_session->write(QByteArray("/exit\n", 6));
            QThread::msleep(100);
            m_session->terminate();
        }
        delete m_session;
        m_session = nullptr;
    }
    m_started = false;
    m_binaryPath.clear();
    m_env.clear();
}

QByteArray CodexPersistentCLISession::readChunk()
{
    if (!m_session) return {};
    return m_session->readOutput(50);
}

void CodexPersistentCLISession::drainOutput()
{
    readChunk();
}

bool CodexPersistentCLISession::send(const QString& text)
{
    if (!m_session || !m_session->isRunning()) return false;
    return m_session->write(text.toUtf8());
}

bool CodexPersistentCLISession::containsStatusMarker(const QString& text)
{
    QString lower = text.toLower();
    return lower.contains("credits:") ||
           lower.contains("5h limit") ||
           lower.contains("5-hour limit") ||
           lower.contains("weekly limit");
}

bool CodexPersistentCLISession::containsUpdatePrompt(const QString& text)
{
    QString lower = text.toLower();
    return lower.contains("update available") && lower.contains("codex");
}

CodexPersistentCLISession::CaptureResult CodexPersistentCLISession::captureStatus(
    const QString& binary,
    int cols,
    int rows,
    int timeoutMs,
    const QHash<QString, QString>& env)
{
    CaptureResult result;
    result.success = false;

    if (!ensureStarted(binary, cols, rows, env)) {
        result.errorMessage = "Failed to start persistent CLI session";
        return result;
    }

    // Wait for session to stabilize (reduced from 400ms after ConPTY event-driven optimization)
    QThread::msleep(200);
    drainOutput();

    QByteArray buffer;
    QDateTime deadline = QDateTime::currentDateTimeUtc().addMSecs(timeoutMs);
    bool sentScript = false;
    bool sawStatus = false;
    bool skippedUpdate = false;
    int enterRetries = 0;
    int resendRetries = 0;
    QDateTime lastEnter = QDateTime::currentDateTimeUtc().addSecs(-10);
    QDateTime scriptSentAt;

    while (QDateTime::currentDateTimeUtc() < deadline) {
        QByteArray newData = readChunk();
        if (!newData.isEmpty()) {
            buffer.append(newData);
        }

        QString bufferText = QString::fromUtf8(buffer);

        // Check for update prompt
        if (!skippedUpdate && containsUpdatePrompt(bufferText)) {
            qDebug() << "[CodexPersistentCLI] Detected update prompt, skipping...";
            send("\x1b[B"); // Down arrow
            QThread::msleep(80);
            send("\r");
            QThread::msleep(80);
            send("\r");
            skippedUpdate = true;
            sentScript = false;
            buffer.clear();
            sawStatus = false;
            QThread::msleep(150);
            continue;
        }

        // Check for status markers
        if (containsStatusMarker(bufferText)) {
            sawStatus = true;
        }

        // Send /status command
        if (!sentScript && (skippedUpdate || !containsUpdatePrompt(bufferText))) {
            send("/status\r");
            sentScript = true;
            scriptSentAt = QDateTime::currentDateTimeUtc();
            lastEnter = QDateTime::currentDateTimeUtc();
            QThread::msleep(100);
            continue;
        }

        // Retry with enter if no response
        if (sentScript && !sawStatus) {
            if (lastEnter.secsTo(QDateTime::currentDateTimeUtc()) >= 1.0 && enterRetries < 5) {
                send("\r");
                enterRetries++;
                lastEnter = QDateTime::currentDateTimeUtc();
                QThread::msleep(50);
                continue;
            }
            if (scriptSentAt.secsTo(QDateTime::currentDateTimeUtc()) >= 2.5 && resendRetries < 2) {
                send("/status\r");
                resendRetries++;
                buffer.clear();
                sawStatus = false;
                scriptSentAt = QDateTime::currentDateTimeUtc();
                lastEnter = QDateTime::currentDateTimeUtc();
                QThread::msleep(100);
                continue;
            }
        }

        if (sawStatus) break;

        if (!m_session || !m_session->isRunning()) {
            result.errorMessage = "CLI session exited unexpectedly";
            cleanup();
            return result;
        }

        QThread::msleep(50);
    }

    // Wait for output to settle
    if (sawStatus) {
        QDateTime settleDeadline = QDateTime::currentDateTimeUtc().addMSecs(800);
        while (QDateTime::currentDateTimeUtc() < settleDeadline) {
            QByteArray newData = readChunk();
            if (!newData.isEmpty()) {
                buffer.append(newData);
            }
            QThread::msleep(50);
        }
    }

    if (buffer.isEmpty()) {
        result.errorMessage = "Timed out waiting for status output";
        return result;
    }

    result.success = true;
    result.text = QString::fromUtf8(buffer);
    return result;
}

void CodexPersistentCLISession::reset()
{
    cleanup();
}
