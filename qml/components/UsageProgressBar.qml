import QtQuick 2.15
import QtQuick.Layouts 1.15
import ".."

Rectangle {
    id: root
    property double value: 0.0
    property double paceValue: -1.0
    property color tintColor: AppTheme.accentColor

    height: AppTheme.progressBarHeight
    radius: height / 2
    color: AppTheme.bgHover
    Layout.fillWidth: true
    Layout.minimumWidth: 120

    Rectangle {
        id: fill
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Math.min(parent.width, parent.width * (root.value / 100.0))
        radius: parent.radius
        color: root.tintColor

        Behavior on width {
            NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
        }
    }

    // Pace indicator
    Rectangle {
        id: paceIndicator
        visible: root.paceValue >= 0
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        x: Math.min(parent.width - width, parent.width * (root.paceValue / 100.0))
        width: 2
        color: root.paceValue > root.value ? AppTheme.statusOk : AppTheme.statusOutage
        opacity: 0.8
    }
}
