import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: card
    property string providerId
    property var snapshot: null
    property var branding: null
    property var paceData: null

    color: "#2a2a3e"
    radius: 8
    border.color: "#3b3b5c"
    border.width: 1

    implicitWidth: 260
    implicitHeight: paceData ? 110 : 80

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
                text: providerId
                font.bold: true
                font.pixelSize: 13
                color: "white"
                Layout.fillWidth: true
            }

            Label {
                text: snapshot && snapshot.primary ? formatPercent(snapshot.primary.remainingPercent) : "-"
                font.pixelSize: 13
                color: percentColor(snapshot && snapshot.primary ? snapshot.primary.remainingPercent : 100)
            }
        }

        ProgressMeter {
            Layout.fillWidth: true
            percentValue: snapshot && snapshot.primary ? snapshot.primary.usedPercent : 0
            label: qsTr("Session")
            barColor: "#4CAF50"
            pacePercent: paceData && paceData.primaryPacePercent !== undefined ? paceData.primaryPacePercent : -1
            paceOnTop: paceData && paceData.primaryPaceOnTop !== undefined ? paceData.primaryPaceOnTop : true
        }

        ProgressMeter {
            Layout.fillWidth: true
            percentValue: snapshot && snapshot.secondary ? snapshot.secondary.usedPercent : 0
            label: qsTr("Weekly")
            barColor: "#2196F3"
            pacePercent: paceData && paceData.secondaryPacePercent !== undefined ? paceData.secondaryPacePercent : -1
            paceOnTop: paceData && paceData.secondaryPaceOnTop !== undefined ? paceData.secondaryPaceOnTop : true
        }

        RowLayout {
            visible: paceData && paceData.primaryPaceLeftLabel !== undefined
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: paceData ? (paceData.primaryPaceLeftLabel || "") : ""
                font.pixelSize: 10
                color: "#aaa"
                Layout.fillWidth: true
            }

            Label {
                text: paceData ? (paceData.primaryPaceRightLabel || "") : ""
                font.pixelSize: 10
                color: "#888"
            }
        }
    }

    function formatPercent(pct) {
        if (pct === undefined || pct === null) return "-"
        return pct.toFixed(0) + "%"
    }

    function percentColor(pct) {
        if (pct === undefined || pct === null) return "#888"
        if (pct > 50) return "#4CAF50"
        if (pct > 20) return "#FFC107"
        return "#F44336"
    }
}
