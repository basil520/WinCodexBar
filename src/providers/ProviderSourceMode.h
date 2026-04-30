#pragma once

#include <QString>

enum class ProviderSourceMode {
    Auto,
    Web,
    CLI,
    OAuth,
    API
};

inline ProviderSourceMode sourceModeFromString(const QString& s) {
    if (s == "web") return ProviderSourceMode::Web;
    if (s == "cli") return ProviderSourceMode::CLI;
    if (s == "oauth") return ProviderSourceMode::OAuth;
    if (s == "api") return ProviderSourceMode::API;
    return ProviderSourceMode::Auto;
}

inline QString sourceModeToString(ProviderSourceMode mode) {
    switch (mode) {
    case ProviderSourceMode::Web: return "web";
    case ProviderSourceMode::CLI: return "cli";
    case ProviderSourceMode::OAuth: return "oauth";
    case ProviderSourceMode::API: return "api";
    case ProviderSourceMode::Auto: return "auto";
    }
    return "auto";
}
