#include "CookieImporter.h"
#include "BrowserDetection.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkCookie>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QVariant>

#include <optional>

#include <windows.h>
#include <wincrypt.h>
#include <bcrypt.h>

namespace {

bool hostMatchesDomain(QString host, QString domain) {
    host = host.trimmed().toLower();
    domain = domain.trimmed().toLower();
    while (host.startsWith('.')) host.remove(0, 1);
    while (domain.startsWith('.')) domain.remove(0, 1);
    if (host.isEmpty() || domain.isEmpty()) return false;
    return host == domain
        || host.endsWith("." + domain)
        || domain.endsWith("." + host);
}

bool hostMatchesAnyDomain(const QString& host, const QStringList& domains) {
    for (const auto& domain : domains) {
        if (hostMatchesDomain(host, domain)) return true;
    }
    return false;
}

QString copyDatabase(const QString& sourcePath) {
    if (!QFileInfo::exists(sourcePath)) return {};
    QString target = QDir::tempPath() + "/wincodexbar-cookies-"
        + QUuid::createUuid().toString(QUuid::Id128) + ".sqlite";
    if (!QFile::copy(sourcePath, target)) {
        return {};
    }
    return target;
}

QByteArray decryptDpapi(const QByteArray& encrypted) {
    if (encrypted.isEmpty()) return {};

    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(encrypted.constData()));
    in.cbData = static_cast<DWORD>(encrypted.size());

    DATA_BLOB out;
    ZeroMemory(&out, sizeof(out));
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out)) {
        return {};
    }

    QByteArray decrypted(reinterpret_cast<const char*>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return decrypted;
}

std::optional<QByteArray> chromiumMasterKey(CookieImporter::Browser browser) {
    QString path = BrowserDetection::localStatePath(browser);
    if (path.isEmpty()) return std::nullopt;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return std::nullopt;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }

    QString encoded = doc.object().value("os_crypt").toObject()
        .value("encrypted_key").toString();
    if (encoded.isEmpty()) return std::nullopt;

    QByteArray encrypted = QByteArray::fromBase64(encoded.toUtf8());
    if (encrypted.startsWith("DPAPI")) {
        encrypted.remove(0, 5);
    }

    QByteArray key = decryptDpapi(encrypted);
    if (key.isEmpty()) return std::nullopt;
    return key;
}

QByteArray decryptAesGcm(const QByteArray& key, const QByteArray& encrypted) {
    if (key.isEmpty() || encrypted.size() < 3 + 12 + 16) return {};
    if (!encrypted.startsWith("v10") && !encrypted.startsWith("v11")) return {};

    QByteArray nonce = encrypted.mid(3, 12);
    QByteArray payload = encrypted.mid(15);
    if (payload.size() <= 16) return {};
    QByteArray cipherText = payload.left(payload.size() - 16);
    QByteArray tag = payload.right(16);
    QByteArray plain(cipherText.size(), Qt::Uninitialized);

    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_KEY_HANDLE keyHandle = nullptr;
    QByteArray keyObject;
    DWORD cbData = 0;
    DWORD keyObjectLength = 0;
    ULONG plainLength = 0;
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (status < 0) goto cleanup;

    status = BCryptSetProperty(alg,
                               BCRYPT_CHAINING_MODE,
                               reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)),
                               static_cast<ULONG>((wcslen(BCRYPT_CHAIN_MODE_GCM) + 1) * sizeof(wchar_t)),
                               0);
    if (status < 0) goto cleanup;

    status = BCryptGetProperty(alg,
                               BCRYPT_OBJECT_LENGTH,
                               reinterpret_cast<PUCHAR>(&keyObjectLength),
                               sizeof(keyObjectLength),
                               &cbData,
                               0);
    if (status < 0) goto cleanup;

    keyObject.resize(static_cast<int>(keyObjectLength));
    status = BCryptGenerateSymmetricKey(
        alg,
        &keyHandle,
        reinterpret_cast<PUCHAR>(keyObject.data()),
        keyObjectLength,
        reinterpret_cast<PUCHAR>(const_cast<char*>(key.constData())),
        static_cast<ULONG>(key.size()),
        0);
    if (status < 0) goto cleanup;

    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = reinterpret_cast<PUCHAR>(nonce.data());
    authInfo.cbNonce = static_cast<ULONG>(nonce.size());
    authInfo.pbTag = reinterpret_cast<PUCHAR>(tag.data());
    authInfo.cbTag = static_cast<ULONG>(tag.size());

    status = BCryptDecrypt(
        keyHandle,
        reinterpret_cast<PUCHAR>(cipherText.data()),
        static_cast<ULONG>(cipherText.size()),
        &authInfo,
        nullptr,
        0,
        reinterpret_cast<PUCHAR>(plain.data()),
        static_cast<ULONG>(plain.size()),
        &plainLength,
        0);
    if (status < 0) {
        plain.clear();
        goto cleanup;
    }
    plain.resize(static_cast<int>(plainLength));

cleanup:
    if (keyHandle) BCryptDestroyKey(keyHandle);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    return plain;
}

QByteArray decryptChromiumCookie(CookieImporter::Browser browser,
                                 const QByteArray& value,
                                 const QByteArray& encryptedValue,
                                 const std::optional<QByteArray>& masterKey)
{
    if (!value.isEmpty()) return value;
    if (encryptedValue.isEmpty()) return {};

    if (encryptedValue.startsWith("v20")) {
        // v20 is App-Bound Encryption (Chrome 127+, Edge 127+)
        // This requires system-level access and cannot be decrypted by third-party apps
        static bool warned = false;
        if (!warned) {
            qDebug() << "[CookieImporter] v20 (App-Bound Encryption) detected. "
                     << "Modern browsers use this encryption which prevents automatic cookie import. "
                     << "Please manually configure cookies in settings.";
            warned = true;
        }
        return {};
    }

    if ((encryptedValue.startsWith("v10") || encryptedValue.startsWith("v11")) && masterKey.has_value()) {
        return decryptAesGcm(*masterKey, encryptedValue);
    }

    Q_UNUSED(browser)
    return decryptDpapi(encryptedValue);
}

QDateTime chromiumExpiry(qint64 chromeTime) {
    if (chromeTime <= 0) return {};
    constexpr qint64 windowsToUnixSeconds = 11644473600LL;
    qint64 unixSeconds = chromeTime / 1000000LL - windowsToUnixSeconds;
    if (unixSeconds <= 0) return {};
    return QDateTime::fromSecsSinceEpoch(unixSeconds, Qt::UTC);
}

QVector<QNetworkCookie> importChromiumCookies(
    CookieImporter::Browser browser,
    const QString& dbPath,
    const QStringList& domains,
    const std::optional<QByteArray>& masterKey)
{
    QVector<QNetworkCookie> cookies;
    QString copy = copyDatabase(dbPath);
    if (copy.isEmpty()) {
        qDebug() << "[CookieImporter] Failed to copy database:" << dbPath;
        return cookies;
    }

    int totalRows = 0;
    int domainMatches = 0;
    int decryptSuccess = 0;
    int decryptFail = 0;

    QString connectionName = "wincodexbar-cookies-" + QUuid::createUuid().toString(QUuid::Id128);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(copy);
        if (db.open()) {
            QSqlQuery query(db);
            if (query.exec("SELECT host_key, name, value, encrypted_value, path, expires_utc, is_secure, is_httponly FROM cookies")) {
                while (query.next()) {
                    totalRows++;
                    QString host = query.value(0).toString();
                    if (!hostMatchesAnyDomain(host, domains)) continue;
                    domainMatches++;

                    QByteArray name = query.value(1).toByteArray();
                    QByteArray value = query.value(2).toByteArray();
                    QByteArray encrypted = query.value(3).toByteArray();
                    QByteArray decrypted = decryptChromiumCookie(browser, value, encrypted, masterKey);
                    if (name.isEmpty() || decrypted.isEmpty()) {
                        decryptFail++;
                        continue;
                    }
                    decryptSuccess++;

                    QNetworkCookie cookie(name, decrypted);
                    cookie.setDomain(host);
                    QString path = query.value(4).toString();
                    cookie.setPath(path.isEmpty() ? "/" : path);
                    auto expires = chromiumExpiry(query.value(5).toLongLong());
                    if (expires.isValid()) cookie.setExpirationDate(expires);
                    cookie.setSecure(query.value(6).toInt() != 0);
                    cookie.setHttpOnly(query.value(7).toInt() != 0);
                    cookies.append(cookie);
                }
            } else {
                qDebug() << "[CookieImporter] Query failed:" << db.lastError().text();
            }
        } else {
            qDebug() << "[CookieImporter] Failed to open database:" << copy << db.lastError().text();
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    QFile::remove(copy);

    qDebug() << "[CookieImporter] Stats: total=" << totalRows << "domainMatch=" << domainMatches
             << "decryptOk=" << decryptSuccess << "decryptFail=" << decryptFail;

    return cookies;
}

QVector<QNetworkCookie> importFirefoxCookies(const QString& dbPath, const QStringList& domains) {
    QVector<QNetworkCookie> cookies;
    QString copy = copyDatabase(dbPath);
    if (copy.isEmpty()) return cookies;

    QString connectionName = "wincodexbar-firefox-cookies-" + QUuid::createUuid().toString(QUuid::Id128);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(copy);
        if (db.open()) {
            QSqlQuery query(db);
            if (query.exec("SELECT host, name, value, path, expiry, isSecure, isHttpOnly FROM moz_cookies")) {
                while (query.next()) {
                    QString host = query.value(0).toString();
                    if (!hostMatchesAnyDomain(host, domains)) continue;

                    QByteArray name = query.value(1).toByteArray();
                    QByteArray value = query.value(2).toByteArray();
                    if (name.isEmpty() || value.isEmpty()) continue;

                    QNetworkCookie cookie(name, value);
                    cookie.setDomain(host);
                    QString path = query.value(3).toString();
                    cookie.setPath(path.isEmpty() ? "/" : path);
                    qint64 expiry = query.value(4).toLongLong();
                    if (expiry > 0) cookie.setExpirationDate(QDateTime::fromSecsSinceEpoch(expiry, Qt::UTC));
                    cookie.setSecure(query.value(5).toInt() != 0);
                    cookie.setHttpOnly(query.value(6).toInt() != 0);
                    cookies.append(cookie);
                }
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    QFile::remove(copy);
    return cookies;
}

} // namespace

QVector<QNetworkCookie> CookieImporter::importCookies(
    Browser browser,
    const QStringList& domains,
    QObject* parent)
{
    Q_UNUSED(parent)
    QVector<QNetworkCookie> result;
    if (domains.isEmpty()) return result;

    if (browser == Firefox) {
        for (const auto& profile : BrowserDetection::profilePaths(browser)) {
            QString dbPath = profile + "/cookies.sqlite";
            qDebug() << "[CookieImporter] Firefox profile:" << profile << "db exists:" << QFileInfo::exists(dbPath);
            result += importFirefoxCookies(dbPath, domains);
        }
        return result;
    }

    auto masterKey = chromiumMasterKey(browser);
    qDebug() << "[CookieImporter] Chromium browser" << static_cast<int>(browser)
             << "master key available:" << masterKey.has_value();
    for (const auto& profile : BrowserDetection::profilePaths(browser)) {
        QString dbPath = profile + "/Network/Cookies";
        if (!QFileInfo::exists(dbPath)) dbPath = profile + "/Cookies";
        qDebug() << "[CookieImporter] Chromium profile:" << profile << "db:" << dbPath
                 << "exists:" << QFileInfo::exists(dbPath);
        result += importChromiumCookies(browser, dbPath, domains, masterKey);
    }
    return result;
}

QVector<CookieImporter::Browser> CookieImporter::importOrder() {
    return { Edge, Firefox, Chrome, Brave, Opera, Vivaldi };
}

bool CookieImporter::isBrowserInstalled(Browser browser) {
    return !BrowserDetection::profilePaths(browser).isEmpty();
}

bool CookieImporter::hasUsableProfileData(Browser browser) {
    return !BrowserDetection::profilePaths(browser).isEmpty();
}
