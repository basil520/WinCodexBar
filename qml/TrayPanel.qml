import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CodexBar 1.0

Rectangle {
    id: root
    width: 300
    height: 520
    color: "#1a1a2e"
    radius: 12
    clip: true
    antialiasing: true
    border.color: "#3a3a5c"
    border.width: 1

    property var costData: UsageStore.costUsageData()
    property bool isRefreshing: UsageStore.isRefreshing
    property bool costExpanded: true
    property var expandedCards: ({})
    property int rev: LanguageManager.translationRevision

    Connections {
        target: UsageStore
        function onCostUsageChanged() { root.costData = UsageStore.costUsageData() }
        function onCostUsageRefreshingChanged() { root.costData = UsageStore.costUsageData() }
        function onRefreshingChanged() { root.isRefreshing = UsageStore.isRefreshing }
    }

    // Drop shadow mimic
    Rectangle {
        anchors.fill: parent
        anchors.margins: -1
        radius: 13
        color: "transparent"
        border.color: "#101020"
        border.width: 1
        z: -1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // === Header ===
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "#12122a"
            radius: 12

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 36
                color: parent.color
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#2a2a4a"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 8
                Text {
                    text: qsTr("CodexBar")
                    color: "white"
                    font.pixelSize: 15
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: UsageStore.providerIDs.length + " " + qsTr("providers")
                    color: "#666"
                    font.pixelSize: 11
                }
            }

            MouseArea {
                id: headerDragArea
                anchors.fill: parent
                cursorShape: Qt.SizeAllCursor
                property int pressX: 0
                property int pressY: 0
                onPressed: {
                    pressX = mouseX
                    pressY = mouseY
                }
                onPositionChanged: {
                    AppController.moveTrayPanel(mouseX - pressX, mouseY - pressY)
                }
            }
        }

        // === Token Usage Card ===
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: costSection.implicitHeight
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 10
            Layout.bottomMargin: 8
            color: "transparent"
            clip: true

            ColumnLayout {
                id: costSection
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                spacing: 0

                // Header row
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    radius: 8
                    color: costMouse.containsMouse ? "#252545" : "#1f1f38"

                    Behavior on color { ColorAnimation { duration: 150 } }

                    MouseArea {
                        id: costMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (costData.hasData) root.costExpanded = !root.costExpanded
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 8

                        Text {
                            Layout.preferredWidth: 12
                            text: root.costExpanded && costData.hasData ? "▾" : "▸"
                            color: "#888"
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Token Usage")
                            color: "#ccc"
                            font.pixelSize: 12
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Rectangle {
                            Layout.preferredWidth: 8
                            Layout.preferredHeight: 8
                            Layout.alignment: Qt.AlignVCenter
                            radius: 4
                            color: UsageStore.costUsageRefreshing ? "#FFC107"
                                 : costData.hasData ? "#4CAF50" : "#555"

                            SequentialAnimation on opacity {
                                running: UsageStore.costUsageRefreshing
                                loops: Animation.Infinite
                                NumberAnimation { from: 1.0; to: 0.3; duration: 500 }
                                NumberAnimation { from: 0.3; to: 1.0; duration: 500 }
                            }
                        }
                        Text {
                            Layout.maximumWidth: 86
                            text: costData.hasData
                                ? "$" + costData.sessionCostUSD.toFixed(2) + " " + qsTr("today")
                                : UsageStore.costUsageRefreshing ? qsTr("scanning...") : qsTr("no data")
                            color: "#888"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }
                }

                // Body
                ColumnLayout {
                    id: costBody
                    Layout.fillWidth: true
                    visible: root.costExpanded && costData.hasData
                    spacing: 8
                    Layout.topMargin: 8

                    // Today + 30d summary
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        radius: 8
                        color: "#1c1c32"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            anchors.topMargin: 8
                            anchors.bottomMargin: 8
                            spacing: 10

                            CostMetricCell {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                Layout.preferredWidth: 1
                                title: qsTr("Today")
                                value: "$" + formatCost(costData.sessionCostUSD)
                                detail: fmtNum(costData.sessionTokens) + " " + qsTr("tokens")
                                valueColor: "#4CAF50"
                            }

                            Rectangle {
                                Layout.preferredWidth: 1
                                Layout.fillHeight: true
                                color: "#2a2a4a"
                            }

                            CostMetricCell {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                Layout.preferredWidth: 1
                                title: qsTr("30 days")
                                value: "$" + formatCost(costData.last30DaysCostUSD)
                                detail: fmtNum(costData.last30DaysTokens) + " " + qsTr("tokens")
                                valueColor: "#2196F3"
                            }
                        }
                    }

                    // Daily bars
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 60
                        radius: 8
                        color: "#1c1c32"
                        clip: true

                        Row {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 2
                            layoutDirection: Qt.RightToLeft

                            Repeater {
                                model: costData.daily ? costData.daily.slice(-21) : []
                                delegate: Rectangle {
                                    width: Math.max(2, parent.width / 21 - 2)
                                    height: {
                                        var maxCost = 0
                                        for (var i = 0; i < costData.daily.length; i++)
                                            maxCost = Math.max(maxCost, costData.daily[i].costUSD)
                                        var chartHeight = Math.max(2, parent ? parent.height : 44)
                                        return maxCost > 0
                                            ? Math.max(2, chartHeight * modelData.costUSD / maxCost)
                                            : 2
                                    }
                                    y: Math.max(0, (parent ? parent.height : 44) - height)
                                    radius: 1
                                    color: index % 7 === 0 ? "#2196F3" : "#3a3a6a"

                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        width: parent.width
                                        height: 1
                                        color: "#2a2a4a"
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4
                            Text { text: qsTr("max"); color: "#444"; font.pixelSize: 8 }
                            Text { text: "0"; color: "#444"; font.pixelSize: 8 }
                        }
                    }

                    // Legend
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Rectangle { width: 10; height: 10; radius: 5; color: "#2196F3" }
                        Text { text: qsTr("Mon"); color: "#888"; font.pixelSize: 9 }
                        Rectangle { width: 1; height: 10; color: "#2a2a4a" }
                        Rectangle { width: 10; height: 10; radius: 5; color: "#3a3a6a" }
                        Text { text: qsTr("other day"); color: "#888"; font.pixelSize: 9 }
                    }

                    // Per-provider breakdown
                    Repeater {
                        model: UsageStore.providerCostUsageList()
                        delegate: ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            property bool providerExpanded: false

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                radius: 6
                                color: providerMouse.containsMouse ? "#252545" : "#1c1c32"

                                Behavior on color { ColorAnimation { duration: 150 } }

                                MouseArea {
                                    id: providerMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: providerExpanded = !providerExpanded
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    spacing: 6

                                    Text {
                                        Layout.preferredWidth: 12
                                        text: providerExpanded ? "▾" : "▸"
                                        color: "#888"
                                        font.pixelSize: 10
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    Rectangle {
                                        Layout.preferredWidth: 8
                                        Layout.preferredHeight: 8
                                        radius: 4
                                        color: brandColorFor(modelData.providerId)
                                    }
                                    Text {
                                        text: {
                                            var names = {
                                                "codex": "Codex", "claude": "Claude",
                                                "opencodego": "OpenCode Go",
                                                "deepseek": "DeepSeek"
                                            }
                                            return names[modelData.providerId] || modelData.providerId
                                        }
                                        color: "#ccc"
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: "$" + formatCost(modelData.last30DaysCostUSD)
                                        color: "#888"
                                        font.pixelSize: 10
                                    }
                                    Text {
                                        text: fmtNum(modelData.last30DaysTokens) + " " + qsTr("tokens")
                                        color: "#666"
                                        font.pixelSize: 9
                                    }
                                }
                            }

                            // Model breakdown (expanded)
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: providerExpanded
                                spacing: 2
                                Layout.leftMargin: 28

                                Repeater {
                                    model: modelData.models || []
                                    delegate: RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Text {
                                            text: "└─"
                                            color: "#555"
                                            font.pixelSize: 9
                                        }
                                        Text {
                                            text: modelData.name
                                            color: "#999"
                                            font.pixelSize: 10
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: "$" + formatCost(modelData.costUSD)
                                            color: "#888"
                                            font.pixelSize: 9
                                        }
                                        Text {
                                            text: fmtNum(modelData.tokens) + " tk"
                                            color: "#666"
                                            font.pixelSize: 9
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // === Provider List ===
        ListView {
            id: providerList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            topMargin: 2
            bottomMargin: 10
            leftMargin: 12
            rightMargin: 12

            model: UsageStore.providerIDs
            delegate: Rectangle {
                id: cardDelegate
                width: providerList.width - 24
                height: cardContent.height + 24
                radius: 10
                color: mouseArea.containsMouse ? "#252545" : "#202038"
                border.color: mouseArea.containsMouse ? "#4a4a7a" : "#2a2a4a"
                border.width: 1

                property var snap: {
                    LanguageManager.translationRevision
                    UsageStore.snapshotRevision
                    return UsageStore.snapshotData(modelData)
                }
                property bool expanded: root.expandedCards[modelData] === true
                property color brandColor: brandColorFor(modelData)
                property string primaryLabel: snap.displayName === "OpenRouter" && snap.openRouterUsage !== undefined
                    ? qsTr("API key limit") : snap.sessionLabel

                Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                Behavior on border.color { ColorAnimation { duration: 150 } }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: root.isRefreshing ? Qt.ArrowCursor : Qt.PointingHandCursor
                    onClicked: {
                        if (!root.isRefreshing) {
                            var cards = root.expandedCards
                            cards[modelData] = !cards[modelData]
                            root.expandedCards = cards
                            UsageStore.refreshProvider(modelData)
                        }
                    }
                }

                ColumnLayout {
                    id: cardContent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 6

                    // === Title row ===
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            text: snap.displayName
                            color: "white"
                            font.pixelSize: 14
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        Text {
                            text: snap.loginMethod && snap.loginMethod !== "" ? snap.loginMethod : ""
                            color: "#888"
                            font.pixelSize: 10
                            visible: !!snap.loginMethod && snap.loginMethod !== "" && !cardDelegate.expanded
                        }
                    }

                    // === Subtitle row ===
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        visible: snap.error !== ""
                        Text {
                            text: snap.error
                            color: "#F44336"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                        }
                    }

                    // === Refresh status ===
                    Text {
                        Layout.fillWidth: true
                        visible: !snap.hasUsage && snap.error === ""
                        text: root.isRefreshing ? qsTr("Refreshing...") : qsTr("No usage yet")
                        color: "#555"
                        font.pixelSize: 12
                    }

                    // === Primary bar ===
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasUsage
                        spacing: 6
                        Text {
                            text: cardDelegate.primaryLabel
                            color: "#aaa"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.preferredWidth: 80
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 6
                            radius: 3
                            color: "#2a2a4a"
                            Rectangle {
                                width: Math.max(0, parent.width * snap.primaryUsed / 100)
                                height: parent.height
                                radius: 3
                                color: cardDelegate.brandColor
                                Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                            }
                            Rectangle {
                                visible: snap.primaryPacePercent !== undefined && snap.primaryPacePercent >= 0
                                x: Math.max(0, Math.min(parent.width - 3, parent.width * (snap.primaryPacePercent || 0) / 100 - 1))
                                width: 3
                                height: parent.height
                                color: (snap.primaryPaceOnTop !== false) ? "#4CAF50" : "#F44336"
                                radius: 1
                                Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                            }
                        }
                        Text {
                            text: snap.primaryRemaining.toFixed(0) + "%"
                            color: snap.primaryRemaining > 50 ? "#4CAF50"
                                 : snap.primaryRemaining > 20 ? "#FFC107"
                                 : "#F44336"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.preferredWidth: 50
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    // Primary pace + reset
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasUsage && snap.primaryPaceLeftLabel !== undefined
                        spacing: 4
                        Text {
                            text: snap.primaryPaceLeftLabel || ""
                            color: "#aaa"
                            font.pixelSize: 10
                            Layout.preferredWidth: 80
                        }
                        Text {
                            text: snap.primaryPaceRightLabel || ""
                            color: "#888"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasUsage && snap.primaryResetDesc !== undefined && snap.primaryResetDesc !== ""
                        spacing: 4
                        Item { Layout.preferredWidth: 80 }
                        Text {
                            text: qsTr("Resets") + " " + (snap.primaryResetDesc || "")
                            color: "#666"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }

                    // === Secondary bar ===
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasUsage && snap.hasSecondary === true
                        spacing: 6
                        Text {
                            text: snap.weeklyLabel
                            color: "#aaa"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.preferredWidth: 80
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 6
                            radius: 3
                            color: "#2a2a4a"
                            Rectangle {
                                width: Math.max(0, parent.width * snap.secondaryUsed / 100)
                                height: parent.height
                                radius: 3
                                color: cardDelegate.brandColor
                                Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                            }
                            Rectangle {
                                visible: snap.secondaryPacePercent !== undefined && snap.secondaryPacePercent >= 0
                                x: Math.max(0, Math.min(parent.width - 3, parent.width * (snap.secondaryPacePercent || 0) / 100 - 1))
                                width: 3
                                height: parent.height
                                color: (snap.secondaryPaceOnTop !== false) ? "#4CAF50" : "#F44336"
                                radius: 1
                                Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                            }
                        }
                        Text {
                            text: snap.secondaryRemaining.toFixed(0) + "%"
                            color: snap.secondaryRemaining > 50 ? "#2196F3"
                                 : snap.secondaryRemaining > 20 ? "#FFC107"
                                 : "#F44336"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.preferredWidth: 50
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    // Secondary pace + reset
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasUsage && snap.hasSecondary === true && snap.secondaryPaceLeftLabel !== undefined
                        spacing: 4
                        Text {
                            text: snap.secondaryPaceLeftLabel || ""
                            color: "#aaa"
                            font.pixelSize: 10
                            Layout.preferredWidth: 80
                        }
                        Text {
                            text: snap.secondaryPaceRightLabel || ""
                            color: "#888"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasUsage && snap.hasSecondary === true && snap.secondaryResetDesc !== undefined && snap.secondaryResetDesc !== ""
                        spacing: 4
                        Item { Layout.preferredWidth: 80 }
                        Text {
                            text: qsTr("Resets") + " " + (snap.secondaryResetDesc || "")
                            color: "#666"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }

                    // === Tertiary bar (e.g. Sonnet, 5h, Opus) ===
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasUsage && snap.hasTertiary === true
                        spacing: 6
                        Text {
                            text: snap.opusLabel || qsTr("Opus")
                            color: "#aaa"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.preferredWidth: 80
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 6
                            radius: 3
                            color: "#2a2a4a"
                            Rectangle {
                                width: Math.max(0, parent.width * (snap.tertiaryUsed || 0) / 100)
                                height: parent.height
                                radius: 3
                                color: cardDelegate.brandColor
                                Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                            }
                        }
                        Text {
                            text: (snap.tertiaryRemaining !== undefined ? snap.tertiaryRemaining : 100).toFixed(0) + "%"
                            color: snap.tertiaryRemaining > 50 ? "#9C27B0"
                                 : snap.tertiaryRemaining > 20 ? "#FFC107"
                                 : "#F44336"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.preferredWidth: 50
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasUsage && snap.hasTertiary === true && snap.tertiaryResetDesc !== undefined && snap.tertiaryResetDesc !== ""
                        spacing: 4
                        Item { Layout.preferredWidth: 80 }
                        Text {
                            text: qsTr("Resets") + " " + (snap.tertiaryResetDesc || "")
                            color: "#666"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }

                    // === Provider cost bar ===
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasProviderCost === true
                        spacing: 6
                        Text {
                            text: snap.providerCostCurrency === "Quota" ? qsTr("Quota usage") : qsTr("Extra usage")
                            color: "#aaa"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.preferredWidth: 80
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 6
                            radius: 3
                            color: "#2a2a4a"
                            Rectangle {
                                width: Math.max(0, parent.width * Math.min(100, (snap.providerCostUsed || 0) / Math.max(1, snap.providerCostLimit || 1) * 100) / 100)
                                height: parent.height
                                radius: 3
                                color: "#FF9800"
                                Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        visible: snap.hasProviderCost === true
                        spacing: 4
                        Item { Layout.preferredWidth: 80 }
                        Text {
                            text: qsTr("This month") + ": $" + (snap.providerCostUsed || 0).toFixed(2) + " / $" + (snap.providerCostLimit || 0).toFixed(2)
                            color: "#888"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                        Text {
                            text: {
                                var pct = snap.providerCostLimit > 0 ? ((snap.providerCostUsed || 0) / (snap.providerCostLimit || 1) * 100).toFixed(0) : "0"
                                return qsTr("%1 used").arg(pct)
                            }
                            color: "#666"
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    // === Updated timestamp ===
                    Text {
                        Layout.fillWidth: true
                        visible: snap.hasUsage && snap.updatedAt > 0 && !root.isRefreshing
                        text: qsTr("Updated") + " " + timeAgo(snap.updatedAt)
                        color: "#555"
                        font.pixelSize: 9
                    }

                    // === Expanded detail row ===
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: cardDelegate.expanded
                        spacing: 4
                        Layout.topMargin: 4

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#2a2a4a"
                        }

                        // Zai MCP details
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: snap.zaiUsage !== undefined && snap.zaiUsage.timeLimit !== undefined
                            spacing: 2
                            Text {
                                text: qsTr("MCP details")
                                color: "#aaa"
                                font.pixelSize: 10
                                font.bold: true
                            }
                            Repeater {
                                model: snap.zaiUsage && snap.zaiUsage.timeLimit ? snap.zaiUsage.timeLimit.usageDetails : []
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Text {
                                        text: modelData.modelCode
                                        color: "#888"
                                        font.pixelSize: 9
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: fmtNum(modelData.usage)
                                        color: "#aaa"
                                        font.pixelSize: 9
                                    }
                                }
                            }
                        }

                        // OpenRouter key quota status
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: snap.openRouterUsage !== undefined
                            spacing: 2
                            Text {
                                visible: !!snap.openRouterUsage && snap.openRouterUsage.keyQuotaStatus === 1
                                text: qsTr("No limit set for the API key")
                                color: "#FFC107"
                                font.pixelSize: 10
                            }
                            Text {
                                visible: !!snap.openRouterUsage && snap.openRouterUsage.keyQuotaStatus === 2
                                text: qsTr("API key limit unavailable right now")
                                color: "#F44336"
                                font.pixelSize: 10
                            }
                            Text {
                                visible: !!snap.openRouterUsage && snap.openRouterUsage.keyRemaining !== undefined
                                text: snap.openRouterUsage && snap.openRouterUsage.keyRemaining !== undefined
                                    ? "$" + (snap.openRouterUsage.keyRemaining || 0).toFixed(2) + "/$" + (snap.openRouterUsage.keyLimit || 0).toFixed(2) + " " + qsTr("left")
                                    : ""
                                color: "#aaa"
                                font.pixelSize: 10
                            }
                        }

                        // Subscription Utilization chart
                        PlanUtilizationChart {
                            visible: modelData === "codex" || modelData === "claude"
                            Layout.fillWidth: true
                            providerId: modelData
                            hasTertiarySeries: snap.hasTertiary === true
                            tertiarySeriesLabel: snap.opusLabel || qsTr("Opus")
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Text {
                                text: qsTr("Updated")
                                color: "#888"
                                font.pixelSize: 10
                                Layout.preferredWidth: 80
                            }
                            Text {
                                text: snap.updatedAt && snap.updatedAt > 0
                                    ? new Date(snap.updatedAt).toLocaleTimeString(Qt.locale(), "hh:mm:ss")
                                    : "-"
                                color: "#666"
                                font.pixelSize: 10
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }

        // === Footer ===
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#12122a"
            radius: 12

            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                height: 8; color: parent.color
            }
            Rectangle {
                anchors.top: parent.top
                width: parent.width; height: 1
                color: "#2a2a4a"
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                ActionButton {
                    text: root.isRefreshing ? qsTr("Refreshing...") : qsTr("Refresh")
                    enabled: !root.isRefreshing
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    onClicked: UsageStore.refresh()
                }
                ActionButton {
                    text: qsTr("Settings")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    onClicked: AppController.toggleSettings()
                }
                ActionButton {
                    text: qsTr("Quit")
                    textColor: "#e06060"
                    hoverColor: "#4a3030"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    onClicked: AppController.quitApp()
                }
            }
        }
    }

    function fmtNum(n) {
        if (n === undefined || n === null) return "0"
        if (n >= 1000000) return (n / 1000000).toFixed(1) + "M"
        if (n >= 1000) return (n / 1000).toFixed(1) + "K"
        return n.toString()
    }

    function formatCost(n) {
        var value = Number(n || 0)
        if (Math.abs(value) >= 1000) return value.toFixed(1)
        if (Math.abs(value) >= 100) return value.toFixed(2)
        return value.toFixed(4)
    }

    function brandColorFor(providerId) {
        var colors = {
            "codex": "#49A3B0",
            "claude": "#CC7C5E",
            "cursor": "#5B8DFA",
            "gemini": "#8860D0",
            "copilot": "#2DA44E",
            "zai": "#E85A6A",
            "opencode": "#E44D26",
            "deepseek": "#1E3A8A",
            "warp": "#00BCD4",
            "mistral": "#F77F00",
            "openrouter": "#FF6B6B",
            "ollama": "#E6EF6C",
            "kilo": "#7C3AED",
            "kiro": "#F59E0B",
            "kimik2": "#06B6D4",
            "minimax": "#EC4899",
            "perplexity": "#22C55E",
            "kimi": "#8B5CF6",
            "abacus": "#6366F1",
            "alibaba": "#F97316",
            "augment": "#14B8A6",
            "amp": "#D946EF",
            "factory": "#84CC16",
            "jetbrains": "#F000F0",
            "vertexai": "#4285F4"
        }
        return colors[providerId] || "#4A90D9"
    }

    function timeAgo(ms) {
        if (!ms || ms <= 0) return qsTr("never")
        var ago = Date.now() - ms
        if (ago < 60000) return qsTr("just now")
        if (ago < 3600000) return Math.floor(ago / 60000) + qsTr("m ago")
        if (ago < 86400000) return Math.floor(ago / 3600000) + qsTr("h ago")
        return Math.floor(ago / 86400000) + qsTr("d ago")
    }

    component CostMetricCell: Item {
        id: metricCell
        property string title: ""
        property string value: ""
        property string detail: ""
        property color valueColor: "#aaa"

        implicitHeight: cellColumn.implicitHeight
        clip: true

        Column {
            id: cellColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1

            Text {
                width: parent.width
                text: metricCell.title
                color: "#888"
                font.pixelSize: 10
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: metricCell.value
                color: metricCell.valueColor
                font.pixelSize: 16
                minimumPixelSize: 10
                fontSizeMode: Text.HorizontalFit
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: metricCell.detail
                color: "#666"
                font.pixelSize: 10
                elide: Text.ElideRight
            }
        }
    }

    component ActionButton: Rectangle {
        property string text: ""
        property color textColor: "#aaa"
        property color hoverColor: "#3a3a5c"
        signal clicked()

        radius: 6
        color: btnMouse.containsMouse ? hoverColor : "transparent"
        Behavior on color { ColorAnimation { duration: 150 } }

        Text {
            anchors.centerIn: parent
            text: parent.text
            color: parent.textColor
            font.pixelSize: 12
        }

        MouseArea {
            id: btnMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
