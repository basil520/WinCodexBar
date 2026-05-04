#include "CLIEntry.h"
#include "CLIRenderer.h"
#include "CLIUsageCommand.h"
#include "CLICostCommand.h"
#include "CLIConfigCommand.h"

#include "../providers/ProviderRegistry.h"
#include "../providers/zai/ZaiProvider.h"
#include "../providers/openrouter/OpenRouterProvider.h"
#include "../providers/copilot/CopilotProvider.h"
#include "../providers/kimik2/KimiK2Provider.h"
#include "../providers/kilo/KiloProvider.h"
#include "../providers/kiro/KiroProvider.h"
#include "../providers/mistral/MistralProvider.h"
#include "../providers/ollama/OllamaProvider.h"
#include "../providers/codex/CodexProvider.h"
#include "../providers/claude/ClaudeProvider.h"
#include "../providers/cursor/CursorProvider.h"
#include "../providers/kimi/KimiProvider.h"
#include "../providers/opencode/OpenCodeProvider.h"
#include "../providers/opencode/OpenCodeGoProvider.h"
#include "../providers/alibaba/AlibabaProvider.h"
#include "../providers/deepseek/DeepSeekProvider.h"
#include "../providers/minimax/MiniMaxProvider.h"
#include "../providers/synthetic/SyntheticProvider.h"
#include "../providers/perplexity/PerplexityProvider.h"
#include "../providers/amp/AmpProvider.h"
#include "../providers/augment/AugmentProvider.h"
#include "../providers/gemini/GeminiProvider.h"
#include "../providers/vertexai/VertexAIProvider.h"
#include "../providers/jetbrains/JetBrainsProvider.h"
#include "../providers/factory/FactoryProvider.h"
#include "../providers/antigravity/AntigravityProvider.h"
#include "../providers/warp/WarpProvider.h"
#include "../providers/abacus/AbacusProvider.h"

#include "../runtime/ProviderRuntimeManager.h"
#include "../runtime/GenericRuntime.h"
#include "../runtime/CodexRuntime.h"
#include "../runtime/AugmentRuntime.h"
#include "../account/TokenAccountStore.h"
#include "../app/SettingsStore.h"
#include "../app/UsageStore.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <QDebug>

bool CLIEntry::isCliCommand(const QString& arg)
{
    static const QStringList commands = {"usage", "cost", "config"};
    return commands.contains(arg);
}

void CLIEntry::initBackend()
{
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
    ProviderRegistry::instance().registerProvider(new AlibabaProvider());
    ProviderRegistry::instance().registerProvider(new DeepSeekProvider());
    ProviderRegistry::instance().registerProvider(new MiniMaxProvider());
    ProviderRegistry::instance().registerProvider(new SyntheticProvider());
    ProviderRegistry::instance().registerProvider(new PerplexityProvider());
    ProviderRegistry::instance().registerProvider(new AmpProvider());
    ProviderRegistry::instance().registerProvider(new AugmentProvider());
    ProviderRegistry::instance().registerProvider(new GeminiProvider());
    ProviderRegistry::instance().registerProvider(new VertexAIProvider());
    ProviderRegistry::instance().registerProvider(new JetBrainsProvider());
    ProviderRegistry::instance().registerProvider(new FactoryProvider());
    ProviderRegistry::instance().registerProvider(new AntigravityProvider());
    ProviderRegistry::instance().registerProvider(new WarpProvider());
    ProviderRegistry::instance().registerProvider(new AbacusProvider());

    // Register runtimes
    auto* runtimeManager = ProviderRuntimeManager::instance();
    runtimeManager->registerRuntime("codex", new CodexRuntime());
    runtimeManager->registerRuntime("augment", new AugmentRuntime());
    for (auto* provider : ProviderRegistry::instance().allProviders()) {
        QString pid = provider->id();
        if (!runtimeManager->hasRuntime(pid)) {
            runtimeManager->registerRuntime(pid, new GenericRuntime(provider));
        }
    }
    runtimeManager->startAll();

    SettingsStore* settings = new SettingsStore();
    UsageStore* usageStore = new UsageStore();
    usageStore->setSettingsStore(settings);

    TokenAccountStore* tokenStore = TokenAccountStore::instance();
    tokenStore->loadFromDisk();
    tokenStore->migrateFromLegacy(settings);
}

int CLIEntry::run(int argc, char* argv[])
{
    QCommandLineParser parser;
    parser.setApplicationDescription("WinCodexBar CLI");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption verboseOption({"v", "verbose"}, "Verbose output");
    parser.addOption(verboseOption);

    parser.addPositionalArgument("command", "usage | cost | config");
    parser.process(QCoreApplication::arguments());

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        parser.showHelp(4);
        return 4;
    }

    const QString cmd = args.first();
    try {
        initBackend();

        int rc = 0;
        if (cmd == "usage") rc = runUsage(args.mid(1));
        else if (cmd == "cost") rc = runCost(args.mid(1));
        else if (cmd == "config") rc = runConfig(args.mid(1));
        else {
            QTextStream(stderr) << "Unknown command: " << cmd << "\n";
            rc = 4;
        }

        return rc;
    } catch (const std::exception& e) {
        QTextStream(stderr) << "Fatal: " << e.what() << "\n";
        return 1;
    }
}

int CLIEntry::runUsage(const QStringList& args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Query provider usage");
    parser.addHelpOption();

    QCommandLineOption providerOption("provider", "Provider ID or 'all'", "id", "all");
    QCommandLineOption sourceOption("source", "Source mode (auto/web/cli/oauth/api)", "mode", "auto");
    QCommandLineOption statusOption("status", "Include provider health status");
    QCommandLineOption formatOption("format", "Output format (text/json)", "fmt", "text");
    QCommandLineOption prettyOption("pretty", "Pretty-print JSON");
    QCommandLineOption noColorOption("no-color", "Disable ANSI colors");
    QCommandLineOption timeoutOption("web-timeout", "Timeout for web strategies (ms)", "ms", "15000");

    parser.addOption(providerOption);
    parser.addOption(sourceOption);
    parser.addOption(statusOption);
    parser.addOption(formatOption);
    parser.addOption(prettyOption);
    parser.addOption(noColorOption);
    parser.addOption(timeoutOption);
    parser.process(args);

    CLIUsageOptions opts;
    opts.providerId = parser.value(providerOption);
    opts.sourceMode = parser.value(sourceOption);
    opts.includeStatus = parser.isSet(statusOption);
    opts.prettyJson = parser.isSet(prettyOption);
    opts.noColor = parser.isSet(noColorOption);
    opts.webTimeoutMs = parser.value(timeoutOption).toInt();

    bool jsonMode = parser.value(formatOption) == "json";
    CLIRenderer renderer(jsonMode, opts.prettyJson, opts.noColor);
    CLIUsageCommand cmd;
    return cmd.execute(opts, renderer);
}

int CLIEntry::runCost(const QStringList& args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Query local cost usage");
    parser.addHelpOption();

    QCommandLineOption providerOption("provider", "Provider ID or 'all'", "id", "all");
    QCommandLineOption refreshOption("refresh", "Force rescan ignoring cache");
    QCommandLineOption sinceOption("since", "Start date (YYYY-MM-DD)", "date");
    QCommandLineOption untilOption("until", "End date (YYYY-MM-DD)", "date");
    QCommandLineOption formatOption("format", "Output format (text/json)", "fmt", "text");
    QCommandLineOption prettyOption("pretty", "Pretty-print JSON");
    QCommandLineOption noColorOption("no-color", "Disable ANSI colors");

    parser.addOption(providerOption);
    parser.addOption(refreshOption);
    parser.addOption(sinceOption);
    parser.addOption(untilOption);
    parser.addOption(formatOption);
    parser.addOption(prettyOption);
    parser.addOption(noColorOption);
    parser.process(args);

    CLICostOptions opts;
    opts.providerId = parser.value(providerOption);
    opts.forceRefresh = parser.isSet(refreshOption);
    if (parser.isSet(sinceOption)) opts.since = QDate::fromString(parser.value(sinceOption), Qt::ISODate);
    if (parser.isSet(untilOption)) opts.until = QDate::fromString(parser.value(untilOption), Qt::ISODate);
    opts.prettyJson = parser.isSet(prettyOption);
    opts.noColor = parser.isSet(noColorOption);

    bool jsonMode = parser.value(formatOption) == "json";
    CLIRenderer renderer(jsonMode, opts.prettyJson, opts.noColor);
    CLICostCommand cmd;
    return cmd.execute(opts, renderer);
}

int CLIEntry::runConfig(const QStringList& args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Configuration commands");
    parser.addHelpOption();

    parser.addPositionalArgument("subcommand", "validate | dump");
    parser.process(args);

    const QStringList posArgs = parser.positionalArguments();
    if (posArgs.isEmpty()) {
        parser.showHelp(4);
        return 4;
    }

    CLIConfigOptions opts;
    opts.subcommand = posArgs.first();

    CLIRenderer renderer(false, false, false);
    CLIConfigCommand cmd;
    return cmd.execute(opts, renderer);
}
