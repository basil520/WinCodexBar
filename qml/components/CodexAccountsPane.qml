import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import CodexBar 1.0
import ".."

Rectangle {
    id: root
    color: AppTheme.bgSecondary
    radius: 8

    property var accounts: []
    property string activeAccountID: ""
    property bool isAuthenticating: false
    property bool isRemoving: false
    property string authenticatingAccountID: ""
    property string removingAccountID: ""
    property bool hasUnreadableStore: false
    property string authState: "idle"
    property string authMessage: ""
    property string authError: ""
    property string verificationUri: ""
    property string userCode: ""

    signal setActiveAccount(string accountID)
    signal addAccount()
    signal cancelAuthentication()
    signal openVerificationUrl(string url)
    signal copyText(string text)
    signal removeAccount(string accountID)
    signal reauthenticateAccount(string accountID)
    signal promoteAccount(string accountID)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: qsTr("Accounts")
                color: AppTheme.textPrimary
                font.pixelSize: 16
                font.bold: true
            }

            Button {
                text: root.isAuthenticating && root.authenticatingAccountID === ""
                    ? qsTr("Adding...")
                    : qsTr("Add Account")
                enabled: !root.hasUnreadableStore && !root.isAuthenticating && !root.isRemoving
                onClicked: root.addAccount()

                background: Rectangle {
                    color: parent.enabled ? AppTheme.accentColor : AppTheme.bgTertiary
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "white" : AppTheme.textSecondary
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: authLayout.implicitHeight + 18
            radius: 6
            color: root.authError !== ""
                ? Qt.rgba(AppTheme.statusOutage.r, AppTheme.statusOutage.g, AppTheme.statusOutage.b, 0.12)
                : Qt.rgba(AppTheme.statusDegraded.r, AppTheme.statusDegraded.g, AppTheme.statusDegraded.b, 0.12)
            border.width: 1
            border.color: root.authError !== ""
                ? Qt.rgba(AppTheme.statusOutage.r, AppTheme.statusOutage.g, AppTheme.statusOutage.b, 0.40)
                : Qt.rgba(AppTheme.statusDegraded.r, AppTheme.statusDegraded.g, AppTheme.statusDegraded.b, 0.40)
            visible: root.isAuthenticating || root.authError !== "" || root.userCode !== ""

            ColumnLayout {
                id: authLayout
                anchors.fill: parent
                anchors.margins: 9
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: root.authError !== ""
                        ? root.authError
                        : (root.authMessage !== "" ? root.authMessage : qsTr("Waiting for Codex authorization..."))
                    color: root.authError !== "" ? AppTheme.statusOutage : AppTheme.statusDegraded
                    font.pixelSize: AppTheme.fontSizeSm
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: root.userCode !== "" || root.verificationUri !== ""

                    Rectangle {
                        Layout.preferredWidth: Math.max(110, codeLabel.implicitWidth + 18)
                        Layout.preferredHeight: 30
                        radius: 5
                        color: AppTheme.bgPrimary
                        border.width: 1
                        border.color: AppTheme.borderColor
                        visible: root.userCode !== ""

                        Label {
                            id: codeLabel
                            anchors.centerIn: parent
                            text: root.userCode
                            color: AppTheme.textPrimary
                            font.pixelSize: AppTheme.fontSizeMd
                            font.bold: true
                        }
                    }

                    Button {
                        text: qsTr("Open")
                        enabled: root.verificationUri !== ""
                        onClicked: root.openVerificationUrl(root.verificationUri)
                    }

                    Button {
                        text: qsTr("Copy")
                        enabled: root.userCode !== "" || root.verificationUri !== ""
                        onClicked: root.copyText(root.userCode !== "" ? root.userCode : root.verificationUri)
                    }

                    Button {
                        text: qsTr("Cancel")
                        enabled: root.isAuthenticating
                        onClicked: root.cancelAuthentication()
                    }

                    Item { Layout.fillWidth: true }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Select the active Codex account for usage tracking.")
            color: AppTheme.textSecondary
            font.pixelSize: AppTheme.fontSizeSm
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: AppTheme.borderColor
        }

        ListView {
            id: accountList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.accounts
            spacing: 8

            delegate: Rectangle {
                width: accountList.width
                height: 72
                color: modelData.isActive ? AppTheme.bgTertiary : "transparent"
                radius: 6
                border.color: modelData.isActive ? AppTheme.accentColor : AppTheme.borderColor
                border.width: modelData.isActive ? 2 : 1

                MouseArea {
                    anchors.fill: parent
                    z: 0
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!modelData.isActive) {
                            root.setActiveAccount(modelData.id)
                        }
                    }
                }

                RowLayout {
                    z: 1
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        Layout.alignment: Qt.AlignTop
                        radius: 20
                        color: modelData.isLive ? "#4CAF50" : AppTheme.accentColor

                        Label {
                            anchors.centerIn: parent
                            text: modelData.isLive ? "S" : (modelData.email ? modelData.email.charAt(0).toUpperCase() : "A")
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: modelData.displayName
                                color: AppTheme.textPrimary
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                visible: modelData.isLive
                                Layout.preferredWidth: liveLabel.implicitWidth + 12
                                Layout.preferredHeight: 20
                                Layout.alignment: Qt.AlignVCenter
                                radius: 4
                                color: "#4CAF50"

                                Label {
                                    id: liveLabel
                                    anchors.centerIn: parent
                                    text: qsTr("System")
                                    color: "white"
                                    font.pixelSize: 10
                                }
                            }

                            Rectangle {
                                visible: modelData.isActive
                                Layout.preferredWidth: activeLabel.implicitWidth + 12
                                Layout.preferredHeight: 20
                                Layout.alignment: Qt.AlignVCenter
                                radius: 4
                                color: AppTheme.accentColor

                                Label {
                                    id: activeLabel
                                    anchors.centerIn: parent
                                    text: qsTr("Active")
                                    color: "white"
                                    font.pixelSize: 10
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: modelData.email || qsTr("No email")
                            color: AppTheme.textSecondary
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignTop
                        spacing: 4

                        Button {
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 28
                            width: 46
                            height: 28
                            visible: !modelData.isActive
                            enabled: !root.isAuthenticating && !root.isRemoving
                            onClicked: root.setActiveAccount(modelData.id)

                            background: Rectangle {
                                color: parent.hovered ? AppTheme.bgTertiary : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: qsTr("Use")
                                color: AppTheme.accentColor
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            ToolTip.text: qsTr("Set as active")
                            ToolTip.visible: hovered
                        }

                        Button {
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 28
                            width: 46
                            height: 28
                            visible: !modelData.isLive
                            enabled: !root.isAuthenticating && !root.isRemoving
                            onClicked: root.reauthenticateAccount(modelData.id)

                            background: Rectangle {
                                color: parent.hovered ? AppTheme.bgTertiary : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: qsTr("Auth")
                                color: AppTheme.textSecondary
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            ToolTip.text: root.isAuthenticating && root.authenticatingAccountID === modelData.id
                                ? qsTr("Re-authenticating...")
                                : qsTr("Re-authenticate")
                            ToolTip.visible: hovered
                        }

                        Button {
                            Layout.preferredWidth: 58
                            Layout.preferredHeight: 28
                            width: 58
                            height: 28
                            visible: !modelData.isLive
                            enabled: !root.isAuthenticating && !root.isRemoving
                            onClicked: root.promoteAccount(modelData.id)

                            background: Rectangle {
                                color: parent.hovered ? "#4CAF50" : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: qsTr("Promote")
                                color: "#4CAF50"
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            ToolTip.text: qsTr("Promote to system account")
                            ToolTip.visible: hovered
                        }

                        Button {
                            Layout.preferredWidth: 58
                            Layout.preferredHeight: 28
                            width: 58
                            height: 28
                            visible: !modelData.isLive
                            enabled: !root.isAuthenticating && !root.isRemoving
                            onClicked: root.removeAccount(modelData.id)

                            background: Rectangle {
                                color: parent.hovered ? "#ff4444" : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: qsTr("Remove")
                                color: root.isRemoving && root.removingAccountID === modelData.id
                                    ? AppTheme.textSecondary
                                    : "#ff4444"
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            ToolTip.text: root.isRemoving && root.removingAccountID === modelData.id
                                ? qsTr("Removing...")
                                : qsTr("Remove account")
                            ToolTip.visible: hovered
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.accounts.length === 0
            text: qsTr("No accounts configured. Click 'Add Account' to add a Codex account.")
            color: AppTheme.textSecondary
            font.pixelSize: AppTheme.fontSizeSm
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
