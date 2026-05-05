import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ".."

ColumnLayout {
    id: root
    property string providerId: ""
    property var descriptor: ({})
    property var accounts: []
    property string defaultAccountId: ""

    signal addAccount(string displayName, int sourceMode, string apiKey)
    signal removeAccount(string accountId)
    signal setDefaultAccount(string accountId)
    signal setSourceMode(string accountId, int sourceMode)
    signal setVisibility(string accountId, int visibility)

    spacing: 8

    function tokenAccountConfig() {
        return descriptor && descriptor.tokenAccount ? descriptor.tokenAccount : ({})
    }

    function requiresApiKey() {
        var creds = tokenAccountConfig().requiredCredentialTypes || []
        for (var i = 0; i < creds.length; i++) {
            if (creds[i] === "apiKey") return true
        }
        return false
    }

    function modeValue(mode) {
        if (mode === "web") return 1
        if (mode === "cli") return 2
        if (mode === "oauth") return 3
        if (mode === "api") return 4
        return 0
    }

    function modeName(value) {
        if (value === 1) return "web"
        if (value === 2) return "cli"
        if (value === 3) return "oauth"
        if (value === 4) return "api"
        return "auto"
    }

    function visibilityValue(visibility) {
        if (visibility === "hidden") return 1
        if (visibility === "archived") return 2
        return 0
    }

    function visibilityOptions() {
        return [
            { value: 0, label: qsTr("Visible") },
            { value: 1, label: qsTr("Hidden") }
        ]
    }

    function sourceModeOptions() {
        var modes = descriptor && descriptor.sourceModes ? descriptor.sourceModes : []
        if (modes.length === 0) modes = ["auto"]
        var result = []
        for (var i = 0; i < modes.length; i++) {
            var mode = modes[i]
            result.push({ value: modeValue(mode), label: mode.toUpperCase() })
        }
        return result
    }

    function selectedModeFor(account) {
        return modeValue(account && account.sourceMode ? account.sourceMode : "auto")
    }

    function resetAddFields() {
        accountNameField.text = ""
        accountApiKeyField.text = ""
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        TextField {
            id: accountNameField
            objectName: "accountNameField"
            Layout.fillWidth: true
            placeholderText: qsTr("Account name")
            placeholderTextColor: AppTheme.textTertiary
            color: AppTheme.textPrimary
            font.pixelSize: AppTheme.fontSizeSm
            background: Rectangle {
                radius: 6
                color: AppTheme.bgPrimary
                border.width: 1
                border.color: parent.activeFocus ? AppTheme.borderAccent : AppTheme.borderColor
            }
        }

        SettingsComboBox {
            id: addSourceMode
            Layout.preferredWidth: 112
            model: root.sourceModeOptions()
            selectedValue: model.length > 0 ? model[0].value : 0
        }

        TextField {
            id: accountApiKeyField
            objectName: "accountApiKeyField"
            Layout.preferredWidth: 190
            visible: root.requiresApiKey()
            placeholderText: qsTr("API key")
            placeholderTextColor: AppTheme.textTertiary
            echoMode: TextInput.Password
            color: AppTheme.textPrimary
            font.pixelSize: AppTheme.fontSizeSm
            background: Rectangle {
                radius: 6
                color: AppTheme.bgPrimary
                border.width: 1
                border.color: parent.activeFocus ? AppTheme.borderAccent : AppTheme.borderColor
            }
        }

        SettingsButton {
            objectName: "addAccountButton"
            text: qsTr("Add Account")
            primary: true
            enabled: accountNameField.text.trim() !== ""
                && (!root.requiresApiKey() || accountApiKeyField.text.trim() !== "")
            onClicked: {
                root.addAccount(accountNameField.text.trim(),
                                addSourceMode.selectedValue,
                                accountApiKeyField.text.trim())
                root.resetAddFields()
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: !root.accounts || root.accounts.length === 0
        text: qsTr("No accounts configured.")
        color: AppTheme.textTertiary
        font.pixelSize: AppTheme.fontSizeSm
    }

    Repeater {
        model: root.accounts || []

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: accountRow.implicitHeight + 16
            radius: 7
            color: AppTheme.bgPrimary
            border.width: 1
            border.color: AppTheme.borderColor

            RowLayout {
                id: accountRow
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: modelData.displayName || modelData.accountId
                        color: AppTheme.textPrimary
                        font.pixelSize: AppTheme.fontSizeSm
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData.visibility === "hidden"
                            ? qsTr("Hidden")
                            : (modelData.sourceMode || "auto").toUpperCase()
                        color: AppTheme.textTertiary
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                }

                Label {
                    visible: modelData.accountId === root.defaultAccountId
                    text: qsTr("Default")
                    color: AppTheme.accentColor
                    font.pixelSize: 10
                    font.bold: true
                }

                SettingsComboBox {
                    Layout.preferredWidth: 104
                    model: root.sourceModeOptions()
                    selectedValue: root.selectedModeFor(modelData)
                    onValueActivated: function(value) {
                        root.setSourceMode(modelData.accountId, value)
                    }
                }

                SettingsComboBox {
                    Layout.preferredWidth: 104
                    model: root.visibilityOptions()
                    selectedValue: root.visibilityValue(modelData.visibility)
                    onValueActivated: function(value) {
                        root.setVisibility(modelData.accountId, value)
                    }
                }

                SettingsButton {
                    text: qsTr("Use")
                    enabled: modelData.accountId !== root.defaultAccountId
                    onClicked: root.setDefaultAccount(modelData.accountId)
                }

                SettingsButton {
                    text: qsTr("Remove")
                    danger: true
                    onClicked: root.removeAccount(modelData.accountId)
                }
            }
        }
    }
}
