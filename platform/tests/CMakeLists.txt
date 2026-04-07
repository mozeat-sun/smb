# Platform Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(ZooPlatformTests C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add ZOO cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

# Find required packages
find_package(Threads REQUIRED)
find_package(Unity REQUIRED)

# Include directories
include_directories(../inc)

# Add executable for platform tests
add_executable(zoo_platform_tests
    test_platform_basic.c
    test_platform_threading.c
    test_platform_atomic.c
    test_platform_time.c
    test_platform_rtthread.c
    test_runner.c
    ${Unity_SOURCE}
)

# Include Unity headers
target_include_directories(zoo_platform_tests PRIVATE ${Unity_INCLUDE_DIR})

target_compile_definitions(zoo_platform_tests PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)

# Link libraries
target_link_libraries(zoo_platform_tests
    Threads::Threads
)

# Enable testing
enable_testing()

# Add test
add_test(NAME ZooPlatformTests COMMAND zoo_platform_tests)

# Compiler flags for C
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# Add coverage flags if needed
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()
