#include "CLIRenderer.h"

CLIRenderer::CLIRenderer(bool jsonMode, bool pretty, bool noColor)
    : m_jsonMode(jsonMode)
    , m_pretty(pretty)
    , m_noColor(noColor)
    , m_out(stdout)
    , m_err(stderr)
{
}

void CLIRenderer::heading(const QString& text)
{
    if (m_jsonMode) return;
    m_out << color(text, "\x1B[1;36m") << "\n";
    m_out << QString(text.length(), '=') << "\n";
    m_out.flush();
}

void CLIRenderer::line(const QString& text)
{
    if (m_jsonMode) return;
    m_out << text << "\n";
    m_out.flush();
}

void CLIRenderer::bullet(const QString& text)
{
    if (m_jsonMode) return;
    m_out << "  - " << text << "\n";
    m_out.flush();
}

void CLIRenderer::error(const QString& text)
{
    if (m_jsonMode) return;
    m_err << color(text, "\x1B[1;31m") << "\n";
    m_err.flush();
}

void CLIRenderer::blank()
{
    if (m_jsonMode) return;
    m_out << "\n";
    m_out.flush();
}

void CLIRenderer::setRootObject(const QJsonObject& obj)
{
    m_root = obj;
}

void CLIRenderer::flush()
{
    if (m_jsonMode) {
        QJsonDocument doc(m_root);
        QByteArray bytes = m_pretty ? doc.toJson(QJsonDocument::Indented)
                                    : doc.toJson(QJsonDocument::Compact);
        m_out << QString::fromUtf8(bytes) << "\n";
        m_out.flush();
    }
}

QString CLIRenderer::color(const QString& text, const char* ansiCode) const
{
    if (m_noColor) return text;
    return QString::fromUtf8(ansiCode) + text + "\x1B[0m";
}
