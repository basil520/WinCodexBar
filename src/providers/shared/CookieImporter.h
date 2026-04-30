#pragma once

#include <QObject>
#include <QNetworkCookie>
#include <QVector>
#include <QStringList>

class CookieImporter : public QObject {
    Q_OBJECT
public:
    enum Browser {
        Chrome,
        Edge,
        Firefox,
        Brave,
        Opera,
        Vivaldi
    };

    enum CookieStrategy {
        Cached,
        EdgeDPAPI,
        FirefoxSQLite,
        LegacyChromiumDPAPI,
        AppOwnedWebView,
        Manual,
        RemoteDebugNonDefault
    };

    static QVector<QNetworkCookie> importCookies(
        Browser browser,
        const QStringList& domains,
        QObject* parent = nullptr);

    static QVector<Browser> importOrder();
    static bool isBrowserInstalled(Browser browser);
    static bool hasUsableProfileData(Browser browser);
};
