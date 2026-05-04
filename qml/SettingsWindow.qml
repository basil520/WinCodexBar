import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CodexBar 1.0
import "panes"

Rectangle {
    id: settingsWindow
    width: 900
    color: AppTheme.bgPrimary

    property int rev: LanguageManager.translationRevision
    property var tabs: {
        settingsWindow.rev
        var items = [
            { label: qsTr("General"), icon: "G" },
            { label: qsTr("Providers"), icon: "P" },
            { label: qsTr("Display"), icon: "D" },
            { label: qsTr("Advanced"), icon: "A" },
            { label: qsTr("About"), icon: "i" }
        ]
        if (SettingsStore.debugMenuEnabled) items.push({ label: qsTr("Debug"), icon: "{}" })
        return items
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 1
        border.color: AppTheme.borderColor
        z: 20
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: titleBar
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#10101f"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: AppTheme.borderColor
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 8
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    radius: 5
                    color: AppTheme.accentColor

                    Label {
                        anchors.centerIn: parent
                        text: "C"
                        color: "white"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                Label {
                    text: qsTr("CodexBar Settings")
                    color: AppTheme.textPrimary
                    font.pixelSize: AppTheme.fontSizeMd
                    font.bold: true
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onPressed: AppController.startSettingsMove()
                        onDoubleClicked: AppController.toggleSettingsMaximized()
                    }
                }

                TitleButton {
                    text: "_"
                    onClicked: AppController.minimizeSettings()
                }

                TitleButton {
                    text: AppController.settingsMaximized ? "[]" : "[ ]"
                    onClicked: AppController.toggleSettingsMaximized()
                }

                TitleButton {
                    text: "x"
                    danger: true
                    onClicked: AppController.closeSettings()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                color: AppTheme.bgSecondary

                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 1
                    color: AppTheme.borderColor
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    Label {
                        text: qsTr("Settings")
                        color: AppTheme.textPrimary
                        font.pixelSize: AppTheme.fontSizeXl
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Configure providers and app behavior")
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
                        id: tabList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: 0
                        spacing: 4
                        clip: true
                        model: settingsWindow.tabs

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 38
                            radius: 6
                            color: tabList.currentIndex === index
                                ? AppTheme.bgSelected
                                : (tabMouse.containsMouse ? AppTheme.bgHover : "transparent")

                            Rectangle {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 3
                                height: 22
                                radius: 2
                                color: AppTheme.accentColor
                                visible: tabList.currentIndex === index
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 10
                                spacing: 10

                                Rectangle {
                                    Layout.preferredWidth: 22
                                    Layout.preferredHeight: 22
                                    radius: 5
                                    color: tabList.currentIndex === index
                                        ? AppTheme.accentColor
                                        : AppTheme.bgCard

                                    Label {
                                        anchors.centerIn: parent
                                        text: modelData.icon
                                        color: tabList.currentIndex === index ? "white" : AppTheme.textSecondary
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.label
                                    color: tabList.currentIndex === index ? AppTheme.textPrimary : AppTheme.textSecondary
                                    font.pixelSize: AppTheme.fontSizeMd
                                    elide: Text.ElideRight
                                }
                            }

                            MouseArea {
                                id: tabMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: tabList.currentIndex = index
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: AppTheme.bgPrimary

                StackLayout {
                    id: contentStack
                    anchors.fill: parent
                    currentIndex: tabList.currentIndex

                    GeneralPane {}

                    ProvidersPane {
                        id: providersPane
                        providers: UsageStore.providerList()
                        selectedProvider: ""
                        selectedDescriptor: null
                        selectedConnectionTest: ({"state": "idle", "message": "", "details": "", "startedAt": 0, "finishedAt": 0, "durationMs": 0})
                        selectedProviderStatus: ({"state": "unknown"})
                        selectedProviderError: ""
                        selectedUsageSnapshot: null

                        function reloadProvider(providerId) {
                            if (providerId === "") return
                            providersPane.selectedProvider = providerId
                            providersPane.selectedDescriptor = UsageStore.providerDescriptorData(providerId)
                            providersPane.selectedConnectionTest = UsageStore.providerConnectionTest(providerId)
                            providersPane.selectedProviderStatus = UsageStore.providerStatus(providerId)
                            providersPane.selectedProviderError = UsageStore.providerError(providerId)
                            providersPane.selectedUsageSnapshot = UsageStore.providerUsageSnapshot(providerId)
                        }

                        onProviderSelected: function(providerId) {
                            reloadProvider(providerId)
                        }
                        onProviderEnabled: function(providerId, enabled) {
                            UsageStore.setProviderEnabled(providerId, enabled)
                            providersPane.providers = UsageStore.providerList()
                            reloadProvider(providerId)
                        }
                        onTestConnection: function(providerId) {
                            UsageStore.testProviderConnection(providerId)
                            reloadProvider(providerId)
                        }
                        onRefreshProvider: function(providerId) {
                            UsageStore.refreshProvider(providerId)
                        }
                        onSettingChanged: function(providerId, key, value) {
                            UsageStore.setProviderSetting(providerId, key, value)
                            reloadProvider(providerId)
                        }
                        onSecretSaveRequested: function(providerId, key, value) {
                            UsageStore.setProviderSecret(providerId, key, value)
                            reloadProvider(providerId)
                        }
                        onSecretClearRequested: function(providerId, key) {
                            UsageStore.clearProviderSecret(providerId, key)
                            reloadProvider(providerId)
                        }

                        Connections {
                            enabled: settingsWindow.visible
                            target: UsageStore
                            function onProviderIDsChanged() {
                                providersPane.providers = UsageStore.providerList()
                            }
                            function onProviderConnectionTestChanged(providerId) {
                                providersPane.providers = UsageStore.providerList()
                                if (providersPane.selectedProvider === providerId) providersPane.reloadProvider(providerId)
                            }
                            function onProviderStatusChanged(providerId) {
                                providersPane.providers = UsageStore.providerList()
                                if (providersPane.selectedProvider === providerId) providersPane.reloadProvider(providerId)
                            }
                            function onSnapshotChanged(providerId) {
                                providersPane.providers = UsageStore.providerList()
                                if (providersPane.selectedProvider === providerId) providersPane.reloadProvider(providerId)
                            }
                        }
                    }

                    DisplayPane {}
                    AdvancedPane {}
                    AboutPane {}
                    DebugPane {}
                }
            }
        }
    }

    ResizeHandle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; edge: Qt.LeftEdge }
    ResizeHandle { anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; edge: Qt.RightEdge }
    ResizeHandle { anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; edge: Qt.TopEdge }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; edge: Qt.BottomEdge }
    ResizeHandle { anchors.left: parent.left; anchors.top: parent.top; width: 10; height: 10; edge: Qt.LeftEdge | Qt.TopEdge }
    ResizeHandle { anchors.right: parent.right; anchors.top: parent.top; width: 10; height: 10; edge: Qt.RightEdge | Qt.TopEdge }
    ResizeHandle { anchors.left: parent.left; anchors.bottom: parent.bottom; width: 10; height: 10; edge: Qt.LeftEdge | Qt.BottomEdge }
    ResizeHandle { anchors.right: parent.right; anchors.bottom: parent.bottom; width: 10; height: 10; edge: Qt.RightEdge | Qt.BottomEdge }

    component TitleButton: Rectangle {
        id: titleButton
        property alias text: label.text
        property bool danger: false
        signal clicked()

        Layout.preferredWidth: 36
        Layout.preferredHeight: 30
        radius: 5
        color: mouseArea.containsMouse
            ? (danger ? AppTheme.statusOutage : AppTheme.bgHover)
            : "transparent"

        Label {
            id: label
            anchors.centerIn: parent
            color: "white"
            font.pixelSize: 13
            font.bold: true
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: titleButton.clicked()
        }
    }

    component ResizeHandle: MouseArea {
        property int edge: Qt.LeftEdge
        width: 6
        height: 6
        hoverEnabled: true
        z: 30
        onPressed: AppController.startSettingsResize(edge)
    }
}
