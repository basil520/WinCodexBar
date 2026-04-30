#pragma once

#include <QIcon>
#include <QPixmap>
#include <QCache>
#include <optional>

class TrayIconRenderer {
public:
    enum class IconStyle {
        Default,
        Codex,
        Claude,
        Gemini,
        Cursor,
    };

    static QIcon makeDefaultIcon();
    static QIcon makeIcon(const std::optional<double>& primaryRemaining,
                           const std::optional<double>& weeklyRemaining,
                           const std::optional<double>& creditsRemaining,
                           bool stale,
                           IconStyle style,
                           double blink = 0.0);
    static QPixmap renderPixmap(int size,
                                 const std::optional<double>& primary,
                                 const std::optional<double>& weekly,
                                 bool stale,
                                 IconStyle style);
};
