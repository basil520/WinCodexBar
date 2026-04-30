import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import ".."

Rectangle {
    id: root
    property string providerName: ""
    property string providerId: ""
    property color brandColor: AppTheme.accentColor
    property bool isEnabled: false
    property bool isSelected: false
    property var usageData: null
    property string status: "unknown"
    property string lastUpdated: ""
    property int itemIndex: 0

    signal clicked()
    signal toggleChanged(bool checked)
    signal dragStarted(int index)
    signal dragFinished(int fromIndex, int toIndex)

    height: 46
    radius: 7
    color: {
        if (root.isSelected) return AppTheme.bgSelected
        if (rowMouse.containsMouse) return AppTheme.bgHover
        return "transparent"
    }

    Drag.active: dragArea.drag.active
    Drag.source: root
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2

    MouseArea {
        id: rowMouse
        anchors.fill: parent
        z: 0
        hoverEnabled: true
        onClicked: root.clicked()
    }

    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 3
        height: 24
        radius: 2
        color: AppTheme.accentColor
        visible: root.isSelected
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 8
        spacing: 8
        z: 1

        Item {
            id: dragHandle
            Layout.preferredWidth: 10
            Layout.preferredHeight: 22
            Layout.alignment: Qt.AlignVCenter
            opacity: rowMouse.containsMouse || root.isSelected || dragArea.drag.active ? 1.0 : 0.0

            Column {
                anchors.centerIn: parent
                spacing: 2

                Repeater {
                    model: 3
                    Row {
                        spacing: 2
                        Repeater {
                            model: 2
                            Rectangle {
                                width: 2
                                height: 2
                                radius: 1
                                color: AppTheme.textTertiary
                            }
                        }
                    }
                }
            }

            MouseArea {
                id: dragArea
                anchors.fill: parent
                cursorShape: Qt.OpenHandCursor
                drag.target: root
                drag.axis: Drag.YAxis
                onPressed: {
                    cursorShape = Qt.ClosedHandCursor
                    root.dragStarted(root.itemIndex)
                }
                onReleased: {
                    cursorShape = Qt.OpenHandCursor
                    root.dragFinished(root.itemIndex, -1)
                    root.y = 0
                }
            }
        }

        Image {
            id: providerIcon
            Layout.preferredWidth: 22
            Layout.preferredHeight: 22
            Layout.alignment: Qt.AlignVCenter
            source: "qrc:/icons/ProviderIcon-" + root.providerId + ".svg"
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 22
            sourceSize.height: 22

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: root.brandColor
                visible: providerIcon.status !== Image.Ready

                Label {
                    anchors.centerIn: parent
                    text: root.providerName.length > 0 ? root.providerName.charAt(0).toUpperCase() : "?"
                    color: "white"
                    font.pixelSize: 10
                    font.bold: true
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.providerName
                color: root.isEnabled ? AppTheme.textPrimary : AppTheme.textSecondary
                font.pixelSize: AppTheme.fontSizeMd
                font.bold: root.isSelected
                elide: Text.ElideRight
            }

            UsageProgressBar {
                Layout.fillWidth: true
                Layout.minimumWidth: 44
                Layout.maximumWidth: 92
                visible: root.usageData !== null
                    && root.usageData !== undefined
                    && root.usageData.percent !== undefined
                value: root.usageData !== null
                    && root.usageData !== undefined
                    && root.usageData.percent !== undefined
                    ? root.usageData.percent
                    : 0
                tintColor: root.brandColor
            }
        }

        Rectangle {
            Layout.preferredWidth: 8
            Layout.preferredHeight: 8
            Layout.alignment: Qt.AlignVCenter
            radius: width / 2
            color: {
                switch (root.status) {
                case "ok": return AppTheme.statusOk
                case "degraded": return AppTheme.statusDegraded
                case "outage": return AppTheme.statusOutage
                default: return AppTheme.statusUnknown
                }
            }
            opacity: root.isEnabled ? 1.0 : 0.45
        }

        SettingsSwitch {
            Layout.alignment: Qt.AlignVCenter
            checked: root.isEnabled
            onToggled: function(checked) {
                root.toggleChanged(checked)
            }
        }
    }

    DropArea {
        anchors.fill: parent
        onEntered: {
            if (drag.source !== root && drag.source && drag.source.itemIndex !== undefined) {
                root.dragFinished(drag.source.itemIndex, root.itemIndex)
            }
        }
    }
}
