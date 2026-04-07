# ==============================================================================
# ZOO Thread Pool - Unity Test Configuration
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

# Only build tests if we're the top-level project or explicitly requested
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR ZOO_THREAD_POOL_BUILD_TESTS OR ZOO_ENABLE_TESTS)

    get_filename_component(ZOO_THREAD_POOL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
    get_filename_component(ZOO_REPO_ROOT_DIR "${ZOO_THREAD_POOL_ROOT_DIR}/.." ABSOLUTE)
    if(NOT DEFINED ZOO_PLATFORM_DIR)
        set(ZOO_PLATFORM_DIR "${ZOO_REPO_ROOT_DIR}/platform")
    endif()

    # ==============================================================================
    # UNITY TEST FRAMEWORK SETUP
    # ==============================================================================
    
    # Find Unity test framework
    set(UNITY_SOURCE_DIR "${ZOO_REPO_ROOT_DIR}/third_party/test/unity")
    
    if(NOT EXISTS "${UNITY_SOURCE_DIR}/src/unity.c")
        message(FATAL_ERROR "Unity test framework not found at ${UNITY_SOURCE_DIR}")
    endif()
    
    # Unity source files
    set(UNITY_SOURCES
        ${UNITY_SOURCE_DIR}/src/unity.c
    )
    
    # Unity include directory
    set(UNITY_INCLUDE_DIR ${UNITY_SOURCE_DIR}/src)
    
    # ==============================================================================
    # UNITY TEST EXECUTABLES
    # ==============================================================================
    
    # Create Unity test for basic thread pool functionality
    add_executable(test_thread_pool_basic_unity
        test_zoo_thread_pool_basic_unity.c
        ${UNITY_SOURCES}
    )
    
    target_include_directories(test_thread_pool_basic_unity
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../inc
            ${ZOO_PLATFORM_DIR}/inc
            ${UNITY_INCLUDE_DIR}
    )
    
    target_link_libraries(test_thread_pool_basic_unity
        PRIVATE
            zoo_thread_pool
    )
    
    # Apply platform-specific configurations
    if(COMMAND zoo_thread_pool_apply_compiler_flags)
        zoo_thread_pool_apply_compiler_flags(test_thread_pool_basic_unity)
    endif()
    
    if(COMMAND zoo_thread_pool_configure_threading)
        zoo_thread_pool_configure_threading(test_thread_pool_basic_unity)
    endif()
    
    # Create Unity test for stress testing
    add_executable(test_thread_pool_stress_unity
        test_zoo_thread_pool_stress_unity.c
        ${UNITY_SOURCES}
    )
    
    target_include_directories(test_thread_pool_stress_unity
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../inc
            ${ZOO_PLATFORM_DIR}/inc
            ${UNITY_INCLUDE_DIR}
    )
    
    target_link_libraries(test_thread_pool_stress_unity
        PRIVATE
            zoo_thread_pool
    )
    
    # Apply platform-specific configurations
    if(COMMAND zoo_thread_pool_apply_compiler_flags)
        zoo_thread_pool_apply_compiler_flags(test_thread_pool_stress_unity)
    endif()
    
    if(COMMAND zoo_thread_pool_configure_threading)
        zoo_thread_pool_configure_threading(test_thread_pool_stress_unity)
    endif()
    
    # ==============================================================================
    # TEST DEFINITIONS
    # ==============================================================================
    
    # Register Unity test executables
    add_test(NAME ZOO_Thread_Pool_Basic_Unity_Tests 
             COMMAND test_thread_pool_basic_unity)
    set_tests_properties(ZOO_Thread_Pool_Basic_Unity_Tests PROPERTIES
        LABELS "thread_pool;unity;basic"
        TIMEOUT 60
    )
    
    add_test(NAME ZOO_Thread_Pool_Stress_Unity_Tests 
             COMMAND test_thread_pool_stress_unity)
    set_tests_properties(ZOO_Thread_Pool_Stress_Unity_Tests PROPERTIES
        LABELS "thread_pool;unity;stress"
        TIMEOUT 180
    )
    
    # ==============================================================================
    # CUSTOM TEST TARGETS
    # ==============================================================================
    
    # Quick test target (basic functionality)
    add_custom_target(thread_pool_quick_test
        COMMAND ${CMAKE_CTEST_COMMAND} -L "basic" --output-on-failure
        DEPENDS test_thread_pool_basic_unity
        COMMENT "Running quick thread pool Unity tests"
    )
    
    # All Unity tests target
    add_custom_target(thread_pool_test_all
        COMMAND ${CMAKE_CTEST_COMMAND} -L "unity" --output-on-failure
        DEPENDS test_thread_pool_basic_unity test_thread_pool_stress_unity
        COMMENT "Running all thread pool Unity tests"
    )
    
    message(STATUS "Thread Pool Unity Tests configured:")
    message(STATUS "  Unity framework: ${UNITY_SOURCE_DIR}")
    message(STATUS "  Test executables: test_thread_pool_basic_unity, test_thread_pool_stress_unity")
    message(STATUS "  Platform: ${ZOO_PLATFORM_DIR}")

endif() # CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR ZOO_THREAD_POOL_BUILD_TESTS OR ZOO_ENABLE_TESTS
