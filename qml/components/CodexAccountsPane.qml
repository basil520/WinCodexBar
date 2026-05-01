import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import CodexBar 1.0
import ".."
import "../components"

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

    signal setActiveAccount(string accountID)
    signal addAccount()
    signal removeAccount(string accountID)
    signal reauthenticateAccount(string accountID)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: qsTr("Accounts")
                color: AppTheme.textPrimary
                font.pixelSize: 16
                font.bold: true
                Layout.fillWidth: true
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
                }
            }
        }

        Label {
            text: qsTr("Select the active Codex account for usage tracking.")
            color: AppTheme.textSecondary
            font.pixelSize: AppTheme.fontSizeSm
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
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

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Rectangle {
                        width: 40
                        height: 40
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
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: modelData.displayName
                                color: AppTheme.textPrimary
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                visible: modelData.isLive
                                width: liveLabel.implicitWidth + 12
                                height: 20
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
                                width: activeLabel.implicitWidth + 12
                                height: 20
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
                            text: modelData.email || qsTr("No email")
                            color: AppTheme.textSecondary
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        spacing: 4

                        Button {
                            width: 28
                            height: 28
                            visible: !modelData.isActive
                            enabled: !root.isAuthenticating && !root.isRemoving
                            onClicked: root.setActiveAccount(modelData.id)

                            background: Rectangle {
                                color: parent.hovered ? AppTheme.bgTertiary : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: "✓"
                                color: AppTheme.accentColor
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            ToolTip.text: qsTr("Set as active")
                            ToolTip.visible: hovered
                        }

                        Button {
                            width: 28
                            height: 28
                            visible: !modelData.isLive
                            enabled: !root.isAuthenticating && !root.isRemoving
                            onClicked: root.reauthenticateAccount(modelData.id)

                            background: Rectangle {
                                color: parent.hovered ? AppTheme.bgTertiary : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: "🔄"
                                color: AppTheme.textSecondary
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            ToolTip.text: root.isAuthenticating && root.authenticatingAccountID === modelData.id 
                                ? qsTr("Re-authenticating...") 
                                : qsTr("Re-authenticate")
                            ToolTip.visible: hovered
                        }

                        Button {
                            width: 28
                            height: 28
                            visible: !modelData.isLive
                            enabled: !root.isAuthenticating && !root.isRemoving
                            onClicked: root.removeAccount(modelData.id)

                            background: Rectangle {
                                color: parent.hovered ? "#ff4444" : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: "×"
                                color: root.isRemoving && root.removingAccountID === modelData.id 
                                    ? AppTheme.textSecondary 
                                    : "#ff4444"
                                font.pixelSize: 16
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            ToolTip.text: root.isRemoving && root.removingAccountID === modelData.id 
                                ? qsTr("Removing...") 
                                : qsTr("Remove account")
                            ToolTip.visible: hovered
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!modelData.isActive) {
                            root.setActiveAccount(modelData.id)
                        }
                    }
                }
            }
        }

        Label {
            visible: root.accounts.length === 0
            text: qsTr("No accounts configured. Click 'Add Account' to add a Codex account.")
            color: AppTheme.textSecondary
            font.pixelSize: AppTheme.fontSizeSm
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
