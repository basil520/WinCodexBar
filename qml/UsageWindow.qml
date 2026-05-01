import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CodexBar 1.0
import "panes"

Rectangle {
    id: usageWindow
    width: 800
    height: 600
    color: AppTheme.bgPrimary

    property int rev: LanguageManager.translationRevision

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
                        text: "U"
                        color: "white"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                Label {
                    text: qsTr("Usage Details")
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
                        onPressed: AppController.startUsageMove()
                    }
                }

                TitleButton {
                    text: "_"
                    onClicked: AppController.minimizeUsage()
                }

                TitleButton {
                    text: "x"
                    danger: true
                    onClicked: AppController.closeUsage()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: AppTheme.bgPrimary

            TokenUsagePane {
                anchors.fill: parent
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
        onPressed: AppController.startUsageResize(edge)
    }
}
