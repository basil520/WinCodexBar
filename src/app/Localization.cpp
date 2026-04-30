#include "Localization.h"

#include <QCoreApplication>

#include <iterator>

namespace {

QString translateKnown(const char* context,
                       const QString& source,
                       const char* const* candidates,
                       qsizetype count)
{
    for (qsizetype i = 0; i < count; ++i) {
        if (source == QLatin1String(candidates[i])) {
            return QCoreApplication::translate(context, candidates[i]);
        }
    }
    return source;
}

const char* const providerLabelSources[] = {
    QT_TRANSLATE_NOOP("ProviderLabels", "Session"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Weekly"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Credits"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Tokens"),
    QT_TRANSLATE_NOOP("ProviderLabels", "MCP"),
    QT_TRANSLATE_NOOP("ProviderLabels", "5-hour"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Opus"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Total"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Auto"),
    QT_TRANSLATE_NOOP("ProviderLabels", "API"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Pass"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Monthly"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Quota"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Premium"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Chat"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Bonus"),
};

const char* const providerSettingSources[] = {
    QT_TRANSLATE_NOOP("ProviderSettings", "Data source"),
    QT_TRANSLATE_NOOP("ProviderSettings", "Cookie source"),
    QT_TRANSLATE_NOOP("ProviderSettings", "API region"),
};

const char* const providerErrorSources[] = {
    QT_TRANSLATE_NOOP("ProviderErrors", "fetchSync not implemented"),
    QT_TRANSLATE_NOOP("ProviderErrors", "strategy timed out"),
    QT_TRANSLATE_NOOP("ProviderErrors", "no available fetch strategy"),
    QT_TRANSLATE_NOOP("ProviderErrors", "empty or invalid response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "API error"),
    QT_TRANSLATE_NOOP("ProviderErrors", "no cookies found"),
    QT_TRANSLATE_NOOP("ProviderErrors", "empty response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "empty HTML"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Claude OAuth credentials not found. Run `claude` to authenticate."),
    QT_TRANSLATE_NOOP("ProviderErrors", "empty or invalid response from Claude OAuth API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "OAuth error"),
    QT_TRANSLATE_NOOP("ProviderErrors", "no usage data in Claude OAuth response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No Claude session key found in browser cookies."),
    QT_TRANSLATE_NOOP("ProviderErrors", "No Claude organization found."),
    QT_TRANSLATE_NOOP("ProviderErrors", "No usage data from Claude web API."),
    QT_TRANSLATE_NOOP("ProviderErrors", "No Cursor session cookies found in browser."),
    QT_TRANSLATE_NOOP("ProviderErrors", "No Cursor session cookie found."),
    QT_TRANSLATE_NOOP("ProviderErrors", "empty response from Cursor usage API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "no usage data in Cursor response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "OAuth device flow failed"),
    QT_TRANSLATE_NOOP("ProviderErrors", "empty copilot response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "kiro-cli timed out"),
    QT_TRANSLATE_NOOP("ProviderErrors", "empty CLI output"),
    QT_TRANSLATE_NOOP("ProviderErrors", "no results in tRPC response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Codex auth.json not found. Run `codex` to log in."),
    QT_TRANSLATE_NOOP("ProviderErrors", "empty or invalid response from Codex API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "no rate limit data in response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "empty credits response"),
};

}

QString Localization::providerLabel(const QString& source) {
    return translateKnown("ProviderLabels", source, providerLabelSources, std::size(providerLabelSources));
}

QString Localization::providerSettingLabel(const QString& source) {
    return translateKnown("ProviderSettings", source, providerSettingSources, std::size(providerSettingSources));
}

QString Localization::providerError(const QString& source) {
    return translateKnown("ProviderErrors", source, providerErrorSources, std::size(providerErrorSources));
}
