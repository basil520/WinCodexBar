#include "CodexSystemAccountObserver.h"
#include "CodexHomeScope.h"
#include "../../models/CodexUsageResponse.h"

#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

CodexSystemAccountObserver::CodexSystemAccountObserver()
{
}

std::optional<ObservedSystemCodexAccount> CodexSystemAccountObserver::loadSystemAccount(
    const QHash<QString, QString>& env) const
{
    auto account = loadAuthBackedCodexAccount(env);

    QString rawEmail = normalizeEmail(account.email);
    if (rawEmail.isEmpty()) {
        return std::nullopt;
    }

    ObservedSystemCodexAccount observed;
    observed.email = rawEmail.toLower();
    observed.workspaceLabel = QString();
    observed.workspaceAccountId = account.identity.type() == CodexIdentityType::ProviderAccount
        ? account.identity.accountId() : QString();
    observed.codexHomePath = CodexHomeScope::ambientHomeURL(env);
    observed.observedAt = QDateTime::currentDateTime();
    observed.identity = account.identity;

    return observed;
}

CodexSystemAccountObserver::AuthBackedAccount CodexSystemAccountObserver::loadAuthBackedCodexAccount(
    const QHash<QString, QString>& env) const
{
    auto credentials = CodexOAuthCredentials::load(env);
    if (!credentials.has_value()) {
        return {QString(), CodexIdentity::unresolved()};
    }

    if (credentials->idToken.isEmpty()) {
        return {QString(), CodexIdentity::unresolved()};
    }

    QJsonObject payload = parseJWTPayload(credentials->idToken);
    if (payload.isEmpty()) {
        return {QString(), CodexIdentity::unresolved()};
    }

    QJsonObject profile = payload.value("https://api.openai.com/profile").toObject();
    QString email = payload.value("email").toString().trimmed();
    if (email.isEmpty()) {
        email = profile.value("email").toString().trimmed();
    }

    QString accountId = credentials->accountId.trimmed();
    CodexIdentity identity = CodexIdentityResolver::resolve(accountId, email);

    return {email, identity};
}

QString CodexSystemAccountObserver::normalizeEmail(const QString& email)
{
    return email.trimmed().toLower();
}
