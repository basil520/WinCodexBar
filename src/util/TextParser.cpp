#include "TextParser.h"

QString TextParser::stripAnsiEscapes(const QString& text) {
    static QRegularExpression ansiRe("\x1b\\[[0-9;]*[a-zA-Z]");
    QString result = text;
    result.replace(ansiRe, "");
    result.replace("\r", "");
    return result;
}

double TextParser::extractPercent(const QString& text) {
    static QRegularExpression percentRe("(\\d+(?:\\.\\d+)?)\\s*%");
    QRegularExpressionMatch match = percentRe.match(text);
    if (match.hasMatch()) {
        return match.captured(1).toDouble();
    }
    return 0.0;
}

QString TextParser::extractResetDescription(const QString& text) {
    static QRegularExpression resetRe("resets?\\s+(.+)");
    QRegularExpressionMatch match = resetRe.match(text);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    return {};
}
