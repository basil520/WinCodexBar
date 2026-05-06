cmake_minimum_required(VERSION 3.21)

find_package(Qt6 6.5 COMPONENTS Core Test Qml Network Sql QUIET)
if(NOT Qt6_FOUND)
    return()
endif()

set(COMMON_INCLUDE "${CMAKE_SOURCE_DIR}/src")
set(TEST_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}")
set(COMMON_DEFINES NOMINMAX)

set(QT_RUNTIME_DIR "")
if(WIN32 AND TARGET Qt6::Core)
    get_target_property(QT_CORE_DLL Qt6::Core IMPORTED_LOCATION_RELEASE)
    if(NOT QT_CORE_DLL)
        get_target_property(QT_CORE_DLL Qt6::Core IMPORTED_LOCATION)
    endif()
    if(QT_CORE_DLL)
        get_filename_component(QT_RUNTIME_DIR "${QT_CORE_DLL}" DIRECTORY)
    endif()
endif()

set(QT_PLUGIN_DIR "")
if(WIN32 AND QT_RUNTIME_DIR)
    get_filename_component(QT_ROOT_DIR "${QT_RUNTIME_DIR}" DIRECTORY)
    set(QT_PLUGIN_DIR "${QT_ROOT_DIR}/plugins")
endif()

function(add_wincodexbar_test TEST_NAME TARGET_NAME)
    if(WIN32 AND QT_RUNTIME_DIR)
        add_test(NAME ${TEST_NAME}
            COMMAND ${CMAKE_COMMAND} -E env
                "PATH=${QT_RUNTIME_DIR};$ENV{PATH}"
                "QT_PLUGIN_PATH=${QT_PLUGIN_DIR}"
                $<TARGET_FILE:${TARGET_NAME}>)
    else()
        add_test(NAME ${TEST_NAME} COMMAND ${TARGET_NAME})
    endif()
endfunction()

add_executable(tst_RateWindow
    tst_RateWindow.cpp
    ../src/models/RateWindow.cpp
    ../src/models/RateWindow.h
)
target_include_directories(tst_RateWindow PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_RateWindow PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_RateWindow PRIVATE Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_RateWindow tst_RateWindow)

add_executable(tst_UsagePace
    tst_UsagePace.cpp
    ../src/models/UsagePace.cpp
    ../src/models/UsagePace.h
)
target_include_directories(tst_UsagePace PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_UsagePace PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_UsagePace PRIVATE Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_UsagePace tst_UsagePace)

add_executable(tst_Localization
    tst_Localization.cpp
    ../src/app/LanguageManager.cpp
    ../src/app/LanguageManager.h
    ../src/app/Localization.cpp
    ../src/app/Localization.h
    ../src/models/UsagePace.cpp
    ../src/models/UsagePace.h
    ../src/util/UsagePaceText.cpp
    ../src/util/UsagePaceText.h
)
target_include_directories(tst_Localization PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_Localization PRIVATE
    ${COMMON_DEFINES}
    TEST_TRANSLATION_DIR="${CMAKE_BINARY_DIR}/translations"
)
target_link_libraries(tst_Localization PRIVATE Qt6::Core Qt6::Test Qt6::Qml)
if(TARGET translations)
    add_dependencies(tst_Localization translations)
endif()
add_wincodexbar_test(tst_Localization tst_Localization)

add_executable(tst_AppTheme
    tst_AppTheme.cpp
)
target_include_directories(tst_AppTheme PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_AppTheme PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_AppTheme PRIVATE codexbar_core Qt6::Core Qt6::Gui Qt6::Test)
add_wincodexbar_test(tst_AppTheme tst_AppTheme)

add_executable(tst_TextParser
    tst_TextParser.cpp
    tst_TextParser.h
    ../src/util/TextParser.cpp
    ../src/util/TextParser.h
)
target_include_directories(tst_TextParser PRIVATE ${COMMON_INCLUDE} ${TEST_INCLUDE})
target_compile_definitions(tst_TextParser PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_TextParser PRIVATE Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_TextParser tst_TextParser)

add_executable(tst_SingleInstanceGuard
    tst_SingleInstanceGuard.cpp
)
target_include_directories(tst_SingleInstanceGuard PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_SingleInstanceGuard PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_SingleInstanceGuard PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_SingleInstanceGuard tst_SingleInstanceGuard)

add_executable(tst_ZaiProvider
    tst_ZaiProvider.cpp
)
target_include_directories(tst_ZaiProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_ZaiProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_ZaiProvider PRIVATE Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_ZaiProvider tst_ZaiProvider)

add_executable(tst_CostUsageScanner
    tst_CostUsageScanner.cpp
    ../src/util/CostUsageScanner.cpp
    ../src/util/CostUsageScanner.h
    ../src/util/CostUsageCache.cpp
    ../src/util/CostUsageCache.h
)
target_include_directories(tst_CostUsageScanner PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_CostUsageScanner PRIVATE
    ${COMMON_DEFINES}
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
target_link_libraries(tst_CostUsageScanner PRIVATE Qt6::Core Qt6::Test Qt6::Sql)
add_wincodexbar_test(tst_CostUsageScanner tst_CostUsageScanner)

add_executable(tst_ProviderPipeline
    tst_ProviderPipeline.cpp
)
target_include_directories(tst_ProviderPipeline PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_ProviderPipeline PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_ProviderPipeline PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_ProviderPipeline tst_ProviderPipeline)

add_executable(tst_CodexProvider
    tst_CodexProvider.cpp
)
target_include_directories(tst_CodexProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_CodexProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_CodexProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_CodexProvider tst_CodexProvider)

add_executable(tst_CodexLoginRunner
    tst_CodexLoginRunner.cpp
)
target_include_directories(tst_CodexLoginRunner PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_CodexLoginRunner PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_CodexLoginRunner PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_CodexLoginRunner tst_CodexLoginRunner)

add_executable(tst_FetchContext
    tst_FetchContext.cpp
)
target_include_directories(tst_FetchContext PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_FetchContext PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_FetchContext PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_FetchContext tst_FetchContext)

add_executable(tst_CookieImporter
    tst_CookieImporter.cpp
)
target_include_directories(tst_CookieImporter PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_CookieImporter PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_CookieImporter PRIVATE codexbar_core Qt6::Core Qt6::Test Qt6::Sql Qt6::Network)
add_wincodexbar_test(tst_CookieImporter tst_CookieImporter)

add_executable(tst_ConPTYSession
    tst_ConPTYSession.cpp
)
target_include_directories(tst_ConPTYSession PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_ConPTYSession PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_ConPTYSession PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_ConPTYSession tst_ConPTYSession)

add_executable(tst_PerProviderCostUsage
    tst_PerProviderCostUsage.cpp
)
target_include_directories(tst_PerProviderCostUsage PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_PerProviderCostUsage PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_PerProviderCostUsage PRIVATE Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_PerProviderCostUsage tst_PerProviderCostUsage)

# Temporary debug test for OpenCode Go SQLite
add_executable(opencode_debug_test
    opencode_debug_test.cpp
)
target_include_directories(opencode_debug_test PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(opencode_debug_test PRIVATE ${COMMON_DEFINES})
target_link_libraries(opencode_debug_test PRIVATE Qt6::Core Qt6::Test Qt6::Sql)
add_wincodexbar_test(opencode_debug_test opencode_debug_test)

# QML smoke test (load, render, interaction)
add_executable(tst_QmlSmoke
    tst_QmlSmoke.cpp
)
target_include_directories(tst_QmlSmoke PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_QmlSmoke PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_QmlSmoke PRIVATE Qt6::Core Qt6::Test Qt6::Quick Qt6::Qml)
target_sources(tst_QmlSmoke PRIVATE ${CMAKE_SOURCE_DIR}/resources/qml.qrc)
add_wincodexbar_test(tst_QmlSmoke tst_QmlSmoke)

# Provider unit tests (API Key)
add_executable(tst_OpenCodeGoProvider tst_OpenCodeGoProvider.cpp)
target_include_directories(tst_OpenCodeGoProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_OpenCodeGoProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_OpenCodeGoProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_OpenCodeGoProvider tst_OpenCodeGoProvider)

add_executable(tst_DeepSeekProvider tst_DeepSeekProvider.cpp)
target_include_directories(tst_DeepSeekProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_DeepSeekProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_DeepSeekProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_DeepSeekProvider tst_DeepSeekProvider)

add_executable(tst_CodebuffProvider tst_CodebuffProvider.cpp)
target_include_directories(tst_CodebuffProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_CodebuffProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_CodebuffProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_CodebuffProvider tst_CodebuffProvider)

add_executable(tst_ProviderBootstrap tst_ProviderBootstrap.cpp)
target_include_directories(tst_ProviderBootstrap PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_ProviderBootstrap PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_ProviderBootstrap PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_ProviderBootstrap tst_ProviderBootstrap)

add_executable(tst_MiniMaxProvider tst_MiniMaxProvider.cpp)
target_include_directories(tst_MiniMaxProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_MiniMaxProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_MiniMaxProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_MiniMaxProvider tst_MiniMaxProvider)

add_executable(tst_SyntheticProvider tst_SyntheticProvider.cpp)
target_include_directories(tst_SyntheticProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_SyntheticProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_SyntheticProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_SyntheticProvider tst_SyntheticProvider)

# Provider unit tests (Web Cookie)
add_executable(tst_PerplexityProvider tst_PerplexityProvider.cpp)
target_include_directories(tst_PerplexityProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_PerplexityProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_PerplexityProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_PerplexityProvider tst_PerplexityProvider)

add_executable(tst_AmpProvider tst_AmpProvider.cpp)
target_include_directories(tst_AmpProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_AmpProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_AmpProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_AmpProvider tst_AmpProvider)

add_executable(tst_AugmentProvider tst_AugmentProvider.cpp)
target_include_directories(tst_AugmentProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_AugmentProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_AugmentProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_AugmentProvider tst_AugmentProvider)

# Provider unit tests (Complex)
add_executable(tst_GeminiProvider tst_GeminiProvider.cpp)
target_include_directories(tst_GeminiProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_GeminiProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_GeminiProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_GeminiProvider tst_GeminiProvider)

add_executable(tst_VertexAIProvider tst_VertexAIProvider.cpp)
target_include_directories(tst_VertexAIProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_VertexAIProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_VertexAIProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_VertexAIProvider tst_VertexAIProvider)

add_executable(tst_JetBrainsProvider tst_JetBrainsProvider.cpp)
target_include_directories(tst_JetBrainsProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_JetBrainsProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_JetBrainsProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_JetBrainsProvider tst_JetBrainsProvider)

add_executable(tst_FactoryProvider tst_FactoryProvider.cpp)
target_include_directories(tst_FactoryProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_FactoryProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_FactoryProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_FactoryProvider tst_FactoryProvider)

# Provider unit tests (P3 Backlog)
add_executable(tst_WarpProvider tst_WarpProvider.cpp)
target_include_directories(tst_WarpProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_WarpProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_WarpProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_WarpProvider tst_WarpProvider)

add_executable(tst_AbacusProvider tst_AbacusProvider.cpp)
target_include_directories(tst_AbacusProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_AbacusProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_AbacusProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_AbacusProvider tst_AbacusProvider)

add_executable(tst_WindsurfProvider tst_WindsurfProvider.cpp)
target_include_directories(tst_WindsurfProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_WindsurfProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_WindsurfProvider PRIVATE codexbar_core Qt6::Core Qt6::Test Qt6::Sql)
if(WIN32 AND QT_RUNTIME_DIR)
    add_test(NAME tst_WindsurfProvider
        COMMAND ${CMAKE_COMMAND} -E env
            "PATH=${QT_RUNTIME_DIR};$ENV{PATH}"
            "QT_PLUGIN_PATH=${QT_PLUGIN_DIR}"
            cmd.exe /c "$<SHELL_PATH:$<TARGET_FILE:tst_WindsurfProvider>> -o tst_WindsurfProvider.log,txt")
else()
    add_wincodexbar_test(tst_WindsurfProvider tst_WindsurfProvider)
endif()

# CostUsageCache unit tests
add_executable(tst_CostUsageCache tst_CostUsageCache.cpp)
target_include_directories(tst_CostUsageCache PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_CostUsageCache PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_CostUsageCache PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_CostUsageCache tst_CostUsageCache)

# TokenAccountStore unit tests
add_executable(tst_TokenAccountStore tst_TokenAccountStore.cpp)
target_include_directories(tst_TokenAccountStore PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_TokenAccountStore PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_TokenAccountStore PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_TokenAccountStore tst_TokenAccountStore)

# ProviderRuntime unit tests
add_executable(tst_ProviderRuntime tst_ProviderRuntime.cpp)
target_include_directories(tst_ProviderRuntime PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_ProviderRuntime PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_ProviderRuntime PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_ProviderRuntime tst_ProviderRuntime)

# CLI unit tests
add_executable(tst_CLI tst_CLI.cpp)
target_include_directories(tst_CLI PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_CLI PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_CLI PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_CLI tst_CLI)

# KimiProvider unit tests
add_executable(tst_KimiProvider tst_KimiProvider.cpp)
target_include_directories(tst_KimiProvider PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_KimiProvider PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_KimiProvider PRIVATE codexbar_core Qt6::Core Qt6::Test)
add_wincodexbar_test(tst_KimiProvider tst_KimiProvider)
