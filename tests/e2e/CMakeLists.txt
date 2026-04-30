cmake_minimum_required(VERSION 3.21)

find_package(Qt6 6.5 COMPONENTS Core Test Network QUIET)
if(NOT Qt6_FOUND)
    return()
endif()

set(COMMON_INCLUDE "${CMAKE_SOURCE_DIR}/src")
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

function(add_e2e_test TEST_NAME TARGET_NAME)
    if(WIN32 AND QT_RUNTIME_DIR)
        add_test(NAME ${TEST_NAME}
            COMMAND ${CMAKE_COMMAND} -E env
                "PATH=${QT_RUNTIME_DIR}"
                "QT_PLUGIN_PATH=${QT_PLUGIN_DIR}"
                $<TARGET_FILE:${TARGET_NAME}>)
    else()
        add_test(NAME ${TEST_NAME} COMMAND ${TARGET_NAME})
    endif()
    set_tests_properties(${TEST_NAME} PROPERTIES LABELS "e2e")
endfunction()

set(E2E_UTILS
    E2ETestUtils.cpp
    E2ETestUtils.h
)

add_executable(tst_E2E_Codex
    tst_E2E_Codex.cpp
    ${E2E_UTILS}
)
target_include_directories(tst_E2E_Codex PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_E2E_Codex PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_E2E_Codex PRIVATE codexbar_core Qt6::Core Qt6::Test Qt6::Network)
add_e2e_test(tst_E2E_Codex tst_E2E_Codex)

add_executable(tst_E2E_Copilot
    tst_E2E_Copilot.cpp
    ${E2E_UTILS}
)
target_include_directories(tst_E2E_Copilot PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_E2E_Copilot PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_E2E_Copilot PRIVATE codexbar_core Qt6::Core Qt6::Test Qt6::Network)
add_e2e_test(tst_E2E_Copilot tst_E2E_Copilot)

add_executable(tst_E2E_Kimi
    tst_E2E_Kimi.cpp
    ${E2E_UTILS}
)
target_include_directories(tst_E2E_Kimi PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_E2E_Kimi PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_E2E_Kimi PRIVATE codexbar_core Qt6::Core Qt6::Test Qt6::Network)
add_e2e_test(tst_E2E_Kimi tst_E2E_Kimi)

add_executable(tst_E2E_OpenCode
    tst_E2E_OpenCode.cpp
    ${E2E_UTILS}
)
target_include_directories(tst_E2E_OpenCode PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_E2E_OpenCode PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_E2E_OpenCode PRIVATE codexbar_core Qt6::Core Qt6::Test Qt6::Network)
add_e2e_test(tst_E2E_OpenCode tst_E2E_OpenCode)

add_executable(tst_E2E_OpenCodeGo
    tst_E2E_OpenCodeGo.cpp
    ${E2E_UTILS}
)
target_include_directories(tst_E2E_OpenCodeGo PRIVATE ${COMMON_INCLUDE})
target_compile_definitions(tst_E2E_OpenCodeGo PRIVATE ${COMMON_DEFINES})
target_link_libraries(tst_E2E_OpenCodeGo PRIVATE codexbar_core Qt6::Core Qt6::Test Qt6::Network)
add_e2e_test(tst_E2E_OpenCodeGo tst_E2E_OpenCodeGo)
