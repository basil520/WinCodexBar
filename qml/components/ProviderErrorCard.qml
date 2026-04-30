import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import CodexBar 1.0
import ".."

SettingsGroupBox {
    id: root
    property string errorTitle: "Error"
    property string errorMessage: ""
    property bool expanded: false

    color: Qt.rgba(AppTheme.statusOutage.r, AppTheme.statusOutage.g, AppTheme.statusOutage.b, 0.10)
    border.color: Qt.rgba(AppTheme.statusOutage.r, AppTheme.statusOutage.g, AppTheme.statusOutage.b, 0.35)
    border.width: 1

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                color: AppTheme.statusOutage
            }

            Label {
                text: root.errorTitle
                color: AppTheme.statusOutage
                font.pixelSize: AppTheme.fontSizeMd
                font.bold: true
                Layout.fillWidth: true
            }
        }

        Label {
            text: root.errorMessage
            color: AppTheme.textPrimary
            font.pixelSize: AppTheme.fontSizeSm
            wrapMode: Text.WrapAnywhere
            Layout.fillWidth: true
            visible: root.expanded
            maximumLineCount: 10
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            SettingsButton {
                text: root.expanded ? qsTr("Hide Details") : qsTr("Show Details")
                onClicked: root.expanded = !root.expanded
            }

            SettingsButton {
                text: qsTr("Copy")
                onClicked: AppController.copyText(root.errorMessage)
            }
        }
    }
}
