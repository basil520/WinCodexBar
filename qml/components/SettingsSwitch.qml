import QtQuick 2.15
import ".."

Rectangle {
    id: root
    property bool checked: false
    signal toggled(bool checked)

    implicitWidth: 34
    implicitHeight: 20
    radius: height / 2
    opacity: enabled ? 1.0 : 0.45
    color: checked
        ? Qt.rgba(AppTheme.accentColor.r, AppTheme.accentColor.g, AppTheme.accentColor.b, 0.95)
        : AppTheme.bgHover
    border.width: 1
    border.color: checked ? AppTheme.accentHover : AppTheme.borderColor

    Rectangle {
        id: knob
        width: 14
        height: 14
        radius: 7
        x: root.checked ? root.width - width - 3 : 3
        y: 3
        color: root.checked ? "#eef0ff" : AppTheme.textSecondary

        Behavior on x {
            NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.toggled(!root.checked)
        }
    }
}
