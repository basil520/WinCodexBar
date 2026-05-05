#include "CLIEntry.h"
#include "CLIRenderer.h"
#include "CLIUsageCommand.h"
#include "CLICostCommand.h"
#include "CLIConfigCommand.h"

#include "../providers/ProviderBootstrap.h"
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
    ProviderBootstrap::registerAllProviders();

    SettingsStore* settings = new SettingsStore();
    UsageStore* usageStore = new UsageStore();
    usageStore->setSettingsStore(settings);

    TokenAccountStore* tokenStore = TokenAccountStore::instance();
    tokenStore->loadFromDisk();
    tokenStore->migrateFromLegacy(settings);
    ProviderBootstrap::applyStoredProviderEnabledStates(settings, usageStore);
    ProviderBootstrap::syncEnabledProviderRuntimes();
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
