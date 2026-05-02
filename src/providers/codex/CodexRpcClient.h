#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonValue>
#include <QHash>
#include <QProcess>
#include <optional>

class CodexRpcClient {
public:
    struct RpcResult {
        bool success = false;
        QJsonObject result;
        QString errorMessage;
    };

    explicit CodexRpcClient(const QHash<QString, QString>& env = {});
    ~CodexRpcClient();

    bool start(const QString& binary, const QStringList& args, int startTimeoutMs = 5000);
    void shutdown();
    bool isRunning() const;

    RpcResult initialize(const QString& clientName, const QString& clientVersion, int timeoutMs = 15000);
    RpcResult sendRequest(const QString& method, const QJsonValue& params, int timeoutMs = 15000);
    bool sendNotification(const QString& method, const QJsonValue& params = QJsonValue(QJsonObject()));

private:
    QHash<QString, QString> m_env;
    QProcess* m_process = nullptr;
    int m_nextRequestId = 1;

    static QString quoteWindowsCommandArg(QString arg);
    static void configureProcess(QProcess& process, const QString& binary, const QStringList& args, const QHash<QString, QString>& env);
    static bool writeRpcMessage(QProcess& process, const QJsonObject& payload, QString* error);
    static std::optional<QJsonObject> readRpcResponse(QProcess& process, int requestId, int timeoutMs, QString* error);
};
