#include "ManagedCodexAccount.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>

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
    QVector<ManagedCodexAccount> accounts;

    QFile file(m_storePath);
    if (!file.exists()) return accounts;
    if (!file.open(QIODevice::ReadOnly)) return accounts;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return accounts;

    QJsonArray array = doc.array();
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

    QDir().mkpath(QFileInfo(m_storePath).absolutePath());
    QFile file(m_storePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
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
