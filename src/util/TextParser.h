#pragma once

#include <QString>
#include <QRegularExpression>
#include <QVector>

class TextParser {
public:
    static QString stripAnsiEscapes(const QString& text);
    static double extractPercent(const QString& text);
    static QString extractResetDescription(const QString& text);
};
