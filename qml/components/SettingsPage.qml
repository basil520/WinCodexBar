import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ".."

ScrollView {
    id: root
    property string title: ""
    property string subtitle: ""
    property int maxContentWidth: 640
    default property alias content: body.data

    clip: true
    contentWidth: availableWidth
    ScrollBar.vertical.policy: ScrollBar.AsNeeded

    Item {
        width: root.availableWidth
        height: body.implicitHeight + 48

        ColumnLayout {
            id: body
            x: 28
            y: 24
            width: Math.max(0, Math.min(root.availableWidth - 56, root.maxContentWidth))
            spacing: 12

            Label {
                text: root.title
                color: AppTheme.textPrimary
                font.pixelSize: 22
                font.bold: true
                Layout.fillWidth: true
            }

            Label {
                text: root.subtitle
                color: AppTheme.textSecondary
                font.pixelSize: AppTheme.fontSizeMd
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: text !== ""
                Layout.bottomMargin: 8
            }
        }
    }
}
