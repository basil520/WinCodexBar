#pragma once

#include <QString>
#include <QHash>
#include <QByteArray>
#include <QMutex>
#include <optional>

class ConPTYSession;

class CodexPersistentCLISession {
public:
    static CodexPersistentCLISession& instance();

    struct CaptureResult {
        bool success;
        QString text;
        QString errorMessage;
    };

    CaptureResult captureStatus(
        const QString& binary,
        int cols,
        int rows,
        int timeoutMs,
        const QHash<QString, QString>& env);

    void reset();

private:
    CodexPersistentCLISession();
    ~CodexPersistentCLISession();
    CodexPersistentCLISession(const CodexPersistentCLISession&) = delete;
    CodexPersistentCLISession& operator=(const CodexPersistentCLISession&) = delete;

    bool ensureStarted(
        const QString& binary,
        int cols,
        int rows,
        const QHash<QString, QString>& env);

    void cleanup();
    QByteArray readChunk();
    void drainOutput();
    bool send(const QString& text);

    ConPTYSession* m_session = nullptr;
    QString m_binaryPath;
    int m_cols = 0;
    int m_rows = 0;
    QHash<QString, QString> m_env;
    bool m_started = false;

    static bool containsStatusMarker(const QString& text);
    static bool containsUpdatePrompt(const QString& text);
};
