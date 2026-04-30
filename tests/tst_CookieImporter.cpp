#include <QtTest/QtTest>

#include "../src/providers/shared/BrowserDetection.h"
#include "../src/providers/shared/CookieImporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkCookie>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>

#include <filesystem>

namespace {

bool ensureParentDirectory(const QString& path) {
    const QString absoluteParent = QDir::toNativeSeparators(QFileInfo(path).absolutePath());
    std::error_code ec;
    std::filesystem::path parent(absoluteParent.toStdWString());
    return std::filesystem::create_directories(parent, ec) || std::filesystem::exists(parent);
}

void createChromiumCookieDb(const QString& path) {
    QVERIFY2(ensureParentDirectory(path), qPrintable(QFileInfo(path).absolutePath()));
    QString sqlitePath = path + ".sqlite";
    QString connectionName = "chromium-test-" + QUuid::createUuid().toString(QUuid::Id128);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(sqlitePath);
        QVERIFY2(db.open(), qPrintable(db.lastError().text() + " " + sqlitePath));
        QSqlQuery q(db);
        QVERIFY(q.exec("CREATE TABLE cookies ("
                       "host_key TEXT, name TEXT, value TEXT, encrypted_value BLOB, "
                       "path TEXT, expires_utc INTEGER, is_secure INTEGER, is_httponly INTEGER)"));
        QVERIFY(q.exec("INSERT INTO cookies VALUES ('.cursor.com', 'session', 'abc123', X'', '/', 0, 1, 1)"));
        QVERIFY(q.exec("INSERT INTO cookies VALUES ('.example.com', 'other', 'nope', X'', '/', 0, 0, 0)"));
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    QFile::remove(path);
    QVERIFY2(QFile::copy(sqlitePath, path), qPrintable(path));
}

void createFirefoxCookieDb(const QString& path) {
    QVERIFY2(ensureParentDirectory(path), qPrintable(QFileInfo(path).absolutePath()));
    QString connectionName = "firefox-test-" + QUuid::createUuid().toString(QUuid::Id128);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(path);
        QVERIFY2(db.open(), qPrintable(db.lastError().text() + " " + path));
        QSqlQuery q(db);
        QVERIFY(q.exec("CREATE TABLE moz_cookies ("
                       "host TEXT, name TEXT, value TEXT, path TEXT, expiry INTEGER, "
                       "isSecure INTEGER, isHttpOnly INTEGER)"));
        QVERIFY(q.exec("INSERT INTO moz_cookies VALUES ('.claude.ai', 'sessionKey', 'sk-ant-test', '/', 4102444800, 1, 1)"));
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

} // namespace

class tst_CookieImporter : public QObject {
    Q_OBJECT
private slots:
    void importsPlaintextChromiumCookie() {
        qunsetenv("CODEXBAR_CHROME_USER_DATA_DIR");
        QString root = QDir::fromNativeSeparators(QDir::currentPath());
        QString dbPath = root + "/Cookies";
        QFile::remove(dbPath);
        QFile::remove(dbPath + ".sqlite");
        createChromiumCookieDb(dbPath);
        qputenv("CODEXBAR_CHROME_USER_DATA_DIR", root.toUtf8());

        QVERIFY(CookieImporter::isBrowserInstalled(CookieImporter::Chrome));
        QVERIFY(BrowserDetection::profilePaths(CookieImporter::Chrome).contains(root));

        auto cookies = CookieImporter::importCookies(CookieImporter::Chrome, {"cursor.com"});
        QCOMPARE(cookies.size(), 1);
        QCOMPARE(QString::fromUtf8(cookies.first().name()), QString("session"));
        QCOMPARE(QString::fromUtf8(cookies.first().value()), QString("abc123"));
        QVERIFY(cookies.first().isSecure());
        QVERIFY(cookies.first().isHttpOnly());

        auto misses = CookieImporter::importCookies(CookieImporter::Chrome, {"claude.ai"});
        QVERIFY(misses.isEmpty());

        QFile::remove(dbPath);
        QFile::remove(dbPath + ".sqlite");
    }

    void importsFirefoxCookie() {
        qunsetenv("CODEXBAR_FIREFOX_PROFILES_DIR");
        QString root = QDir::fromNativeSeparators(QDir::currentPath());
        QString profile = root + "/build";
        QFile::remove(profile + "/cookies.sqlite");
        createFirefoxCookieDb(profile + "/cookies.sqlite");
        qputenv("CODEXBAR_FIREFOX_PROFILES_DIR", root.toUtf8());

        QVERIFY(CookieImporter::isBrowserInstalled(CookieImporter::Firefox));
        auto cookies = CookieImporter::importCookies(CookieImporter::Firefox, {"claude.ai"});
        QCOMPARE(cookies.size(), 1);
        QCOMPARE(QString::fromUtf8(cookies.first().name()), QString("sessionKey"));
        QCOMPARE(QString::fromUtf8(cookies.first().value()), QString("sk-ant-test"));

        QFile::remove(profile + "/cookies.sqlite");
    }
};

QTEST_MAIN(tst_CookieImporter)
#include "tst_CookieImporter.moc"
