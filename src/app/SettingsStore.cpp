#include "SettingsStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QSettings>

SettingsStore::SettingsStore(QObject* parent)
    : QObject(parent)
    , m_settings("HKEY_CURRENT_USER\\Software\\CodexBar", QSettings::NativeFormat)
{
    m_refreshFrequency = m_settings.value("refreshFrequency", 5).toInt();
    m_launchAtLogin = m_settings.value("launchAtLogin", false).toBool();
    m_checkForUpdates = m_settings.value("checkForUpdates", true).toBool();
    m_debugMenuEnabled = m_settings.value("debugMenuEnabled", false).toBool();
    m_codexVerboseLogging = m_settings.value("codexVerboseLogging", false).toBool();
    m_codexWebDebugDumpHTML = m_settings.value("codexWebDebugDumpHTML", false).toBool();
    m_mergeIcons = m_settings.value("mergeIcons", true).toBool();
    m_statusChecksEnabled = m_settings.value("statusChecksEnabled", true).toBool();
    m_usageBarsShowUsed = m_settings.value("usageBarsShowUsed", false).toBool();
    m_resetTimesShowAbsolute = m_settings.value("resetTimesShowAbsolute", false).toBool();
    m_showOptionalCreditsAndExtraUsage = m_settings.value("showOptionalCreditsAndExtraUsage", true).toBool();
    m_sessionQuotaNotificationsEnabled = m_settings.value("sessionQuotaNotificationsEnabled", true).toBool();
    m_language = m_settings.value("language", "en").toString();
    loadConfig();
}

QString SettingsStore::configPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/config.json";
}

void SettingsStore::setRefreshFrequency(int minutes) {
    if (m_refreshFrequency != minutes) {
        m_refreshFrequency = minutes;
        m_settings.setValue("refreshFrequency", minutes);
        emit refreshFrequencyChanged();
    }
}

void SettingsStore::setLaunchAtLogin(bool enable) {
    if (m_launchAtLogin != enable) {
        m_launchAtLogin = enable;
        m_settings.setValue("launchAtLogin", enable);
        QSettings runReg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                         QSettings::NativeFormat);
        if (enable) {
            runReg.setValue("CodexBar", QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
        } else {
            runReg.remove("CodexBar");
        }
        emit launchAtLoginChanged();
    }
}

void SettingsStore::setCheckForUpdates(bool enable) {
    if (m_checkForUpdates != enable) {
        m_checkForUpdates = enable;
        m_settings.setValue("checkForUpdates", enable);
        emit checkForUpdatesChanged();
    }
}

void SettingsStore::setDebugMenuEnabled(bool enable) {
    if (m_debugMenuEnabled != enable) {
        m_debugMenuEnabled = enable;
        m_settings.setValue("debugMenuEnabled", enable);
        emit debugMenuEnabledChanged();
    }
}

void SettingsStore::setCodexVerboseLogging(bool enable) {
    if (m_codexVerboseLogging != enable) {
        m_codexVerboseLogging = enable;
        m_settings.setValue("codexVerboseLogging", enable);
        emit codexVerboseLoggingChanged();
    }
}

void SettingsStore::setCodexWebDebugDumpHTML(bool enable) {
    if (m_codexWebDebugDumpHTML != enable) {
        m_codexWebDebugDumpHTML = enable;
        m_settings.setValue("codexWebDebugDumpHTML", enable);
        emit codexWebDebugDumpHTMLChanged();
    }
}

void SettingsStore::setMergeIcons(bool enable) {
    if (m_mergeIcons != enable) {
        m_mergeIcons = enable;
        m_settings.setValue("mergeIcons", enable);
        emit mergeIconsChanged();
    }
}

void SettingsStore::setStatusChecksEnabled(bool enable) {
    if (m_statusChecksEnabled != enable) {
        m_statusChecksEnabled = enable;
        m_settings.setValue("statusChecksEnabled", enable);
        emit statusChecksEnabledChanged();
    }
}

void SettingsStore::setUsageBarsShowUsed(bool enable) {
    if (m_usageBarsShowUsed != enable) {
        m_usageBarsShowUsed = enable;
        m_settings.setValue("usageBarsShowUsed", enable);
        emit usageBarsShowUsedChanged();
    }
}

void SettingsStore::setResetTimesShowAbsolute(bool enable) {
    if (m_resetTimesShowAbsolute != enable) {
        m_resetTimesShowAbsolute = enable;
        m_settings.setValue("resetTimesShowAbsolute", enable);
        emit resetTimesShowAbsoluteChanged();
    }
}

void SettingsStore::setShowOptionalCreditsAndExtraUsage(bool enable) {
    if (m_showOptionalCreditsAndExtraUsage != enable) {
        m_showOptionalCreditsAndExtraUsage = enable;
        m_settings.setValue("showOptionalCreditsAndExtraUsage", enable);
        emit showOptionalCreditsAndExtraUsageChanged();
    }
}

void SettingsStore::setSessionQuotaNotificationsEnabled(bool enable) {
    if (m_sessionQuotaNotificationsEnabled != enable) {
        m_sessionQuotaNotificationsEnabled = enable;
        m_settings.setValue("sessionQuotaNotificationsEnabled", enable);
        emit sessionQuotaNotificationsEnabledChanged();
    }
}

void SettingsStore::setLanguage(const QString& lang) {
    if (m_language != lang) {
        m_language = lang;
        m_settings.setValue("language", lang);
        emit languageChanged();
    }
}

bool SettingsStore::isProviderEnabled(const QString& id) const {
    return m_settings.value("providers/" + id + "/enabled", false).toBool();
}

void SettingsStore::setProviderEnabled(const QString& id, bool enabled) {
    m_settings.setValue("providers/" + id + "/enabled", enabled);
}

QStringList SettingsStore::providerOrder() const {
    return m_providerOrder;
}

void SettingsStore::setProviderOrder(const QStringList& order) {
    m_providerOrder = order;
    saveConfig();
    emit providerOrderChanged();
}

QVariant SettingsStore::providerSetting(const QString& providerID,
                                         const QString& key,
                                         const QVariant& defaultValue) const
{
    auto it = m_providerSettings.constFind(providerID);
    if (it != m_providerSettings.constEnd()) {
        auto kit = it->constFind(key);
        if (kit != it->constEnd()) {
            return kit.value();
        }
    }
    return defaultValue;
}

void SettingsStore::setProviderSetting(const QString& providerID,
                                        const QString& key,
                                        const QVariant& value)
{
    m_providerSettings[providerID][key] = value;
    saveConfig();
    emit providerSettingChanged(providerID, key);
}

void SettingsStore::loadConfig() {
    QFile file(configPath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        m_config = doc.object();
        file.close();

        QJsonObject providers = m_config.value("providers").toObject();
        for (auto it = providers.begin(); it != providers.end(); ++it) {
            QJsonObject settings = it.value().toObject();
            QHash<QString, QVariant> map;
            for (auto sit = settings.begin(); sit != settings.end(); ++sit) {
                map[sit.key()] = sit.value().toVariant();
            }
            m_providerSettings[it.key()] = map;
        }

        QJsonArray orderArr = m_config.value("providerOrder").toArray();
        m_providerOrder.clear();
        for (const auto& v : orderArr) {
            m_providerOrder.append(v.toString());
        }
    }
}

void SettingsStore::saveConfig() {
    QJsonObject obj;
    QJsonObject providers;
    for (auto it = m_providerSettings.constBegin(); it != m_providerSettings.constEnd(); ++it) {
        QJsonObject settings;
        for (auto sit = it.value().constBegin(); sit != it.value().constEnd(); ++sit) {
            settings[sit.key()] = QJsonValue::fromVariant(sit.value());
        }
        providers[it.key()] = settings;
    }
    obj["providers"] = providers;

    QJsonArray orderArr;
    for (const auto& id : m_providerOrder) {
        orderArr.append(id);
    }
    obj["providerOrder"] = orderArr;

    QDir().mkpath(QFileInfo(configPath()).absolutePath());
    QFile file(configPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}

void SettingsStore::resetToDefaults() {
    setRefreshFrequency(5);
    setLaunchAtLogin(false);
    setCheckForUpdates(true);
    setDebugMenuEnabled(false);
    setMergeIcons(true);
    setStatusChecksEnabled(true);
    setUsageBarsShowUsed(false);
    setResetTimesShowAbsolute(false);
    setShowOptionalCreditsAndExtraUsage(true);
    setSessionQuotaNotificationsEnabled(true);
    setLanguage("en");
    m_providerSettings.clear();
    m_providerOrder.clear();
    saveConfig();
    emit providerOrderChanged();
}
