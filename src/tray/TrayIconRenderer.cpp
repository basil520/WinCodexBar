#include "TrayIconRenderer.h"

#include <QPainter>
#include <QLinearGradient>

QIcon TrayIconRenderer::makeDefaultIcon() {
    return makeIcon(100.0, 100.0, std::nullopt, false, IconStyle::Default);
}

QIcon TrayIconRenderer::makeIcon(
    const std::optional<double>& primaryRemaining,
    const std::optional<double>& weeklyRemaining,
    const std::optional<double>& creditsRemaining,
    bool stale,
    IconStyle style,
    double blink)
{
    Q_UNUSED(creditsRemaining)
    Q_UNUSED(blink)

    double p = primaryRemaining.value_or(100.0);
    double w = weeklyRemaining.value_or(100.0);

    QIcon icon;
    icon.addPixmap(renderPixmap(16, p, w, stale, style));
    icon.addPixmap(renderPixmap(24, p, w, stale, style));
    icon.addPixmap(renderPixmap(32, p, w, stale, style));
    return icon;
}

QPixmap TrayIconRenderer::renderPixmap(
    int size,
    const std::optional<double>& primary,
    const std::optional<double>& weekly,
    bool stale,
    IconStyle style)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    double pRem = std::clamp(primary.value_or(100.0) / 100.0, 0.0, 1.0);
    double wRem = std::clamp(weekly.value_or(100.0) / 100.0, 0.0, 1.0);

    QColor primaryColor = pRem > 0.5 ? QColor(64, 200, 64) :
                           pRem > 0.2 ? QColor(220, 180, 40) :
                           QColor(220, 60, 60);
    QColor weeklyColor = wRem > 0.5 ? QColor(64, 160, 200) :
                          wRem > 0.2 ? QColor(200, 160, 40) :
                          QColor(200, 60, 60);

    if (stale) {
        primaryColor = primaryColor.darker(200);
        weeklyColor = weeklyColor.darker(200);
    }

    int barHeight = qMax(2, size / 10);
    int spacing = qMax(1, size / 24);
    int topY = spacing;
    int bottomY = spacing + barHeight + spacing;

    painter.fillRect(QRect(0, topY, static_cast<int>(size * pRem), barHeight), primaryColor);
    painter.fillRect(QRect(0, topY, size, barHeight), QColor(60, 60, 60));

    painter.fillRect(QRect(0, topY, static_cast<int>(size * pRem), barHeight), primaryColor);

    barHeight = qMax(1, size / 14);
    painter.fillRect(QRect(0, bottomY, size, barHeight), QColor(60, 60, 60));
    painter.fillRect(QRect(0, bottomY, static_cast<int>(size * wRem), barHeight), weeklyColor);

    painter.end();
    return pixmap;
}
