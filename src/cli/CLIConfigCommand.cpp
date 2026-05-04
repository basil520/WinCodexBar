#include "CLIConfigCommand.h"
#include "../app/SettingsStore.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

int CLIConfigCommand::execute(const CLIConfigOptions& opts, CLIRenderer& out)
{
    SettingsStore settings;
    settings.loadConfig();

    if (opts.subcommand == "validate") {
        // Basic validation: check config file exists and is readable
        QString path = settings.configPath();
        bool valid = QFile::exists(path);

        if (out.isJsonMode()) {
            QJsonObject root;
            root["valid"] = valid;
            root["configPath"] = path;
            out.setRootObject(root);
        } else {
            if (valid) {
                out.line("Config is valid.");
            } else {
                out.error("Config file not found: " + path);
            }
        }
        out.flush();
        return valid ? 0 : 5;
    }

    if (opts.subcommand == "dump") {
        QJsonObject root;
        root["refreshFrequency"] = settings.refreshFrequency();
        root["launchAtLogin"] = settings.launchAtLogin();
        root["checkForUpdates"] = settings.checkForUpdates();
        root["statusChecksEnabled"] = settings.statusChecksEnabled();
        root["language"] = settings.language();
        root["providerOrder"] = QJsonArray::fromStringList(settings.providerOrder());

        if (out.isJsonMode()) {
            out.setRootObject(root);
        } else {
            out.heading("Configuration");
            out.line(QString("Refresh frequency: %1 min").arg(settings.refreshFrequency()));
            out.line(QString("Launch at login: %1").arg(settings.launchAtLogin() ? "yes" : "no"));
            out.line(QString("Check for updates: %1").arg(settings.checkForUpdates() ? "yes" : "no"));
            out.line(QString("Status checks: %1").arg(settings.statusChecksEnabled() ? "yes" : "no"));
            out.line(QString("Language: %1").arg(settings.language()));
        }
        out.flush();
        return 0;
    }

    out.error("Unknown config subcommand: " + opts.subcommand);
    out.flush();
    return 4;
}
