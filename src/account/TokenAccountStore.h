#pragma once

#include "TokenAccount.h"
#include <QObject>
#include <QHash>
#include <QReadWriteLock>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>

class SettingsStore;

/**
 * @brief Singleton store for TokenAccount CRUD, persistence, and queries.
 *
 * All operations are thread-safe (protected by QReadWriteLock).
 * Accounts are persisted to %AppData%/CodexBar/accounts.json.
 * Credential secrets are stored separately via ProviderCredentialStore.
 */
class TokenAccountStore : public QObject {
    Q_OBJECT
public:
    static TokenAccountStore* instance();

    // CRUD
    QString addAccount(const TokenAccount& account);
    bool updateAccount(const QString& accountId, const TokenAccount& account);
    bool removeAccount(const QString& accountId);
    std::optional<TokenAccount> account(const QString& accountId) const;

    // Queries
    QList<TokenAccount> accountsForProvider(const QString& providerId) const;
    QList<TokenAccount> visibleAccountsForProvider(const QString& providerId) const;
    QList<TokenAccount> allAccounts() const;
    QStringList accountIdsForProvider(const QString& providerId) const;
    bool hasAccount(const QString& accountId) const;

    // Default account (used when no explicit selection)
    QString defaultAccountId(const QString& providerId) const;
    void setDefaultAccountId(const QString& providerId, const QString& accountId);

    // Migration from legacy single-account config
    void migrateFromLegacy(SettingsStore* settings);

    // Persistence
    bool loadFromDisk();
    bool saveToDisk();

    // Credential storage helpers (wraps ProviderCredentialStore)
    bool saveAccountCredentials(const QString& accountId, const TokenAccountCredentials& creds);
    TokenAccountCredentials loadAccountCredentials(const QString& accountId) const;
    bool removeAccountCredentials(const QString& accountId);

    // Utility
    int accountCount() const;
    int accountCountForProvider(const QString& providerId) const;

signals:
    void accountAdded(const QString& accountId);
    void accountUpdated(const QString& accountId);
    void accountRemoved(const QString& accountId);
    void accountsChanged(const QString& providerId);
    void defaultAccountChanged(const QString& providerId, const QString& accountId);

private:
    explicit TokenAccountStore(QObject* parent = nullptr);
    QString storagePath() const;
    QString credentialTargetFor(const QString& accountId, const QString& type) const;

    QHash<QString, TokenAccount> m_accounts; // key = accountId
    QHash<QString, QString> m_defaultAccountIds; // providerId -> accountId
    mutable QReadWriteLock m_lock;
    QString m_storagePath;
    bool m_loaded = false;
};
