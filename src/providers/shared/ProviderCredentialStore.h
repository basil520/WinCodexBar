#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <memory>
#include <optional>

class ProviderCredentialBackend {
public:
    virtual ~ProviderCredentialBackend() = default;
    virtual bool write(const QString& target, const QString& username, const QByteArray& secret) = 0;
    virtual std::optional<QByteArray> read(const QString& target) = 0;
    virtual bool remove(const QString& target) = 0;
    virtual bool exists(const QString& target) = 0;
};

class ProviderCredentialStore {
public:
    static bool write(const QString& target, const QString& username, const QByteArray& secret);
    static std::optional<QByteArray> read(const QString& target);
    static bool remove(const QString& target);
    static bool exists(const QString& target);

    static void setBackendForTesting(std::shared_ptr<ProviderCredentialBackend> backend);
    static void resetBackendForTesting();
};

class InMemoryCredentialBackend : public ProviderCredentialBackend {
public:
    bool write(const QString& target, const QString& username, const QByteArray& secret) override;
    std::optional<QByteArray> read(const QString& target) override;
    bool remove(const QString& target) override;
    bool exists(const QString& target) override;

private:
    QHash<QString, QByteArray> m_values;
};
