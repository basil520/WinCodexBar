import QtQuick 2.15

Rectangle {
    id: root
    property double percentValue: 0
    property string label: ""
    property color barColor: "#4CAF50"
    property double pacePercent: -1
    property bool paceOnTop: true

    height: 16
    color: "#3b3b5c"
    radius: 3

    Rectangle {
        width: Math.max(0, Math.min(parent.width, parent.width * root.percentValue / 100))
        height: parent.height
        color: root.barColor
        radius: parent.radius

        Behavior on width {
            NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
        }
    }

    Rectangle {
        visible: root.pacePercent >= 0 && root.pacePercent <= 100
        x: Math.max(0, Math.min(parent.width - width, parent.width * root.pacePercent / 100 - 1))
        width: 3
        height: parent.height
        color: root.paceOnTop ? "#4CAF50" : "#F44336"
        radius: 1

        Behavior on x {
            NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
        }
    }
}
