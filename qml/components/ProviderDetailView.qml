import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import CodexBar 1.0
import ".."

ScrollView {
    id: root
    property string providerId: ""
    property var descriptor: null
    property var connectionTest: ({"state": "idle"})
    property var providerStatus: ({"state": "unknown"})
    property var providerError: ""
    property var usageSnapshot: null
    property bool detailsExpanded: false

    property color brandColor: descriptor && descriptor.brandColor ? descriptor.brandColor : AppTheme.accentColor
    property string connectionState: connectionTest && connectionTest.state ? connectionTest.state : "idle"
    property string connectionMessage: connectionTest && connectionTest.message ? connectionTest.message : ""

    signal testConnectionRequested()
    signal refreshRequested()
    signal toggleEnabled(bool enabled)
    signal settingChanged(string key, var value)
    signal secretSaveRequested(string key, string value)
    signal secretClearRequested(string key)

    clip: true
    contentWidth: availableWidth
    ScrollBar.vertical.policy: ScrollBar.AsNeeded

    onProviderIdChanged: detailsExpanded = false

    function statusText(state) {
        if (state === "ok") return qsTr("Operational")
        if (state === "degraded") return qsTr("Degraded")
        if (state === "outage") return qsTr("Outage")
        return qsTr("Unknown")
    }

    function statusColor(state) {
        if (state === "ok" || state === "succeeded") return AppTheme.statusOk
        if (state === "degraded" || state === "testing") return AppTheme.statusDegraded
        if (state === "outage" || state === "failed") return AppTheme.statusOutage
        return AppTheme.statusUnknown
    }

    function connectionTitle() {
        if (connectionState === "testing") return qsTr("Testing connection")
        if (connectionState === "succeeded") return qsTr("Connection OK")
        if (connectionState === "failed") return qsTr("Connection failed")
        return qsTr("Not tested")
    }

    function hasUsage() {
        return usageSnapshot !== null
            && (usageSnapshot.primary !== undefined
                || usageSnapshot.secondary !== undefined
                || usageSnapshot.tertiary !== undefined)
    }

    function durationLabel(ms) {
        var value = Number(ms || 0)
        if (value <= 0) return ""
        if (value < 1000) return Math.round(value) + " ms"
        return (value / 1000.0).toFixed(1) + " s"
    }

    function timeLabel(ms) {
        var value = Number(ms || 0)
        if (value <= 0) return ""
        return Qt.formatDateTime(new Date(value), "yyyy-MM-dd hh:mm:ss")
    }

    Item {
        width: root.availableWidth
        height: body.implicitHeight + 48

        ColumnLayout {
            id: body
            x: 24
            y: 22
            width: Math.max(0, Math.min(root.availableWidth - 48, 760))
            spacing: 12

            SettingsGroupBox {
                color: AppTheme.bgCard

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Image {
                        id: detailIcon
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 42
                        Layout.alignment: Qt.AlignTop
                        source: root.providerId ? "qrc:/icons/ProviderIcon-" + root.providerId + ".svg" : ""
                        fillMode: Image.PreserveAspectFit
                        sourceSize.width: 42
                        sourceSize.height: 42

                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: root.brandColor
                            visible: detailIcon.status !== Image.Ready

                            Label {
                                anchors.centerIn: parent
                                text: root.descriptor && root.descriptor.displayName
                                    ? root.descriptor.displayName.charAt(0).toUpperCase()
                                    : "?"
                                color: "white"
                                font.pixelSize: 18
                                font.bold: true
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: root.descriptor ? root.descriptor.displayName : root.providerId
                                color: AppTheme.textPrimary
                                font.pixelSize: 22
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            StatusPill {
                                text: root.statusText(root.providerStatus && root.providerStatus.state ? root.providerStatus.state : "unknown")
                                toneColor: root.statusColor(root.providerStatus && root.providerStatus.state ? root.providerStatus.state : "unknown")
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.descriptor && root.descriptor.sourceModes && root.descriptor.sourceModes.length > 0
                                ? qsTr("Sources: ") + root.descriptor.sourceModes.join(", ")
                                : qsTr("Descriptor-driven provider settings")
                            color: AppTheme.textSecondary
                            font.pixelSize: AppTheme.fontSizeSm
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            SettingsButton {
                                text: qsTr("Dashboard")
                                visible: root.descriptor && root.descriptor.dashboardURL
                                enabled: visible
                                onClicked: AppController.openExternalUrl(root.descriptor.dashboardURL)
                            }

                            SettingsButton {
                                text: qsTr("Status")
                                visible: root.descriptor && root.descriptor.statusPageURL
                                enabled: visible
                                onClicked: AppController.openExternalUrl(root.descriptor.statusPageURL)
                            }

                            SettingsButton {
                                text: qsTr("Refresh")
                                onClicked: root.refreshRequested()
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: qsTr("Enabled")
                                color: AppTheme.textSecondary
                                font.pixelSize: AppTheme.fontSizeSm
                            }

                            SettingsSwitch {
                                checked: root.descriptor ? root.descriptor.enabled : false
                                onToggled: function(checked) {
                                    root.toggleEnabled(checked)
                                }
                            }
                        }
                    }
                }
            }

            SettingsGroupBox {
                visible: root.hasUsage()

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    SectionTitle { text: qsTr("Usage") }

                    UsageMetricRow {
                        label: root.descriptor ? root.descriptor.sessionLabel : qsTr("Session")
                        metric: root.usageSnapshot ? root.usageSnapshot.primary : null
                        tintColor: root.brandColor
                    }

                    UsageMetricRow {
                        label: root.descriptor ? root.descriptor.weeklyLabel : qsTr("Weekly")
                        metric: root.usageSnapshot ? root.usageSnapshot.secondary : null
                        tintColor: root.brandColor
                    }

                    UsageMetricRow {
                        label: root.descriptor ? root.descriptor.opusLabel || qsTr("Monthly") : qsTr("Monthly")
                        metric: root.usageSnapshot ? root.usageSnapshot.tertiary : null
                        tintColor: root.brandColor
                    }
                }
            }

            SettingsGroupBox {
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        SectionTitle {
                            text: qsTr("Connection")
                            Layout.fillWidth: true
                        }

                        ConnectionStatus {
                            statusState: root.connectionState
                            message: root.connectionState === "idle" ? qsTr("Not tested") : root.connectionMessage
                        }

                        SettingsButton {
                            text: root.connectionState === "testing" ? qsTr("Testing...") : qsTr("Test Connection")
                            primary: true
                            enabled: root.connectionState !== "testing"
                            onClicked: root.testConnectionRequested()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: connectionPanel.implicitHeight + 20
                        radius: 7
                        visible: root.connectionState !== "idle"
                        color: Qt.rgba(root.statusColor(root.connectionState).r,
                                       root.statusColor(root.connectionState).g,
                                       root.statusColor(root.connectionState).b,
                                       root.connectionState === "testing" ? 0.10 : 0.13)
                        border.width: 1
                        border.color: Qt.rgba(root.statusColor(root.connectionState).r,
                                              root.statusColor(root.connectionState).g,
                                              root.statusColor(root.connectionState).b,
                                              0.42)

                        ColumnLayout {
                            id: connectionPanel
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Rectangle {
                                    Layout.preferredWidth: 8
                                    Layout.preferredHeight: 8
                                    radius: 4
                                    color: root.statusColor(root.connectionState)
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: root.connectionTitle()
                                    color: AppTheme.textPrimary
                                    font.pixelSize: AppTheme.fontSizeMd
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: root.durationLabel(root.connectionTest ? root.connectionTest.durationMs : 0)
                                    color: AppTheme.textSecondary
                                    font.pixelSize: AppTheme.fontSizeSm
                                    visible: text !== ""
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.connectionMessage
                                    || (root.connectionState === "testing" ? qsTr("Running one non-interactive provider refresh.") : "")
                                color: AppTheme.textSecondary
                                font.pixelSize: AppTheme.fontSizeSm
                                wrapMode: Text.WrapAnywhere
                                maximumLineCount: 3
                                elide: Text.ElideRight
                                visible: text !== ""
                            }

                            Label {
                                Layout.fillWidth: true
                                text: {
                                    var ts = root.timeLabel(root.connectionTest ? root.connectionTest.finishedAt : 0)
                                    return ts === "" ? "" : qsTr("Last tested: ") + ts
                                }
                                color: AppTheme.textTertiary
                                font.pixelSize: AppTheme.fontSizeSm
                                visible: text !== ""
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.connectionTest && root.connectionTest.details
                                    ? root.connectionTest.details
                                    : root.connectionMessage
                                color: AppTheme.textSecondary
                                font.pixelSize: AppTheme.fontSizeSm
                                wrapMode: Text.WrapAnywhere
                                maximumLineCount: 8
                                elide: Text.ElideRight
                                visible: root.connectionState === "failed" && root.detailsExpanded
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                visible: root.connectionState === "failed"

                                SettingsButton {
                                    text: root.detailsExpanded ? qsTr("Hide Details") : qsTr("Show Details")
                                    onClicked: root.detailsExpanded = !root.detailsExpanded
                                }

                                SettingsButton {
                                    text: qsTr("Copy")
                                    onClicked: AppController.copyText((root.connectionTest && root.connectionTest.details)
                                        ? root.connectionTest.details
                                        : root.connectionMessage)
                                }

                                Item { Layout.fillWidth: true }

                                SettingsButton {
                                    text: qsTr("Retry")
                                    primary: true
                                    onClicked: root.testConnectionRequested()
                                }
                            }
                        }
                    }
                }
            }

            SettingsGroupBox {
                visible: root.descriptor
                    && root.descriptor.settingsFields
                    && root.descriptor.settingsFields.length > 0

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    SectionTitle { text: qsTr("Settings") }

                    Repeater {
                        model: root.descriptor ? root.descriptor.settingsFields : []

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.label
                                    color: AppTheme.textPrimary
                                    font.pixelSize: AppTheme.fontSizeSm
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                StatusPill {
                                    visible: modelData.sensitive === true
                                    text: qsTr("Secret")
                                    toneColor: AppTheme.accentColor
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.helpText || ""
                                color: AppTheme.textTertiary
                                font.pixelSize: AppTheme.fontSizeSm
                                wrapMode: Text.WordWrap
                                visible: text !== ""
                            }

                            Loader {
                                Layout.fillWidth: true
                                sourceComponent: {
                                    if (modelData.type === "secret") return secretFieldComponent
                                    if (modelData.type === "picker") return pickerComponent
                                    if (modelData.type === "bool" || modelData.type === "boolean") return boolFieldComponent
                                    if (modelData.multiline === true) return textAreaComponent
                                    return textFieldComponent
                                }

                                Component {
                                    id: textFieldComponent
                                    TextField {
                                        Layout.fillWidth: true
                                        color: AppTheme.textPrimary
                                        font.pixelSize: AppTheme.fontSizeMd
                                        text: modelData.value || ""
                                        placeholderText: modelData.placeholder || ""
                                        placeholderTextColor: AppTheme.textTertiary

                                        background: Rectangle {
                                            radius: 6
                                            color: AppTheme.bgPrimary
                                            border.color: parent.activeFocus ? AppTheme.borderAccent : AppTheme.borderColor
                                            border.width: 1
                                        }

                                        onTextEdited: function() {
                                            root.settingChanged(modelData.key, text)
                                        }
                                    }
                                }

                                Component {
                                    id: textAreaComponent
                                    TextArea {
                                        Layout.fillWidth: true
                                        implicitHeight: 88
                                        color: AppTheme.textPrimary
                                        font.pixelSize: AppTheme.fontSizeMd
                                        text: modelData.value || ""
                                        placeholderText: modelData.placeholder || ""
                                        placeholderTextColor: AppTheme.textTertiary
                                        wrapMode: TextEdit.Wrap

                                        background: Rectangle {
                                            radius: 6
                                            color: AppTheme.bgPrimary
                                            border.color: parent.activeFocus ? AppTheme.borderAccent : AppTheme.borderColor
                                            border.width: 1
                                        }

                                        onActiveFocusChanged: {
                                            if (!activeFocus) root.settingChanged(modelData.key, text)
                                        }
                                    }
                                }

                                Component {
                                    id: secretFieldComponent
                                    SecretInput {
                                        Layout.fillWidth: true
                                        placeholder: modelData.placeholder || ""
                                        providerId: root.providerId
                                        fieldKey: modelData.key
                                        secretStatus: modelData.secretStatus || ({"configured": false, "source": "none"})
                                        onSaveRequested: function(value) {
                                            root.secretSaveRequested(modelData.key, value)
                                        }
                                        onClearRequested: function() {
                                            root.secretClearRequested(modelData.key)
                                        }
                                    }
                                }

                                Component {
                                    id: pickerComponent
                                    SettingsComboBox {
                                        Layout.fillWidth: true
                                        model: modelData.options || []
                                        selectedValue: modelData.value !== undefined ? modelData.value : modelData.defaultValue
                                        onValueActivated: function(value) {
                                            root.settingChanged(modelData.key, value)
                                        }
                                    }
                                }

                                Component {
                                    id: boolFieldComponent
                                    SettingsSwitch {
                                        checked: modelData.value || false
                                        onToggled: function(checked) {
                                            root.settingChanged(modelData.key, checked)
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: AppTheme.borderColor
                                opacity: 0.55
                                visible: index < (root.descriptor.settingsFields.length - 1)
                            }
                        }
                    }
                }
            }

            SettingsGroupBox {
                visible: root.providerId === "codex"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    SectionTitle { text: qsTr("Account Management") }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Manage multiple Codex accounts. Switch between accounts to track usage separately.")
                        color: AppTheme.textSecondary
                        font.pixelSize: AppTheme.fontSizeSm
                        wrapMode: Text.WordWrap
                    }

                    CodexAccountsPane {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 280
                        visible: root.providerId === "codex"
                        
                        accounts: UsageStore.codexAccounts()
                        activeAccountID: UsageStore.codexActiveAccountID()
                        isAuthenticating: UsageStore.isCodexAuthenticating()
                        isRemoving: UsageStore.isCodexRemoving()
                        authenticatingAccountID: UsageStore.codexAuthenticatingAccountID()
                        removingAccountID: UsageStore.codexRemovingAccountID()
                        hasUnreadableStore: UsageStore.hasCodexUnreadableStore()

                        onSetActiveAccount: function(accountID) {
                            UsageStore.setCodexActiveAccount(accountID)
                        }
                        onAddAccount: function() {
                            var email = "user@example.com"
                            var homePath = ""
                            UsageStore.addCodexAccount(email, homePath)
                        }
                        onRemoveAccount: function(accountID) {
                            UsageStore.removeCodexAccount(accountID)
                        }
                        onReauthenticateAccount: function(accountID) {
                            UsageStore.reauthenticateCodexAccount(accountID)
                        }
                    }
                }
            }

            ProviderErrorCard {
                visible: root.providerError !== ""
                errorTitle: qsTr("Last Provider Error")
                errorMessage: root.providerError
            }
        }
    }

    component SectionTitle: Label {
        color: AppTheme.textPrimary
        font.pixelSize: AppTheme.fontSizeMd
        font.bold: true
        Layout.fillWidth: true
    }

    component StatusPill: Rectangle {
        id: pill
        property string text: ""
        property color toneColor: AppTheme.statusUnknown

        readonly property int desiredWidth: Math.max(58, label.implicitWidth + 18)

        Layout.preferredWidth: desiredWidth
        Layout.minimumWidth: desiredWidth
        Layout.maximumWidth: desiredWidth
        Layout.preferredHeight: 24
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: desiredWidth
        implicitHeight: 24
        radius: 12
        clip: true
        color: Qt.rgba(toneColor.r, toneColor.g, toneColor.b, 0.14)
        border.width: 1
        border.color: Qt.rgba(toneColor.r, toneColor.g, toneColor.b, 0.42)

        Label {
            id: label
            anchors.fill: parent
            anchors.leftMargin: 9
            anchors.rightMargin: 9
            text: pill.text
            color: pill.toneColor
            font.pixelSize: AppTheme.fontSizeSm
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    component UsageMetricRow: RowLayout {
        property string label: ""
        property var metric: null
        property color tintColor: AppTheme.accentColor
        property double percent: metric && metric.percent !== undefined ? metric.percent : 0

        Layout.fillWidth: true
        Layout.preferredHeight: 36
        spacing: 10
        visible: metric !== null && metric !== undefined

        Label {
            Layout.preferredWidth: 116
            text: parent.label
            color: AppTheme.textSecondary
            font.pixelSize: AppTheme.fontSizeSm
            elide: Text.ElideRight
        }

        UsageProgressBar {
            Layout.fillWidth: true
            value: parent.percent
            tintColor: parent.tintColor
        }

        Label {
            Layout.preferredWidth: 48
            text: parent.percent.toFixed(0) + "%"
            color: AppTheme.textPrimary
            font.pixelSize: AppTheme.fontSizeSm
            horizontalAlignment: Text.AlignRight
        }
    }
}
