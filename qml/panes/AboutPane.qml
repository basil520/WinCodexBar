import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import CodexBar 1.0
import ".."
import "../components"

SettingsPage {
    title: qsTr("About")
    subtitle: qsTr("Build information and project links.")

    SettingsGroupBox {
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: 10
                color: AppTheme.accentColor

                Label {
                    anchors.centerIn: parent
                    text: "C"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: qsTr("CodexBar")
                    color: AppTheme.textPrimary
                    font.pixelSize: AppTheme.fontSizeXl
                    font.bold: true
                }

                Label {
                    text: qsTr("Version ") + "0.1.0"
                    color: AppTheme.textSecondary
                    font.pixelSize: AppTheme.fontSizeSm
                }

                Label {
                    text: qsTr("Windows system tray app for tracking AI provider usage limits.")
                    color: AppTheme.textSecondary
                    font.pixelSize: AppTheme.fontSizeSm
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
    }

    SettingsGroupBox {
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            spacing: 12

            Label {
                Layout.fillWidth: true
                text: qsTr("Project Repository")
                color: AppTheme.textPrimary
                font.pixelSize: AppTheme.fontSizeMd
                font.bold: true
            }

            SettingsButton {
                text: qsTr("Open GitHub")
                onClicked: AppController.openExternalUrl("https://github.com/anomalyco/CodexBar")
            }
        }
    }
}
