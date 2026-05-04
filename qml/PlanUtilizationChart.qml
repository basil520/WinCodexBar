import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CodexBar 1.0

Rectangle {
    id: chartRoot
    property string providerId
    property string currentSeries: "session"
    property var chartPoints: []
    property bool hasTertiarySeries: false
    property string tertiarySeriesKey: "opus"
    property string tertiarySeriesLabel: qsTr("Opus")
    property string noDataText: qsTr("No session utilization data yet.")
    property int rev: LanguageManager.translationRevision
    property bool ready: false

    color: "#1c1c32"
    radius: 8
    height: 130

    function seriesModel() {
        var items = [
            { key: "session", label: qsTr("Session") },
            { key: "weekly", label: qsTr("Weekly") }
        ];

        if (chartRoot.hasTertiarySeries) {
            items.push({
                key: chartRoot.tertiarySeriesKey,
                label: chartRoot.tertiarySeriesLabel || qsTr("Opus")
            });
        }

        return items;
    }

    function ensureValidSeries() {
        var items = seriesModel();
        for (var i = 0; i < items.length; i++) {
            if (items[i].key === chartRoot.currentSeries) {
                return;
            }
        }
        chartRoot.currentSeries = items.length > 0 ? items[0].key : "session";
    }

    function refreshChart() {
        if (!chartRoot.ready) {
            return;
        }

        ensureValidSeries();
        chartRoot.chartPoints = UsageStore.utilizationChartData(chartRoot.providerId, chartRoot.currentSeries);
        hoverDetail.text = "";
        chartCanvas.requestPaint();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Repeater {
                model: {
                    chartRoot.rev
                    chartRoot.hasTertiarySeries
                    chartRoot.tertiarySeriesKey
                    chartRoot.tertiarySeriesLabel
                    return chartRoot.seriesModel()
                }
                delegate: Rectangle {
                    property bool isSelected: chartRoot.currentSeries === modelData.key
                    width: seriesLabel.width + 12
                    height: 20
                    radius: 4
                    color: isSelected ? "#4a4a7a" : "transparent"
                    border.color: isSelected ? "#6a6aaa" : "transparent"

                    Text {
                        id: seriesLabel
                        anchors.centerIn: parent
                        text: modelData.label
                        color: isSelected ? "white" : "#888"
                        font.pixelSize: 10
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            chartRoot.currentSeries = modelData.key
                            chartRoot.refreshChart()
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: chartPoints.length > 0 ? qsTr("%1 pts").arg(chartPoints.length) : qsTr("No data")
                color: "#666"
                font.pixelSize: 9
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Canvas {
                id: chartCanvas
                anchors.fill: parent

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.fillStyle = "#1c1c32";
                    ctx.fillRect(0, 0, width, height);

                    if (chartPoints.length === 0) {
                        ctx.fillStyle = "#555";
                        ctx.font = "10px sans-serif";
                        ctx.textAlign = "center";
                        ctx.fillText(chartRoot.noDataText, width / 2, height / 2);
                        return;
                    }

                    var barWidth = Math.max(2, (width - 20) / chartPoints.length - 2);
                    var startX = 10;

                    for (var i = 0; i < chartPoints.length; i++) {
                        var pct = chartPoints[i].usedPercent;
                        var barHeight = Math.max(1, (pct / 100.0) * (height - 20));

                        ctx.fillStyle = "#2a2a4a";
                        ctx.fillRect(startX + i * (barWidth + 2), height - 10, barWidth, -(height - 20));

                        ctx.fillStyle = "#4CAF50";
                        if (pct > 80) ctx.fillStyle = "#F44336";
                        else if (pct > 60) ctx.fillStyle = "#FFC107";
                        ctx.fillRect(startX + i * (barWidth + 2), height - 10, barWidth, -barHeight);
                    }

                    ctx.fillStyle = "#444";
                    ctx.font = "8px sans-serif";
                    ctx.textAlign = "left";
                    ctx.fillText("100%", 0, 12);
                    ctx.fillText("0%", 0, height - 4);
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onPositionChanged: function(mouse) {
                    if (chartPoints.length === 0) return;
                    var barWidth = Math.max(2, (chartCanvas.width - 20) / chartPoints.length - 2);
                    var idx = Math.floor((mouse.x - 10) / (barWidth + 2));
                    if (idx >= 0 && idx < chartPoints.length) {
                        hoverDetail.text = qsTr("%1: %2% used")
                            .arg(chartPoints[idx].dateLabel)
                            .arg(chartPoints[idx].usedPercent.toFixed(1));
                    } else {
                        hoverDetail.text = "";
                    }
                }
            }

            Text {
                id: hoverDetail
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#aaa"
                font.pixelSize: 9
            }
        }
    }

    Component.onCompleted: {
        ready = true
        refreshChart()
    }

    onProviderIdChanged: {
        refreshChart()
    }

    onHasTertiarySeriesChanged: {
        refreshChart()
    }

    onTertiarySeriesKeyChanged: {
        refreshChart()
    }

    onTertiarySeriesLabelChanged: {
        refreshChart()
    }

    Connections {
        enabled: chartRoot.visible
        target: UsageStore
        function onSnapshotRevisionChanged() {
            chartRoot.refreshChart()
        }
    }

    Connections {
        enabled: chartRoot.visible
        target: LanguageManager
        function onTranslationRevisionChanged() {
            chartRoot.ensureValidSeries()
            hoverDetail.text = ""
            chartCanvas.requestPaint()
        }
    }
}
