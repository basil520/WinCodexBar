#include "CodexOpenAIWorkspaceResolver.h"
#include "../../network/NetworkManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

const QString CodexOpenAIWorkspaceResolver::AccountsURL = "https://chatgpt.com/backend-api/accounts";

bool CodexOpenAIWorkspaceIdentity::operator==(const CodexOpenAIWorkspaceIdentity& other) const
{
    return workspaceAccountID == other.workspaceAccountID && workspaceLabel == other.workspaceLabel;
}

bool CodexOpenAIWorkspaceIdentity::operator!=(const CodexOpenAIWorkspaceIdentity& other) const
{
    return !(*this == other);
}

QString CodexOpenAIWorkspaceIdentity::normalizeWorkspaceAccountID(const QString& value)
{
    return value.trimmed().toLower();
}

QString CodexOpenAIWorkspaceIdentity::normalizeWorkspaceLabel(const QString& value)
{
    QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? QString() : trimmed;
}

std::optional<CodexOpenAIWorkspaceIdentity> CodexOpenAIWorkspaceResolver::resolve(
    const CodexOAuthCredentials& credentials,
    const QHash<QString, QString>& env)
{
    Q_UNUSED(env);

    QString workspaceAccountID = normalizeWorkspaceAccountID(credentials.accountId);
    if (workspaceAccountID.isEmpty()) {
        return std::nullopt;
    }

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + credentials.accessToken;
    headers["User-Agent"] = "codex-cli";
    headers["Accept"] = "application/json";
    headers["ChatGPT-Account-Id"] = workspaceAccountID;

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl(AccountsURL), headers, 20000);

    if (json.isEmpty()) {
        return CodexOpenAIWorkspaceIdentity{workspaceAccountID, QString()};
    }

    QJsonArray items = json.value("items").toArray();
    for (const auto& item : items) {
        QJsonObject obj = item.toObject();
        QString id = obj.value("id").toString();
        if (normalizeWorkspaceAccountID(id) == workspaceAccountID) {
            AccountItem account;
            account.id = id;
            account.name = obj.value("name").toString();
            return CodexOpenAIWorkspaceIdentity{
                workspaceAccountID,
                resolveWorkspaceLabel(account)
            };
        }
    }

    return CodexOpenAIWorkspaceIdentity{workspaceAccountID, QString()};
}

QString CodexOpenAIWorkspaceResolver::normalizeWorkspaceAccountID(const QString& value)
{
    return CodexOpenAIWorkspaceIdentity::normalizeWorkspaceAccountID(value);
}

QString CodexOpenAIWorkspaceResolver::resolveWorkspaceLabel(const AccountItem& account)
{
    QString name = account.name.trimmed();
    if (!name.isEmpty()) return name;
    return "Personal";
}
