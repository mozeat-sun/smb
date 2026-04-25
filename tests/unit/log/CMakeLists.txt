cmake_minimum_required(VERSION 3.14)
project(ZooLogTests LANGUAGES C)

get_filename_component(ZOO_LOG_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(ZOO_REPO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

message(STATUS "=== DEBUG: Configuring ZooLogTests ===")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

# ----------------------
# Dependencies  
# ----------------------
find_package(Threads REQUIRED)

# Find Unity framework
set(UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/thirdparty/unity")
if(NOT EXISTS "${UNITY_ROOT}")
    set(UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/thirdparty/test/unity")
endif()
message(STATUS "Looking for Unity at: ${UNITY_ROOT}")
if(EXISTS "${UNITY_ROOT}")
    message(STATUS "Found Unity: ${UNITY_ROOT}")
else()
    message(FATAL_ERROR "Unity test framework not found under thirdparty/unity or thirdparty/test/unity")
endif()

# ----------------------
# Test Configuration
# ----------------------

# Include directories
set(TEST_INCLUDE_DIRS
    ${ZOO_REPO_ROOT_DIR}/include/platform/log
    ${ZOO_REPO_ROOT_DIR}/include/platform
    ${ZOO_REPO_ROOT_DIR}/include/platform/util
    ${UNITY_ROOT}
    ${UNITY_ROOT}/src
)

# Test sources
set(TEST_SOURCES
    test_zoo_log.c
    test_runner.c
    ${UNITY_ROOT}/src/unity.c
)

# ----------------------
# Test Executable
# ----------------------
add_executable(zoo_log_tests ${TEST_SOURCES})

target_include_directories(zoo_log_tests PRIVATE ${TEST_INCLUDE_DIRS})

# Link against the zoo_log library
target_link_libraries(zoo_log_tests 
    PRIVATE 
    zoo_log_static
    Threads::Threads
)

# Compiler flags for tests
target_compile_options(zoo_log_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O2>
)

# Test definitions
target_compile_definitions(zoo_log_tests PRIVATE
    UNITY_INCLUDE_CONFIG_H=0
    UNITY_EXCLUDE_SETJMP_H=1
    $<$<PLATFORM_ID:Linux>:ZOO_PLATFORM_LINUX=1>
    $<$<PLATFORM_ID:Windows>:ZOO_PLATFORM_WINDOWS=1>
    $<$<PLATFORM_ID:Darwin>:ZOO_PLATFORM_MACOS=1>
)

# ----------------------
# CTest Integration
# ----------------------
add_test(
    NAME ZOO_Log_Unit_Tests
    COMMAND zoo_log_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Set test properties
set_tests_properties(ZOO_Log_Unit_Tests PROPERTIES
    LABELS "log;core;unit"
    TIMEOUT 60
)

# ----------------------
# Custom Test Targets
# ----------------------

# Quick test target (runs tests with minimal output)
add_custom_target(log_quick_test
    COMMAND zoo_log_tests
    DEPENDS zoo_log_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running ZOO Log quick tests"
)

# Verbose test target (runs tests with verbose output)
add_custom_target(log_test_verbose
    COMMAND zoo_log_tests -v
    DEPENDS zoo_log_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running ZOO Log tests with verbose output"
)

# ----------------------
# Install Test Executable
# ----------------------
install(TARGETS zoo_log_tests
    RUNTIME DESTINATION bin/tests
    COMPONENT Tests
)

# ----------------------
# Summary Information
# ----------------------
message(STATUS "")
message(STATUS "==============================================================================")
message(STATUS "ZOO Log Tests Configuration Summary")
message(STATUS "==============================================================================")
message(STATUS "Test Framework: Unity C Testing Framework")
message(STATUS "Unity Location: ${UNITY_ROOT}")
message(STATUS "Test Executable: zoo_log_tests")
message(STATUS "Test Categories: Unit, Thread Safety, Performance")
message(STATUS "Custom Targets: log_quick_test, log_test_verbose")
message(STATUS "Coverage: Disabled")
message(STATUS "Run Tests: ./zoo_log_tests or ctest")
message(STATUS "Run Quick Tests: make log_quick_test")
message(STATUS "==============================================================================")
message(STATUS "")
