#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>
#include <QUuid>
#include <optional>

class ManagedCodexAccount {
public:
    QString id;
    QString email;
    QString providerAccountId;
    QString workspaceLabel;
    QString workspaceAccountId;
    QString managedHomePath;
    QDateTime createdAt;
    QDateTime updatedAt;
    QDateTime lastAuthenticatedAt;

    ManagedCodexAccount();

    static ManagedCodexAccount create(const QString& email, const QString& homePath);
    void updateTimestamp();

    bool operator==(const ManagedCodexAccount& other) const;
    bool operator!=(const ManagedCodexAccount& other) const;

    static QString normalizeEmail(const QString& email);
    static QString normalizeProviderAccountID(const QString& id);
};

class ManagedCodexAccountStore {
public:
    ManagedCodexAccountStore();

    QVector<ManagedCodexAccount> loadAccounts();
    void saveAccounts(const QVector<ManagedCodexAccount>& accounts);
    std::optional<ManagedCodexAccount> account(const QString& id);
    void addAccount(const ManagedCodexAccount& account);
    void updateAccount(const ManagedCodexAccount& account);
    void removeAccount(const QString& id);

    QString storePath() const;

private:
    QString m_storePath;

    static QString defaultStorePath();
};
