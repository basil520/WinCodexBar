#include "MiniMaxUsageSnapshot.h"
#include "UsageSnapshot.h"

QString MiniMaxUsageSnapshot::windowDescription() const {
    if (!windowMinutes.has_value()) return {};
    int mins = *windowMinutes;
    if (mins % 10080 == 0) return QString("%1 week%2").arg(mins / 10080).arg(mins / 10080 != 1 ? "s" : "");
    if (mins % 1440 == 0) return QString("%1 day%2").arg(mins / 1440).arg(mins / 1440 != 1 ? "s" : "");
    if (mins % 60 == 0) return QString("%1 hour%2").arg(mins / 60).arg(mins / 60 != 1 ? "s" : "");
    return QString("%1 minutes").arg(mins);
}

QString MiniMaxUsageSnapshot::limitDescription() const {
    if (!availablePrompts.has_value()) return {};
    QString desc = QString("%1 prompts").arg(*availablePrompts);
    QString win = windowDescription();
    if (!win.isEmpty()) desc += " / " + win;
    return desc;
}

UsageSnapshot MiniMaxUsageSnapshot::toUsageSnapshot() const {
    UsageSnapshot snap;

    if (usedPercent.has_value()) {
        QString desc = limitDescription();
        snap.primary = RateWindow{
            *usedPercent,
            windowMinutes,
            resetsAt,
            desc.isEmpty() ? std::optional<QString>() : desc,
            std::nullopt
        };
    }

    if (planName.has_value()) {
        snap.identity = ProviderIdentitySnapshot{
            UsageProvider::minimax, std::nullopt, std::nullopt, *planName
        };
    }

    snap.minimaxUsage = *this;
    snap.updatedAt = updatedAt;
    return snap;
}
