#pragma once

#include "ProviderSourceMode.h"
#include "ProviderFetchKind.h"
#include "ProviderSettingsSnapshot.h"
#include "../account/TokenAccountCredentials.h"

#include <QString>
#include <QHash>
#include <optional>

struct ProviderFetchContext {
    QString providerId;
    ProviderSourceMode sourceMode = ProviderSourceMode::Auto;
    ProviderSettingsSnapshot settings;
    QHash<QString, QString> env;
    bool isAppRuntime = true;
    bool allowInteractiveAuth = false;
    bool verbose = false;
    bool webDebugDumpHTML = false;
    int networkTimeoutMs = 15000;
    QString accountID;
    TokenAccountCredentials accountCredentials;
    std::optional<QString> manualCookieHeader;
};
