import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import ".."

RowLayout {
    id: root
    property string title: ""
    property string subtitle: ""
    property bool checked: false
    signal toggled(bool checked)

    Layout.fillWidth: true
    Layout.preferredHeight: 56
    spacing: 12

    ColumnLayout {
        Layout.fillWidth: true
        spacing: AppTheme.spacingXs

        Label {
            text: root.title
            color: AppTheme.textPrimary
            font.pixelSize: AppTheme.fontSizeMd
            font.bold: true
        }

        Label {
            text: root.subtitle
            color: AppTheme.textSecondary
            font.pixelSize: AppTheme.fontSizeSm
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            visible: root.subtitle !== ""
        }
    }

    SettingsSwitch {
        id: toggleSwitch
        Layout.alignment: Qt.AlignVCenter
        checked: root.checked
        onToggled: function(checked) {
            root.toggled(checked)
        }
    }
}
