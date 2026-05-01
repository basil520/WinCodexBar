#pragma once

#include <QObject>
#include <QString>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>
#include <optional>

class CodexLoginRunner : public QObject {
    Q_OBJECT
public:
    struct DeviceAuthPrompt {
        QString verificationUri;
        QString userCode;
    };

    struct Result {
        enum Outcome { Success, TimedOut, Failed, MissingBinary, LaunchFailed, InProgress };
        Outcome outcome;
        QString output;
        int exitCode;
    };

    explicit CodexLoginRunner(QObject* parent = nullptr);
    ~CodexLoginRunner() override;

    void start(const QString& homePath = QString(), int timeoutMs = 120000);
    void cancel();
    bool isRunning() const;

    static std::optional<DeviceAuthPrompt> parseDeviceAuthPrompt(const QString& output);

signals:
    void finished(const CodexLoginRunner::Result& result);
    void progressUpdate(const QString& message);
    void deviceAuthPromptReady(const QString& verificationUri, const QString& userCode);

private slots:
    void onReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onTimeout();
    void onProgressTick();

private:
    QProcess* m_process = nullptr;
    QTimer* m_timeoutTimer = nullptr;
    QTimer* m_progressTimer = nullptr;
    int m_elapsedSeconds = 0;
    QString m_homePath;
    QString m_output;
    bool m_cancelled = false;
    bool m_promptEmitted = false;

    Result collectResult(int exitCode, QProcess::ExitStatus exitStatus);
};
