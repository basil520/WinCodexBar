import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import ".."

RowLayout {
    id: root
    property string statusState: "idle"
    property string message: ""

    spacing: 6

    Rectangle {
        Layout.preferredWidth: 18
        Layout.preferredHeight: 18
        radius: 9
        color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.14)
        border.width: 1
        border.color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.45)
        visible: root.statusState !== "idle"

        property color statusColor: {
            if (root.statusState === "testing") return AppTheme.statusDegraded
            if (root.statusState === "succeeded") return AppTheme.statusOk
            if (root.statusState === "failed") return AppTheme.statusOutage
            return AppTheme.statusUnknown
        }

        Rectangle {
            anchors.centerIn: parent
            width: root.statusState === "testing" ? 9 : 6
            height: root.statusState === "testing" ? 2 : 6
            radius: root.statusState === "testing" ? 1 : 3
            color: parent.statusColor

            RotationAnimation on rotation {
                running: root.statusState === "testing"
                from: 0
                to: 360
                duration: 850
                loops: Animation.Infinite
            }
        }
    }

    Label {
        text: root.message || root.statusState
        color: {
            if (root.statusState === "succeeded") return AppTheme.statusOk
            if (root.statusState === "failed") return AppTheme.statusOutage
            if (root.statusState === "testing") return AppTheme.statusDegraded
            return AppTheme.textSecondary
        }
        font.pixelSize: AppTheme.fontSizeSm
        elide: Text.ElideRight
        Layout.maximumWidth: 180
    }
}
