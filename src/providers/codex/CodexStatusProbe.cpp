#include "CodexStatusProbe.h"
#include "../shared/ConPTYSession.h"
#include "../../util/TextParser.h"
#include "../../util/BinaryLocator.h"

#include <QThread>
#include <QDateTime>
#include <QRegularExpression>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDebug>

CodexStatusProbe::CodexStatusProbe()
    : m_timeoutMs(8000)
{
}

CodexStatusProbe::ProbeResult CodexStatusProbe::fetch()
{
    return fetchInternal(120, 30, m_timeoutMs);
}

CodexStatusProbe::ProbeResult CodexStatusProbe::fetchWithRetry()
{
    ProbeResult result = fetchInternal(200, 60, m_timeoutMs);
    if (!result.success && result.error == ProbeError::ParseFailed) {
        qDebug() << "[CodexStatusProbe] First attempt parse failed, retrying with different dimensions...";
        result = fetchInternal(220, 70, 4000);
    }
    return result;
}

CodexStatusProbe::ProbeResult CodexStatusProbe::fetchInternal(int cols, int rows, int timeoutMs)
{
    ProbeResult result;
    result.success = false;
    result.error = ProbeError::CodexNotInstalled;

    QString binary = m_codexBinary;
    if (binary.isEmpty()) {
        binary = BinaryLocator::resolve("codex");
    }
    if (binary.isEmpty()) {
        result.errorMessage = "codex CLI not found in PATH. Install via `npm i -g @openai/codex`";
        return result;
    }

    if (!ConPTYSession::isConPtyAvailable()) {
        result.errorMessage = "ConPTY is not available on this Windows version (requires Windows 10 1809+).";
        return result;
    }

    ConPTYSession session;
    QStringList args;
    args << "--no-alt-screen";

    QProcessEnvironment processEnv;
    for (auto it = m_env.constBegin(); it != m_env.constEnd(); ++it) {
        processEnv.insert(it.key(), it.value());
    }

    qDebug() << "[CodexStatusProbe] Starting ConPTY session:" << binary << args.join(' ');
    if (!session.start(binary, args, processEnv, cols, rows)) {
        result.errorMessage = "Failed to start ConPTY session for codex CLI";
        return result;
    }

    QThread::msleep(2000);

    if (!session.isRunning()) {
        result.errorMessage = "codex CLI exited before we could send /status";
        return result;
    }

    session.write(QByteArray("\x15/status\r\n", 10));
    qDebug() << "[CodexStatusProbe] Sent /status command";

    QThread::msleep(1500);

    QByteArray accumulatedOutput;
    QDateTime deadline = QDateTime::currentDateTimeUtc().addSecs(timeoutMs / 1000);

    while (QDateTime::currentDateTimeUtc() < deadline) {
        QByteArray chunk = session.readOutput(500);
        if (!chunk.isEmpty()) {
            accumulatedOutput.append(chunk);
        }

        if (!session.isRunning()) {
            qDebug() << "[CodexStatusProbe] Session ended";
            break;
        }

        QThread::msleep(100);
    }

    if (session.isRunning()) {
        session.terminate();
    }

    QByteArray remaining = session.readOutput(1000);
    accumulatedOutput.append(remaining);

    qDebug() << "[CodexStatusProbe] Total output length:" << accumulatedOutput.length();

    if (accumulatedOutput.isEmpty()) {
        result.errorMessage = "empty CLI output";
        result.error = ProbeError::TimedOut;
        return result;
    }

    QString combined = QString::fromUtf8(accumulatedOutput);
    qDebug() << "[CodexStatusProbe] Raw output preview:" << combined.left(2000);

    QString lower = combined.toLower();
    if (lower.contains("stdin is not a terminal")) {
        result.errorMessage = "codex CLI still reports 'stdin is not a terminal' even with ConPTY.";
        return result;
    }

    if (containsUpdatePrompt(combined)) {
        result.errorMessage = "Codex CLI update needed. Run `npm i -g @openai/codex` to continue.";
        result.error = ProbeError::UpdateRequired;
        return result;
    }

    if (lower.contains("data not available")) {
        result.errorMessage = "Codex data not available yet; will retry shortly.";
        result.error = ProbeError::ParseFailed;
        return result;
    }

    try {
        Snapshot snapshot = parse(combined);
        if (!snapshot.hasData()) {
            QString preview = TextParser::stripAnsiEscapes(combined).simplified();
            if (preview.length() > 300) preview = preview.left(300) + "...";
            qDebug() << "[CodexStatusProbe] Sanitized unparsed output preview:" << preview;
            result.errorMessage = "Could not parse codex CLI status output.";
            result.error = ProbeError::ParseFailed;
            return result;
        }

        result.success = true;
        result.snapshot = snapshot;
        return result;
    } catch (const std::exception& e) {
        result.errorMessage = QString("Parse error: %1").arg(e.what());
        result.error = ProbeError::ParseFailed;
        return result;
    }
}

CodexStatusProbe::Snapshot CodexStatusProbe::parse(const QString& text, const QDateTime& now)
{
    QString clean = TextParser::stripAnsiEscapes(text);
    if (clean.isEmpty()) {
        throw std::runtime_error("Empty status output");
    }

    if (clean.contains("data not available yet", Qt::CaseInsensitive)) {
        throw std::runtime_error("data not available yet");
    }

    Snapshot snapshot;
    snapshot.rawText = clean;
    snapshot.credits = parseCredits(clean);

    QString fiveLine = firstLine("5h limit[^\n]*", clean);
    QString weekLine = firstLine("Weekly limit[^\n]*", clean);

    if (!fiveLine.isEmpty()) {
        snapshot.fiveHourPercentLeft = parsePercentLeft(fiveLine);
        snapshot.fiveHourResetDescription = parseResetString(fiveLine);
        snapshot.fiveHourResetsAt = parseResetDate(snapshot.fiveHourResetDescription, now);
    }

    if (!weekLine.isEmpty()) {
        snapshot.weeklyPercentLeft = parsePercentLeft(weekLine);
        snapshot.weeklyResetDescription = parseResetString(weekLine);
        snapshot.weeklyResetsAt = parseResetDate(snapshot.weeklyResetDescription, now);
    }

    if (!snapshot.credits.has_value() && !snapshot.fiveHourPercentLeft.has_value() && !snapshot.weeklyPercentLeft.has_value()) {
        throw std::runtime_error("No parseable data in status output");
    }

    return snapshot;
}

std::optional<QDateTime> CodexStatusProbe::parseResetDate(const QString& text, const QDateTime& now)
{
    if (text.isEmpty()) return std::nullopt;

    QString raw = text.trimmed();
    raw.remove('(');
    raw.remove(')');
    raw = raw.simplified();

    static QRegularExpression timeOnDate1("^([0-9]{1,2}:[0-9]{2}) on ([0-9]{1,2} [A-Za-z]{3})$");
    auto match1 = timeOnDate1.match(raw);
    if (match1.hasMatch()) {
        QString dateStr = match1.captured(2) + " " + match1.captured(1);
        QDateTime dt = QDateTime::fromString(dateStr, "d MMM HH:mm");
        if (dt.isValid()) {
            if (dt < now) dt = dt.addYears(1);
            return dt;
        }
    }

    static QRegularExpression timeOnDate2("^([0-9]{1,2}:[0-9]{2}) on ([A-Za-z]{3} [0-9]{1,2})$");
    auto match2 = timeOnDate2.match(raw);
    if (match2.hasMatch()) {
        QString dateStr = match2.captured(2) + " " + match2.captured(1);
        QDateTime dt = QDateTime::fromString(dateStr, "MMM d HH:mm");
        if (dt.isValid()) {
            if (dt < now) dt = dt.addYears(1);
            return dt;
        }
    }

    QDateTime timeOnly = QDateTime::fromString(raw, "HH:mm");
    if (!timeOnly.isValid()) {
        timeOnly = QDateTime::fromString(raw, "H:mm");
    }
    if (timeOnly.isValid()) {
        QTime time = timeOnly.time();
        QDateTime anchored = QDateTime(now.date(), time);
        if (anchored <= now) {
            anchored = anchored.addDays(1);
        }
        return anchored;
    }

    return std::nullopt;
}

bool CodexStatusProbe::containsUpdatePrompt(const QString& text)
{
    QString lower = text.toLower();
    return lower.contains("update available") && lower.contains("codex");
}

std::optional<double> CodexStatusProbe::parseCredits(const QString& text)
{
    static QRegularExpression creditsRe("Credits:\\s*([0-9][0-9.,]*)");
    auto match = creditsRe.match(text);
    if (match.hasMatch()) {
        QString creditsStr = match.captured(1);
        creditsStr.remove(',');
        bool ok;
        double credits = creditsStr.toDouble(&ok);
        if (ok && credits >= 0) {
            return credits;
        }
    }
    return std::nullopt;
}

std::optional<int> CodexStatusProbe::parsePercentLeft(const QString& line)
{
    if (line.isEmpty()) return std::nullopt;
    static QRegularExpression pctRe("(\\d+(?:\\.\\d+)?)\\s*%");
    auto match = pctRe.match(line);
    if (match.hasMatch()) {
        bool ok;
        double pct = match.captured(1).toDouble(&ok);
        if (ok) {
            return std::clamp(static_cast<int>(pct), 0, 100);
        }
    }
    return std::nullopt;
}

QString CodexStatusProbe::parseResetString(const QString& line)
{
    if (line.isEmpty()) return {};
    static QRegularExpression resetRe("resets?\\s+(.+)");
    auto match = resetRe.match(line);
    if (match.hasMatch()) {
        QString reset = match.captured(1).trimmed();
        if (reset.endsWith(')')) reset.chop(1);
        if (reset.startsWith('(')) reset = reset.mid(1);
        return reset.trimmed();
    }
    return {};
}

QString CodexStatusProbe::firstLine(const QString& pattern, const QString& text)
{
    static QRegularExpression re(pattern);
    auto match = re.match(text);
    if (match.hasMatch()) {
        return match.captured(0);
    }
    return {};
}

void CodexStatusProbe::setCodexBinary(const QString& binary)
{
    m_codexBinary = binary;
}

void CodexStatusProbe::setTimeout(int timeoutMs)
{
    m_timeoutMs = timeoutMs;
}

void CodexStatusProbe::setEnvironment(const QHash<QString, QString>& env)
{
    m_env = env;
}
