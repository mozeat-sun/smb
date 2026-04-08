################################################################################
# Copyright (C) 2025, ZOO Ltd
#
# Product: ZOO
# Module: Socket
# Component ID: ZOO_SOCKET
# File name: CMakeLists.txt
# Description: CMake build configuration for ZOO Socket Library
#
# Change History:
# Version   Date           Author          Description
# -------   ----------     -----------     ---------------------------------
# 1.0       2025-08-05     assistant       Initial creation
################################################################################

cmake_minimum_required(VERSION 3.16)

# Project definition
project(zoo_socket
    VERSION 1.0.0
    DESCRIPTION "Cross-platform socket abstraction library"
    LANGUAGES C
)

# ==============================================================================
# BUILD CONFIGURATION
# ==============================================================================

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()

# Platform detection and configuration
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)

# ==============================================================================
# PLATFORM DETECTION
# ==============================================================================

# Check for POSIX socket support
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_function_exists("socket" HAVE_SOCKET_FUNC)
check_function_exists("select" HAVE_SELECT_FUNC)

# Check for lwIP support
check_include_file("lwip/sockets.h" HAVE_LWIP_SOCKETS_H)

# Determine platform capabilities
if(HAVE_SYS_SOCKET_H AND HAVE_SOCKET_FUNC)
    set(ZOO_HAS_POSIX_SOCKETS ON)
else()
    set(ZOO_HAS_POSIX_SOCKETS OFF)
endif()

if(HAVE_LWIP_SOCKETS_H)
    set(ZOO_HAS_LWIP ON)
else()
    set(ZOO_HAS_LWIP OFF)
endif()

# ==============================================================================
# LIBRARY DEPENDENCIES
# ==============================================================================

# Find required ZOO framework components
set(ZOO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "ZOO framework root directory" FORCE)

# Platform headers
set(ZOO_PLATFORM_INCLUDE "${ZOO_ROOT}/include/platform")

# Utility headers  
set(ZOO_UTIL_INCLUDE "${ZOO_ROOT}/include/util")

# Check if platform headers exist
if(NOT EXISTS "${ZOO_PLATFORM_INCLUDE}/zoo_platform.h")
    message(FATAL_ERROR "Required ZOO platform headers not found at ${ZOO_PLATFORM_INCLUDE}")
endif()

# ==============================================================================
# LIBRARY SOURCES
# ==============================================================================

# Socket library headers
set(ZOO_SOCKET_HEADERS
    ${ZOO_ROOT}/include/socket/zoo_socket.h
    ${ZOO_ROOT}/include/socket/zoo_select.h
)

# Socket library sources
set(ZOO_SOCKET_SOURCES
    ${ZOO_ROOT}/src/socket/zoo_socket.c
    ${ZOO_ROOT}/src/socket/zoo_select.c
)

# ==============================================================================
# COMPILER FLAGS
# ==============================================================================

# Common compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

# Debug flags
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3 -DDEBUG")

# Release flags
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")

# Platform-specific flags
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
endif()

if(UNIX AND NOT APPLE)
    # Linux-specific flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
endif()

# ==============================================================================
# LIBRARY TARGET
# ==============================================================================

# Create static library
add_library(zoo_socket_static STATIC ${ZOO_SOCKET_SOURCES})
set_target_properties(zoo_socket_static PROPERTIES
    OUTPUT_NAME "zoo_socket"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Create shared library
add_library(zoo_socket_shared SHARED ${ZOO_SOCKET_SOURCES})
set_target_properties(zoo_socket_shared PROPERTIES
    OUTPUT_NAME "zoo_socket"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Create interface library for header-only usage
add_library(zoo_socket INTERFACE)

# ==============================================================================
# INCLUDE DIRECTORIES
# ==============================================================================

# Set include directories for static and shared libraries
foreach(target zoo_socket_static zoo_socket_shared)
    target_include_directories(${target} 
        PUBLIC
            $<BUILD_INTERFACE:${ZOO_ROOT}/include/socket>
            $<BUILD_INTERFACE:${ZOO_PLATFORM_INCLUDE}>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${ZOO_UTIL_INCLUDE}
    )
    target_compile_options(${target} PRIVATE -Wno-unused-value)
endforeach()

# Set include directories for interface library
target_include_directories(zoo_socket
    INTERFACE
        $<BUILD_INTERFACE:${ZOO_ROOT}/include/socket>
        $<BUILD_INTERFACE:${ZOO_PLATFORM_INCLUDE}>
        $<INSTALL_INTERFACE:include>
)

# ==============================================================================
# COMPILE DEFINITIONS
# ==============================================================================

# Platform-specific definitions for static and shared libraries
if(ZOO_HAS_POSIX_SOCKETS)
    foreach(target zoo_socket_static zoo_socket_shared)
        target_compile_definitions(${target} PUBLIC ZOO_HAS_POSIX=1)
    endforeach()
    target_compile_definitions(zoo_socket INTERFACE ZOO_HAS_POSIX=1)
endif()

if(ZOO_HAS_LWIP)
    foreach(target zoo_socket_static zoo_socket_shared)
        target_compile_definitions(${target} PUBLIC ZOO_USE_LWIP=1)
    endforeach()
    target_compile_definitions(zoo_socket INTERFACE ZOO_USE_LWIP=1)
endif()

# ==============================================================================
# LINK LIBRARIES
# ==============================================================================

# Link system libraries for POSIX platforms
if(ZOO_HAS_POSIX_SOCKETS)
    if(UNIX)
        foreach(target zoo_socket_static zoo_socket_shared)
            target_link_libraries(${target} PUBLIC pthread)
        endforeach()
    endif()
    
    if(WIN32)
        foreach(target zoo_socket_static zoo_socket_shared)
            target_link_libraries(${target} PUBLIC ws2_32 wsock32)
        endforeach()
    endif()
endif()

# ==============================================================================
# TESTING SUPPORT
# ==============================================================================

# Option to build tests
option(ZOO_SOCKET_BUILD_TESTS "Build ZOO Socket tests" ON)

if(ZOO_SOCKET_BUILD_TESTS)
    enable_testing()
    if(EXISTS "${ZOO_ROOT}/tests/unit/socket/CMakeLists.txt")
        add_subdirectory(${ZOO_ROOT}/tests/unit/socket ${CMAKE_CURRENT_BINARY_DIR}/tests)
    endif()
endif()

# ==============================================================================
# EXAMPLES
# ==============================================================================

# Option to build examples
option(ZOO_SOCKET_BUILD_EXAMPLES "Build ZOO Socket examples" ON)

if(ZOO_SOCKET_BUILD_EXAMPLES)
    if(EXISTS "${ZOO_ROOT}/examples/socket/CMakeLists.txt")
        add_subdirectory(${ZOO_ROOT}/examples/socket ${CMAKE_CURRENT_BINARY_DIR}/examples)
    endif()
endif()

# ==============================================================================
# INSTALLATION
# ==============================================================================

# Install library targets
install(TARGETS zoo_socket_static zoo_socket_shared zoo_socket
    EXPORT ZooSocketTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(FILES ${ZOO_SOCKET_HEADERS}
    DESTINATION include/zoo
)

# Install export targets
install(EXPORT ZooSocketTargets
    FILE ZooSocketTargets.cmake
    NAMESPACE Zoo::
    DESTINATION lib/cmake/ZooSocket
)

install(EXPORT ZooSocketTargets
    FILE ZooSocketTargets.cmake
    NAMESPACE Zoo::
    DESTINATION lib/cmake/zoo_socket
)

# Generate and install config files
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZooSocketConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ZooSocketConfig.cmake"
    INSTALL_DESTINATION lib/cmake/ZooSocket
    PATH_VARS CMAKE_INSTALL_PREFIX
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ZooSocketConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ZooSocketConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ZooSocketConfigVersion.cmake"
    DESTINATION lib/cmake/ZooSocket
)

# Compatibility package path for previous lowercase package naming
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ZooSocketConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ZooSocketConfigVersion.cmake"
    DESTINATION lib/cmake/zoo_socket
)

# ==============================================================================
# PACKAGE CONFIGURATION
# ==============================================================================

# Set package properties (standalone module build only)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CPACK_PACKAGE_NAME "zoo_socket")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform socket abstraction library")
    set(CPACK_PACKAGE_VENDOR "ZOO Ltd")
    set(CPACK_PACKAGE_CONTACT "support@zoo.com")

    # Include CPack
    include(CPack)
endif()

# ==============================================================================
# STATUS SUMMARY
# ==============================================================================

message(STATUS "")
message(STATUS "ZOO Socket Library Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Platform Support:")
message(STATUS "  POSIX sockets: ${ZOO_HAS_POSIX_SOCKETS}")
message(STATUS "  lwIP support: ${ZOO_HAS_LWIP}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  Build tests: ${ZOO_SOCKET_BUILD_TESTS}")
message(STATUS "  Build examples: ${ZOO_SOCKET_BUILD_EXAMPLES}")
message(STATUS "")
