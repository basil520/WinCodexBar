#pragma once

#include <QString>
#include <QObject>
#include <optional>

class CredentialAccessGate : public QObject {
    Q_OBJECT
public:
    enum class AccessPolicy {
        NeverPrompt,
        OnlyOnUserAction,
        AlwaysAllow
    };

    enum class AccessResult {
        Accessible,
        NeedsPrompt,
        Denied
    };

    explicit CredentialAccessGate(QObject* parent = nullptr);

    AccessResult preflight(const QString& target);
    std::optional<QByteArray> readWithPolicy(const QString& target, AccessPolicy policy);
    std::optional<QByteArray> readCached(const QString& target);
    bool write(const QString& target, const QByteArray& secret);
    bool remove(const QString& target);

private:
    static constexpr int CACHE_TTL_SECS = 1800;
};
