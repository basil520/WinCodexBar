#include "ManagedCodexAccount.h"
#include "../../models/CodexUsageResponse.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QFileInfo>

ManagedCodexAccount::ManagedCodexAccount()
{
}

ManagedCodexAccount ManagedCodexAccount::create(const QString& email, const QString& homePath)
{
    ManagedCodexAccount account;
    account.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    account.email = normalizeEmail(email);
    account.managedHomePath = homePath;
    account.createdAt = QDateTime::currentDateTime();
    account.updatedAt = account.createdAt;
    return account;
}

void ManagedCodexAccount::updateTimestamp()
{
    updatedAt = QDateTime::currentDateTime();
}

bool ManagedCodexAccount::operator==(const ManagedCodexAccount& other) const
{
    return id == other.id;
}

bool ManagedCodexAccount::operator!=(const ManagedCodexAccount& other) const
{
    return !(*this == other);
}

QString ManagedCodexAccount::normalizeEmail(const QString& email)
{
    return email.trimmed().toLower();
}

QString ManagedCodexAccount::normalizeProviderAccountID(const QString& id)
{
    return id.trimmed();
}

ManagedCodexAccountStore::ManagedCodexAccountStore()
    : m_storePath(defaultStorePath())
{
}

QVector<ManagedCodexAccount> ManagedCodexAccountStore::loadAccounts() const
{
    QFile file(m_storePath);
    if (!file.exists()) return {};
    if (!file.open(QIODevice::ReadOnly)) return {};

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return {};

    if (doc.isArray()) {
        return migrateV1(doc.array());
    }

    QJsonObject root = doc.object();
    int version = root.value("version").toInt(0);
    QJsonArray array = root.value("accounts").toArray();

    if (version <= 0) return {};
    if (version == 1) return migrateV1(array);
    if (version == 2) return migrateV2(array);

    if (version > currentVersion()) return {};
    return migrateV2(array);
}

int ManagedCodexAccountStore::currentVersion()
{
    return 2;
}

QVector<ManagedCodexAccount> ManagedCodexAccountStore::migrateV1(const QJsonArray& array) const
{
    QVector<ManagedCodexAccount> accounts;
    for (const auto& value : array) {
        QJsonObject obj = value.toObject();
        ManagedCodexAccount account;
        account.id = obj.value("id").toString();
        account.email = obj.value("email").toString();
        account.providerAccountId = obj.value("providerAccountId").toString();
        account.workspaceLabel = obj.value("workspaceLabel").toString();
        account.workspaceAccountId = obj.value("workspaceAccountId").toString();
        account.managedHomePath = obj.value("managedHomePath").toString();
        account.createdAt = QDateTime::fromString(obj.value("createdAt").toString(), Qt::ISODate);
        account.updatedAt = QDateTime::fromString(obj.value("updatedAt").toString(), Qt::ISODate);
        account.lastAuthenticatedAt = QDateTime::fromString(obj.value("lastAuthenticatedAt").toString(), Qt::ISODate);

        if (account.providerAccountId.isEmpty() && !account.managedHomePath.isEmpty()) {
            QHash<QString, QString> scopedEnv;
            scopedEnv["CODEX_HOME"] = account.managedHomePath;
            auto creds = CodexOAuthCredentials::load(scopedEnv);
            if (creds.has_value() && !creds->accountId.isEmpty()) {
                account.providerAccountId = ManagedCodexAccount::normalizeProviderAccountID(creds->accountId);
            }
        }

        accounts.append(account);
    }
    return accounts;
}

QVector<ManagedCodexAccount> ManagedCodexAccountStore::migrateV2(const QJsonArray& array) const
{
    QVector<ManagedCodexAccount> accounts;
    for (const auto& value : array) {
        QJsonObject obj = value.toObject();
        ManagedCodexAccount account;
        account.id = obj.value("id").toString();
        account.email = obj.value("email").toString();
        account.providerAccountId = obj.value("providerAccountId").toString();
        account.workspaceLabel = obj.value("workspaceLabel").toString();
        account.workspaceAccountId = obj.value("workspaceAccountId").toString();
        account.managedHomePath = obj.value("managedHomePath").toString();
        account.createdAt = QDateTime::fromString(obj.value("createdAt").toString(), Qt::ISODate);
        account.updatedAt = QDateTime::fromString(obj.value("updatedAt").toString(), Qt::ISODate);
        account.lastAuthenticatedAt = QDateTime::fromString(obj.value("lastAuthenticatedAt").toString(), Qt::ISODate);
        accounts.append(account);
    }
    return accounts;
}

void ManagedCodexAccountStore::saveAccounts(const QVector<ManagedCodexAccount>& accounts)
{
    QJsonArray array;
    for (const auto& account : accounts) {
        QJsonObject obj;
        obj["id"] = account.id;
        obj["email"] = account.email;
        obj["providerAccountId"] = account.providerAccountId;
        obj["workspaceLabel"] = account.workspaceLabel;
        obj["workspaceAccountId"] = account.workspaceAccountId;
        obj["managedHomePath"] = account.managedHomePath;
        obj["createdAt"] = account.createdAt.toString(Qt::ISODate);
        obj["updatedAt"] = account.updatedAt.toString(Qt::ISODate);
        if (account.lastAuthenticatedAt.isValid()) {
            obj["lastAuthenticatedAt"] = account.lastAuthenticatedAt.toString(Qt::ISODate);
        }
        array.append(obj);
    }

    QJsonObject root;
    root["version"] = currentVersion();
    root["accounts"] = array;

    QDir().mkpath(QFileInfo(m_storePath).absolutePath());
    QFile file(m_storePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        file.close();
        QFile::setPermissions(m_storePath, QFile::ReadOwner | QFile::WriteOwner);
    }
}

std::optional<ManagedCodexAccount> ManagedCodexAccountStore::account(const QString& id) const
{
    auto accounts = loadAccounts();
    for (const auto& account : accounts) {
        if (account.id == id) {
            return account;
        }
    }
    return std::nullopt;
}

void ManagedCodexAccountStore::addAccount(const ManagedCodexAccount& account)
{
    auto accounts = loadAccounts();
    accounts.append(account);
    saveAccounts(accounts);
}

void ManagedCodexAccountStore::updateAccount(const ManagedCodexAccount& account)
{
    auto accounts = loadAccounts();
    for (int i = 0; i < accounts.size(); ++i) {
        if (accounts[i].id == account.id) {
            accounts[i] = account;
            break;
        }
    }
    saveAccounts(accounts);
}

void ManagedCodexAccountStore::removeAccount(const QString& id)
{
    auto accounts = loadAccounts();
    accounts.erase(std::remove_if(accounts.begin(), accounts.end(),
        [&id](const ManagedCodexAccount& a) { return a.id == id; }),
        accounts.end());
    saveAccounts(accounts);
}

QString ManagedCodexAccountStore::storePath() const
{
    return m_storePath;
}

QString ManagedCodexAccountStore::defaultStorePath()
{
    QString overridePath = qEnvironmentVariable("CODEXBAR_MANAGED_CODEX_ACCOUNTS_PATH").trimmed();
    if (!overridePath.isEmpty()) return overridePath;

    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) base = QDir::homePath() + "/.codexbar";
    return base + "/managed-codex-accounts.json";
}
