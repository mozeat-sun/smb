#[[
==============================================================================
Platform Detection for ZOO Buffer Module
==============================================================================
Description: Cross-platform detection and configuration
Features:
- Windows, Linux, FreeRTOS, CMSIS-RTOS, Bare Metal support
- Platform-specific preprocessor definitions
- Threading library detection and configuration

Author: weiwang.sun
Date: 2025-07-30
==============================================================================
]]

# ==============================================================================
# PLATFORM DETECTION AND CONFIGURATION
# ==============================================================================

# Detect target platform
if(WIN32)
    set(ZOO_TARGET_OS "Windows")
    add_definitions(-DZOO_OS_WINDOWS)
elseif(UNIX AND NOT APPLE)
    set(ZOO_TARGET_OS "Linux")
    add_definitions(-DZOO_OS_LINUX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    # Embedded/Bare metal detection
    if(DEFINED ARM_CORTEX_M)
        set(ZOO_TARGET_OS "ARM Cortex-M")
        add_definitions(-DZOO_OS_BAREMETAL -DZOO_PLATFORM_ARM_CORTEX_M)
        
        # Check for RTOS
        if(DEFINED FREERTOS)
            add_definitions(-DZOO_OS_FREERTOS)
            set(ZOO_TARGET_OS "${ZOO_TARGET_OS} + FreeRTOS")
        elseif(DEFINED CMSIS_RTOS)
            add_definitions(-DZOO_OS_CMSIS_RTOS)
            set(ZOO_TARGET_OS "${ZOO_TARGET_OS} + CMSIS-RTOS")
        endif()
    else()
        set(ZOO_TARGET_OS "Generic Embedded")
        add_definitions(-DZOO_OS_BAREMETAL)
    endif()
else()
    set(ZOO_TARGET_OS "Unknown")
    add_definitions(-DZOO_OS_UNKNOWN)
endif()

message(STATUS "ZOO Buffer Module - Target OS: ${ZOO_TARGET_OS}")

# ==============================================================================
# THREADING SUPPORT CONFIGURATION
# ==============================================================================

# Function to configure threading for target
function(zoo_buffer_configure_threading target_name)
    # Platform-specific threading libraries
    if(ZOO_TARGET_OS STREQUAL "Linux")
        find_package(Threads REQUIRED)
        target_link_libraries(${target_name} PUBLIC Threads::Threads)
    elseif(ZOO_TARGET_OS STREQUAL "Windows")
        # Windows threading is built-in
        target_link_libraries(${target_name} PUBLIC kernel32)
    elseif(ZOO_TARGET_OS MATCHES "FreeRTOS")
        # FreeRTOS - no additional libraries needed
        message(STATUS "ZOO Buffer Module - FreeRTOS threading support enabled")
    elseif(ZOO_TARGET_OS MATCHES "CMSIS-RTOS")
        # CMSIS-RTOS - no additional libraries needed
        message(STATUS "ZOO Buffer Module - CMSIS-RTOS threading support enabled")
    else()
        # Bare metal or unknown - threading stubs
        message(STATUS "ZOO Buffer Module - Threading support disabled (bare metal)")
    endif()
endfunction()
