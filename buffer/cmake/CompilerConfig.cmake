#[[
==============================================================================
Compiler Configuration for ZOO Buffer Module
==============================================================================
Description: Compiler flags and platform-specific configuration
Features:
- Cross-platform compiler detection and configuration
- Debug/Release build configurations
- Thread safety flags for supported platforms
- Warning and optimization settings

Author: weiwang.sun
Date: 2025-07-30
==============================================================================
]]

# ==============================================================================
# COMPILER FLAGS CONFIGURATION
# ==============================================================================

# Common compiler flags
set(ZOO_COMMON_FLAGS "")
set(ZOO_DEBUG_FLAGS "")
set(ZOO_RELEASE_FLAGS "")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # GCC/Clang flags
    list(APPEND ZOO_COMMON_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wformat=2
        -Wformat-security
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wshadow
        -fno-common
    )
    
    list(APPEND ZOO_DEBUG_FLAGS
        -g3
        -O0
        -DDEBUG
        -DZOO_DEBUG_MODE=1
        -fstack-protector-strong
        -fasynchronous-unwind-tables
    )
    
    list(APPEND ZOO_RELEASE_FLAGS
        -O2
        -DNDEBUG
        -DZOO_DEBUG_MODE=0
        -ffunction-sections
        -fdata-sections
    )
    
    # Thread safety flags for platforms that support threading
    if(NOT ZOO_TARGET_OS STREQUAL "Generic Embedded" AND NOT ZOO_TARGET_OS MATCHES "ARM Cortex-M")
        list(APPEND ZOO_COMMON_FLAGS -pthread)
    endif()
    
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    # MSVC flags
    list(APPEND ZOO_COMMON_FLAGS
        /W4
        /wd4996  # Disable deprecated function warnings
        /D_CRT_SECURE_NO_WARNINGS
    )
    
    list(APPEND ZOO_DEBUG_FLAGS
        /Od
        /Zi
        /DDEBUG
        /DZOO_DEBUG_MODE=1
    )
    
    list(APPEND ZOO_RELEASE_FLAGS
        /O2
        /DNDEBUG
        /DZOO_DEBUG_MODE=0
    )
endif()

# Function to apply compiler flags to target
function(zoo_buffer_apply_compiler_flags target_name)
    target_compile_options(${target_name} PRIVATE ${ZOO_COMMON_FLAGS})
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${target_name} PRIVATE ${ZOO_DEBUG_FLAGS})
    else()
        target_compile_options(${target_name} PRIVATE ${ZOO_RELEASE_FLAGS})
    endif()
endfunction()
