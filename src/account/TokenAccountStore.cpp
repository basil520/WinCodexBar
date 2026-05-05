#include "TokenAccountStore.h"
#include "../providers/shared/ProviderCredentialStore.h"
#include "../app/SettingsStore.h"
#include "../providers/ProviderRegistry.h"
#include "../providers/IProvider.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QDebug>

TokenAccountStore::TokenAccountStore(QObject* parent)
    : QObject(parent)
{
    m_storagePath = storagePath();
}

TokenAccountStore* TokenAccountStore::instance()
{
    static TokenAccountStore* s_instance = new TokenAccountStore();
    return s_instance;
}

QString TokenAccountStore::storagePath() const
{
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appData);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absoluteFilePath("accounts.json");
}

QString TokenAccountStore::credentialTargetFor(const QString& accountId, const QString& type) const
{
    return QString("CodexBar/account/%1/%2").arg(accountId, type);
}

QString TokenAccountStore::addAccount(const TokenAccount& account)
{
    TokenAccount acc = account;
    if (acc.accountId.isEmpty()) {
        acc.accountId = TokenAccount::generateAccountId();
    }
    if (!acc.createdAt.isValid()) {
        acc.createdAt = QDateTime::currentDateTimeUtc();
    }

    {
        QWriteLocker lock(&m_lock);
        m_accounts.insert(acc.accountId, acc);
    }

    // Save credentials to secure storage
    if (!acc.credentials.isEmpty()) {
        saveAccountCredentials(acc.accountId, acc.credentials);
    }

    emit accountAdded(acc.accountId);
    emit accountsChanged(acc.providerId);
    return acc.accountId;
}

bool TokenAccountStore::updateAccount(const QString& accountId, const TokenAccount& account)
{
    QWriteLocker lock(&m_lock);
    if (!m_accounts.contains(accountId)) {
        return false;
    }

    TokenAccount updated = account;
    updated.accountId = accountId; // Enforce ID immutability
    QString oldProviderId = m_accounts[accountId].providerId;
    m_accounts[accountId] = updated;

    lock.unlock();

    // Remove old credentials and save new ones
    removeAccountCredentials(accountId);
    if (!updated.credentials.isEmpty()) {
        saveAccountCredentials(accountId, updated.credentials);
    }

    emit accountUpdated(accountId);
    emit accountsChanged(updated.providerId);
    if (oldProviderId != updated.providerId) {
        emit accountsChanged(oldProviderId);
    }
    return true;
}

bool TokenAccountStore::removeAccount(const QString& accountId)
{
    QString providerId;
    {
        QWriteLocker lock(&m_lock);
        auto it = m_accounts.find(accountId);
        if (it == m_accounts.end()) {
            return false;
        }
        providerId = it->providerId;
        m_accounts.erase(it);

        // Remove from default mapping
        auto defIt = m_defaultAccountIds.find(providerId);
        if (defIt != m_defaultAccountIds.end() && defIt.value() == accountId) {
            m_defaultAccountIds.erase(defIt);
        }
    }

    // Remove credentials from secure storage
    removeAccountCredentials(accountId);

    emit accountRemoved(accountId);
    emit accountsChanged(providerId);
    return true;
}

std::optional<TokenAccount> TokenAccountStore::account(const QString& accountId) const
{
    QReadLocker lock(&m_lock);
    auto it = m_accounts.constFind(accountId);
    if (it == m_accounts.constEnd()) {
        return std::nullopt;
    }
    TokenAccount acc = it.value();
    lock.unlock();

    // Load credentials from secure storage
    acc.credentials = loadAccountCredentials(accountId);
    return acc;
}

QList<TokenAccount> TokenAccountStore::accountsForProvider(const QString& providerId) const
{
    QReadLocker lock(&m_lock);
    QList<TokenAccount> result;
    for (const auto& acc : m_accounts) {
        if (acc.providerId == providerId) {
            result.append(acc);
        }
    }
    return result;
}

QList<TokenAccount> TokenAccountStore::visibleAccountsForProvider(const QString& providerId) const
{
    QReadLocker lock(&m_lock);
    QList<TokenAccount> result;
    for (const auto& acc : m_accounts) {
        if (acc.providerId == providerId && acc.visibility == AccountVisibility::Visible) {
            result.append(acc);
        }
    }
    return result;
}

QList<TokenAccount> TokenAccountStore::allAccounts() const
{
    QReadLocker lock(&m_lock);
    return m_accounts.values();
}

QStringList TokenAccountStore::accountIdsForProvider(const QString& providerId) const
{
    QReadLocker lock(&m_lock);
    QStringList result;
    for (const auto& acc : m_accounts) {
        if (acc.providerId == providerId) {
            result.append(acc.accountId);
        }
    }
    return result;
}

bool TokenAccountStore::hasAccount(const QString& accountId) const
{
    QReadLocker lock(&m_lock);
    return m_accounts.contains(accountId);
}

QString TokenAccountStore::defaultAccountId(const QString& providerId) const
{
    QReadLocker lock(&m_lock);
    return m_defaultAccountIds.value(providerId);
}

void TokenAccountStore::setDefaultAccountId(const QString& providerId, const QString& accountId)
{
    {
        QWriteLocker lock(&m_lock);
        if (!accountId.isEmpty() && !m_accounts.contains(accountId)) {
            return;
        }
        if (accountId.isEmpty()) {
            m_defaultAccountIds.remove(providerId);
        } else {
            m_defaultAccountIds[providerId] = accountId;
        }
    }
    emit defaultAccountChanged(providerId, accountId);
    emit accountsChanged(providerId);
}

bool TokenAccountStore::saveAccountCredentials(const QString& accountId, const TokenAccountCredentials& creds)
{
    bool ok = true;
    if (creds.api.has_value() && creds.api->isValid()) {
        const QByteArray blob = creds.api->apiKey.data();
        ok &= ProviderCredentialStore::write(
            credentialTargetFor(accountId, "api"), "apiKey", blob);
        if (!creds.api->baseUrl.isEmpty()) {
            ok &= ProviderCredentialStore::write(
                credentialTargetFor(accountId, "api_baseurl"), "baseUrl",
                creds.api->baseUrl.toString().toUtf8());
        }
        if (!creds.api->organizationId.isEmpty()) {
            ok &= ProviderCredentialStore::write(
                credentialTargetFor(accountId, "api_org"), "orgId",
                creds.api->organizationId.toUtf8());
        }
    }
    if (creds.web.has_value() && creds.web->isValid()) {
        ok &= ProviderCredentialStore::write(
            credentialTargetFor(accountId, "web_cookie"), creds.web->cookieName,
            creds.web->cookieValue.data());
        ok &= ProviderCredentialStore::write(
            credentialTargetFor(accountId, "web_domain"), "domain",
            creds.web->cookieDomain.toUtf8());
        if (creds.web->expiry.isValid()) {
            ok &= ProviderCredentialStore::write(
                credentialTargetFor(accountId, "web_expiry"), "expiry",
                creds.web->expiry.toString(Qt::ISODate).toUtf8());
        }
    }
    if (creds.oauth.has_value() && creds.oauth->isValid()) {
        ok &= ProviderCredentialStore::write(
            credentialTargetFor(accountId, "oauth_access"), "accessToken",
            creds.oauth->accessToken.data());
        if (!creds.oauth->refreshToken.isEmpty()) {
            ok &= ProviderCredentialStore::write(
                credentialTargetFor(accountId, "oauth_refresh"), "refreshToken",
                creds.oauth->refreshToken.data());
        }
        if (creds.oauth->expiresAt.isValid()) {
            ok &= ProviderCredentialStore::write(
                credentialTargetFor(accountId, "oauth_expires"), "expiresAt",
                creds.oauth->expiresAt.toString(Qt::ISODate).toUtf8());
        }
        if (!creds.oauth->tokenType.isEmpty()) {
            ok &= ProviderCredentialStore::write(
                credentialTargetFor(accountId, "oauth_type"), "tokenType",
                creds.oauth->tokenType.toUtf8());
        }
    }
    return ok;
}

TokenAccountCredentials TokenAccountStore::loadAccountCredentials(const QString& accountId) const
{
    TokenAccountCredentials creds;

    // API credentials
    if (ProviderCredentialStore::exists(credentialTargetFor(accountId, "api"))) {
        APICredentials api;
        auto keyOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "api"));
        if (keyOpt) api.apiKey = SecureString(*keyOpt);

        auto urlOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "api_baseurl"));
        if (urlOpt) api.baseUrl = QUrl(QString::fromUtf8(*urlOpt));

        auto orgOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "api_org"));
        if (orgOpt) api.organizationId = QString::fromUtf8(*orgOpt);

        if (api.isValid()) creds.api = std::move(api);
    }

    // Web credentials
    if (ProviderCredentialStore::exists(credentialTargetFor(accountId, "web_cookie"))) {
        WebCredentials web;
        auto cookieOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "web_cookie"));
        if (cookieOpt) web.cookieValue = SecureString(*cookieOpt);

        auto domainOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "web_domain"));
        if (domainOpt) web.cookieDomain = QString::fromUtf8(*domainOpt);

        auto nameOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "web_cookie"));
        // The username field of the credential stores the cookie name
        // We need a way to retrieve the username; for now we store it separately
        // Actually ProviderCredentialStore::read only returns the secret, not the username
        // Let's store cookie name as a separate credential
        auto nameOpt2 = ProviderCredentialStore::read(credentialTargetFor(accountId, "web_cookiename"));
        if (nameOpt2) web.cookieName = QString::fromUtf8(*nameOpt2);

        auto expiryOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "web_expiry"));
        if (expiryOpt) web.expiry = QDateTime::fromString(QString::fromUtf8(*expiryOpt), Qt::ISODate);

        if (web.isValid()) creds.web = std::move(web);
    }

    // OAuth credentials
    if (ProviderCredentialStore::exists(credentialTargetFor(accountId, "oauth_access"))) {
        OAuthCredentials oauth;
        auto accessOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "oauth_access"));
        if (accessOpt) oauth.accessToken = SecureString(*accessOpt);

        auto refreshOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "oauth_refresh"));
        if (refreshOpt) oauth.refreshToken = SecureString(*refreshOpt);

        auto expiresOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "oauth_expires"));
        if (expiresOpt) oauth.expiresAt = QDateTime::fromString(QString::fromUtf8(*expiresOpt), Qt::ISODate);

        auto typeOpt = ProviderCredentialStore::read(credentialTargetFor(accountId, "oauth_type"));
        if (typeOpt) oauth.tokenType = QString::fromUtf8(*typeOpt);

        if (oauth.isValid()) creds.oauth = std::move(oauth);
    }

    return creds;
}

bool TokenAccountStore::removeAccountCredentials(const QString& accountId)
{
    const QStringList targets = {
        "api", "api_baseurl", "api_org",
        "web_cookie", "web_domain", "web_cookiename", "web_expiry",
        "oauth_access", "oauth_refresh", "oauth_expires", "oauth_type"
    };
    bool ok = true;
    for (const QString& type : targets) {
        ok &= ProviderCredentialStore::remove(credentialTargetFor(accountId, type));
    }
    return ok;
}

bool TokenAccountStore::loadFromDisk()
{
    QFile file(m_storagePath);
    if (!file.exists()) {
        m_loaded = true;
        return true;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "TokenAccountStore: cannot read" << m_storagePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "TokenAccountStore: invalid JSON in" << m_storagePath;
        return false;
    }

    QJsonObject root = doc.object();
    int version = root.value("version").toInt(1);
    Q_UNUSED(version) // For future schema migrations

    QWriteLocker lock(&m_lock);
    m_accounts.clear();
    m_defaultAccountIds.clear();

    // Load default accounts
    QJsonObject defaults = root.value("defaultAccounts").toObject();
    for (const QString& providerId : defaults.keys()) {
        m_defaultAccountIds[providerId] = defaults.value(providerId).toString();
    }

    // Load accounts
    QJsonArray accounts = root.value("accounts").toArray();
    for (const auto& v : accounts) {
        QJsonObject obj = v.toObject();
        TokenAccount acc = TokenAccount::fromVariantMap(obj.toVariantMap());
        if (acc.isValid()) {
            m_accounts.insert(acc.accountId, acc);
        }
    }

    m_loaded = true;
    return true;
}

bool TokenAccountStore::saveToDisk()
{
    QJsonObject root;
    root["version"] = 1;

    QReadLocker lock(&m_lock);

    // Save default accounts
    QJsonObject defaults;
    for (auto it = m_defaultAccountIds.constBegin(); it != m_defaultAccountIds.constEnd(); ++it) {
        defaults[it.key()] = it.value();
    }
    root["defaultAccounts"] = defaults;

    // Save accounts (metadata only, no credentials)
    QJsonArray accounts;
    for (const auto& acc : m_accounts) {
        QJsonObject obj = QJsonObject::fromVariantMap(acc.toVariantMap());
        accounts.append(obj);
    }
    root["accounts"] = accounts;

    lock.unlock();

    QJsonDocument doc(root);
    QFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "TokenAccountStore: cannot write" << m_storagePath;
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void TokenAccountStore::migrateFromLegacy(SettingsStore* settings)
{
    if (!settings) return;

    // Check if already migrated
    {
        QWriteLocker lock(&m_lock);
        if (!m_accounts.isEmpty() || m_loaded) {
            return;
        }
    }

    // Check if accounts.json already exists
    if (QFile::exists(m_storagePath)) {
        loadFromDisk();
        return;
    }

    // Migrate from legacy config: create a default account for each enabled provider
    const QStringList knownProviderIds = {
        "zai", "openrouter", "copilot", "kimik2", "kilo", "kiro", "mistral",
        "ollama", "codex", "claude", "cursor", "kimi", "opencode", "alibaba",
        "deepseek", "minimax", "synthetic", "perplexity", "amp", "augment",
        "gemini", "vertexai", "jetbrains", "factory", "antigravity", "warp", "abacus",
        "codebuff"
    };

    QWriteLocker lock(&m_lock);
    for (const QString& providerId : knownProviderIds) {
        if (!settings->isProviderEnabled(providerId)) {
            continue;
        }

        TokenAccount account;
        account.accountId = TokenAccount::generateAccountId();
        account.providerId = providerId;

        // Try to get display name from provider registry
        ProviderRegistry& reg = ProviderRegistry::instance();
        if (IProvider* provider = reg.provider(providerId)) {
            account.displayName = provider->displayName();
        }
        if (account.displayName.isEmpty()) {
            account.displayName = providerId;
        }

        // Read legacy source mode from settings
        QVariant sourceVar = settings->providerSetting(providerId, "sourceMode");
        if (sourceVar.isValid()) {
            account.sourceMode = sourceModeFromString(sourceVar.toString());
        }

        account.visibility = AccountVisibility::Visible;
        account.createdAt = QDateTime::currentDateTimeUtc();
        account.lastUsedAt = account.createdAt;

        m_accounts[account.accountId] = account;
        m_defaultAccountIds[providerId] = account.accountId;
    }

    m_loaded = true;
    lock.unlock();
    saveToDisk();
}

int TokenAccountStore::accountCount() const
{
    QReadLocker lock(&m_lock);
    return m_accounts.size();
}

int TokenAccountStore::accountCountForProvider(const QString& providerId) const
{
    QReadLocker lock(&m_lock);
    int count = 0;
    for (const auto& acc : m_accounts) {
        if (acc.providerId == providerId) ++count;
    }
    return count;
}
