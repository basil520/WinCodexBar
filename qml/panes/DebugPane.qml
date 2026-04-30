import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import CodexBar 1.0
import ".."
import "../components"

SettingsPage {
    title: qsTr("Debug")
    subtitle: qsTr("Manual actions for local validation and troubleshooting.")

    SettingsGroupBox {
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Label {
                    text: qsTr("Refresh All Providers")
                    color: AppTheme.textPrimary
                    font.pixelSize: AppTheme.fontSizeMd
                    font.bold: true
                }

                Label {
                    text: qsTr("Run a normal refresh for every enabled provider.")
                    color: AppTheme.textSecondary
                    font.pixelSize: AppTheme.fontSizeSm
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            SettingsButton {
                text: qsTr("Refresh")
                primary: true
                onClicked: UsageStore.refreshAll()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Label {
                    text: qsTr("Clear Cache")
                    color: AppTheme.textPrimary
                    font.pixelSize: AppTheme.fontSizeMd
                    font.bold: true
                }

                Label {
                    text: qsTr("Drop in-memory snapshots and connection test results.")
                    color: AppTheme.textSecondary
                    font.pixelSize: AppTheme.fontSizeSm
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            SettingsButton {
                text: qsTr("Clear")
                onClicked: UsageStore.clearCache()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Label {
                    text: qsTr("Reset Settings")
                    color: AppTheme.textPrimary
                    font.pixelSize: AppTheme.fontSizeMd
                    font.bold: true
                }

                Label {
                    text: qsTr("Restore app-level settings and provider order.")
                    color: AppTheme.textSecondary
                    font.pixelSize: AppTheme.fontSizeSm
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            SettingsButton {
                text: qsTr("Reset")
                danger: true
                onClicked: SettingsStore.resetToDefaults()
            }
        }
    }
}
