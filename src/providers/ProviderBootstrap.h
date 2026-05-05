#pragma once

class SettingsStore;
class UsageStore;

namespace ProviderBootstrap {

void registerAllProviders();
void applyStoredProviderEnabledStates(SettingsStore* settings, UsageStore* usageStore);
void syncEnabledProviderRuntimes();

}
