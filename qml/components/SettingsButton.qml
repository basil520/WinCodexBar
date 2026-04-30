import QtQuick 2.15
import QtQuick.Controls 2.15
import ".."

Button {
    id: root
    property bool danger: false
    property bool primary: false

    implicitHeight: 32
    leftPadding: 12
    rightPadding: 12
    topPadding: 6
    bottomPadding: 6
    font.pixelSize: AppTheme.fontSizeSm
    font.bold: root.primary

    contentItem: Text {
        text: root.text
        font: root.font
        color: {
            if (!root.enabled) return AppTheme.textDisabled
            if (root.primary) return "white"
            if (root.danger) return AppTheme.statusOutage
            return AppTheme.textPrimary
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 6
        color: {
            if (!root.enabled) return Qt.rgba(AppTheme.bgHover.r, AppTheme.bgHover.g, AppTheme.bgHover.b, 0.35)
            if (root.primary) return root.down ? AppTheme.accentHover : AppTheme.accentColor
            if (root.hovered || root.down) return AppTheme.bgHover
            return "transparent"
        }
        border.width: root.primary ? 0 : 1
        border.color: root.danger
            ? Qt.rgba(AppTheme.statusOutage.r, AppTheme.statusOutage.g, AppTheme.statusOutage.b, 0.55)
            : AppTheme.borderColor
    }
}
