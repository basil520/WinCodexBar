#include "AppTheme.h"

#include <QColor>

QVariantMap makeAppTheme() {
    QVariantMap theme;
    theme["bgPrimary"] = QColor(0x1a1a2e);
    theme["bgSecondary"] = QColor(0x15152a);
    theme["bgCard"] = QColor(0x1f1f38);
    theme["bgHover"] = QColor(0x2a2a4a);
    theme["bgSelected"] = QColor(0x3a3a5c);
    theme["bgPressed"] = QColor(0x4a4a7c);
    theme["borderColor"] = QColor(0x2a2a4a);
    theme["borderAccent"] = QColor(0x4a4a7c);
    theme["textPrimary"] = QColor(0xffffff);
    theme["textSecondary"] = QColor(0xaaaaaa);
    theme["textTertiary"] = QColor(0x888888);
    theme["textDisabled"] = QColor(0x555555);
    theme["statusOk"] = QColor(0x4CAF50);
    theme["statusDegraded"] = QColor(0xFFC107);
    theme["statusOutage"] = QColor(0xF44336);
    theme["statusUnknown"] = QColor(0x888888);
    theme["accentColor"] = QColor(0x6b6bff);
    theme["accentHover"] = QColor(0x8a8aff);
    theme["spacingXs"] = 4;
    theme["spacingSm"] = 8;
    theme["spacingMd"] = 12;
    theme["spacingLg"] = 16;
    theme["spacingXl"] = 24;
    theme["radiusSm"] = 4;
    theme["radiusMd"] = 8;
    theme["radiusLg"] = 12;
    theme["fontSizeSm"] = 11;
    theme["fontSizeMd"] = 13;
    theme["fontSizeLg"] = 16;
    theme["fontSizeXl"] = 20;
    theme["sidebarWidth"] = 240;
    theme["listItemHeight"] = 48;
    theme["iconSizeSm"] = 18;
    theme["iconSizeMd"] = 24;
    theme["iconSizeLg"] = 28;
    theme["statusDotSize"] = 6;
    theme["progressBarHeight"] = 6;
    return theme;
}
