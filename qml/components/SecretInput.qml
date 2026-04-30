import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import ".."

RowLayout {
    id: root
    property var secretStatus: ({"configured": false, "source": "none"})
    property string placeholder: ""
    property string providerId: ""
    property string fieldKey: ""

    signal saveRequested(string value)
    signal clearRequested()

    spacing: AppTheme.spacingSm

    TextField {
        id: secretInput
        Layout.fillWidth: true
        implicitHeight: 32
        color: AppTheme.textPrimary
        font.pixelSize: AppTheme.fontSizeMd
        echoMode: TextInput.Password
        enabled: root.secretStatus.source !== "env"
        placeholderText: {
            if (root.secretStatus.configured) {
                if (root.secretStatus.source === "env") return qsTr("Configured by environment")
                return qsTr("Stored in Windows Credential Manager")
            }
            return root.placeholder || qsTr("Enter value...")
        }
        placeholderTextColor: AppTheme.textTertiary

        background: Rectangle {
            radius: AppTheme.radiusSm
            color: AppTheme.bgPrimary
            border.color: parent.activeFocus ? AppTheme.borderAccent : AppTheme.borderColor
            border.width: 1
        }

        onTextEdited: {
            if (text.length > 0) {
                root.saveRequested(text)
            }
        }
    }

    SettingsButton {
        text: qsTr("Clear")
        enabled: root.secretStatus.source !== "env" && root.secretStatus.configured
        onClicked: {
            root.clearRequested()
            secretInput.text = ""
        }
    }
}
