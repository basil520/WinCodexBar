#pragma once

#include <QObject>
#include <QSettings>
#include <QJsonObject>
#include <QStringList>
#include <QTimer>

class SettingsStore : public QObject {
    Q_OBJECT

    Q_PROPERTY(int refreshFrequency READ refreshFrequency WRITE setRefreshFrequency NOTIFY refreshFrequencyChanged)
    Q_PROPERTY(bool launchAtLogin READ launchAtLogin WRITE setLaunchAtLogin NOTIFY launchAtLoginChanged)
    Q_PROPERTY(bool checkForUpdates READ checkForUpdates WRITE setCheckForUpdates NOTIFY checkForUpdatesChanged)
    Q_PROPERTY(bool debugMenuEnabled READ debugMenuEnabled WRITE setDebugMenuEnabled NOTIFY debugMenuEnabledChanged)
    Q_PROPERTY(bool mergeIcons READ mergeIcons WRITE setMergeIcons NOTIFY mergeIconsChanged)
    Q_PROPERTY(bool statusChecksEnabled READ statusChecksEnabled WRITE setStatusChecksEnabled NOTIFY statusChecksEnabledChanged)
    Q_PROPERTY(bool usageBarsShowUsed READ usageBarsShowUsed WRITE setUsageBarsShowUsed NOTIFY usageBarsShowUsedChanged)
    Q_PROPERTY(bool resetTimesShowAbsolute READ resetTimesShowAbsolute WRITE setResetTimesShowAbsolute NOTIFY resetTimesShowAbsoluteChanged)
    Q_PROPERTY(bool showOptionalCreditsAndExtraUsage READ showOptionalCreditsAndExtraUsage WRITE setShowOptionalCreditsAndExtraUsage NOTIFY showOptionalCreditsAndExtraUsageChanged)
    Q_PROPERTY(bool sessionQuotaNotificationsEnabled READ sessionQuotaNotificationsEnabled WRITE setSessionQuotaNotificationsEnabled NOTIFY sessionQuotaNotificationsEnabledChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit SettingsStore(QObject* parent = nullptr);

    int refreshFrequency() const { return m_refreshFrequency; }
    void setRefreshFrequency(int minutes);

    bool launchAtLogin() const { return m_launchAtLogin; }
    void setLaunchAtLogin(bool enable);

    bool checkForUpdates() const { return m_checkForUpdates; }
    void setCheckForUpdates(bool enable);

    bool debugMenuEnabled() const { return m_debugMenuEnabled; }
    void setDebugMenuEnabled(bool enable);

    bool mergeIcons() const { return m_mergeIcons; }
    void setMergeIcons(bool enable);

    bool statusChecksEnabled() const { return m_statusChecksEnabled; }
    void setStatusChecksEnabled(bool enable);

    bool usageBarsShowUsed() const { return m_usageBarsShowUsed; }
    void setUsageBarsShowUsed(bool enable);

    bool resetTimesShowAbsolute() const { return m_resetTimesShowAbsolute; }
    void setResetTimesShowAbsolute(bool enable);

    bool showOptionalCreditsAndExtraUsage() const { return m_showOptionalCreditsAndExtraUsage; }
    void setShowOptionalCreditsAndExtraUsage(bool enable);

    bool sessionQuotaNotificationsEnabled() const { return m_sessionQuotaNotificationsEnabled; }
    void setSessionQuotaNotificationsEnabled(bool enable);

    QString language() const { return m_language; }
    void setLanguage(const QString& lang);

    Q_INVOKABLE bool isProviderEnabled(const QString& id) const;
    Q_INVOKABLE void setProviderEnabled(const QString& id, bool enabled);
    Q_INVOKABLE QStringList providerOrder() const;
    Q_INVOKABLE void setProviderOrder(const QStringList& order);

    Q_INVOKABLE QVariant providerSetting(const QString& providerID,
                                          const QString& key,
                                          const QVariant& defaultValue = {}) const;
    Q_INVOKABLE void setProviderSetting(const QString& providerID,
                                         const QString& key,
                                         const QVariant& value);

    void loadConfig();
    Q_INVOKABLE void saveConfig();
    Q_INVOKABLE void resetToDefaults();
    QString configPath() const;

signals:
    void refreshFrequencyChanged();
    void launchAtLoginChanged();
    void checkForUpdatesChanged();
    void debugMenuEnabledChanged();
    void mergeIconsChanged();
    void statusChecksEnabledChanged();
    void usageBarsShowUsedChanged();
    void resetTimesShowAbsoluteChanged();
    void showOptionalCreditsAndExtraUsageChanged();
    void sessionQuotaNotificationsEnabledChanged();
    void languageChanged();
    void providerOrderChanged();
    void providerSettingChanged(const QString& providerID, const QString& key);

private:
    QSettings m_settings;
    QJsonObject m_config;

    int m_refreshFrequency = 5;
    bool m_launchAtLogin = false;
    bool m_checkForUpdates = true;
    bool m_debugMenuEnabled = false;
    bool m_mergeIcons = true;
    bool m_statusChecksEnabled = true;
    bool m_usageBarsShowUsed = false;
    bool m_resetTimesShowAbsolute = false;
    bool m_showOptionalCreditsAndExtraUsage = true;
    bool m_sessionQuotaNotificationsEnabled = true;
    QString m_language;
    QStringList m_providerOrder;
    QHash<QString, QHash<QString, QVariant>> m_providerSettings;
};
