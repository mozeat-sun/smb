################################################################################
# Copyright (C) 2025, ZOO Ltd
#
# Product: ZOO
# Module: Root
# Component ID: ZOO_ROOT
# File name: CMakeLists.txt
# Description: Top-level CMake build configuration for ZOO Platform Library
#
# Change History:
# Version   Date           Author          Description
# -------   ----------     -----------     ---------------------------------
# 1.0       2025-12-18     assistant       Initial creation
################################################################################

cmake_minimum_required(VERSION 3.16)

# ==============================================================================
# PROJECT CONFIGURATION
# ==============================================================================

# Main project definition
project(ZOO
    VERSION 1.0.0
    DESCRIPTION "ZOO Cross-Platform Library Collection"
    LANGUAGES C CXX
)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()

# Enable testing globally
enable_testing()

# ==============================================================================
# GLOBAL BUILD OPTIONS
# ==============================================================================

# Option to build all modules
option(ZOO_BUILD_ALL "Build all ZOO modules" ON)

# Individual module options
option(ZOO_BUILD_BUFFER "Build buffer module" ${ZOO_BUILD_ALL})
option(ZOO_BUILD_DISPATCHER "Build dispatcher module" ${ZOO_BUILD_ALL})
option(ZOO_BUILD_LOG "Build log module" ${ZOO_BUILD_ALL})
option(ZOO_BUILD_MEMORY_POOL "Build memory pool module" ${ZOO_BUILD_ALL})
option(ZOO_BUILD_PLATFORM "Build platform module" ${ZOO_BUILD_ALL})
option(ZOO_BUILD_SMB "Build SMB module" ${ZOO_BUILD_ALL})
option(ZOO_BUILD_SOCKET "Build socket module" ${ZOO_BUILD_ALL})
option(ZOO_BUILD_THREAD_POOL "Build thread pool module" ${ZOO_BUILD_ALL})
option(ZOO_BUILD_TIMER "Build timer module" ${ZOO_BUILD_ALL})
option(ZOO_BUILD_UTIL "Build util module" ${ZOO_BUILD_ALL})

# Option to build examples and tests
option(ZOO_BUILD_EXAMPLES "Build example programs" OFF)
option(ZOO_BUILD_TESTS "Build unit tests" ON)

# ==============================================================================
# GLOBAL COMPILER FLAGS AND DEFINITIONS
# ==============================================================================

# Common compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Only treat warnings as errors in Release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-Werror)
    endif()
endif()

# Global include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Unified output directories at repository root
set(ZOO_BIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")
set(ZOO_LIB_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ZOO_BIN_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ZOO_LIB_OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ZOO_LIB_OUTPUT_DIR}")

# Keep multi-config generators consistent (e.g. Ninja Multi-Config, Visual Studio)
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${cfg}" cfg_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg_upper} "${ZOO_BIN_OUTPUT_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg_upper} "${ZOO_LIB_OUTPUT_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfg_upper} "${ZOO_LIB_OUTPUT_DIR}")
endforeach()

# ==============================================================================
# CMAKE MODULE PATH
# ==============================================================================

# Add cmake directory to module path for custom finders
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ==============================================================================
# DEPENDENCY DETECTION
# ==============================================================================

# Find threading support
find_package(Threads QUIET)

# Find Unity testing framework
if(ZOO_BUILD_TESTS)
    find_package(Unity QUIET)
    if(NOT Unity_FOUND)
        message(WARNING "Unity testing framework not found. Tests will be disabled.")
        set(ZOO_BUILD_TESTS OFF)
    endif()
endif()

# ==============================================================================
# SUBDIRECTORY CONFIGURATION
# ==============================================================================

# Build platform module first (dependencies)
if(ZOO_BUILD_PLATFORM)
    add_subdirectory(src/platform)
endif()

# Build core modules (dependency-aware order)
if(ZOO_BUILD_UTIL)
    add_subdirectory(src/util)
endif()

if(ZOO_BUILD_LOG)
    add_subdirectory(src/log)
endif()

if(ZOO_BUILD_MEMORY_POOL)
    add_subdirectory(src/memory_pool)
endif()

if(ZOO_BUILD_BUFFER)
    add_subdirectory(src/buffer)
endif()

if(ZOO_BUILD_SOCKET)
    add_subdirectory(src/socket)
endif()

if(ZOO_BUILD_THREAD_POOL)
    add_subdirectory(src/thread_pool)
endif()

if(ZOO_BUILD_DISPATCHER)
    add_subdirectory(src/dispatcher)
endif()

if(ZOO_BUILD_TIMER)
    add_subdirectory(src/timer)
endif()

if(ZOO_BUILD_SMB)
    add_subdirectory(src/smb)
    if(ZOO_BUILD_TESTS)
        add_subdirectory(tests/smb ${CMAKE_CURRENT_BINARY_DIR}/smb/tests)
    endif()
    if(ZOO_BUILD_EXAMPLES)
        add_subdirectory(examples/smb ${CMAKE_CURRENT_BINARY_DIR}/smb/examples)
    endif()
endif()

# ==============================================================================
# INSTALLATION CONFIGURATION
# ==============================================================================

# Install headers
install(DIRECTORY include/
    DESTINATION include/zoo
    FILES_MATCHING PATTERN "*.h"
)

# ==============================================================================
# PACKAGE CONFIGURATION
# ==============================================================================

# Create package config files for find_package support
include(CMakePackageConfigHelpers)

# Generate version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ZooConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Generate config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZooConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ZooConfig.cmake"
    INSTALL_DESTINATION lib/cmake/Zoo
    PATH_VARS
        CMAKE_INSTALL_PREFIX
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ZooConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ZooConfigVersion.cmake"
    DESTINATION lib/cmake/Zoo
)

# ==============================================================================
# SUMMARY
# ==============================================================================

message(STATUS "ZOO Library Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  CXX Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests: ${ZOO_BUILD_TESTS}")
message(STATUS "  Build Examples: ${ZOO_BUILD_EXAMPLES}")
message(STATUS "")
message(STATUS "Modules to build:")
if(ZOO_BUILD_PLATFORM)
    message(STATUS "  - Platform")
endif()
if(ZOO_BUILD_BUFFER)
    message(STATUS "  - Buffer")
endif()
if(ZOO_BUILD_DISPATCHER)
    message(STATUS "  - Dispatcher")
endif()
if(ZOO_BUILD_LOG)
    message(STATUS "  - Log")
endif()
if(ZOO_BUILD_MEMORY_POOL)
    message(STATUS "  - Memory Pool")
endif()
if(ZOO_BUILD_SMB)
    message(STATUS "  - SMB")
endif()
if(ZOO_BUILD_SOCKET)
    message(STATUS "  - Socket")
endif()
if(ZOO_BUILD_THREAD_POOL)
    message(STATUS "  - Thread Pool")
endif()
if(ZOO_BUILD_TIMER)
    message(STATUS "  - Timer")
endif()
if(ZOO_BUILD_UTIL)
    message(STATUS "  - Util")
endif()