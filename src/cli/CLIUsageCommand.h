#pragma once

#include "CLIRenderer.h"
#include "../providers/ProviderFetchContext.h"

struct CLIUsageOptions {
    QString providerId = "all";
    QString sourceMode = "auto";
    bool includeStatus = false;
    bool prettyJson = false;
    bool noColor = false;
    int webTimeoutMs = 15000;
};

class CLIUsageCommand {
public:
    int execute(const CLIUsageOptions& opts, CLIRenderer& out);
};
