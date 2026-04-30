#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFuture>
#include <QtConcurrent>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    static NetworkManager& instance();

    QFuture<QJsonObject> getJson(const QUrl& url,
                                  const QHash<QString, QString>& headers = {});
    QFuture<QString> getString(const QUrl& url,
                                const QHash<QString, QString>& headers = {});
    QFuture<QJsonObject> postJson(const QUrl& url,
                                   const QJsonObject& body,
                                   const QHash<QString, QString>& headers = {});
    QFuture<QJsonObject> postForm(const QUrl& url,
                                   const QByteArray& formData,
                                   const QHash<QString, QString>& headers = {});

    QJsonObject getJsonSync(const QUrl& url,
                            const QHash<QString, QString>& headers = {},
                            int timeoutMs = -1,
                            bool http2Allowed = true);
    QString getStringSync(const QUrl& url,
                          const QHash<QString, QString>& headers = {},
                          int timeoutMs = -1,
                          bool http2Allowed = true);
    QJsonObject postJsonSync(const QUrl& url,
                             const QJsonObject& body,
                             const QHash<QString, QString>& headers = {},
                             int timeoutMs = -1,
                             bool http2Allowed = true);
    QJsonObject postFormSync(const QUrl& url,
                             const QByteArray& formData,
                             const QHash<QString, QString>& headers = {},
                             int timeoutMs = -1,
                             bool http2Allowed = true);

    void setDefaultTimeout(int ms) { m_defaultTimeout = ms; }

private:
    NetworkManager();
    int m_defaultTimeout = 15000;
};
