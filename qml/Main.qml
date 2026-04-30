import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: root
    visible: false
    flags: Qt.FramelessWindowHint

    TrayPanel {
        id: trayPanel
    }
}
