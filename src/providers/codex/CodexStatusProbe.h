#pragma once

#include <QString>
#include <QDateTime>
#include <QHash>
#include <optional>

class CodexStatusProbe {
public:
    struct Snapshot {
        std::optional<double> credits;
        std::optional<int> fiveHourPercentLeft;
        std::optional<int> weeklyPercentLeft;
        QString fiveHourResetDescription;
        QString weeklyResetDescription;
        std::optional<QDateTime> fiveHourResetsAt;
        std::optional<QDateTime> weeklyResetsAt;
        QString rawText;

        bool hasData() const {
            return credits.has_value() || fiveHourPercentLeft.has_value() || weeklyPercentLeft.has_value();
        }
    };

    enum class ProbeError {
        CodexNotInstalled,
        ParseFailed,
        TimedOut,
        UpdateRequired
    };

    struct ProbeResult {
        bool success;
        std::optional<Snapshot> snapshot;
        ProbeError error;
        QString errorMessage;
    };

    CodexStatusProbe();

    ProbeResult fetch();
    ProbeResult fetchWithRetry();
    static Snapshot parse(const QString& text, const QDateTime& now = QDateTime::currentDateTime());
    static std::optional<QDateTime> parseResetDate(const QString& text, const QDateTime& now);

    void setCodexBinary(const QString& binary);
    void setTimeout(int timeoutMs);
    void setEnvironment(const QHash<QString, QString>& env);

private:
    QString m_codexBinary;
    int m_timeoutMs;
    QHash<QString, QString> m_env;

    ProbeResult fetchInternal(int cols, int rows, int timeoutMs);
    static bool containsUpdatePrompt(const QString& text);
    static std::optional<double> parseCredits(const QString& text);
    static std::optional<int> parsePercentLeft(const QString& line);
    static QString parseResetString(const QString& line);
    static QString firstLine(const QString& pattern, const QString& text);
};
