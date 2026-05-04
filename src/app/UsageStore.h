#pragma once

#include <QObject>
#include <QHash>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>
#include <QAtomicInt>
#include <QThreadPool>
#include <optional>
#include "../models/UsageSnapshot.h"
#include "../models/CreditsSnapshot.h"
#include "../models/CostUsageReport.h"
#include "SessionQuotaNotifications.h"
#include "../providers/IFetchStrategy.h"
#include "../providers/ProviderFetchContext.h"
#include "../providers/codex/CodexConsumerProjection.h"
#include "../providers/codex/CodexCreditsFetcher.h"

class ProviderRegistry;
class ProviderPipeline;
class SettingsStore;
class PlanUtilizationHistoryStore;

class UsageStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRefreshing READ isRefreshing NOTIFY refreshingChanged)
    Q_PROPERTY(QStringList providerIDs READ providerIDs NOTIFY providerIDsChanged)
    Q_PROPERTY(bool costUsageEnabled READ costUsageEnabled WRITE setCostUsageEnabled NOTIFY costUsageEnabledChanged)
    Q_PROPERTY(bool costUsageRefreshing READ costUsageRefreshing NOTIFY costUsageRefreshingChanged)
    Q_PROPERTY(int snapshotRevision READ snapshotRevision NOTIFY snapshotRevisionChanged)
    Q_PROPERTY(int statusRevision READ statusRevision NOTIFY statusRevisionChanged)
    Q_PROPERTY(QVariantMap codexAccountState READ codexAccountState NOTIFY codexAccountStateChanged)
    Q_PROPERTY(QVariantList codexFetchAttempts READ codexFetchAttempts NOTIFY codexFetchAttemptsChanged)
    Q_PROPERTY(QString lastKnownSessionWindowSource READ lastKnownSessionWindowSource NOTIFY lastKnownSessionWindowSourceChanged)

public:
    explicit UsageStore(QObject* parent = nullptr);

    Q_INVOKABLE UsageSnapshot snapshot(const QString& providerId) const;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE void refreshProvider(const QString& providerId);
    Q_INVOKABLE void setProviderEnabled(const QString& id, bool enabled);
    Q_INVOKABLE bool isProviderEnabled(const QString& id) const;
    Q_INVOKABLE QString providerDisplayName(const QString& id) const;
    Q_INVOKABLE QVariantMap snapshotData(const QString& id) const;
    Q_INVOKABLE QString providerError(const QString& id) const;
    Q_INVOKABLE QVariantList providerList() const;
    Q_INVOKABLE void moveProvider(int fromIndex, int toIndex);
    Q_INVOKABLE QVariantList providerSettingsFields(const QString& id) const;
    Q_INVOKABLE QVariantMap providerDescriptorData(const QString& id) const;
    Q_INVOKABLE void setProviderSetting(const QString& providerId, const QString& key, const QVariant& value);
    Q_INVOKABLE QVariantMap providerSecretStatus(const QString& providerId, const QString& key) const;
    Q_INVOKABLE bool setProviderSecret(const QString& providerId, const QString& key, const QString& value);
    Q_INVOKABLE bool clearProviderSecret(const QString& providerId, const QString& key);
    Q_INVOKABLE void testProviderConnection(const QString& providerId);
    Q_INVOKABLE QVariantMap providerConnectionTest(const QString& providerId) const;
    Q_INVOKABLE void startProviderLogin(const QString& providerId);
    Q_INVOKABLE void cancelProviderLogin(const QString& providerId);
    Q_INVOKABLE QVariantMap providerLoginState(const QString& providerId) const;
    Q_INVOKABLE void refreshProviderStatuses();
    Q_INVOKABLE QVariantMap providerStatus(const QString& providerId) const;
    Q_INVOKABLE QVariantMap providerUsageSnapshot(const QString& providerId) const;
    Q_INVOKABLE QStringList allProviderIDs() const;

    Q_INVOKABLE void refreshCostUsage();
    Q_INVOKABLE QVariantMap costUsageData() const;
    Q_INVOKABLE QVariantList providerCostUsageList() const;
    Q_INVOKABLE QVariantMap providerCostUsageData(const QString& providerId) const;

    Q_INVOKABLE QVariantList utilizationChartData(const QString& providerId, const QString& seriesName) const;
    Q_INVOKABLE QVariantList codexFetchAttempts() const;
    Q_INVOKABLE QVariantMap providerDashboardData(const QString& providerId) const;
    QString lastKnownSessionWindowSource() const;

    // Codex multi-account management
    Q_INVOKABLE QVariantList codexAccounts() const;
    Q_INVOKABLE QVariantMap codexAccountState() const;
    Q_INVOKABLE QString codexActiveAccountID() const;
    Q_INVOKABLE void setCodexActiveAccount(const QString& accountID);
    Q_INVOKABLE bool addCodexAccount(const QString& email, const QString& homePath);
    Q_INVOKABLE void cancelCodexAuthentication();
    Q_INVOKABLE bool removeCodexAccount(const QString& accountID);
    Q_INVOKABLE bool reauthenticateCodexAccount(const QString& accountID);
    Q_INVOKABLE bool promoteCodexAccount(const QString& accountID);
    Q_INVOKABLE bool isCodexAuthenticating() const;

    // Token Account management (generic multi-account support)
    Q_INVOKABLE QVariantList tokenAccountsForProvider(const QString& providerId) const;
    Q_INVOKABLE QString addTokenAccount(const QString& providerId, const QString& displayName, int sourceMode);
    Q_INVOKABLE bool removeTokenAccount(const QString& accountId);
    Q_INVOKABLE bool setTokenAccountVisibility(const QString& accountId, int visibility);
    Q_INVOKABLE bool setTokenAccountSourceMode(const QString& accountId, int sourceMode);
    Q_INVOKABLE bool setDefaultTokenAccount(const QString& providerId, const QString& accountId);
    Q_INVOKABLE QString defaultTokenAccount(const QString& providerId) const;

    QThreadPool* threadPool() const { return m_threadPool; }
    void shutdown();
    Q_INVOKABLE bool isCodexRemoving() const;
    Q_INVOKABLE QString codexAuthenticatingAccountID() const;
    Q_INVOKABLE QString codexRemovingAccountID() const;
    Q_INVOKABLE bool hasCodexUnreadableStore() const;

    bool costUsageEnabled() const { return m_costUsageEnabled; }
    void setCostUsageEnabled(bool v);
    bool costUsageRefreshing() const { return m_costUsageRefreshing; }

    QStringList providerIDs() const;
    void updateProviderIDs();

    void startAutoRefresh(int intervalMinutes);
    void stopAutoRefresh();
    bool isRefreshing() const { return m_isRefreshing; }
    int snapshotRevision() const { return m_snapshotRevision; }
    int statusRevision() const { return m_statusRevision; }
    QString error(const QString& providerId) const;
    ProviderFetchContext buildFetchContextForProvider(const QString& providerId) const;

    void setSettingsStore(SettingsStore* s);

    // Preload all provider credentials into cache (runs on background thread)
    void preloadCredentials();

signals:
    void snapshotChanged(const QString& providerId);
    void refreshingChanged();
    void providerIDsChanged();
    void errorOccurred(const QString& providerId, const QString& message);
    void costUsageEnabledChanged();
    void costUsageRefreshingChanged();
    void costUsageChanged();
    void snapshotRevisionChanged();
    void providerConnectionTestChanged(const QString& providerId);
    void providerLoginStateChanged(const QString& providerId);
    void providerStatusChanged(const QString& providerId);
    void statusRevisionChanged();

    // Codex multi-account signals
    void codexAccountsChanged();
    void codexActiveAccountChanged(const QString& accountID);
    void codexAuthenticationStarted(const QString& accountID);
    void codexAuthenticationFinished(const QString& accountID, bool success);
    void codexRemovalStarted(const QString& accountID);
    void codexRemovalFinished(const QString& accountID, bool success);
    void codexAccountStateChanged();

    // Codex credits signals
    void codexCreditsChanged();
    void codexFetchAttemptsChanged();
    void lastKnownSessionWindowSourceChanged();

private:
    std::optional<ProviderSettingsDescriptor> settingDescriptor(const QString& providerId,
                                                                const QString& key) const;
    void setProviderLoginState(const QString& providerId, const QVariantMap& state);
    void setProviderConnectionTest(const QString& providerId, const QVariantMap& state);
    void setProviderStatus(const QString& providerId, const QVariantMap& status);
    void configureStatusPolling();

    QTimer m_refreshTimer;
    QTimer m_statusTimer;
    QHash<QString, UsageSnapshot> m_snapshots;
    QHash<QString, QString> m_errors;
    QHash<QString, QVariantMap> m_connectionTests;
    QHash<QString, QVariantMap> m_loginStates;
    QHash<QString, QVariantMap> m_providerStatuses;
    QHash<QString, QSharedPointer<QAtomicInt>> m_loginCancelFlags;
    QHash<QString, std::optional<double>> m_lastKnownSessionRemaining;
    QHash<QString, QVector<ProviderFetchAttempt>> m_lastFetchAttempts;
    QHash<QString, QVariantMap> m_dashboardData;
    QString m_lastKnownSessionWindowSource;
    QStringList m_providerIDs;
    bool m_isRefreshing = false;

    bool m_costUsageEnabled = false;
    bool m_costUsageRefreshing = false;
    CostUsageSnapshot m_costUsage;
    QHash<QString, CostUsageSnapshot> m_perProviderCostUsage;
    QVector<ProviderCostUsageSnapshot> m_allProviderCostUsage;
    ProviderPipeline* m_pipeline = nullptr;
    SettingsStore* m_settingsStore = nullptr;
    PlanUtilizationHistoryStore* m_historyStore = nullptr;
    int m_pendingRefreshes = 0;
    int m_snapshotRevision = 0;
    int m_statusRevision = 0;
    bool m_batchRefreshInProgress = false;

    // Codex multi-account
    class ManagedCodexAccountService* m_codexAccountService = nullptr;

    // Credential cache to avoid blocking main thread with WinCred API calls
    struct CredentialEntry {
        QByteArray data;
        QDateTime cachedAt;
    };
    mutable QMutex m_credentialCacheMutex;
    mutable QHash<QString, CredentialEntry> m_credentialCache;
    mutable QHash<QString, bool> m_credentialMissing;
    static constexpr int CREDENTIAL_CACHE_TTL_MS = 300000; // 5 minutes

    // snapshotData() result cache — invalidated on snapshotChanged / snapshotRevisionChanged
    mutable QHash<QString, QVariantMap> m_snapshotDataCache;

    // Codex credits cache
    struct CodexCreditsCache {
        std::optional<CreditsSnapshot> snapshot;
        QString accountKey;
        QDateTime updatedAt;
        int failureStreak = 0;
        QString lastError;
    };
    CodexCreditsCache m_codexCreditsCache;

    // Codex account refresh guard
    struct CodexAccountRefreshGuard {
        QString source;  // "liveSystem" or "managedAccount"
        QString identity;
        QString accountKey;

        bool operator==(const CodexAccountRefreshGuard& other) const {
            return source == other.source && identity == other.identity && accountKey == other.accountKey;
        }
        bool operator!=(const CodexAccountRefreshGuard& other) const {
            return !(*this == other);
        }
        bool isEmpty() const {
            return source.isEmpty() && identity.isEmpty() && accountKey.isEmpty();
        }
    };
    CodexAccountRefreshGuard m_lastCodexRefreshGuard;

    CodexAccountRefreshGuard currentCodexAccountRefreshGuard() const;
    bool shouldApplyCodexScopedNonUsageResult(const CodexAccountRefreshGuard& expectedGuard) const;

    // Codex credits methods
    void refreshCodexCredits(const CodexAccountRefreshGuard& expectedGuard = {});
    void applyCodexCreditsFetchResult(const CodexCreditsFetcher::FetchResult& result,
                                       const CodexAccountRefreshGuard& expectedGuard = {});
    QString currentCodexAccountKey() const;
    std::optional<CreditsSnapshot> cachedCodexCredits() const;
    QString codexCreditsError() const;
    bool codexCreditsRefreshing() const { return m_codexCreditsRefreshing; }

    // Codex consumer projection
    QVariantMap codexConsumerProjectionData() const;

    // Wait for codex snapshot to be at least as fresh as minimumUpdatedAt (mirrors original CodexBar)
    UsageSnapshot waitForCodexSnapshot(const QDateTime& minimumUpdatedAt, int timeoutMs = 6000) const;

    void clearCodexOpenAIWebState();
    void doRefresh(const QStringList& ids);
    QThreadPool* m_threadPool = nullptr;

    bool m_codexCreditsRefreshing = false;
    int m_pendingCreditsRefresh = 0;

    // Test injection point for credits fetching (mirrors original _test_codexCreditsLoaderOverride)
    std::function<std::optional<CreditsSnapshot>()> _test_codexCreditsLoaderOverride;
};
