cmake_minimum_required(VERSION 3.21)
project(WinCodexBar VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 COMPONENTS Core Gui Widgets Quick Qml Network Sql WebSockets Concurrent QUIET)
set(WINCODEXBAR_QT_RUNTIME_FOUND ${Qt6_FOUND})
if(NOT WINCODEXBAR_QT_RUNTIME_FOUND)
    message(WARNING "Qt 6.5+ not found. Install Qt 6.5+ (msvc2022_64) and set CMAKE_PREFIX_PATH.")
    message(WARNING "  Example: cmake -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2022_64")
else()
    find_package(Qt6 6.5 COMPONENTS LinguistTools QUIET)
endif()

set(CORE_SOURCES
    src/models/RateWindow.cpp
    src/models/UsageSnapshot.cpp
    src/models/UsagePace.cpp
    src/models/ProviderCostSnapshot.cpp
    src/models/CreditsSnapshot.cpp
    src/models/ProviderIdentitySnapshot.cpp
    src/models/ProviderDescriptor.cpp
    src/models/WidgetSnapshot.cpp
    src/models/ZaiUsageSnapshot.cpp
    src/models/OpenRouterUsageSnapshot.cpp
    src/models/MiniMaxUsageSnapshot.cpp
    src/models/PlanUtilizationHistory.cpp
    src/models/CodexUsageResponse.cpp
    src/models/ClaudeUsageSnapshot.cpp
    src/models/CursorUsageSummary.cpp

    src/providers/ProviderRegistry.cpp
    src/providers/ProviderBootstrap.cpp
    src/providers/ProviderPipeline.cpp
    src/providers/shared/ProviderCredentialStore.cpp

    src/providers/zai/ZaiProvider.cpp
    src/providers/openrouter/OpenRouterProvider.cpp
    src/providers/copilot/CopilotProvider.cpp
    src/providers/kimik2/KimiK2Provider.cpp
    src/providers/kilo/KiloProvider.cpp
    src/providers/kiro/KiroProvider.cpp
    src/providers/mistral/MistralProvider.cpp
    src/providers/ollama/OllamaProvider.cpp
    src/providers/codex/CodexProvider.cpp
    src/providers/codex/CodexUIErrorMapper.cpp
    src/providers/codex/CodexConsumerProjection.cpp
    src/providers/codex/CodexIdentity.cpp
    src/providers/codex/CodexRateWindowNormalizer.cpp
    src/providers/codex/CodexReconciledState.cpp
    src/providers/codex/CodexTokenRefresher.cpp
    src/providers/codex/CodexOAuthUsageFetcher.cpp
    src/providers/codex/CodexStatusProbe.cpp
    src/providers/codex/CodexDashboardAuthority.cpp
    src/providers/codex/CodexDashboardAuthorityContext.cpp
    src/providers/codex/CodexDashboardCache.cpp
    src/providers/codex/CodexPersistentCLISession.cpp
    src/providers/codex/CodexRpcClient.cpp
    src/providers/codex/CodexCreditsFetcher.cpp
    src/providers/codex/OpenAIDashboardFetcher.cpp
    src/providers/codex/CodexSystemAccountObserver.cpp
    src/providers/codex/ManagedCodexAccount.cpp
    src/providers/codex/CodexAccountReconciliation.cpp
    src/providers/codex/CodexHomeScope.cpp
    src/providers/codex/CodexOpenAIWorkspaceResolver.cpp
    src/providers/codex/CodexOpenAIWorkspaceIdentityCache.cpp
    src/providers/codex/CodexPlanFormatting.cpp
    src/providers/codex/CodexProviderSettingsBuilder.cpp
    src/providers/codex/CodexUsageDataSource.cpp
    src/providers/codex/ManagedCodexAccountService.cpp
    src/providers/codex/CodexLoginRunner.cpp
    src/providers/claude/ClaudeProvider.cpp
    src/providers/cursor/CursorProvider.cpp
    src/providers/kimi/KimiProvider.cpp
    src/providers/kimi/KimiTokenResolver.cpp
    src/providers/opencode/OpenCodeProvider.cpp
    src/providers/opencode/OpenCodeGoProvider.cpp
    src/providers/opencode/OpenCodeUtils.cpp
    src/providers/alibaba/AlibabaProvider.cpp

    src/providers/deepseek/DeepSeekProvider.cpp
    src/providers/minimax/MiniMaxProvider.cpp
    src/providers/synthetic/SyntheticProvider.cpp
    src/providers/perplexity/PerplexityProvider.cpp
    src/providers/amp/AmpProvider.cpp
    src/providers/augment/AugmentProvider.cpp
    src/providers/gemini/GeminiProvider.cpp
    src/providers/vertexai/VertexAIProvider.cpp
    src/providers/jetbrains/JetBrainsProvider.cpp
    src/providers/factory/FactoryProvider.cpp
    src/providers/antigravity/AntigravityProvider.cpp
    src/providers/warp/WarpProvider.cpp
    src/providers/abacus/AbacusProvider.cpp
    src/providers/codebuff/CodebuffProvider.cpp

    src/network/NetworkManager.cpp

    src/app/CodexBarApp.cpp
    src/app/AppTheme.cpp
    src/app/SettingsStore.cpp
    src/app/UsageStore.cpp
    src/app/BatchUpdateController.cpp
    src/app/CredentialAccessGate.cpp
    src/app/PlanUtilizationHistoryStore.cpp
    src/app/SessionQuotaNotifications.cpp
    src/app/LanguageManager.cpp
    src/app/Localization.cpp

    src/tray/StatusItemController.cpp
    src/tray/TrayPopupPositioner.cpp
    src/tray/TrayIconRenderer.cpp

    src/util/TextParser.cpp
    src/util/BinaryLocator.cpp
    src/util/CostUsageScanner.cpp
    src/util/CostUsageCache.cpp
    src/util/SingleInstanceGuard.cpp
    src/util/UsagePaceText.cpp

    src/account/TokenAccountStore.cpp

    src/runtime/IProviderRuntime.cpp
    src/runtime/IProviderLoginFlow.cpp
    src/runtime/ProviderRuntimeManager.cpp
    src/runtime/GenericRuntime.cpp
    src/runtime/CodexRuntime.cpp
    src/runtime/AugmentRuntime.cpp
    src/runtime/OAuthDeviceFlow.cpp

    src/cli/CLIEntry.cpp
    src/cli/CLIRenderer.cpp
    src/cli/CLIUsageCommand.cpp
    src/cli/CLICostCommand.cpp
    src/cli/CLIConfigCommand.cpp
)

set(CORE_HEADERS
    src/models/RateWindow.h
    src/models/UsageSnapshot.h
    src/models/UsagePace.h
    src/models/ProviderCostSnapshot.h
    src/models/CreditsSnapshot.h
    src/models/ProviderIdentitySnapshot.h
    src/models/ProviderDescriptor.h
    src/models/WidgetSnapshot.h
    src/models/CostUsageReport.h
    src/models/ZaiUsageSnapshot.h
    src/models/OpenRouterUsageSnapshot.h
    src/models/MiniMaxUsageSnapshot.h
    src/models/CursorRequestUsage.h
    src/models/CursorUsageSummary.h
    src/models/PlanUtilizationHistory.h

    src/models/CodexUsageResponse.h
    src/models/ClaudeUsageSnapshot.h

    src/providers/codex/CodexIdentity.h
    src/providers/codex/CodexUIErrorMapper.h
    src/providers/codex/CodexConsumerProjection.h
    src/providers/codex/CodexRateWindowNormalizer.h
    src/providers/codex/CodexReconciledState.h
    src/providers/codex/CodexTokenRefresher.h
    src/providers/codex/CodexOAuthUsageFetcher.h
    src/providers/codex/CodexStatusProbe.h
    src/providers/codex/CodexCreditsFetcher.h
    src/providers/codex/CodexDashboardAuthority.h
    src/providers/codex/CodexSystemAccountObserver.h
    src/providers/codex/ManagedCodexAccount.h
    src/providers/codex/CodexAccountReconciliation.h
    src/providers/codex/CodexHomeScope.h
    src/providers/codex/CodexOpenAIWorkspaceResolver.h
    src/providers/codex/CodexOpenAIWorkspaceIdentityCache.h
    src/providers/codex/CodexPlanFormatting.h
    src/providers/codex/CodexProviderSettingsBuilder.h
    src/providers/codex/CodexUsageDataSource.h
    src/providers/codex/ManagedCodexAccountService.h
    src/providers/codex/CodexLoginRunner.h
    src/providers/kimi/KimiProvider.h
    src/providers/kimi/KimiAPIError.h
    src/providers/kimi/KimiTokenResolver.h
    src/providers/kimi/KimiModels.h
    src/providers/opencode/OpenCodeProvider.h
    src/providers/opencode/OpenCodeGoProvider.h
    src/providers/opencode/OpenCodeUtils.h
    src/providers/alibaba/AlibabaProvider.h

    src/providers/deepseek/DeepSeekProvider.h
    src/providers/minimax/MiniMaxProvider.h
    src/providers/synthetic/SyntheticProvider.h
    src/providers/perplexity/PerplexityProvider.h
    src/providers/amp/AmpProvider.h
    src/providers/augment/AugmentProvider.h
    src/providers/gemini/GeminiProvider.h
    src/providers/vertexai/VertexAIProvider.h
    src/providers/jetbrains/JetBrainsProvider.h
    src/providers/factory/FactoryProvider.h
    src/providers/antigravity/AntigravityProvider.h
    src/providers/warp/WarpProvider.h
    src/providers/abacus/AbacusProvider.h
    src/providers/codebuff/CodebuffProvider.h

    src/providers/IProvider.h
    src/providers/IFetchStrategy.h
    src/providers/ProviderRegistry.h
    src/providers/ProviderBootstrap.h
    src/providers/ProviderFetchContext.h
    src/providers/ProviderFetchResult.h
    src/providers/ProviderPipeline.h
    src/providers/ProviderSettingsSnapshot.h
    src/providers/ProviderFetchKind.h
    src/providers/ProviderSourceMode.h

    src/providers/shared/WindowsCredentialStore.h
    src/providers/shared/ProviderCredentialStore.h
    src/providers/shared/CookieImporter.h
    src/providers/shared/ConPTYSession.h
    src/providers/shared/BrowserDetection.h

    src/network/NetworkManager.h

    src/app/CodexBarApp.h
    src/app/AppTheme.h
    src/app/SettingsStore.h
    src/app/UsageStore.h
    src/app/BatchUpdateController.h
    src/app/CredentialAccessGate.h
    src/app/PlanUtilizationHistoryStore.h
    src/app/SessionQuotaNotifications.h
    src/app/Localization.h

    src/tray/StatusItemController.h
    src/tray/TrayPopupPositioner.h
    src/tray/TrayIconRenderer.h

    src/util/TextParser.h
    src/util/BinaryLocator.h
    src/util/CostUsageScanner.h
    src/util/CostUsageCache.h
    src/util/SingleInstanceGuard.h
    src/util/UsagePaceText.h

    src/account/TokenAccount.h
    src/account/TokenAccountCredentials.h
    src/account/TokenAccountStore.h
    src/account/SecureString.h

    src/runtime/IProviderRuntime.h
    src/runtime/IProviderLoginFlow.h
    src/runtime/ProviderRuntimeManager.h
    src/runtime/GenericRuntime.h
    src/runtime/CodexRuntime.h
    src/runtime/AugmentRuntime.h
    src/runtime/OAuthDeviceFlow.h

    src/cli/CLIEntry.h
    src/cli/CLIRenderer.h
    src/cli/CLIUsageCommand.h
    src/cli/CLICostCommand.h
    src/cli/CLIConfigCommand.h
)

add_library(codexbar_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(codexbar_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(codexbar_core PUBLIC NOMINMAX _WIN32_WINNT=0x0A00 WINVER=0x0A00)

if(WINCODEXBAR_QT_RUNTIME_FOUND)
    target_link_libraries(codexbar_core PUBLIC
        Qt6::Core Qt6::Network Qt6::Sql Qt6::Gui Qt6::Widgets Qt6::Quick Qt6::WebSockets Qt6::Concurrent
    )
endif()

if(WIN32)
    target_sources(codexbar_core PRIVATE
        src/providers/shared/WindowsCredentialStore.cpp
        src/providers/shared/CookieImporter.cpp
        src/providers/shared/ConPTYSession.cpp
        src/providers/shared/BrowserDetection.cpp
    )
    target_link_libraries(codexbar_core PRIVATE
        advapi32 crypt32 bcrypt shell32 user32 dwmapi
    )
endif()

if(WINCODEXBAR_QT_RUNTIME_FOUND)
    add_executable(WinCodexBar WIN32
        src/main.cpp
        resources/qml.qrc
        resources/wincodexbar.rc
    )
    target_link_libraries(WinCodexBar PRIVATE
        codexbar_core
        Qt6::Quick Qt6::Qml Qt6::Widgets
    )

    if(WIN32 AND TARGET Qt6::windeployqt)
        add_custom_command(TARGET WinCodexBar POST_BUILD
            COMMAND Qt6::windeployqt --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml" "$<TARGET_FILE:WinCodexBar>"
            COMMENT "Deploying Qt runtime and QML modules"
        )
    endif()

    # Compile .ts to .qm using lrelease
    if(TARGET Qt6::lrelease)
        set(TS_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/translations/WinCodexBar_en.ts"
            "${CMAKE_CURRENT_SOURCE_DIR}/translations/WinCodexBar_zh_CN.ts"
        )
        set(QM_FILES)
        foreach(TS_FILE ${TS_FILES})
            get_filename_component(TS_NAME ${TS_FILE} NAME_WE)
            set(QM_FILE "${CMAKE_CURRENT_BINARY_DIR}/translations/${TS_NAME}.qm")
            add_custom_command(
                OUTPUT ${QM_FILE}
                COMMAND Qt6::lrelease ${TS_FILE} -qm ${QM_FILE}
                DEPENDS ${TS_FILE}
                COMMENT "Generating ${TS_NAME}.qm"
            )
            list(APPEND QM_FILES ${QM_FILE})
        endforeach()
        add_custom_target(translations ALL DEPENDS ${QM_FILES})
        add_dependencies(WinCodexBar translations)
        add_custom_command(TARGET WinCodexBar POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:WinCodexBar>/translations"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QM_FILES} "$<TARGET_FILE_DIR:WinCodexBar>/translations"
            COMMENT "Deploying translation files"
        )
    else()
        message(WARNING "Qt Linguist lrelease was not found; runtime language switching will not have .qm files.")
    endif()
endif()

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS AND WINCODEXBAR_QT_RUNTIME_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()

option(BUILD_E2E_TESTS "Build end-to-end tests against real accounts" OFF)
if(BUILD_E2E_TESTS AND WINCODEXBAR_QT_RUNTIME_FOUND)
    enable_testing()
    add_subdirectory(tests/e2e)
endif()
