#include "E2ETestUtils.h"

#include "../src/providers/ProviderRegistry.h"
#include "../src/providers/ProviderPipeline.h"
#include "../src/providers/ProviderFetchContext.h"
#include "../src/providers/IProvider.h"
#include "../src/providers/shared/ProviderCredentialStore.h"

#include "../src/providers/zai/ZaiProvider.h"
#include "../src/providers/openrouter/OpenRouterProvider.h"
#include "../src/providers/copilot/CopilotProvider.h"
#include "../src/providers/kimik2/KimiK2Provider.h"
#include "../src/providers/kilo/KiloProvider.h"
#include "../src/providers/kiro/KiroProvider.h"
#include "../src/providers/mistral/MistralProvider.h"
#include "../src/providers/ollama/OllamaProvider.h"
#include "../src/providers/codex/CodexProvider.h"
#include "../src/providers/claude/ClaudeProvider.h"
#include "../src/providers/cursor/CursorProvider.h"
#include "../src/providers/kimi/KimiProvider.h"
#include "../src/providers/opencode/OpenCodeProvider.h"
#include "../src/providers/opencode/OpenCodeGoProvider.h"

#include <QProcessEnvironment>
#include <QDir>
#include <QFile>
#include <QRegularExpression>

namespace E2ETestUtils {

static bool s_providersRegistered = false;

void registerE2EProviders() {
    if (s_providersRegistered) return;
    s_providersRegistered = true;

    ProviderRegistry::instance().registerProvider(new ZaiProvider());
    ProviderRegistry::instance().registerProvider(new OpenRouterProvider());
    ProviderRegistry::instance().registerProvider(new CopilotProvider());
    ProviderRegistry::instance().registerProvider(new KimiK2Provider());
    ProviderRegistry::instance().registerProvider(new KiloProvider());
    ProviderRegistry::instance().registerProvider(new KiroProvider());
    ProviderRegistry::instance().registerProvider(new MistralProvider());
    ProviderRegistry::instance().registerProvider(new OllamaProvider());
    ProviderRegistry::instance().registerProvider(new CodexProvider());
    ProviderRegistry::instance().registerProvider(new ClaudeProvider());
    ProviderRegistry::instance().registerProvider(new CursorProvider());
    ProviderRegistry::instance().registerProvider(new KimiProvider());
    ProviderRegistry::instance().registerProvider(new OpenCodeProvider());
    ProviderRegistry::instance().registerProvider(new OpenCodeGoProvider());
}

bool hasRealCredentials(const QString& providerId) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    if (providerId == "codex") {
        QString home = QDir::homePath();
        if (QFile::exists(home + "/.codex/auth.json")) return true;
        QString codexHome = env.value("CODEX_HOME").trimmed();
        if (!codexHome.isEmpty() && QFile::exists(codexHome + "/auth.json")) return true;
        return false;
    }

    if (providerId == "copilot") {
        if (env.contains("CODEXBAR_E2E_COPILOT_TOKEN")) return true;
        return ProviderCredentialStore::exists("com.codexbar.oauth.copilot");
    }

    if (providerId == "kimi") {
        return env.contains("CODEXBAR_E2E_KIMI_COOKIE")
            || env.contains("KIMI_AUTH_TOKEN")
            || env.contains("KIMI_MANUAL_COOKIE");
    }

    if (providerId == "opencode") {
        return env.contains("CODEXBAR_E2E_OPENCODE_COOKIE")
            || env.contains("OPENCODE_COOKIE")
            || env.contains("OPENCODE_AUTH");
    }

    if (providerId == "opencodego") {
        return env.contains("CODEXBAR_E2E_OPENCODEGO_COOKIE")
            || env.contains("CODEXBAR_E2E_OPENCODE_COOKIE")
            || env.contains("OPENCODE_COOKIE")
            || env.contains("OPENCODE_AUTH");
    }

    return false;
}

ProviderFetchResult runProviderFetch(const QString& providerId,
                                     const QHash<QString, QVariant>& overrides) {
    registerE2EProviders();
    IProvider* provider = ProviderRegistry::instance().provider(providerId);
    if (!provider) {
        ProviderFetchResult r;
        r.success = false;
        r.errorMessage = "Unknown provider: " + providerId;
        return r;
    }

    ProviderFetchContext ctx;
    ctx.providerId = providerId;
    ctx.sourceMode = ProviderSourceMode::Auto;
    ctx.isAppRuntime = true;
    ctx.allowInteractiveAuth = false;
    ctx.networkTimeoutMs = ProviderPipeline::STRATEGY_TIMEOUT_MS;

    QProcessEnvironment sysEnv = QProcessEnvironment::systemEnvironment();
    const auto keys = sysEnv.keys();
    for (const auto& key : keys) {
        ctx.env[key] = sysEnv.value(key);
    }

    // Apply overrides
    if (overrides.contains("sourceMode")) {
        QString sm = overrides.value("sourceMode").toString();
        ctx.sourceMode = sourceModeFromString(sm);
    }
    if (overrides.contains("manualCookieHeader")) {
        ctx.manualCookieHeader = overrides.value("manualCookieHeader").toString();
    }
    if (overrides.contains("apiRegion")) {
        ctx.settings.set("apiRegion", overrides.value("apiRegion"));
    }
    if (overrides.contains("accountID")) {
        ctx.accountID = overrides.value("accountID").toString();
    }
    if (overrides.contains("networkTimeoutMs")) {
        bool ok = false;
        int to = overrides.value("networkTimeoutMs").toInt(&ok);
        if (ok && to > 0) ctx.networkTimeoutMs = to;
    }

    // Resolve strategies
    ProviderPipeline pipeline;
    QVector<IFetchStrategy*> strategies = pipeline.resolveStrategies(provider, ctx);
    if (strategies.isEmpty()) {
        ProviderFetchResult r;
        r.success = false;
        r.errorMessage = "No available strategy for provider " + providerId;
        return r;
    }

    ProviderFetchResult result = pipeline.execute(strategies, ctx);
    qDeleteAll(strategies);
    return result;
}

QString maskSensitive(const QString& text) {
    QString out = text;
    // Mask Authorization: Bearer <token>
    QRegularExpression authRe(R"re((Authorization:\s*Bearer\s+)[A-Za-z0-9_\-\.]+)re",
                               QRegularExpression::CaseInsensitiveOption);
    out.replace(authRe, "\\1***");

    // Mask Cookie: sessionKey=...
    QRegularExpression cookieRe(R"re((Cookie:\s*.*sessionKey\s*=\s*)([^;\s]+))re",
                                 QRegularExpression::CaseInsensitiveOption);
    out.replace(cookieRe, "\\1***");

    // Mask generic api keys / tokens in JSON
    QRegularExpression tokenRe(R"re(("(?:access_token|refresh_token|id_token|api_key|token|secret)"\s*:\s*")([^"]+)("))re");
    out.replace(tokenRe, "\\1***\\3");

    return out;
}

} // namespace E2ETestUtils
