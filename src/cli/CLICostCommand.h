#pragma once

#include "CLIRenderer.h"
#include <QDate>

struct CLICostOptions {
    QString providerId = "all";
    bool forceRefresh = false;
    QDate since;
    QDate until;
    bool prettyJson = false;
    bool noColor = false;
};

class CLICostCommand {
public:
    int execute(const CLICostOptions& opts, CLIRenderer& out);
};
