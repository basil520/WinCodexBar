import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import CodexBar 1.0
import ".."
import "../components"

SettingsPage {
    title: qsTr("Advanced")
    subtitle: qsTr("Refresh cadence, status polling, and diagnostic controls.")

    SettingsGroupBox {
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Label {
                    text: qsTr("Refresh Frequency")
                    color: AppTheme.textPrimary
                    font.pixelSize: AppTheme.fontSizeMd
                    font.bold: true
                }

                Label {
                    text: qsTr("How often enabled providers refresh automatically.")
                    color: AppTheme.textSecondary
                    font.pixelSize: AppTheme.fontSizeSm
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            SettingsComboBox {
                Layout.preferredWidth: 180
                model: [
                    { value: 1, label: qsTr("Every minute") },
                    { value: 5, label: qsTr("Every 5 minutes") },
                    { value: 15, label: qsTr("Every 15 minutes") },
                    { value: 30, label: qsTr("Every 30 minutes") },
                    { value: 0, label: qsTr("Manual only") }
                ]
                selectedValue: SettingsStore.refreshFrequency
                onValueActivated: function(value) {
                    SettingsStore.refreshFrequency = value
                }
            }
        }

        SettingsToggleRow {
            title: qsTr("Check Provider Status")
            subtitle: qsTr("Poll statuspage-style endpoints for provider health.")
            checked: SettingsStore.statusChecksEnabled
            onToggled: function(checked) {
                SettingsStore.statusChecksEnabled = checked
            }
        }
    }

    SettingsGroupBox {
        SettingsToggleRow {
            title: qsTr("Debug Mode")
            subtitle: qsTr("Show debug menus and keep more diagnostic information visible.")
            checked: SettingsStore.debugMenuEnabled
            onToggled: function(checked) {
                SettingsStore.debugMenuEnabled = checked
            }
        }

        SettingsToggleRow {
            title: qsTr("Codex Verbose Logging")
            subtitle: qsTr("Log detailed strategy-level diagnostics during Codex fetches.")
            checked: SettingsStore.codexVerboseLogging
            onToggled: function(checked) {
                SettingsStore.codexVerboseLogging = checked
            }
        }

        SettingsToggleRow {
            title: qsTr("Web Dashboard Debug Dump")
            subtitle: qsTr("Save raw HTML from web dashboard fetches for troubleshooting.")
            checked: SettingsStore.codexWebDebugDumpHTML
            onToggled: function(checked) {
                SettingsStore.codexWebDebugDumpHTML = checked
            }
        }
    }
}
