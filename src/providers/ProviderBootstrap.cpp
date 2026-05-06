#include "ProviderBootstrap.h"

#include "ProviderRegistry.h"
#include "zai/ZaiProvider.h"
#include "openrouter/OpenRouterProvider.h"
#include "copilot/CopilotProvider.h"
#include "kimik2/KimiK2Provider.h"
#include "kilo/KiloProvider.h"
#include "kiro/KiroProvider.h"
#include "mistral/MistralProvider.h"
#include "ollama/OllamaProvider.h"
#include "codex/CodexProvider.h"
#include "claude/ClaudeProvider.h"
#include "cursor/CursorProvider.h"
#include "kimi/KimiProvider.h"
#include "opencode/OpenCodeProvider.h"
#include "opencode/OpenCodeGoProvider.h"
#include "alibaba/AlibabaProvider.h"
#include "deepseek/DeepSeekProvider.h"
#include "minimax/MiniMaxProvider.h"
#include "synthetic/SyntheticProvider.h"
#include "perplexity/PerplexityProvider.h"
#include "amp/AmpProvider.h"
#include "augment/AugmentProvider.h"
#include "gemini/GeminiProvider.h"
#include "vertexai/VertexAIProvider.h"
#include "jetbrains/JetBrainsProvider.h"
#include "factory/FactoryProvider.h"
#include "antigravity/AntigravityProvider.h"
#include "warp/WarpProvider.h"
#include "abacus/AbacusProvider.h"
#include "codebuff/CodebuffProvider.h"
#include "windsurf/WindsurfProvider.h"

#include "../app/SettingsStore.h"
#include "../app/UsageStore.h"
#include "../runtime/ProviderRuntimeManager.h"

#include <QSettings>
#include <QString>

namespace {

template <typename Provider>
void registerProviderIfMissing(const QString& id)
{
    auto& registry = ProviderRegistry::instance();
    if (!registry.provider(id)) {
        registry.registerProvider(new Provider());
    }
}

} // namespace

namespace ProviderBootstrap {

void registerAllProviders()
{
    registerProviderIfMissing<ZaiProvider>(QStringLiteral("zai"));
    registerProviderIfMissing<OpenRouterProvider>(QStringLiteral("openrouter"));
    registerProviderIfMissing<CopilotProvider>(QStringLiteral("copilot"));
    registerProviderIfMissing<KimiK2Provider>(QStringLiteral("kimik2"));
    registerProviderIfMissing<KiloProvider>(QStringLiteral("kilo"));
    registerProviderIfMissing<KiroProvider>(QStringLiteral("kiro"));
    registerProviderIfMissing<MistralProvider>(QStringLiteral("mistral"));
    registerProviderIfMissing<OllamaProvider>(QStringLiteral("ollama"));
    registerProviderIfMissing<CodexProvider>(QStringLiteral("codex"));
    registerProviderIfMissing<ClaudeProvider>(QStringLiteral("claude"));
    registerProviderIfMissing<CursorProvider>(QStringLiteral("cursor"));
    registerProviderIfMissing<KimiProvider>(QStringLiteral("kimi"));
    registerProviderIfMissing<OpenCodeProvider>(QStringLiteral("opencode"));
    registerProviderIfMissing<OpenCodeGoProvider>(QStringLiteral("opencodego"));
    registerProviderIfMissing<AlibabaProvider>(QStringLiteral("alibaba"));
    registerProviderIfMissing<DeepSeekProvider>(QStringLiteral("deepseek"));
    registerProviderIfMissing<MiniMaxProvider>(QStringLiteral("minimax"));
    registerProviderIfMissing<SyntheticProvider>(QStringLiteral("synthetic"));
    registerProviderIfMissing<PerplexityProvider>(QStringLiteral("perplexity"));
    registerProviderIfMissing<AmpProvider>(QStringLiteral("amp"));
    registerProviderIfMissing<AugmentProvider>(QStringLiteral("augment"));
    registerProviderIfMissing<GeminiProvider>(QStringLiteral("gemini"));
    registerProviderIfMissing<VertexAIProvider>(QStringLiteral("vertexai"));
    registerProviderIfMissing<JetBrainsProvider>(QStringLiteral("jetbrains"));
    registerProviderIfMissing<FactoryProvider>(QStringLiteral("factory"));
    registerProviderIfMissing<AntigravityProvider>(QStringLiteral("antigravity"));
    registerProviderIfMissing<WarpProvider>(QStringLiteral("warp"));
    registerProviderIfMissing<AbacusProvider>(QStringLiteral("abacus"));
    registerProviderIfMissing<CodebuffProvider>(QStringLiteral("codebuff"));
    registerProviderIfMissing<WindsurfProvider>(QStringLiteral("windsurf"));
}

void applyStoredProviderEnabledStates(SettingsStore* settings, UsageStore* usageStore)
{
    QSettings reg(QStringLiteral("HKEY_CURRENT_USER\\Software\\CodexBar"), QSettings::NativeFormat);
    const QVector<QString> ids = ProviderRegistry::instance().providerIDs();
    for (const QString& id : ids) {
        const QString key = QStringLiteral("providers/") + id + QStringLiteral("/enabled");
        bool enabled = false;
        if (reg.contains(key)) {
            enabled = reg.value(key).toBool();
        } else if (auto* provider = ProviderRegistry::instance().provider(id)) {
            enabled = provider->defaultEnabled();
        }

        if (usageStore) {
            usageStore->setProviderEnabled(id, enabled);
        } else {
            ProviderRegistry::instance().setProviderEnabled(id, enabled);
        }
        if (settings) {
            settings->setProviderEnabled(id, enabled);
        }
    }
}

void syncEnabledProviderRuntimes()
{
    ProviderRuntimeManager::instance()->syncEnabledProviderRuntimes();
}

} // namespace ProviderBootstrap
