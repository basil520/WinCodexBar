#pragma once

#include <QString>
#include <QHash>
#include <optional>

enum class CodexIdentityType {
    ProviderAccount,
    EmailOnly,
    Unresolved
};

class CodexIdentity {
public:
    CodexIdentity();
    
    static CodexIdentity providerAccount(const QString& id);
    static CodexIdentity emailOnly(const QString& normalizedEmail);
    static CodexIdentity unresolved();

    CodexIdentityType type() const;
    QString accountId() const;
    QString email() const;

    bool operator==(const CodexIdentity& other) const;
    bool operator!=(const CodexIdentity& other) const;

    static QString normalizeEmail(const QString& email);
    static QString normalizeAccountId(const QString& accountId);

private:
    CodexIdentityType m_type;
    QString m_accountId;
    QString m_email;

    CodexIdentity(CodexIdentityType type, const QString& accountId, const QString& email);
};

class CodexIdentityResolver {
public:
    static CodexIdentity resolve(const QString& accountId, const QString& email);
    static QString normalizeEmail(const QString& email);
    static QString normalizeAccountId(const QString& accountId);
};

class CodexIdentityMatcher {
public:
    static bool matches(const CodexIdentity& lhs, const CodexIdentity& rhs);
    static bool matches(const CodexIdentity& lhs, const CodexIdentity& rhs,
                        const QString& lhsEmail, const QString& rhsEmail);
    static CodexIdentity normalized(const CodexIdentity& identity, const QString& fallbackEmail);
    static QString selectionKey(const CodexIdentity& identity, const QString& fallbackEmail);
};

uint qHash(const CodexIdentity& identity, uint seed = 0);
