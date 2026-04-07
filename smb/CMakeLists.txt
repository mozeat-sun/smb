# Minimum CMake version required
cmake_minimum_required(VERSION 3.14)

# Project name and version
project(ZooSMB VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable testing for CTest integration
enable_testing()

# Enable debug information if build type not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

get_filename_component(ZOO_SMB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
get_filename_component(ZOO_REPO_ROOT_DIR "${ZOO_SMB_ROOT_DIR}/.." ABSOLUTE)

# Add header directories to include path
include_directories(${ZOO_SMB_ROOT_DIR}/inc)

# Compiler options
add_compile_options(
    -Wall      # Enable all warnings
    -Wextra    # Enable extra warnings
    -Wpedantic # Enable pedantic warnings
    # -Werror  # Treat warnings as errors (commented out for development)
)

# Output directories configuration
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find required dependencies
find_package(Threads REQUIRED)

# Add ZOO platform dependency
if(EXISTS "${ZOO_REPO_ROOT_DIR}/platform/inc")
    set(ZOO_PLATFORM_INCLUDE_DIR "${ZOO_REPO_ROOT_DIR}/platform/inc")
    set(ZOS_INCLUDE_DIR "${ZOO_REPO_ROOT_DIR}/platform/inc")  # For backward compatibility
    message(STATUS "Found ZOO platform at: ${ZOO_PLATFORM_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "ZOO platform not found at repository root. Build from the monorepo root or ensure platform module is present.")
endif()

# Add subdirectories for source, examples and tests
add_subdirectory(src)
add_subdirectory(tests)     # Unity-based tests
add_subdirectory(examples)

# Function comment compliance check (CI-friendly)
add_custom_target(check_function_comments
    COMMAND bash ${ZOO_SMB_ROOT_DIR}/tools/check_function_comments.sh
    WORKING_DIRECTORY ${ZOO_SMB_ROOT_DIR}
    COMMENT "Running SMB function comment compliance check"
)

add_test(
    NAME smb_comment_compliance
    COMMAND bash ${ZOO_SMB_ROOT_DIR}/tools/check_function_comments.sh
)

# Install header files
install(
    DIRECTORY ${ZOO_SMB_ROOT_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Install ZOO platform header files
install(
    DIRECTORY ${ZOO_PLATFORM_INCLUDE_DIR}/
    DESTINATION include/zoo
    FILES_MATCHING PATTERN "*.h"
)

# Export project configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ZooSMBConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Configure package
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZooSMBConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ZooSMBConfig.cmake"
    INSTALL_DESTINATION lib/cmake/ZooSMB
)

# Install CMake configuration files
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ZooSMBConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ZooSMBConfigVersion.cmake"
    DESTINATION lib/cmake/ZooSMB
)

# Generate and install pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/zoo-smb.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/zoo-smb.pc"
    @ONLY
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/zoo-smb.pc"
    DESTINATION lib/pkgconfig
)