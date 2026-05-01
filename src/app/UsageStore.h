#pragma once

#include <QObject>
#include <QHash>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>
#include <QAtomicInt>
#include <optional>
#include "../models/UsageSnapshot.h"
#include "../models/CreditsSnapshot.h"
#include "../models/CostUsageReport.h"
#include "SessionQuotaNotifications.h"
#include "../providers/IFetchStrategy.h"
#include "../providers/ProviderFetchContext.h"

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

    // Codex multi-account management
    Q_INVOKABLE QVariantList codexAccounts() const;
    Q_INVOKABLE QString codexActiveAccountID() const;
    Q_INVOKABLE void setCodexActiveAccount(const QString& accountID);
    Q_INVOKABLE bool addCodexAccount(const QString& email, const QString& homePath);
    Q_INVOKABLE bool removeCodexAccount(const QString& accountID);
    Q_INVOKABLE bool reauthenticateCodexAccount(const QString& accountID);
    Q_INVOKABLE bool isCodexAuthenticating() const;
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

    // Codex multi-account
    class ManagedCodexAccountService* m_codexAccountService = nullptr;
};
