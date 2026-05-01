#include "TextParser.h"

QString TextParser::stripAnsiEscapes(const QString& text) {
    QString result = text;
    static QRegularExpression oscRe(QStringLiteral("\x1b\\][^\x07]*(?:\x07|\x1b\\\\)"));
    static QRegularExpression csiRe(QStringLiteral("\x1b\\[[0-?]*[ -/]*[@-~]"));
    static QRegularExpression stringControlRe(QStringLiteral("\x1b[PX^_].*?\x1b\\\\"),
                                               QRegularExpression::DotMatchesEverythingOption);
    static QRegularExpression simpleEscapeRe(QStringLiteral("\x1b[@-_]"));

    result.replace(oscRe, "");
    result.replace(stringControlRe, "");
    result.replace(csiRe, "");
    result.replace(simpleEscapeRe, "");
    result.replace(QChar(0x0007), "");
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
