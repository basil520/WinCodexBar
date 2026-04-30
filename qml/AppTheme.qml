import QtQuick 2.15

pragma Singleton

QtObject {
    // Background colors
    property color bgPrimary: "#1a1a2e"
    property color bgSecondary: "#15152a"
    property color bgCard: "#1f1f38"
    property color bgHover: "#2a2a4a"
    property color bgSelected: "#3a3a5c"
    property color bgPressed: "#4a4a7c"

    // Border colors
    property color borderColor: "#2a2a4a"
    property color borderAccent: "#4a4a7c"

    // Text colors
    property color textPrimary: "#ffffff"
    property color textSecondary: "#aaaaaa"
    property color textTertiary: "#888888"
    property color textDisabled: "#555555"

    // Status colors
    property color statusOk: "#4CAF50"
    property color statusDegraded: "#FFC107"
    property color statusOutage: "#F44336"
    property color statusUnknown: "#888888"

    // Accent colors
    property color accentColor: "#6b6bff"
    property color accentHover: "#8a8aff"

    // Spacing system
    property int spacingXs: 4
    property int spacingSm: 8
    property int spacingMd: 12
    property int spacingLg: 16
    property int spacingXl: 24

    // Border radius
    property int radiusSm: 4
    property int radiusMd: 8
    property int radiusLg: 12

    // Font sizes
    property int fontSizeSm: 11
    property int fontSizeMd: 13
    property int fontSizeLg: 16
    property int fontSizeXl: 20

    // Dimensions
    property int sidebarWidth: 240
    property int listItemHeight: 48
    property int iconSizeSm: 18
    property int iconSizeMd: 24
    property int iconSizeLg: 28
    property int statusDotSize: 6
    property int progressBarHeight: 6
}
