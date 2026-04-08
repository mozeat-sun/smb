#[[
==============================================================================
CMake Configuration for ZOO Buffer Module Tests
==============================================================================
Description: Unity C testing framework based unit testing suite for ZOO buffer module
Features:
- Comprehensive test coverage for linked list operations
- Thread safety testing  
- Performance and stress testing
- Cross-platform test execution

Author: weiwang.sun, GitHub Copilot
Date: 2025-08-01 (Migrated from GTest to Unity)
==============================================================================
]]

cmake_minimum_required(VERSION 3.16)

# Set project
project(ZooBufferTests C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Only configure tests if this is the top-level project or explicitly requested
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR ZOO_BUFFER_BUILD_TESTS)
    
    # ==============================================================================
    # UNITY TEST FRAMEWORK CONFIGURATION
    # ==============================================================================
    
    # Add ZOO cmake modules path for Unity
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")
    
    # Find Unity framework
    find_package(Unity REQUIRED)
    
    message(STATUS "ZOO Buffer Tests - Using Unity framework from: ${Unity_INCLUDE_DIR}")
    
    # ==============================================================================
    # TEST SOURCES AND TARGET
    # ==============================================================================
    
    # Test source files (converted from C++ to C)
    set(ZOO_BUFFER_TEST_SOURCES
        test_zoo_list.c
        test_zoo_queue.c
        test_runner.c
        buffer_stubs.c
        zoo_memory_pool_stub.c
        zoo_log_stub.c
        ${Unity_SOURCE}
    )
    
    # Create test executable
    add_executable(zoo_buffer_tests ${ZOO_BUFFER_TEST_SOURCES})
    
    # Set C standard for tests
    set_target_properties(zoo_buffer_tests PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        OUTPUT_NAME "zoo_buffer_tests"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
    
    # ==============================================================================
    # TEST DEPENDENCIES AND LINKING
    # ==============================================================================
    
    # Link test dependencies (using stubs instead of external libraries)
    find_package(Threads)
    if(Threads_FOUND)
        target_link_libraries(zoo_buffer_tests PRIVATE Threads::Threads)
    endif()
    
    # Test include directories
    target_include_directories(zoo_buffer_tests
        PRIVATE
            ${ZOO_REPO_ROOT_DIR}/include/buffer
            ${ZOO_REPO_ROOT_DIR}/include/platform
            ${ZOO_REPO_ROOT_DIR}/include/log
            ${ZOO_REPO_ROOT_DIR}/include/memory_pool
            ${Unity_INCLUDE_DIR}
    )
    
    # Apply compiler flags to tests
    if(COMMAND zoo_buffer_apply_compiler_flags)
        zoo_buffer_apply_compiler_flags(zoo_buffer_tests)
    endif()
    
    # ==============================================================================
    # TEST REGISTRATION AND CONFIGURATION
    # ==============================================================================
    
    # Register tests with CTest
    add_test(
        NAME ZOO_Buffer_Unit_Tests
        COMMAND zoo_buffer_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # Set test properties
    set_tests_properties(ZOO_Buffer_Unit_Tests
        PROPERTIES
            TIMEOUT 300
            LABELS "unit;buffer;core"
    )
    
    # ==============================================================================
    # TEST UTILITIES AND HELPERS
    # ==============================================================================
    
    # Add custom target for running tests
    add_custom_target(buffer_quick_test
        COMMAND zoo_buffer_tests
        DEPENDS zoo_buffer_tests
        COMMENT "Running buffer unit tests"
        VERBATIM
    )
    
    # Add custom target for running tests with verbose output
    add_custom_target(buffer_test_verbose
        COMMAND zoo_buffer_tests
        DEPENDS zoo_buffer_tests
        COMMENT "Running buffer tests with verbose output"
        VERBATIM
    )
    
    # Add custom target for test coverage (if supported)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND 
       (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
        
        find_program(GCOV_PATH gcov)
        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)
        
        if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
            # Add coverage flags to test target
            target_compile_options(zoo_buffer_tests PRIVATE --coverage)
            target_link_options(zoo_buffer_tests PRIVATE --coverage)
            
            # Add coverage target
            add_custom_target(buffer_coverage
                COMMAND ${LCOV_PATH} --directory . --zerocounters
                COMMAND zoo_buffer_tests
                COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
                COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/tests/*' '*/thirdparty/*' --output-file coverage_filtered.info
                COMMAND ${GENHTML_PATH} coverage_filtered.info --output-directory coverage_html
                DEPENDS zoo_buffer_tests
                COMMENT "Generating test coverage report"
                VERBATIM
            )
            
            message(STATUS "ZOO Buffer Tests - Coverage reporting enabled (run 'make coverage')")
        endif()
    endif()
    
    # ==============================================================================
    # TEST SUMMARY
    # ==============================================================================
    
    message(STATUS "")
    message(STATUS "==============================================================================")
    message(STATUS "ZOO Buffer Tests Configuration Summary")
    message(STATUS "==============================================================================")
    message(STATUS "Test Framework: Unity C Testing Framework")
    message(STATUS "Unity Location: ${Unity_INCLUDE_DIR}")
    message(STATUS "Test Executable: zoo_buffer_tests")
    message(STATUS "Test Categories: Unit, Thread Safety, Performance")
    message(STATUS "Custom Targets: quick_test, test_verbose")
    if(TARGET coverage)
        message(STATUS "Coverage: Enabled")
    else()
        message(STATUS "Coverage: Disabled")
    endif()
    message(STATUS "Run Tests: ./zoo_buffer_tests or ctest")
    message(STATUS "Run Quick Tests: make quick_test")
    message(STATUS "==============================================================================")
    message(STATUS "")
    
else()
    message(STATUS "ZOO Buffer Tests - Skipped (testing disabled)")
endif()
