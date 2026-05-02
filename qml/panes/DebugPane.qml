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

    SettingsGroupBox {
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: qsTr("Codex Diagnostics")
                color: AppTheme.textPrimary
                font.pixelSize: AppTheme.fontSizeMd
                font.bold: true
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3

                    Label {
                        text: qsTr("Test Codex Connection")
                        color: AppTheme.textPrimary
                        font.pixelSize: AppTheme.fontSizeMd
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Run a single fetch attempt and record diagnostics.")
                        color: AppTheme.textSecondary
                        font.pixelSize: AppTheme.fontSizeSm
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                SettingsButton {
                    text: qsTr("Test")
                    primary: true
                    onClicked: UsageStore.testProviderConnection("codex")
                }
            }

            // Last known session window source
            RowLayout {
                Layout.fillWidth: true
                visible: UsageStore.lastKnownSessionWindowSource !== ""
                spacing: 8
                Label {
                    text: qsTr("Last session source")
                    color: AppTheme.textSecondary
                    font.pixelSize: AppTheme.fontSizeSm
                }
                Label {
                    text: UsageStore.lastKnownSessionWindowSource
                    color: AppTheme.textPrimary
                    font.pixelSize: AppTheme.fontSizeSm
                    font.bold: true
                }
            }

            // Fetch attempts list
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                visible: UsageStore.codexFetchAttempts.length > 0

                Label {
                    text: qsTr("Recent Fetch Attempts")
                    color: AppTheme.textPrimary
                    font.pixelSize: AppTheme.fontSizeMd
                    font.bold: true
                }

                Repeater {
                    model: UsageStore.codexFetchAttempts
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label {
                            text: modelData.strategyID || ""
                            color: AppTheme.textSecondary
                            font.pixelSize: AppTheme.fontSizeSm
                            Layout.preferredWidth: 120
                        }
                        Label {
                            text: modelData.available ? (modelData.success ? "OK" : "FAIL") : "N/A"
                            color: modelData.available ? (modelData.success ? "#4CAF50" : "#F44336") : "#888"
                            font.pixelSize: AppTheme.fontSizeSm
                            font.bold: true
                            Layout.preferredWidth: 40
                        }
                        Label {
                            text: modelData.errorMessage || ""
                            color: AppTheme.textSecondary
                            font.pixelSize: AppTheme.fontSizeSm
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }
                    }
                }
            }
        }
    }
}
