################################################################################
# Copyright (C) 2025, ZOO Ltd
#
# Product: ZOO
# Module: Socket Tests
# Component ID: ZOO_SOCKET_TESTS
# File name: CMakeLists.txt
# Description: CMake build configuration for ZOO Socket tests
#
# Change History:
# Version   Date           Author          Description
# -------   ----------     -----------     ---------------------------------
# 1.0       2025-08-05     assistant       Initial creation
################################################################################

cmake_minimum_required(VERSION 3.16)

get_filename_component(ZOO_SOCKET_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(ZOO_REPO_ROOT_DIR "${ZOO_SOCKET_ROOT_DIR}/.." ABSOLUTE)
set(ZOO_VENDORED_UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/third_party/test/unity")

# ==============================================================================
# UNITY TEST FRAMEWORK SETUP
# ==============================================================================

# Try to find Unity test framework
find_path(UNITY_INCLUDE_DIR
    NAMES unity.h
    PATHS
        ${ZOO_VENDORED_UNITY_ROOT}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/unity
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/unity
        /usr/local/include
        /usr/include
)

# Use vendored Unity source from the repository when available.
if(EXISTS "${ZOO_VENDORED_UNITY_ROOT}/src/unity.c")
    add_library(zoo_socket_unity STATIC
        ${ZOO_VENDORED_UNITY_ROOT}/src/unity.c
    )
    target_include_directories(zoo_socket_unity PUBLIC
        ${ZOO_VENDORED_UNITY_ROOT}/src
    )
    set(UNITY_INCLUDE_DIR ${ZOO_VENDORED_UNITY_ROOT}/src)
    set(UNITY_LIBRARY zoo_socket_unity)
elseif(NOT UNITY_INCLUDE_DIR)
    message(FATAL_ERROR "Unity test framework not found. Expected vendored Unity at ${ZOO_VENDORED_UNITY_ROOT}")
endif()

# ==============================================================================
# TEST CONFIGURATION
# ==============================================================================

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZOO_SOCKET_ROOT_DIR}/inc
    ${UNITY_INCLUDE_DIR}
    ${ZOO_PLATFORM_INCLUDE}
    ${ZOO_UTIL_INCLUDE}
)

# Compiler flags for tests
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUNITY_INCLUDE_DOUBLE -DUNITY_SUPPORT_64")

# ==============================================================================
# TEST SOURCES
# ==============================================================================

# Test runner
set(TEST_RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/test_runner.c)

# Individual test files
set(TEST_SOURCES
    test_zoo_socket_init.c
    test_zoo_socket_create.c
    test_zoo_socket_address.c
    test_zoo_socket_connect.c
    test_zoo_socket_bind_listen.c
    test_zoo_socket_send_recv.c
    test_zoo_socket_options.c
    test_zoo_socket_select.c
    test_zoo_socket_error.c
)

# ==============================================================================
# TEST TARGETS
# ==============================================================================

# Create test executable
add_executable(zoo_socket_tests
    ${TEST_RUNNER}
    ${TEST_SOURCES}
)

# Link libraries
target_link_libraries(zoo_socket_tests
    zoo_socket_static
    ${UNITY_LIBRARY}
)

# Platform-specific libraries
if(UNIX)
    target_link_libraries(zoo_socket_tests pthread)
endif()

if(WIN32)
    target_link_libraries(zoo_socket_tests ws2_32 wsock32)
endif()

# ==============================================================================
# TEST REGISTRATION
# ==============================================================================

# Add test to CTest
add_test(NAME zoo_socket_unit_tests COMMAND zoo_socket_tests)

# Set test properties
set_tests_properties(zoo_socket_unit_tests PROPERTIES
    TIMEOUT 60
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ==============================================================================
# CUSTOM TEST TARGETS
# ==============================================================================

# Custom target to run tests with verbose output
add_custom_target(socket_test_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS zoo_socket_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Custom target to run tests with coverage (if available)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(GCOV_EXECUTABLE gcov)
    if(GCOV_EXECUTABLE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
        target_link_libraries(zoo_socket_tests gcov)
        
        add_custom_target(socket_coverage
            COMMAND ${CMAKE_CTEST_COMMAND}
            COMMAND gcov -r ${CMAKE_CURRENT_BINARY_DIR}/*.gcno
            DEPENDS zoo_socket_tests
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif()
endif()
