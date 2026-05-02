#include "NetworkManager.h"

#include <QEventLoop>
#include <QTimer>
#include <QUrlQuery>
#include <memory>

static bool g_shuttingDown = false;

void NetworkManager::setShuttingDown(bool shuttingDown)
{
    g_shuttingDown = shuttingDown;
}

namespace {

void applyHeaders(QNetworkRequest& request,
                  const QHash<QString, QString>& headers,
                  const QByteArray& accept = "application/json")
{
    if (!accept.isEmpty()) {
        request.setRawHeader("Accept", accept);
    }
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
}

QByteArray waitForReply(QNetworkReply* reply, int timeoutMs)
{
    if (g_shuttingDown) {
        reply->abort();
        reply->deleteLater();
        return {};
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    // Use shorter timeout when shutting down to allow quick exit
    int actualTimeout = g_shuttingDown ? qMin(1000, timeoutMs) : timeoutMs;
    timer.start(actualTimeout);
    loop.exec();

    QByteArray data;
    if (timer.isActive()) {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError) {
            data = reply->readAll();
        }
    } else {
        reply->abort();
    }
    reply->deleteLater();
    return data;
}

QJsonObject parseJsonObject(const QByteArray& data)
{
    if (data.isEmpty()) return {};
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return {};
    }
    return doc.object();
}

} // namespace

NetworkManager& NetworkManager::instance() {
    static NetworkManager nm;
    return nm;
}

NetworkManager::NetworkManager()
    : QObject(nullptr) {}

QNetworkAccessManager* NetworkManager::threadLocalNam()
{
    static thread_local std::unique_ptr<QNetworkAccessManager> nam;
    if (!nam) {
        nam = std::make_unique<QNetworkAccessManager>();
    }
    return nam.get();
}

QJsonObject NetworkManager::getJsonSync(
    const QUrl& url,
    const QHash<QString, QString>& headers,
    int timeoutMs,
    bool http2Allowed)
{
    QNetworkAccessManager* nam = threadLocalNam();
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, http2Allowed);
    applyHeaders(request, headers);
    return parseJsonObject(waitForReply(
        nam->get(request),
        timeoutMs > 0 ? timeoutMs : m_defaultTimeout));
}

QString NetworkManager::getStringSync(
    const QUrl& url,
    const QHash<QString, QString>& headers,
    int timeoutMs,
    bool http2Allowed)
{
    QNetworkAccessManager* nam = threadLocalNam();
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, http2Allowed);
    applyHeaders(request, headers);
    return QString::fromUtf8(waitForReply(
        nam->get(request),
        timeoutMs > 0 ? timeoutMs : m_defaultTimeout));
}

QJsonObject NetworkManager::postFormSync(
    const QUrl& url,
    const QByteArray& formData,
    const QHash<QString, QString>& headers,
    int timeoutMs,
    bool http2Allowed)
{
    QNetworkAccessManager* nam = threadLocalNam();
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, http2Allowed);
    applyHeaders(request, headers);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    return parseJsonObject(waitForReply(
        nam->post(request, formData),
        timeoutMs > 0 ? timeoutMs : m_defaultTimeout));
}

QJsonObject NetworkManager::postJsonSync(
    const QUrl& url,
    const QJsonObject& body,
    const QHash<QString, QString>& headers,
    int timeoutMs,
    bool http2Allowed)
{
    QNetworkAccessManager* nam = threadLocalNam();
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, http2Allowed);
    applyHeaders(request, headers);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray data = QJsonDocument(body).toJson();
    return parseJsonObject(waitForReply(
        nam->post(request, data),
        timeoutMs > 0 ? timeoutMs : m_defaultTimeout));
}

QFuture<QJsonObject> NetworkManager::getJson(
    const QUrl& url,
    const QHash<QString, QString>& headers)
{
    return QtConcurrent::run([this, url, headers]() {
        return getJsonSync(url, headers);
    });
}

QFuture<QString> NetworkManager::getString(
    const QUrl& url,
    const QHash<QString, QString>& headers)
{
    return QtConcurrent::run([this, url, headers]() {
        return getStringSync(url, headers);
    });
}

QFuture<QJsonObject> NetworkManager::postForm(
    const QUrl& url,
    const QByteArray& formData,
    const QHash<QString, QString>& headers)
{
    return QtConcurrent::run([this, url, formData, headers]() {
        return postFormSync(url, formData, headers);
    });
}

QFuture<QJsonObject> NetworkManager::postJson(
    const QUrl& url,
    const QJsonObject& body,
    const QHash<QString, QString>& headers)
{
    return QtConcurrent::run([this, url, body, headers]() {
        return postJsonSync(url, body, headers);
    });
}
