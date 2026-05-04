#pragma once

#include <QStringList>

class CLIEntry {
public:
    static bool isCliCommand(const QString& arg);
    int run(int argc, char* argv[]);

private:
    int runUsage(const QStringList& args);
    int runCost(const QStringList& args);
    int runConfig(const QStringList& args);
    void initBackend();
};
