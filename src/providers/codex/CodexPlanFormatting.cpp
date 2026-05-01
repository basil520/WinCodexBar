#include "CodexPlanFormatting.h"

#include <QRegularExpression>

QString CodexPlanFormatting::displayName(const QString& raw)
{
    QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) return {};

    QString lower = trimmed.toLower();
    auto exactNames = exactDisplayNames();
    if (exactNames.contains(lower)) {
        return exactNames[lower];
    }

    QStringList parts = trimmed.split(QRegularExpression("[_\\-\\s]"), Qt::SkipEmptyParts);
    QStringList result;
    for (const auto& part : parts) {
        if (part.isEmpty()) continue;
        result.append(wordDisplayName(part));
    }
    return result.join(" ");
}

QHash<QString, QString> CodexPlanFormatting::exactDisplayNames()
{
    return {
        {"prolite", "Pro Lite"},
        {"pro_lite", "Pro Lite"},
        {"pro-lite", "Pro Lite"},
        {"pro lite", "Pro Lite"}
    };
}

QSet<QString> CodexPlanFormatting::uppercaseWords()
{
    return {"cbp", "k12"};
}

QString CodexPlanFormatting::wordDisplayName(const QString& raw)
{
    QString lower = raw.toLower();
    auto exactNames = exactDisplayNames();
    if (exactNames.contains(lower)) {
        return exactNames[lower];
    }

    auto upperWords = uppercaseWords();
    if (upperWords.contains(lower)) {
        return lower.toUpper();
    }

    if (raw == raw.toUpper() && raw.contains(QRegularExpression("[a-zA-Z]"))) {
        return raw;
    }

    if (!raw.isEmpty() && raw[0].isLower()) {
        return raw[0].toUpper() + raw.mid(1).toLower();
    }

    return raw;
}
