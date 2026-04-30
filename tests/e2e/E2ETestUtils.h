#pragma once

#include "../src/providers/ProviderFetchResult.h"
#include <QHash>
#include <QVariant>
#include <QString>

namespace E2ETestUtils {

// Register all production providers into ProviderRegistry.
// Must be called before runProviderFetch() in test processes that do not
// share main.cpp's initialization path.
void registerE2EProviders();

// Check whether real credentials are available for a given provider.
// This inspects environment variables and filesystem state without
// making network calls.
bool hasRealCredentials(const QString& providerId);

// Run the full provider pipeline against the real network using the
// credentials available in the current environment.
//
// The function constructs a ProviderFetchContext, resolves strategies,
// and executes them.  It does NOT go through UsageStore or SettingsStore,
// so no registry or AppData files are modified.
//
// Optional overrides can be used to inject provider-specific settings
// (e.g. manualCookieHeader, apiRegion, sourceMode).
ProviderFetchResult runProviderFetch(const QString& providerId,
                                     const QHash<QString, QVariant>& overrides = {});

// Mask sensitive tokens in a string for safe logging.
// Replaces Authorization / Cookie / Bearer values with ***.
QString maskSensitive(const QString& text);

} // namespace E2ETestUtils
