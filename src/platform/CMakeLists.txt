cmake_minimum_required(VERSION 3.14)

# Project information
project(zoo_platform
    VERSION 1.2.2
    DESCRIPTION "ZOO Cross-Platform Foundation Library"
    LANGUAGES C
)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTS "Build the tests" ON)

# Platform detection
if(WIN32)
    add_definitions(-DZOO_OS_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DZOO_OS_LINUX)
    add_definitions(-DZOO_HAS_POSIX=1)
elseif(APPLE)
    add_definitions(-DZOO_OS_MACOS)
    add_definitions(-DZOO_HAS_POSIX=1)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform)

# Source files
set(PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/zoo_error.c
)

set(PLATFORM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/zoo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/zoo_error.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/zoo_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/zoo_platform.h
)

# Create static library
add_library(zoo_platform_static STATIC ${PLATFORM_SOURCES})
target_compile_options(zoo_platform_static PRIVATE
    -Wall
    -Wextra
)

# Create shared library
add_library(zoo_platform_shared SHARED ${PLATFORM_SOURCES})
target_compile_options(zoo_platform_shared PRIVATE
    -Wall
    -Wextra
)

# Set library properties
set_target_properties(zoo_platform_static PROPERTIES
    OUTPUT_NAME zoo_platform
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
)

set_target_properties(zoo_platform_shared PROPERTIES
    OUTPUT_NAME zoo_platform
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Install targets
install(TARGETS zoo_platform_static zoo_platform_shared
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${PLATFORM_HEADERS}
    DESTINATION include/zoo
)

# Create pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/zoo-platform.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/zoo-platform.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zoo-platform.pc
    DESTINATION lib/pkgconfig
)

# Add tests subdirectory if enabled
if(BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/unit/platform/CMakeLists.txt")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../tests/unit/platform ${CMAKE_CURRENT_BINARY_DIR}/tests)
    endif()
endif()

# Print build information
message(STATUS "Zoo Platform Library Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")

# Build platform-scoped runtime modules.
if(ZOO_BUILD_UTIL)
    add_subdirectory(util)
endif()

if(ZOO_BUILD_LOG)
    add_subdirectory(log)
endif()

if(ZOO_BUILD_MEMORY_POOL)
    add_subdirectory(memory_pool)
endif()

if(ZOO_BUILD_BUFFER)
    add_subdirectory(buffer)
endif()

if(ZOO_BUILD_SOCKET)
    add_subdirectory(socket)
endif()

if(ZOO_BUILD_THREAD_POOL)
    add_subdirectory(thread_pool)
endif()

if(ZOO_BUILD_DISPATCHER)
    add_subdirectory(dispatcher)
endif()

if(ZOO_BUILD_TIMER)
    add_subdirectory(timer)
endif()
