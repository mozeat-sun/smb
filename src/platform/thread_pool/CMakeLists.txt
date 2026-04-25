#******************************************************************************
# Copyright (C) 2025, Basic Software Research Institute ltd
# All rights reserved.
# Product: ZOO
# Module: thread_pool
# Component id: ZOO_THREAD_POOL
# File name: CMakeLists.txt
# Description: CMake configuration for ZOO thread_pool module
# History recorder:
# Version   date           author            context
# 1.0       2025-08-04     AI Assistant      created
#******************************************************************************

cmake_minimum_required(VERSION 3.14)

# Project configuration
project(ZooThreadPool
    VERSION 1.0.0
    DESCRIPTION "ZOO ThreadPool Module"
    LANGUAGES C
)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ==============================================================================
# ENHANCED PLATFORM DETECTION AND CONFIGURATION
# Based on zoo_platform.h definitions
# ==============================================================================

# Initialize platform variables
set(PLATFORM_LIBS "")
set(PLATFORM_DEFINITIONS "")
get_filename_component(ZOO_THREAD_POOL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
get_filename_component(ZOO_REPO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

# Operating System Detection
if(UNIX)
    message(STATUS "Configuring for Linux/Unix platform")
    list(APPEND PLATFORM_DEFINITIONS -D__ZOO_OS_LINUX -DZOO_OS_LINUX)
    
    # GNU source feature detection for Linux
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L")
    list(APPEND PLATFORM_LIBS pthread rt dl)
    
    # Check for additional Linux libraries
    find_library(LIBRT_FOUND rt)
    if(NOT LIBRT_FOUND)
        message(WARNING "librt not found, real-time features may be limited")
    endif()
    
else()
    message(STATUS "Configuring for unknown/embedded platform")
    list(APPEND PLATFORM_DEFINITIONS -D__ZOO_OS_UNKNOWN -DZOO_OS_UNKNOWN)
endif()

# Architecture Detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    message(STATUS "Detected x86_64 architecture")
    list(APPEND PLATFORM_DEFINITIONS -D__ZOO_PLATFORM_X86_64)
    
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i386|i686|x86)$")
    message(STATUS "Detected x86_32 architecture")  
    list(APPEND PLATFORM_DEFINITIONS -D__ZOO_PLATFORM_X86_32)
    
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    message(STATUS "Detected ARM Cortex-A64 architecture")
    list(APPEND PLATFORM_DEFINITIONS -D__ZOO_PLATFORM_ARM_CORTEX_A64)
    
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv7|armv7l)$")
    message(STATUS "Detected ARM Cortex-A32 architecture")
    list(APPEND PLATFORM_DEFINITIONS -D__ZOO_PLATFORM_ARM_CORTEX_A32)
    
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(armv[6-8]m|cortex-m)$")
    message(STATUS "Detected ARM Cortex-M architecture")
    list(APPEND PLATFORM_DEFINITIONS -D__ZOO_PLATFORM_ARM_CORTEX_M)
    
    # Embedded/RTOS platform detection
    if(DEFINED FREERTOS_DIR OR DEFINED CONFIG_FREERTOS)
        message(STATUS "FreeRTOS detected")
        list(APPEND PLATFORM_DEFINITIONS -DZOO_OS_FREERTOS -DFREERTOS)
        # Remove POSIX threading for FreeRTOS
        list(REMOVE_ITEM PLATFORM_LIBS pthread)
    elseif(DEFINED CMSIS_DIR OR DEFINED CMSIS_OS_H_)
        message(STATUS "CMSIS-RTOS detected")
        list(APPEND PLATFORM_DEFINITIONS -DZOO_OS_CMSIS_RTOS -DCMSIS_OS_H_)
        list(REMOVE_ITEM PLATFORM_LIBS pthread)
    elseif(DEFINED RTTHREAD_DIR OR DEFINED RT_THREAD_VERSION)
        message(STATUS "RT-Thread detected")
        list(APPEND PLATFORM_DEFINITIONS -DZOO_OS_RTTHREAD -DRT_THREAD_VERSION)
        list(REMOVE_ITEM PLATFORM_LIBS pthread)
    else()
        message(STATUS "Bare metal ARM Cortex-M platform")
        list(APPEND PLATFORM_DEFINITIONS -DZOO_OS_BAREMETAL)
        list(REMOVE_ITEM PLATFORM_LIBS pthread rt dl)
    endif()
    
else()
    message(WARNING "Unknown processor architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    list(APPEND PLATFORM_DEFINITIONS -D__ZOO_PLATFORM_UNKNOWN)
endif()

# Apply platform definitions
add_definitions(${PLATFORM_DEFINITIONS})

# ==============================================================================
# ENHANCED COMPILER CONFIGURATION
# ==============================================================================

# Enhanced compiler-specific options
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Base warning flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat=2 -Wstrict-prototypes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-prototypes -Wundef -Wshadow")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith -Wcast-align -Wwrite-strings")
    
    # Linux-specific optimizations
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
        if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "4.9")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-clash-protection")
        endif()
    
    # ARM-specific flags
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "cortex-m")
            # Cortex-M specific flags
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb -mcpu=cortex-m4")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=hard -mfpu=fpv4-sp-d16")
        endif()
    endif()
    
    # Build type specific flags
    set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -DDEBUG -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -flto")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
    
elseif(CMAKE_C_COMPILER_ID STREQUAL "IAR")
    # IAR compiler for embedded systems
    message(STATUS "Configuring for IAR compiler")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --warn_about_c_style_casts")
    set(CMAKE_C_FLAGS_DEBUG "--debug -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
    
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMCC")
    # ARM Compiler (Keil)
    message(STATUS "Configuring for ARM Compiler")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --c99 --gnu")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Threading configuration
if("pthread" IN_LIST PLATFORM_LIBS)
    find_package(Threads REQUIRED)
    if(NOT Threads_FOUND)
        message(FATAL_ERROR "Pthread support required but not found")
    endif()
else()
    message(STATUS "Using platform-specific threading (no pthread)")
endif()

# ==============================================================================
# FEATURE DETECTION AND CONFIGURATION
# ==============================================================================

# Platform feature availability flags (matching zoo_platform.h)
set(ZOO_FEATURE_DEFINITIONS "")

# POSIX feature detection
if(UNIX AND NOT ("ZOO_OS_BAREMETAL" IN_LIST PLATFORM_DEFINITIONS OR 
                 "ZOO_OS_FREERTOS" IN_LIST PLATFORM_DEFINITIONS OR
                 "ZOO_OS_CMSIS_RTOS" IN_LIST PLATFORM_DEFINITIONS OR
                 "ZOO_OS_RTTHREAD" IN_LIST PLATFORM_DEFINITIONS))
    # Only define if not already defined in header
    list(APPEND ZOO_FEATURE_DEFINITIONS -DZOO_HAS_POSIX=1)
    message(STATUS "Feature: POSIX support enabled")
else()
    list(APPEND ZOO_FEATURE_DEFINITIONS -DZOO_HAS_POSIX=0)
endif()

# Threading feature detection
if("pthread" IN_LIST PLATFORM_LIBS OR 
   "ZOO_OS_FREERTOS" IN_LIST PLATFORM_DEFINITIONS OR
   "ZOO_OS_CMSIS_RTOS" IN_LIST PLATFORM_DEFINITIONS OR
   "ZOO_OS_RTTHREAD" IN_LIST PLATFORM_DEFINITIONS OR
   "__ZOO_OS_WINDOWS" IN_LIST PLATFORM_DEFINITIONS)
    list(APPEND ZOO_FEATURE_DEFINITIONS -DZOO_HAS_THREADING=1)
    message(STATUS "Feature: Threading support enabled")
else()
    list(APPEND ZOO_FEATURE_DEFINITIONS -DZOO_HAS_THREADING=0)
endif()

# Filesystem feature detection (conditional to avoid redefinition)
if("__ZOO_OS_LINUX" IN_LIST PLATFORM_DEFINITIONS OR 
   "__ZOO_OS_WINDOWS" IN_LIST PLATFORM_DEFINITIONS)
    # Note: ZOO_HAS_FILESYSTEM will be defined in zoo_platform.h, avoid redefinition
    message(STATUS "Feature: Filesystem support enabled")
else()
    # Note: ZOO_HAS_FILESYSTEM will be defined in zoo_platform.h, avoid redefinition  
    message(STATUS "Feature: Filesystem support disabled")
endif()

# Syslog feature detection (conditional to avoid redefinition)
if("__ZOO_OS_LINUX" IN_LIST PLATFORM_DEFINITIONS)
    # Note: ZOO_HAS_SYSLOG will be defined in zoo_platform.h, avoid redefinition
    message(STATUS "Feature: Syslog support enabled")
else()
    # Note: ZOO_HAS_SYSLOG will be defined in zoo_platform.h, avoid redefinition
    message(STATUS "Feature: Syslog support disabled")
endif()

# Embedded system detection
if("-D__ZOO_PLATFORM_ARM_CORTEX_M" IN_LIST PLATFORM_DEFINITIONS AND
   ("ZOO_OS_BAREMETAL" IN_LIST PLATFORM_DEFINITIONS OR
    "ZOO_OS_FREERTOS" IN_LIST PLATFORM_DEFINITIONS OR
    "ZOO_OS_CMSIS_RTOS" IN_LIST PLATFORM_DEFINITIONS OR
    "ZOO_OS_RTTHREAD" IN_LIST PLATFORM_DEFINITIONS))
    list(APPEND ZOO_FEATURE_DEFINITIONS -DZOO_IS_EMBEDDED=1)
    message(STATUS "Feature: Embedded system detected")
else()
    list(APPEND ZOO_FEATURE_DEFINITIONS -DZOO_IS_EMBEDDED=0)
endif()

# Apply feature definitions (only the non-conflicting ones)
add_definitions(${ZOO_FEATURE_DEFINITIONS})

# Check for ZOO platform dependencies
if(EXISTS "${ZOO_REPO_ROOT_DIR}/include/platform")
    set(ZOO_PLATFORM_INCLUDE_DIR "${ZOO_REPO_ROOT_DIR}/include/platform")
    message(STATUS "Found ZOO platform at: ${ZOO_PLATFORM_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "ZOO platform not found. Please ensure zoo platform module is available.")
endif()

# Check for ZOO util (if not util module itself)
if(NOT "${MODULE_LOWER}" STREQUAL "util")
    if(EXISTS "${ZOO_REPO_ROOT_DIR}/include/platform/util")
        set(ZOO_UTIL_INCLUDE_DIR "${ZOO_REPO_ROOT_DIR}/include/platform/util")
        message(STATUS "Found ZOO util at: ${ZOO_UTIL_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "ZOO util not found.")
    endif()
else()
    set(ZOO_UTIL_INCLUDE_DIR "")
endif()

# Check for ZOO log (if not log module itself)
if(NOT "${MODULE_LOWER}" STREQUAL "log")
    if(EXISTS "${ZOO_REPO_ROOT_DIR}/include/platform/log")
        set(ZOO_LOG_INCLUDE_DIR "${ZOO_REPO_ROOT_DIR}/include/platform/log")
        message(STATUS "Found ZOO log at: ${ZOO_LOG_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "ZOO log not found.")
    endif()
else()
    set(ZOO_LOG_INCLUDE_DIR "")
endif()

# Module-specific dependencies
# Check for ZOO memory pool
if(EXISTS "${ZOO_REPO_ROOT_DIR}/include/platform/memory_pool")
    set(ZOO_MEMORY_POOL_INCLUDE_DIR "${ZOO_REPO_ROOT_DIR}/include/platform/memory_pool")
    message(STATUS "Found ZOO memory pool at: ${ZOO_MEMORY_POOL_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "ZOO memory pool not found.")
endif()

# Check for ZOO buffer
if(EXISTS "${ZOO_REPO_ROOT_DIR}/include/platform/buffer")
    set(ZOO_BUFFER_INCLUDE_DIR "${ZOO_REPO_ROOT_DIR}/include/platform/buffer")
    message(STATUS "Found ZOO buffer at: ${ZOO_BUFFER_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "ZOO buffer not found.")
endif()

# Include directories
set(ZOO_THREAD_POOL_INCLUDE_DIRS
    ${ZOO_REPO_ROOT_DIR}/include/platform/thread_pool
    ${ZOO_PLATFORM_INCLUDE_DIR}
    ${ZOO_MEMORY_POOL_INCLUDE_DIR}
    ${ZOO_BUFFER_INCLUDE_DIR}
    ${ZOO_UTIL_INCLUDE_DIR}
    ${ZOO_LOG_INCLUDE_DIR}
)

# Source files
set(ZOO_THREAD_POOL_SOURCES
    ${ZOO_REPO_ROOT_DIR}/src/platform/thread_pool/zoo_thread_pool.c
)

# Header files
set(ZOO_THREAD_POOL_HEADERS
    ${ZOO_REPO_ROOT_DIR}/include/platform/thread_pool/zoo_thread_pool.h
)

# Build static library
add_library(zoo_thread_pool_static STATIC ${ZOO_THREAD_POOL_SOURCES})
set_target_properties(zoo_thread_pool_static PROPERTIES
    OUTPUT_NAME zoo_thread_pool
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_include_directories(zoo_thread_pool_static PUBLIC
    $<BUILD_INTERFACE:${ZOO_REPO_ROOT_DIR}/include/platform/thread_pool>
    $<BUILD_INTERFACE:${ZOO_PLATFORM_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZOO_MEMORY_POOL_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZOO_BUFFER_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZOO_UTIL_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZOO_LOG_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(zoo_thread_pool_static PRIVATE
    ${ZOO_REPO_ROOT_DIR}/include/platform/thread_pool
)

# Enhanced platform-specific linking
target_link_libraries(zoo_thread_pool_static PUBLIC ${PLATFORM_LIBS} zoo_buffer_static)

# Add pthread threading only if available and needed
if("pthread" IN_LIST PLATFORM_LIBS AND TARGET Threads::Threads)
    target_link_libraries(zoo_thread_pool_static PUBLIC Threads::Threads)
endif()

# Build shared library
add_library(zoo_thread_pool_shared SHARED ${ZOO_THREAD_POOL_SOURCES})
set_target_properties(zoo_thread_pool_shared PROPERTIES
    OUTPUT_NAME zoo_thread_pool
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_include_directories(zoo_thread_pool_shared PUBLIC
    $<BUILD_INTERFACE:${ZOO_REPO_ROOT_DIR}/include/platform/thread_pool>
    $<BUILD_INTERFACE:${ZOO_PLATFORM_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZOO_MEMORY_POOL_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZOO_BUFFER_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZOO_UTIL_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZOO_LOG_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(zoo_thread_pool_shared PRIVATE
    ${ZOO_REPO_ROOT_DIR}/include/platform/thread_pool
)

# Enhanced platform-specific linking
target_link_libraries(zoo_thread_pool_shared PUBLIC ${PLATFORM_LIBS} zoo_buffer_shared)

# Add pthread threading only if available and needed
if("pthread" IN_LIST PLATFORM_LIBS AND TARGET Threads::Threads)
    target_link_libraries(zoo_thread_pool_shared PUBLIC Threads::Threads)
endif()

# Enable testing
enable_testing()

# Add subdirectories
if(EXISTS "${ZOO_REPO_ROOT_DIR}/tests/unit/thread_pool/CMakeLists.txt")
    add_subdirectory(${ZOO_REPO_ROOT_DIR}/tests/unit/thread_pool ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()

# Install configuration
include(GNUInstallDirs)

# Install libraries
install(TARGETS zoo_thread_pool_static zoo_thread_pool_shared
    EXPORT ZooThreadPoolTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(FILES ${ZOO_THREAD_POOL_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zoo
)

# Install CMake config files
install(EXPORT ZooThreadPoolTargets
    FILE ZooThreadPoolTargets.cmake
    NAMESPACE Zoo::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZooThreadPool
)

# Create config file (if template exists)
include(CMakePackageConfigHelpers)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZooThreadPoolConfig.cmake.in")
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZooThreadPoolConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ZooThreadPoolConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZooThreadPool
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/ZooThreadPoolConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ZooThreadPoolConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ZooThreadPoolConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZooThreadPool
    )
else()
    message(STATUS "CMake config template not found, skipping config file generation")
endif()

# Create pkg-config file (if template exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/zoo-thread_pool.pc.in")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/zoo-thread_pool.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/zoo-thread_pool.pc"
        @ONLY
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zoo-thread_pool.pc"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
else()
    message(STATUS "pkg-config template not found, skipping pkg-config file generation")
endif()

# CPack configuration for packaging (standalone module build only)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CPACK_PACKAGE_NAME "zoo-thread_pool")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ZOO ThreadPool Module")
    set(CPACK_PACKAGE_VENDOR "Basic Software Research Institute ltd")
    set(CPACK_PACKAGE_CONTACT "weiwang.sun@gmail.com")

    set(CPACK_SOURCE_GENERATOR "TGZ")
    set(CPACK_GENERATOR "TGZ;DEB;RPM")

    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libpthread-stubs0-dev")
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc, glibc-devel")

    include(CPack)
endif()

# ==============================================================================
# ENHANCED CONFIGURATION SUMMARY
# ==============================================================================

# Display configuration summary
message(STATUS "")
message(STATUS "===============================================================================")
message(STATUS "ZOO ThreadPool Cross-Platform Configuration Summary:")
message(STATUS "===============================================================================")
message(STATUS "  Project:              ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "  Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "  Target system:        ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Target processor:     ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  C compiler:           ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  C compiler path:      ${CMAKE_C_COMPILER}")
message(STATUS "  C standard:           C${CMAKE_C_STANDARD}")
message(STATUS "  C flags:              ${CMAKE_C_FLAGS}")
message(STATUS "  Install prefix:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Platform Detection:")
# Check PLATFORM_DEFINITIONS list for operating system
set(OS_DETECTED "Unknown")
if("-D__ZOO_OS_LINUX" IN_LIST PLATFORM_DEFINITIONS)
    set(OS_DETECTED "Linux/Unix")
elseif("-DZOO_OS_FREERTOS" IN_LIST PLATFORM_DEFINITIONS)
    set(OS_DETECTED "FreeRTOS (Embedded)")
elseif("-DZOO_OS_CMSIS_RTOS" IN_LIST PLATFORM_DEFINITIONS)
    set(OS_DETECTED "CMSIS-RTOS (Embedded)")
elseif("-DZOO_OS_RTTHREAD" IN_LIST PLATFORM_DEFINITIONS)
    set(OS_DETECTED "RT-Thread (Embedded)")
elseif("-DZOO_OS_BAREMETAL" IN_LIST PLATFORM_DEFINITIONS)
    set(OS_DETECTED "Bare Metal")
endif()
message(STATUS "  Operating System:     ${OS_DETECTED}")

# Check PLATFORM_DEFINITIONS list for architecture  
set(ARCH_DETECTED "Unknown")
if("-D__ZOO_PLATFORM_X86_64" IN_LIST PLATFORM_DEFINITIONS)
    set(ARCH_DETECTED "x86_64 (64-bit)")
elseif("-D__ZOO_PLATFORM_X86_32" IN_LIST PLATFORM_DEFINITIONS)
    set(ARCH_DETECTED "x86_32 (32-bit)")
elseif("-D__ZOO_PLATFORM_ARM_CORTEX_A64" IN_LIST PLATFORM_DEFINITIONS)
    set(ARCH_DETECTED "ARM Cortex-A64 (64-bit)")
elseif("-D__ZOO_PLATFORM_ARM_CORTEX_A32" IN_LIST PLATFORM_DEFINITIONS)
    set(ARCH_DETECTED "ARM Cortex-A32 (32-bit)")
elseif("-D__ZOO_PLATFORM_ARM_CORTEX_M" IN_LIST PLATFORM_DEFINITIONS)
    set(ARCH_DETECTED "ARM Cortex-M (Embedded)")
endif()
message(STATUS "  Architecture:         ${ARCH_DETECTED}")

message(STATUS "  Platform libraries:   ${PLATFORM_LIBS}")
message(STATUS "  Platform definitions: ${PLATFORM_DEFINITIONS}")
message(STATUS "")
message(STATUS "ZOO Dependencies:")
message(STATUS "  Platform include:     ${ZOO_PLATFORM_INCLUDE_DIR}")
message(STATUS "  Memory pool include:  ${ZOO_MEMORY_POOL_INCLUDE_DIR}")
message(STATUS "  Util include:         ${ZOO_UTIL_INCLUDE_DIR}")
message(STATUS "  Log include:          ${ZOO_LOG_INCLUDE_DIR}")
message(STATUS "")

# Threading configuration summary
if("pthread" IN_LIST PLATFORM_LIBS)
    message(STATUS "Threading Support:    POSIX Threads (pthread)")
elseif("-DZOO_OS_FREERTOS" IN_LIST PLATFORM_DEFINITIONS)
    message(STATUS "Threading Support:    FreeRTOS Tasks")
elseif("-DZOO_OS_CMSIS_RTOS" IN_LIST PLATFORM_DEFINITIONS)
    message(STATUS "Threading Support:    CMSIS-RTOS Threads")
elseif("-DZOO_OS_RTTHREAD" IN_LIST PLATFORM_DEFINITIONS)
    message(STATUS "Threading Support:    RT-Thread")
else()
    message(STATUS "Threading Support:    Platform Abstraction Layer (Stubs)")
endif()

message(STATUS "")
message(STATUS "Feature Definitions:")
message(STATUS "  Note: ZOO_HAS_FILESYSTEM, ZOO_HAS_SYSLOG are auto-detected in zoo_platform.h")
message(STATUS "  ZOO_HAS_POSIX:        ${ZOO_FEATURE_DEFINITIONS}")
message(STATUS "  ZOO_HAS_THREADING:    Enabled (${ARCH_DETECTED})")
message(STATUS "  ZOO_IS_EMBEDDED:      ${ZOO_FEATURE_DEFINITIONS}")

message(STATUS "===============================================================================")
message(STATUS "")
