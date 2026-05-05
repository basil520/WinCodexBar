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
    QT_TRANSLATE_NOOP("ProviderLabels", "Balance"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Prompts"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Window"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Search"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Recurring"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Promo"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Purchased"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Usage"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Pro"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Flash"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Flash Lite"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Standard"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Claude"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Gemini Pro"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Gemini Flash"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Requests"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Interval"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Rate Limit"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Unlimited"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Paid"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Granted"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Promo credits"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Purchased credits"),
    QT_TRANSLATE_NOOP("ProviderLabels", "compute points"),
    QT_TRANSLATE_NOOP("ProviderLabels", "requests"),
    QT_TRANSLATE_NOOP("ProviderLabels", "bonus credits"),
    QT_TRANSLATE_NOOP("ProviderLabels", "Balance unavailable for API calls"),
};

const char* const providerSettingSources[] = {
    QT_TRANSLATE_NOOP("ProviderSettings", "Data source"),
    QT_TRANSLATE_NOOP("ProviderSettings", "Cookie source"),
    QT_TRANSLATE_NOOP("ProviderSettings", "API region"),
    QT_TRANSLATE_NOOP("ProviderSettings", "API key"),
    QT_TRANSLATE_NOOP("ProviderSettings", "API Key"),
    QT_TRANSLATE_NOOP("ProviderSettings", "API key (optional)"),
    QT_TRANSLATE_NOOP("ProviderSettings", "Manual cookie"),
    QT_TRANSLATE_NOOP("ProviderSettings", "Manual cookie header"),
    QT_TRANSLATE_NOOP("ProviderSettings", "Region"),
    QT_TRANSLATE_NOOP("ProviderSettings", "Workspace ID"),
    QT_TRANSLATE_NOOP("ProviderSettings", "Browser or manual cookie"),
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
    // DeepSeek
    QT_TRANSLATE_NOOP("ProviderErrors", "DeepSeek API key not configured."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty or invalid response from DeepSeek API"),
    // MiniMax
    QT_TRANSLATE_NOOP("ProviderErrors", "MiniMax API key not configured."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty or invalid response from MiniMax API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "MiniMax API credentials are invalid or expired."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty response from MiniMax coding plan page"),
    QT_TRANSLATE_NOOP("ProviderErrors", "MiniMax session expired. Please re-import your cookie."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Could not parse MiniMax coding plan data from page"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No MiniMax cookie available. Log in to platform.minimax.io in your browser."),
    // Synthetic
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty or invalid response from Synthetic API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Synthetic API key not configured."),
    // Perplexity
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty or invalid response from Perplexity API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No Perplexity cookie available. Log in to perplexity.ai in your browser or paste cookie manually."),
    // Amp
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty response from Amp"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Not signed in. Log in to ampcode.com in your browser."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Could not parse Amp usage data from page"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Could not find quota in Amp usage data"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No Amp cookie available. Log in to ampcode.com in your browser."),
    // Augment
    QT_TRANSLATE_NOOP("ProviderErrors", "auggie CLI not found in PATH."),
    QT_TRANSLATE_NOOP("ProviderErrors", "auggie account status timed out"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty output from auggie CLI"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty response from Augment API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No Augment cookie available."),
    // Gemini
    QT_TRANSLATE_NOOP("ProviderErrors", "No quota buckets in Gemini response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Gemini OAuth credentials not found. Run 'gemini' CLI to authenticate."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Failed to refresh Gemini OAuth token."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Gemini API key mode does not support usage quotas. Use OAuth mode (run 'gemini' CLI to authenticate)."),
    // VertexAI
    QT_TRANSLATE_NOOP("ProviderErrors", "Google ADC not found. Run 'gcloud auth application-default login'."),
    QT_TRANSLATE_NOOP("ProviderErrors", "GCP project not found. Set GOOGLE_CLOUD_PROJECT or run 'gcloud config set project'."),
    // JetBrains
    QT_TRANSLATE_NOOP("ProviderErrors", "No AIAssistant quota data found in XML"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No quotaInfo in JetBrains data"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No JetBrains IDE installation found."),
    // Factory
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty response from Factory API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No Factory cookie available. Log in to app.factory.ai in your browser."),
    // Antigravity
    QT_TRANSLATE_NOOP("ProviderErrors", "No response from Antigravity local service"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Antigravity is not running."),
    // Warp
    QT_TRANSLATE_NOOP("ProviderErrors", "Warp API key not configured."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty response from Warp GraphQL API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "GraphQL error"),
    // Abacus
    QT_TRANSLATE_NOOP("ProviderErrors", "No Abacus AI cookie available. Log in to apps.abacus.ai in your browser."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty response from Abacus AI API"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Abacus AI API error"),
    // Kimi (structured error messages)
    QT_TRANSLATE_NOOP("ProviderErrors", "Kimi auth token is missing. Please add your JWT token from the Kimi console."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Kimi auth token is invalid or expired. Please refresh your token."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Invalid request"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Kimi network error"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Kimi API error"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Failed to parse Kimi usage data"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Unknown Kimi error"),
    // Codex (pre-existing untranslated)
    QT_TRANSLATE_NOOP("ProviderErrors", "Codex CLI RPC returned no rate limit data."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Codex CLI RPC returned no rate limit windows."),
    QT_TRANSLATE_NOOP("ProviderErrors", "ConPTY is not available on this Windows version (requires Windows 10 1809+). codex CLI needs a terminal."),
    QT_TRANSLATE_NOOP("ProviderErrors", "codex CLI still reports 'stdin is not a terminal' even with ConPTY."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Codex CLI update needed. Run `npm i -g @openai/codex` to continue."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Codex data not available yet; will retry shortly."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Could not parse codex CLI status output. The CLI returned interactive screen output instead of usage data."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty response from web dashboard (HTTP request failed or returned empty)"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Could not parse usage data from web dashboard."),
    QT_TRANSLATE_NOOP("ProviderErrors", "codex CLI not found in PATH. Install via `npm i -g @openai/codex`"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Failed to start ConPTY session for codex CLI"),
    QT_TRANSLATE_NOOP("ProviderErrors", "codex CLI exited before we could send /status"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Could not parse codex CLI status output."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Failed to start persistent CLI session"),
    QT_TRANSLATE_NOOP("ProviderErrors", "CLI session exited unexpectedly"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Timed out waiting for status output"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Empty response from token refresh endpoint"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Refresh token expired. Please run `codex` to log in again."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Refresh token was revoked. Please run `codex` to log in again."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Refresh token was already used. Please run `codex` to log in again."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Token refresh failed"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Invalid refresh response: missing access_token"),
    QT_TRANSLATE_NOOP("ProviderErrors", "RPC client not started"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No credits data in OAuth response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Codex CLI RPC failed to start"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No credits in RPC response"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No credits parsed from CLI status output"),
    // OpenCode Go
    QT_TRANSLATE_NOOP("ProviderErrors", "No OpenCode Go auth cookie found in browser cookies or manual input."),
    QT_TRANSLATE_NOOP("ProviderErrors", "Failed to fetch OpenCode workspace ID."),
    QT_TRANSLATE_NOOP("ProviderErrors", "OpenCode Go workspace page returned no usage data."),
    QT_TRANSLATE_NOOP("ProviderErrors", "No usage data found in OpenCode Go response."),
    // Pipeline / infrastructure
    QT_TRANSLATE_NOOP("ProviderErrors", "Application shutting down"),
    QT_TRANSLATE_NOOP("ProviderErrors", "No available fetch strategy"),
    QT_TRANSLATE_NOOP("ProviderErrors", "test override returned no credits"),
    // Alibaba
    QT_TRANSLATE_NOOP("ProviderErrors", "empty or invalid response from Alibaba web"),
    QT_TRANSLATE_NOOP("ProviderErrors", "Quota information unavailable"),
    QT_TRANSLATE_NOOP("ProviderErrors", "API authenticated"),
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
