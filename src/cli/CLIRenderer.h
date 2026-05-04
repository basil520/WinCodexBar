#pragma once

#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>

/**
 * @brief Unified output renderer for CLI commands.
 *
 * Supports both human-readable text mode and machine-readable JSON mode.
 */
class CLIRenderer {
public:
    explicit CLIRenderer(bool jsonMode, bool pretty, bool noColor);

    // Text helpers (no-op in jsonMode)
    void heading(const QString& text);
    void line(const QString& text);
    void bullet(const QString& text);
    void error(const QString& text);
    void blank();

    // JSON helpers
    void setRootObject(const QJsonObject& obj);
    void flush();

    bool isJsonMode() const { return m_jsonMode; }

private:
    bool m_jsonMode;
    bool m_pretty;
    bool m_noColor;
    QTextStream m_out;
    QTextStream m_err;
    QJsonObject m_root;

    QString color(const QString& text, const char* ansiCode) const;
};
