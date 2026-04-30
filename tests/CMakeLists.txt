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
                "PATH=${QT_RUNTIME_DIR}"
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
)
target_include_directories(tst_CostUsageScanner PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_CostUsageScanner PRIVATE
    ${COMMON_DEFINES}
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
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
