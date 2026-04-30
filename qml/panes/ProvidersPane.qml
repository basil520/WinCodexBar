import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import CodexBar 1.0
import ".."
import "../components"

Rectangle {
    id: root
    color: AppTheme.bgPrimary

    property var providers: []
    property string selectedProvider: ""
    property var selectedDescriptor: null
    property var selectedConnectionTest: ({"state": "idle"})
    property var selectedProviderStatus: ({"state": "unknown"})
    property var selectedProviderError: ""
    property var selectedUsageSnapshot: null

    signal providerSelected(string providerId)
    signal providerEnabled(string providerId, bool enabled)
    signal testConnection(string providerId)
    signal refreshProvider(string providerId)
    signal settingChanged(string providerId, string key, var value)
    signal secretSaveRequested(string providerId, string key, string value)
    signal secretClearRequested(string providerId, string key)

    function selectFirstProviderIfNeeded() {
        if (root.selectedProvider !== "") return
        if (!root.providers || root.providers.length === 0) return
        root.providerSelected(root.providers[0].id)
    }

    Component.onCompleted: selectFirstProviderIfNeeded()
    onProvidersChanged: selectFirstProviderIfNeeded()

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 268
            Layout.fillHeight: true
            color: AppTheme.bgPrimary

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: AppTheme.borderColor
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                Label {
                    text: qsTr("Providers")
                    color: AppTheme.textPrimary
                    font.pixelSize: 22
                    font.bold: true
                }

                Label {
                    text: qsTr("Enable, order, and test each data source.")
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
                    id: providerList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.providers
                    spacing: 4

                    delegate: ProviderListItem {
                        width: providerList.width
                        providerName: modelData.name
                        providerId: modelData.id
                        brandColor: modelData.brandColor || AppTheme.accentColor
                        isEnabled: modelData.enabled
                        isSelected: root.selectedProvider === modelData.id
                        usageData: modelData.usage
                        status: modelData.status || "unknown"
                        lastUpdated: modelData.lastUpdated || ""
                        itemIndex: index

                        onClicked: root.providerSelected(modelData.id)
                        onToggleChanged: function(checked) {
                            root.providerEnabled(modelData.id, checked)
                        }
                        onDragFinished: function(fromIndex, toIndex) {
                            if (fromIndex !== toIndex && fromIndex >= 0 && toIndex >= 0) {
                                UsageStore.moveProvider(fromIndex, toIndex)
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: AppTheme.bgPrimary

            ProviderDetailView {
                anchors.fill: parent
                visible: root.selectedProvider !== ""
                providerId: root.selectedProvider
                descriptor: root.selectedDescriptor
                connectionTest: root.selectedConnectionTest
                providerStatus: root.selectedProviderStatus
                providerError: root.selectedProviderError
                usageSnapshot: root.selectedUsageSnapshot

                onTestConnectionRequested: root.testConnection(root.selectedProvider)
                onRefreshRequested: root.refreshProvider(root.selectedProvider)
                onToggleEnabled: function(enabled) {
                    root.providerEnabled(root.selectedProvider, enabled)
                }
                onSettingChanged: function(key, value) {
                    root.settingChanged(root.selectedProvider, key, value)
                }
                onSecretSaveRequested: function(key, value) {
                    root.secretSaveRequested(root.selectedProvider, key, value)
                }
                onSecretClearRequested: function(key) {
                    root.secretClearRequested(root.selectedProvider, key)
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                width: 280
                spacing: 8
                visible: root.selectedProvider === ""

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Select a provider")
                    color: AppTheme.textPrimary
                    font.pixelSize: AppTheme.fontSizeLg
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Choose a provider from the list to edit connection settings.")
                    color: AppTheme.textSecondary
                    font.pixelSize: AppTheme.fontSizeSm
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
