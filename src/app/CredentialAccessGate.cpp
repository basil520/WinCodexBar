#include "CredentialAccessGate.h"

CredentialAccessGate::CredentialAccessGate(QObject* parent) : QObject(parent) {}

CredentialAccessGate::AccessResult CredentialAccessGate::preflight(const QString& target) {
    Q_UNUSED(target)
    return AccessResult::Accessible;
}

std::optional<QByteArray> CredentialAccessGate::readWithPolicy(const QString& target,
                                                                 AccessPolicy policy) {
    Q_UNUSED(target)
    Q_UNUSED(policy)
    return std::nullopt;
}

std::optional<QByteArray> CredentialAccessGate::readCached(const QString& target) {
    Q_UNUSED(target)
    return std::nullopt;
}

bool CredentialAccessGate::write(const QString& target, const QByteArray& secret) {
    Q_UNUSED(target)
    Q_UNUSED(secret)
    return false;
}

bool CredentialAccessGate::remove(const QString& target) {
    Q_UNUSED(target)
    return false;
}
