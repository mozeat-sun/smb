#******************************************************************************
# Copyright (C) 2025, Basic Software Research Institute ltd
# All rights reserved.
# Product: ZOO
# Module: dispatcher
# Component id: ZOO_DISPATCHER
# File name: CMakeLists.txt
# Description: CMake configuration for ZOO dispatcher tests
# History recorder:
# Version   date           author            context
# 1.0       2025-08-04     AI Assistant      created
#******************************************************************************

# Test configuration
enable_testing()

get_filename_component(ZOO_DISPATCHER_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Include directories for tests
include_directories(
    ${ZOO_DISPATCHER_ROOT_DIR}/inc
    ${ZOO_DISPATCHER_INCLUDE_DIRS}
)

# Unit test executable
add_executable(zoo_dispatcher_tests
    test_zoo_dispatcher.c
)

target_link_libraries(zoo_dispatcher_tests
    zoo_dispatcher_static
    zoo_buffer_static
    zoo_memory_pool_static
    zoo_log_static
    Threads::Threads
)

target_include_directories(zoo_dispatcher_tests PRIVATE
    ${ZOO_DISPATCHER_INCLUDE_DIRS}
)

# Performance test executable
add_executable(zoo_dispatcher_performance_tests_bin
    test_zoo_dispatcher_performance.c
)

target_link_libraries(zoo_dispatcher_performance_tests_bin
    zoo_dispatcher_static
    zoo_buffer_static
    zoo_memory_pool_static
    zoo_log_static
    Threads::Threads
)

target_include_directories(zoo_dispatcher_performance_tests_bin PRIVATE
    ${ZOO_DISPATCHER_INCLUDE_DIRS}
)

target_compile_options(zoo_dispatcher_tests PRIVATE
    -Wno-unused-parameter
    -Wno-cast-function-type
    -Wno-unused-variable
)

target_compile_options(zoo_dispatcher_performance_tests_bin PRIVATE
    -Wno-unused-parameter
    -Wno-cast-function-type
    -Wno-unused-variable
)

# Add individual tests
add_test(NAME zoo_dispatcher_unit_tests 
         COMMAND zoo_dispatcher_tests --unit)

add_test(NAME zoo_dispatcher_integration_tests 
         COMMAND zoo_dispatcher_tests --integration)

add_test(NAME zoo_dispatcher_performance_tests 
         COMMAND zoo_dispatcher_performance_tests_bin --basic)

# Set test properties
set_tests_properties(zoo_dispatcher_unit_tests PROPERTIES
    TIMEOUT 30
    LABELS "unit"
)

set_tests_properties(zoo_dispatcher_integration_tests PROPERTIES
    TIMEOUT 60
    LABELS "integration"
)

set_tests_properties(zoo_dispatcher_performance_tests PROPERTIES
    TIMEOUT 120
    LABELS "performance"
)

# Test coverage (if available)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_COMPILER_IS_GNUCC)
        target_compile_options(zoo_dispatcher_tests PRIVATE --coverage)
        target_link_libraries(zoo_dispatcher_tests PRIVATE --coverage)
    endif()
endif()
