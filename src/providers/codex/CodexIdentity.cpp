#include "CodexIdentity.h"

CodexIdentity::CodexIdentity()
    : m_type(CodexIdentityType::Unresolved)
{
}

CodexIdentity::CodexIdentity(CodexIdentityType type, const QString& accountId, const QString& email)
    : m_type(type)
    , m_accountId(accountId)
    , m_email(email)
{
}

CodexIdentity CodexIdentity::providerAccount(const QString& id)
{
    return CodexIdentity(CodexIdentityType::ProviderAccount, id, QString());
}

CodexIdentity CodexIdentity::emailOnly(const QString& normalizedEmail)
{
    return CodexIdentity(CodexIdentityType::EmailOnly, QString(), normalizedEmail);
}

CodexIdentity CodexIdentity::unresolved()
{
    return CodexIdentity(CodexIdentityType::Unresolved, QString(), QString());
}

CodexIdentityType CodexIdentity::type() const
{
    return m_type;
}

QString CodexIdentity::accountId() const
{
    return m_accountId;
}

QString CodexIdentity::email() const
{
    return m_email;
}

bool CodexIdentity::operator==(const CodexIdentity& other) const
{
    if (m_type != other.m_type) return false;
    switch (m_type) {
    case CodexIdentityType::ProviderAccount:
        return m_accountId == other.m_accountId;
    case CodexIdentityType::EmailOnly:
        return m_email == other.m_email;
    case CodexIdentityType::Unresolved:
        return true;
    }
    return false;
}

bool CodexIdentity::operator!=(const CodexIdentity& other) const
{
    return !(*this == other);
}

QString CodexIdentity::normalizeEmail(const QString& email)
{
    return email.trimmed().toLower();
}

QString CodexIdentity::normalizeAccountId(const QString& accountId)
{
    return accountId.trimmed();
}

uint qHash(const CodexIdentity& identity, uint seed)
{
    switch (identity.type()) {
    case CodexIdentityType::ProviderAccount:
        return qHash(identity.accountId(), seed);
    case CodexIdentityType::EmailOnly:
        return qHash(identity.email(), seed);
    case CodexIdentityType::Unresolved:
        return qHash(0, seed);
    }
    return seed;
}

CodexIdentity CodexIdentityResolver::resolve(const QString& accountId, const QString& email)
{
    QString normalizedId = normalizeAccountId(accountId);
    if (!normalizedId.isEmpty()) {
        return CodexIdentity::providerAccount(normalizedId);
    }
    QString normalizedEmail = normalizeEmail(email);
    if (!normalizedEmail.isEmpty()) {
        return CodexIdentity::emailOnly(normalizedEmail);
    }
    return CodexIdentity::unresolved();
}

QString CodexIdentityResolver::normalizeEmail(const QString& email)
{
    return CodexIdentity::normalizeEmail(email);
}

QString CodexIdentityResolver::normalizeAccountId(const QString& accountId)
{
    return CodexIdentity::normalizeAccountId(accountId);
}

bool CodexIdentityMatcher::matches(const CodexIdentity& lhs, const CodexIdentity& rhs)
{
    if (lhs.type() != rhs.type()) return false;
    switch (lhs.type()) {
    case CodexIdentityType::ProviderAccount:
        return lhs.accountId() == rhs.accountId();
    case CodexIdentityType::EmailOnly:
        return lhs.email() == rhs.email();
    case CodexIdentityType::Unresolved:
        return false;
    }
    return false;
}

CodexIdentity CodexIdentityMatcher::normalized(const CodexIdentity& identity, const QString& fallbackEmail)
{
    switch (identity.type()) {
    case CodexIdentityType::ProviderAccount:
        return identity;
    case CodexIdentityType::EmailOnly:
        return CodexIdentityResolver::resolve(QString(), identity.email());
    case CodexIdentityType::Unresolved:
        return CodexIdentityResolver::resolve(QString(), fallbackEmail);
    }
    return identity;
}

QString CodexIdentityMatcher::selectionKey(const CodexIdentity& identity, const QString& fallbackEmail)
{
    CodexIdentity normalizedIdentity = normalized(identity, fallbackEmail);
    switch (normalizedIdentity.type()) {
    case CodexIdentityType::ProviderAccount:
        return "provider:" + normalizedIdentity.accountId();
    case CodexIdentityType::EmailOnly:
        return "email:" + normalizedIdentity.email();
    case CodexIdentityType::Unresolved:
        return "unresolved:" + fallbackEmail;
    }
    return "unresolved:" + fallbackEmail;
}
