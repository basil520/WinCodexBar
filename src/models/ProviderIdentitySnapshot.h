#pragma once

#include <QString>
#include <optional>

enum class UsageProvider : int {
    codex = 0,
    claude,
    cursor,
    opencode,
    opencodego,
    alibaba,
    factory,
    gemini,
    antigravity,
    copilot,
    zai,
    minimax,
    kimi,
    kilo,
    kiro,
    vertexai,
    augment,
    jetbrains,
    kimik2,
    amp,
    ollama,
    synthetic,
    warp,
    openrouter,
    perplexity,
    abacus,
    mistral,
    deepseek,
    codebuff,
    windsurf,
};

struct ProviderIdentitySnapshot {
    std::optional<UsageProvider> providerID;
    std::optional<QString> accountEmail;
    std::optional<QString> accountOrganization;
    std::optional<QString> loginMethod;
};
