#pragma once

#include "CLIRenderer.h"

struct CLIConfigOptions {
    QString subcommand; // "validate" or "dump"
};

class CLIConfigCommand {
public:
    int execute(const CLIConfigOptions& opts, CLIRenderer& out);
};
