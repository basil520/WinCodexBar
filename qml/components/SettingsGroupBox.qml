import QtQuick 2.15
import QtQuick.Layouts 1.15
import ".."

Rectangle {
    id: root
    color: AppTheme.bgCard
    radius: AppTheme.radiusMd
    border.width: 1
    border.color: AppTheme.borderColor
    Layout.fillWidth: true
    implicitHeight: groupLayout.implicitHeight + 28
    default property alias content: groupLayout.data

    ColumnLayout {
        id: groupLayout
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10
    }
}
