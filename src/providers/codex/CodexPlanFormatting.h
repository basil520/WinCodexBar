#pragma once

#include <QString>
#include <QHash>
#include <QSet>

class CodexPlanFormatting {
public:
    static QString displayName(const QString& raw);

private:
    static QHash<QString, QString> exactDisplayNames();
    static QSet<QString> uppercaseWords();
    static QString wordDisplayName(const QString& raw);
};
