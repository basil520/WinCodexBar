#include "ManagedCodexAccountService.h"
#include "CodexLoginRunner.h"
#include "../../models/CodexUsageResponse.h"
#include "CodexHomeScope.h"

#include <QDebug>
#include <QDir>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>

namespace {

QString sanitizeLoginOutput(const QString& raw)
{
    if (raw.isEmpty()) return raw;
    QString sanitized = raw;

    // Truncate to 500 chars
    if (sanitized.length() > 500)
        sanitized = sanitized.left(500) + QStringLiteral("...");

    // Redact sensitive patterns
    sanitized.replace(QRegularExpression(R"(sk-[a-zA-Z0-9]{20,})"), "[REDACTED]");
    sanitized.replace(QRegularExpression(R"(Bearer\s+\S+)"), "Bearer [REDACTED]");
    sanitized.replace(QRegularExpression(R"("access_token":\s*"[^"]+")"), "\"access_token\":\"[REDACTED]\"");
    sanitized.replace(QRegularExpression(R"("refresh_token":\s*"[^"]+")"), "\"refresh_token\":\"[REDACTED]\"");
    sanitized.replace(QRegularExpression(R"("id_token":\s*"[^"]+")"), "\"id_token\":\"[REDACTED]\"");

    return sanitized;
}

} // namespace

ManagedCodexAccountService::ManagedCodexAccountService(const QHash<QString, QString>& env, QObject* parent)
    : QObject(parent)
    , m_env(env)
    , m_isAuthenticating(false)
    , m_isRemoving(false)
{
    refresh();
}

ManagedCodexAccountService::~ManagedCodexAccountService()
{
    if (m_loginRunner) {
        m_loginRunner->cancel();
        delete m_loginRunner;
    }
}

QVector<CodexVisibleAccount> ManagedCodexAccountService::visibleAccounts() const
{
    QVector<CodexVisibleAccount> accounts;

    // Add stored managed accounts
    for (const auto& stored : m_snapshot.storedAccounts) {
        CodexVisibleAccount visible;
        visible.id = stored.id;
        visible.email = stored.email;
        visible.workspaceLabel = stored.workspaceLabel;
        visible.isLive = false;
        visible.isActive = (stored.id == m_activeAccountID);
        visible.storedAccountID = stored.id;
        visible.displayName = resolveDisplayName(stored);
        accounts.append(visible);
    }

    // Add live system account if not already in stored accounts
    if (m_snapshot.liveSystemAccount.has_value()) {
        bool foundInStored = false;
        for (const auto& visible : accounts) {
            if (visible.isLive) {
                foundInStored = true;
                break;
            }
        }

        if (!foundInStored) {
            CodexVisibleAccount visible;
            visible.id = "live-system";
            visible.email = m_snapshot.liveSystemAccount->email;
            visible.workspaceLabel = m_snapshot.liveSystemAccount->workspaceLabel;
            visible.isLive = true;
            visible.isActive = (m_activeAccountID.isEmpty() || m_activeAccountID == "live-system");
            visible.storedAccountID = QString();
            visible.displayName = resolveDisplayName(*m_snapshot.liveSystemAccount);
            accounts.prepend(visible);
        }
    }

    return accounts;
}

QString ManagedCodexAccountService::activeAccountID() const
{
    if (m_activeAccountID.isEmpty()) {
        return "live-system";
    }
    return m_activeAccountID;
}

QString ManagedCodexAccountService::liveAccountID() const
{
    if (m_snapshot.liveSystemAccount.has_value()) {
        return "live-system";
    }
    return QString();
}

bool ManagedCodexAccountService::addAccount(const QString& email, const QString& homePath)
{
    if (m_isAuthenticating || m_isRemoving) {
        return false;
    }

    m_isAuthenticating = true;
    m_authenticatingAccountID = QString();
    resetAuthenticationStatus();
    m_authMessage = QStringLiteral("Adding Codex account...");
    emit authenticationStarted(QString());
    emit authenticationStateChanged();

    // Create managed account
    ManagedCodexAccount account = ManagedCodexAccount::create(email, homePath);
    
    // Try to load credentials from the home path
    QHash<QString, QString> scopedEnv = CodexHomeScope::scopedEnvironment(m_env, homePath);
    auto credentials = CodexOAuthCredentials::load(scopedEnv);
    
    if (credentials.has_value()) {
        account.providerAccountId = credentials->accountId;
        if (account.email.isEmpty()) {
            account.email = resolveEmailFromCredentials(*credentials);
        }
        account.lastAuthenticatedAt = QDateTime::currentDateTime();
    }

    // Check for duplicate accounts (same email and providerAccountId)
    auto existingAccounts = m_store.loadAccounts();
    for (const auto& existing : existingAccounts) {
        if (existing.email == account.email &&
            existing.providerAccountId == account.providerAccountId &&
            !account.providerAccountId.isEmpty()) {
            qDebug() << "Account already exists, skipping:" << account.email;
            m_isAuthenticating = false;
            m_authenticatingAccountID = QString();
            m_authMessage = QStringLiteral("Codex account already exists.");
            emit authenticationFinished(existing.id, true);
            emit authenticationStateChanged();
            emit accountsChanged();
            return true;
        }
    }

    // Add to store
    m_store.addAccount(account);
    
    // Set as active
    m_activeAccountID = account.id;

    m_isAuthenticating = false;
    m_authenticatingAccountID = QString();
    m_authMessage = QStringLiteral("Codex account added.");
    
    refresh();
    
    emit authenticationFinished(account.id, true);
    emit authenticationStateChanged();
    emit accountsChanged();
    emit activeAccountChanged(account.id);
    
    return true;
}

bool ManagedCodexAccountService::authenticateNewAccount()
{
    if (m_isAuthenticating || m_isRemoving) {
        return false;
    }

    if (m_loginRunner) {
        qWarning() << "Login already in progress";
        return false;
    }

    m_isAuthenticating = true;
    m_authenticatingAccountID = QString();
    resetAuthenticationStatus();
    m_authMessage = QStringLiteral("Starting Codex device login...");
    emit authenticationStarted(QString());
    emit authenticationStateChanged();

    // 1. Create managed home directory
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString homePath = baseDir + "/managed-codex-homes/" + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QDir dir;
    if (!dir.mkpath(homePath)) {
        qWarning() << "Failed to create managed home directory:" << homePath;
        m_isAuthenticating = false;
        m_authError = QStringLiteral("Could not create managed Codex home.");
        m_authMessage.clear();
        emit authenticationFinished(QString(), false);
        emit authenticationStateChanged();
        return false;
    }

    qDebug() << "Created managed home directory:" << homePath;
    m_pendingHomePath = homePath;

    // 2. Start codex login asynchronously
    m_loginRunner = new CodexLoginRunner(this);
    connect(m_loginRunner, &CodexLoginRunner::finished, this, &ManagedCodexAccountService::onLoginFinished);
    connect(m_loginRunner, &CodexLoginRunner::progressUpdate, this, [](const QString& msg) {
        qDebug() << "Login progress:" << msg;
    });
    connect(m_loginRunner, &CodexLoginRunner::progressUpdate, this, [this](const QString& msg) {
        m_authMessage = msg;
        emit authenticationStateChanged();
    });
    connect(m_loginRunner, &CodexLoginRunner::deviceAuthPromptReady,
            this, [this](const QString& verificationUri, const QString& userCode) {
        m_authVerificationUri = verificationUri;
        m_authUserCode = userCode;
        m_authMessage = QStringLiteral("Enter the Codex device code to authorize this account.");
        emit authenticationStateChanged();
    });

    m_loginRunner->start(homePath, 15 * 60 * 1000);

    return true;
}

void ManagedCodexAccountService::onLoginFinished(const CodexLoginRunner::Result& result)
{
    qDebug() << "Login finished with outcome:" << result.outcome;

    QString homePath = m_pendingHomePath;
    m_pendingHomePath.clear();

    if (m_loginRunner) {
        m_loginRunner->deleteLater();
        m_loginRunner = nullptr;
    }

    if (result.outcome != CodexLoginRunner::Result::Success) {
        qWarning() << "Codex login failed:" << result.output.left(1000);
        QString message;
        if (result.outcome == CodexLoginRunner::Result::TimedOut) {
            message = QStringLiteral("Codex login timed out.");
            if (!result.output.isEmpty())
                message += QStringLiteral(" Last output: ") + sanitizeLoginOutput(result.output);
        } else if (result.outcome == CodexLoginRunner::Result::MissingBinary) {
            message = QStringLiteral("codex CLI not found in PATH.");
        } else if (result.outcome == CodexLoginRunner::Result::LaunchFailed) {
            message = QStringLiteral("Could not start codex login.");
            if (!result.output.isEmpty())
                message += QStringLiteral(" Error: ") + sanitizeLoginOutput(result.output);
        } else {
            message = QStringLiteral("Codex login failed.");
            if (!result.output.isEmpty())
                message += QStringLiteral(" Output: ") + sanitizeLoginOutput(result.output);
        }
        finalizeLoginFailure(homePath, message);
        return;
    }

    finalizeLoginSuccess(homePath);
}

void ManagedCodexAccountService::finalizeLoginSuccess(const QString& homePath)
{
    qDebug() << "Codex login successful";

    // Load credentials from auth.json
    QHash<QString, QString> scopedEnv = CodexHomeScope::scopedEnvironment(m_env, homePath);
    auto credentials = CodexOAuthCredentials::load(scopedEnv);

    if (!credentials.has_value()) {
        qWarning() << "Failed to load credentials after login";
        finalizeLoginFailure(homePath, QStringLiteral("Codex login finished but auth.json was not created."));
        return;
    }

    // Create account
    ManagedCodexAccount account;
    account.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    account.email = resolveEmailFromCredentials(*credentials);
    account.providerAccountId = credentials->accountId;
    account.managedHomePath = homePath;
    account.createdAt = QDateTime::currentDateTime();
    account.updatedAt = QDateTime::currentDateTime();
    account.lastAuthenticatedAt = QDateTime::currentDateTime();

    // Check for duplicate accounts (same providerAccountId)
    auto existingAccounts = m_store.loadAccounts();
    for (const auto& existing : existingAccounts) {
        if (existing.providerAccountId == account.providerAccountId &&
            !account.providerAccountId.isEmpty()) {
            qDebug() << "Account already exists, updating:" << existing.email;
            // Update existing account instead of adding duplicate
            ManagedCodexAccount updated = existing;
            updated.lastAuthenticatedAt = account.lastAuthenticatedAt;
            if (updated.email.isEmpty() && !account.email.isEmpty()) {
                updated.email = account.email;
            }
            m_store.updateAccount(updated);
            m_activeAccountID = existing.id;
            m_isAuthenticating = false;
            m_authenticatingAccountID = QString();
            m_authMessage = QStringLiteral("Codex account updated.");
            m_authError.clear();
            m_authVerificationUri.clear();
            m_authUserCode.clear();
            refresh();
            emit authenticationFinished(existing.id, true);
            emit authenticationStateChanged();
            emit accountsChanged();
            emit activeAccountChanged(existing.id);
            return;
        }
    }

    // Save account
    m_store.addAccount(account);
    m_activeAccountID = account.id;

    m_isAuthenticating = false;
    m_authenticatingAccountID = QString();
    m_authMessage = QStringLiteral("Codex account added.");
    m_authError.clear();
    m_authVerificationUri.clear();
    m_authUserCode.clear();

    refresh();

    emit authenticationFinished(account.id, true);
    emit authenticationStateChanged();
    emit accountsChanged();
    emit activeAccountChanged(account.id);
}

void ManagedCodexAccountService::finalizeLoginFailure(const QString& homePath, const QString& message)
{
    // Clean up directory
    if (!homePath.isEmpty()) {
        QDir(homePath).removeRecursively();
    }

    m_isAuthenticating = false;
    m_authenticatingAccountID = QString();
    m_authMessage.clear();
    m_authVerificationUri.clear();
    m_authUserCode.clear();
    m_authError = message.isEmpty() ? QStringLiteral("Codex login failed.") : message;

    emit authenticationFinished(QString(), false);
    emit authenticationStateChanged();
}

bool ManagedCodexAccountService::removeAccount(const QString& accountID)
{
    if (m_isAuthenticating || m_isRemoving) {
        return false;
    }

    // Don't allow removing the live system account
    if (accountID == "live-system") {
        return false;
    }

    m_isRemoving = true;
    m_removingAccountID = accountID;
    emit removalStarted(accountID);

    // Remove from store
    m_store.removeAccount(accountID);

    // If removed account was active, switch to live system
    if (m_activeAccountID == accountID) {
        m_activeAccountID = "live-system";
        emit activeAccountChanged(m_activeAccountID);
    }

    m_isRemoving = false;
    m_removingAccountID = QString();

    refresh();

    emit removalFinished(accountID, true);
    emit accountsChanged();

    return true;
}

void ManagedCodexAccountService::cancelAuthentication()
{
    if (!m_isAuthenticating) return;

    QString homePath = m_pendingHomePath;
    if (m_loginRunner) {
        m_loginRunner->cancel();
        m_loginRunner->deleteLater();
        m_loginRunner = nullptr;
    }
    m_pendingHomePath.clear();
    finalizeLoginFailure(homePath, QStringLiteral("Codex login cancelled."));
}

bool ManagedCodexAccountService::setActiveAccount(const QString& accountID)
{
    if (m_isAuthenticating || m_isRemoving) {
        return false;
    }

    if (m_activeAccountID == accountID) {
        return true;
    }

    m_activeAccountID = accountID;
    emit activeAccountChanged(accountID);
    
    return true;
}

bool ManagedCodexAccountService::reauthenticateAccount(const QString& accountID)
{
    if (m_isAuthenticating || m_isRemoving) {
        return false;
    }

    m_isAuthenticating = true;
    m_authenticatingAccountID = accountID;
    emit authenticationStarted(accountID);

    // Find the account
    auto account = m_store.account(accountID);
    if (!account.has_value()) {
        m_isAuthenticating = false;
        m_authenticatingAccountID = QString();
        emit authenticationFinished(accountID, false);
        return false;
    }

    // Try to load credentials
    QHash<QString, QString> scopedEnv = CodexHomeScope::scopedEnvironment(m_env, account->managedHomePath);
    auto credentials = CodexOAuthCredentials::load(scopedEnv);
    
    if (credentials.has_value()) {
        // Update account
        ManagedCodexAccount updated = *account;
        updated.providerAccountId = credentials->accountId;
        updated.lastAuthenticatedAt = QDateTime::currentDateTime();
        m_store.updateAccount(updated);
        
        m_isAuthenticating = false;
        m_authenticatingAccountID = QString();
        
        refresh();
        
        emit authenticationFinished(accountID, true);
        emit accountsChanged();
        return true;
    }

    m_isAuthenticating = false;
    m_authenticatingAccountID = QString();
    emit authenticationFinished(accountID, false);
    return false;
}

bool ManagedCodexAccountService::isAuthenticating() const
{
    return m_isAuthenticating;
}

bool ManagedCodexAccountService::isRemoving() const
{
    return m_isRemoving;
}

QString ManagedCodexAccountService::authenticatingAccountID() const
{
    return m_authenticatingAccountID;
}

QString ManagedCodexAccountService::removingAccountID() const
{
    return m_removingAccountID;
}

bool ManagedCodexAccountService::hasUnreadableStore() const
{
    return m_snapshot.hasUnreadableAddedAccountStore;
}

QString ManagedCodexAccountService::authMessage() const
{
    return m_authMessage;
}

QString ManagedCodexAccountService::authError() const
{
    return m_authError;
}

QString ManagedCodexAccountService::authVerificationUri() const
{
    return m_authVerificationUri;
}

QString ManagedCodexAccountService::authUserCode() const
{
    return m_authUserCode;
}

QString ManagedCodexAccountService::activeManagedHomePath() const
{
    if (m_activeAccountID.isEmpty() || m_activeAccountID == "live-system") {
        return {};
    }

    auto account = m_store.account(m_activeAccountID);
    if (!account.has_value()) return {};
    return account->managedHomePath;
}

void ManagedCodexAccountService::refresh()
{
    CodexAccountReconciliation reconciliation(m_env);
    m_snapshot = reconciliation.loadSnapshot();
    
    // If no active account set, default to live system
    if (m_activeAccountID.isEmpty()) {
        m_activeAccountID = "live-system";
    }
    
    emit accountsChanged();
}

CodexAccountReconciliationSnapshot ManagedCodexAccountService::currentSnapshot() const
{
    return m_snapshot;
}

QString ManagedCodexAccountService::resolveDisplayName(const ManagedCodexAccount& account) const
{
    if (!account.workspaceLabel.isEmpty()) {
        return account.workspaceLabel + " (" + account.email + ")";
    }
    if (!account.email.isEmpty()) {
        return account.email;
    }
    return "Account " + account.id.left(8);
}

QString ManagedCodexAccountService::resolveDisplayName(const ObservedSystemCodexAccount& account) const
{
    if (!account.workspaceLabel.isEmpty()) {
        return account.workspaceLabel + " (" + account.email + ")";
    }
    if (!account.email.isEmpty()) {
        return account.email + " (System)";
    }
    return "System Account";
}

QString ManagedCodexAccountService::resolveEmailFromCredentials(
    const CodexOAuthCredentials& credentials) const
{
    if (credentials.idToken.isEmpty()) return {};

    QJsonObject payload = parseJWTPayload(credentials.idToken);
    if (payload.isEmpty()) return {};

    QString email = payload.value("email").toString().trimmed();
    if (email.isEmpty()) {
        QJsonObject profile = payload.value("https://api.openai.com/profile").toObject();
        email = profile.value("email").toString().trimmed();
    }
    return ManagedCodexAccount::normalizeEmail(email);
}

void ManagedCodexAccountService::resetAuthenticationStatus()
{
    m_authMessage.clear();
    m_authError.clear();
    m_authVerificationUri.clear();
    m_authUserCode.clear();
}
