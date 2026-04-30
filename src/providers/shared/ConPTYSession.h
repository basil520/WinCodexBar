#pragma once

#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <windows.h>

#include <thread>

class ConPTYSession : public QObject {
    Q_OBJECT
public:
    explicit ConPTYSession(QObject* parent = nullptr);
    ~ConPTYSession() override;

    bool start(const QString& command,
               const QStringList& args,
               const QProcessEnvironment& env = QProcessEnvironment());
    bool write(const QByteArray& data);
    QByteArray readOutput(int timeoutMs = 100);
    bool waitForPattern(const QRegularExpression& pattern, int timeoutMs = 8000);
    void terminate();
    bool isRunning() const;

    static bool isConPtyAvailable();
    static QString quoteArg(const QString& arg);

signals:
    void outputReceived(const QByteArray& data);
    void processFinished(int exitCode);

private:
    void readerLoop();
    void appendOutput(const QByteArray& data);
    bool startWithConPty(const QString& command, const QStringList& args, const QProcessEnvironment& env);
    bool startWithQProcess(const QString& command, const QStringList& args, const QProcessEnvironment& env);

    // ConPTY state
    void* m_ptyHandle = nullptr;
    HANDLE m_hInput = nullptr;
    HANDLE m_hOutput = nullptr;
    HANDLE m_hProcess = nullptr;
    HANDLE m_hThread = nullptr;
    void* m_procInfo = nullptr;

    // QProcess fallback state
    QProcess* m_process = nullptr;

    bool m_running = false;
    bool m_useFallback = false;
    QByteArray m_buffer;
    mutable QMutex m_bufferMutex;
    std::thread m_readerThread;
};