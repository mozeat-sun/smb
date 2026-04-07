#===============================================================================
# ZOO Thread Pool Project Configuration
# Comprehensive project setup and configuration management
#===============================================================================

# Project version and metadata
set(ZOO_THREAD_POOL_VERSION_MAJOR 1)
set(ZOO_THREAD_POOL_VERSION_MINOR 1)
set(ZOO_THREAD_POOL_VERSION_PATCH 0)
set(ZOO_THREAD_POOL_VERSION "${ZOO_THREAD_POOL_VERSION_MAJOR}.${ZOO_THREAD_POOL_VERSION_MINOR}.${ZOO_THREAD_POOL_VERSION_PATCH}")

# Project metadata
set(ZOO_THREAD_POOL_DESCRIPTION "High-performance cross-platform thread pool for ZOO framework")
set(ZOO_THREAD_POOL_HOMEPAGE_URL "https://github.com/zoo-project/thread-pool")
set(ZOO_THREAD_POOL_CONTACT "weiwang.sun@niopower.com")

# Project build options with defaults
option(ZOO_ENABLE_TESTS "Build unit and integration tests" OFF)
option(ZOO_ENABLE_BENCHMARKS "Build performance benchmarks" OFF)
option(ZOO_ENABLE_COVERAGE "Enable code coverage reporting" OFF)
option(ZOO_BUILD_STATIC "Build static library" ON)
option(ZOO_BUILD_SHARED "Build shared library" OFF)
option(ZOO_THREAD_POOL_DEBUG "Enable debug logging" OFF)
option(ZOO_ENABLE_DOCUMENTATION "Generate documentation with Doxygen" OFF)
option(ZOO_ENABLE_EXAMPLES "Build example applications" OFF)
option(ZOO_ENABLE_PACKAGING "Enable package generation" OFF)

# Advanced options
option(ZOO_USE_SYSTEM_GTEST "Use system-installed Google Test" OFF)
option(ZOO_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ZOO_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ZOO_ENABLE_MSAN "Enable MemorySanitizer" OFF)
option(ZOO_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Project directories
set(ZOO_THREAD_POOL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(ZOO_THREAD_POOL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/inc")
set(ZOO_THREAD_POOL_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set(ZOO_THREAD_POOL_EXAMPLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")
set(ZOO_THREAD_POOL_DOC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs")
set(ZOO_THREAD_POOL_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Platform directory (assuming it's in parent directory)
if(NOT DEFINED ZOO_PLATFORM_DIR)
    set(ZOO_PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../platform")
endif()

# Validate platform directory exists
if(NOT EXISTS "${ZOO_PLATFORM_DIR}/inc/zoo.h")
    message(WARNING "ZOO platform directory not found at ${ZOO_PLATFORM_DIR}")
    message(WARNING "Some features may not work correctly")
endif()

# Memory pool directory (assuming it's in parent directory)
if(NOT DEFINED ZOO_MEMORY_POOL_DIR)
    set(ZOO_MEMORY_POOL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../memory_pool")
endif()

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Configuration validation
if(ZOO_ENABLE_COVERAGE AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Code coverage requires Debug build type. Setting CMAKE_BUILD_TYPE to Debug.")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

if(ZOO_ENABLE_ASAN AND ZOO_ENABLE_TSAN)
    message(FATAL_ERROR "AddressSanitizer and ThreadSanitizer cannot be enabled simultaneously")
endif()

# Sanitizer configuration
if(ZOO_ENABLE_ASAN OR ZOO_ENABLE_TSAN OR ZOO_ENABLE_MSAN OR ZOO_ENABLE_UBSAN)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "Sanitizers work best with Debug build type")
    endif()
endif()

# Feature detection - moved to main CMakeLists.txt after project() call
# include(CheckIncludeFile)
# include(CheckFunctionExists)
# include(CheckSymbolExists)
# include(CheckCSourceCompiles)

# Note: All feature checks moved to main CMakeLists.txt after project() call
# to ensure C language is enabled before running checks

# Configuration will be done after project() definition

# Configuration summary function
function(zoo_print_configuration)
    message(STATUS "")
    message(STATUS "==============================================================================")
    message(STATUS "ZOO Thread Pool Configuration Summary")
    message(STATUS "==============================================================================")
    message(STATUS "Version: ${ZOO_THREAD_POOL_VERSION}")
    message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
    message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
    message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "")
    message(STATUS "Build Options:")
    message(STATUS "  Static Library: ${ZOO_BUILD_STATIC}")
    message(STATUS "  Shared Library: ${ZOO_BUILD_SHARED}")
    message(STATUS "  Tests: ${ZOO_ENABLE_TESTS}")
    message(STATUS "  Benchmarks: ${ZOO_ENABLE_BENCHMARKS}")
    message(STATUS "  Examples: ${ZOO_ENABLE_EXAMPLES}")
    message(STATUS "  Documentation: ${ZOO_ENABLE_DOCUMENTATION}")
    message(STATUS "  Debug Logging: ${ZOO_THREAD_POOL_DEBUG}")
    message(STATUS "")
    message(STATUS "Development Options:")
    message(STATUS "  Code Coverage: ${ZOO_ENABLE_COVERAGE}")
    message(STATUS "  AddressSanitizer: ${ZOO_ENABLE_ASAN}")
    message(STATUS "  ThreadSanitizer: ${ZOO_ENABLE_TSAN}")
    message(STATUS "  MemorySanitizer: ${ZOO_ENABLE_MSAN}")
    message(STATUS "  UBSanitizer: ${ZOO_ENABLE_UBSAN}")
    message(STATUS "")
    message(STATUS "Platform Features:")
    message(STATUS "  Threading: ${CMAKE_USE_PTHREADS_INIT}")
    message(STATUS "  Atomic Operations: ${HAVE_STDATOMIC_H}")
    message(STATUS "  High Resolution Timer: ${HAVE_CLOCK_GETTIME}")
    message(STATUS "")
    message(STATUS "Directories:")
    message(STATUS "  Source: ${ZOO_THREAD_POOL_SOURCE_DIR}")
    message(STATUS "  Include: ${ZOO_THREAD_POOL_INCLUDE_DIR}")
    message(STATUS "  Platform: ${ZOO_PLATFORM_DIR}")
    message(STATUS "  Memory Pool: ${ZOO_MEMORY_POOL_DIR}")
    message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "==============================================================================")
    message(STATUS "")
endfunction()

# Target configuration function
function(zoo_configure_target target_name)
    # Set target properties
    set_target_properties(${target_name} PROPERTIES
        VERSION ${ZOO_THREAD_POOL_VERSION}
        SOVERSION ${ZOO_THREAD_POOL_VERSION_MAJOR}
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
    )
    
    # Add compile definitions
    target_compile_definitions(${target_name} PRIVATE
        ZOO_THREAD_POOL_VERSION_MAJOR=${ZOO_THREAD_POOL_VERSION_MAJOR}
        ZOO_THREAD_POOL_VERSION_MINOR=${ZOO_THREAD_POOL_VERSION_MINOR}
        ZOO_THREAD_POOL_VERSION_PATCH=${ZOO_THREAD_POOL_VERSION_PATCH}
    )
    
    if(ZOO_THREAD_POOL_DEBUG)
        target_compile_definitions(${target_name} PRIVATE ZOO_THREAD_POOL_DEBUG=1)
    endif()
    
    # Platform-specific definitions
    if(WIN32)
        target_compile_definitions(${target_name} PRIVATE
            _WIN32_WINNT=0x0601  # Windows 7
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
    endif()
    
    # Include directories
    target_include_directories(${target_name} PUBLIC
        $<BUILD_INTERFACE:${ZOO_THREAD_POOL_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${ZOO_PLATFORM_DIR}/inc>
        $<INSTALL_INTERFACE:include>
    )
    
    # Memory pool integration if available
    if(EXISTS "${ZOO_MEMORY_POOL_DIR}/inc/zoo_memory_pool.h")
        target_include_directories(${target_name} PUBLIC
            $<BUILD_INTERFACE:${ZOO_MEMORY_POOL_DIR}/inc>
        )
        target_compile_definitions(${target_name} PRIVATE ZOO_MEMORY_POOL_AVAILABLE=1)
    endif()
    
    # Link threading library
    target_link_libraries(${target_name} PUBLIC Threads::Threads)
    
    # Platform-specific libraries
    if(WIN32)
        target_link_libraries(${target_name} PRIVATE kernel32 user32)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(${target_name} PRIVATE rt)
    endif()
endfunction()

# Test configuration function
function(zoo_configure_test_target target_name)
    zoo_configure_target(${target_name})
    
    # Additional test-specific settings
    if(ZOO_ENABLE_COVERAGE)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target_name} PRIVATE --coverage)
            target_link_options(${target_name} PRIVATE --coverage)
        endif()
    endif()
    
    # Sanitizer options
    if(ZOO_ENABLE_ASAN)
        target_compile_options(${target_name} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=address)
    endif()
    
    if(ZOO_ENABLE_TSAN)
        target_compile_options(${target_name} PRIVATE -fsanitize=thread)
        target_link_options(${target_name} PRIVATE -fsanitize=thread)
    endif()
    
    if(ZOO_ENABLE_MSAN)
        target_compile_options(${target_name} PRIVATE -fsanitize=memory -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=memory)
    endif()
    
    if(ZOO_ENABLE_UBSAN)
        target_compile_options(${target_name} PRIVATE -fsanitize=undefined)
        target_link_options(${target_name} PRIVATE -fsanitize=undefined)
    endif()
endfunction()

# Package configuration
if(ZOO_ENABLE_PACKAGING)
    set(CPACK_PACKAGE_NAME "zoo-thread-pool")
    set(CPACK_PACKAGE_VERSION ${ZOO_THREAD_POOL_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${ZOO_THREAD_POOL_DESCRIPTION})
    set(CPACK_PACKAGE_VENDOR "Basic Software Research Institute ltd")
    set(CPACK_PACKAGE_CONTACT ${ZOO_THREAD_POOL_CONTACT})
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    
    # Platform-specific package settings
    if(WIN32)
        set(CPACK_GENERATOR "ZIP;NSIS")
        set(CPACK_NSIS_DISPLAY_NAME "ZOO Thread Pool")
        set(CPACK_NSIS_PACKAGE_NAME "ZOO Thread Pool")
    elseif(APPLE)
        set(CPACK_GENERATOR "TGZ;DragNDrop")
    else()
        set(CPACK_GENERATOR "TGZ;DEB;RPM")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libpthread-stubs0-dev")
        set(CPACK_RPM_PACKAGE_REQUIRES "glibc, glibc-devel")
    endif()
    
    include(CPack)
endif()

message(STATUS "ZOO Thread Pool project configuration loaded")
