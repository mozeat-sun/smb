cmake_minimum_required(VERSION 3.14)

# === DEBUG: Configuring ZooMemoryPoolTests ===
message(STATUS "=== DEBUG: Configuring ZooMemoryPoolTests ===")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

get_filename_component(ZOO_MEMORY_POOL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(ZOO_REPO_ROOT_DIR "${ZOO_MEMORY_POOL_ROOT_DIR}/.." ABSOLUTE)

# Unity Framework Path (relative to memory_pool directory)
set(UNITY_DIR "${ZOO_REPO_ROOT_DIR}/thirdparty/test/unity")
set(CMOCK_DIR "${ZOO_REPO_ROOT_DIR}/thirdparty/test/CMock/src")
message(STATUS "Looking for Unity at: ${UNITY_DIR}")
message(STATUS "Looking for CMock at: ${CMOCK_DIR}")

# Check if Unity and CMock exist
if(EXISTS "${UNITY_DIR}" AND EXISTS "${CMOCK_DIR}")
    message(STATUS "Found Unity: ${UNITY_DIR}")
    message(STATUS "Found CMock: ${CMOCK_DIR}")
    
    # Include Unity and CMock
    include_directories("${UNITY_DIR}")
    include_directories("${UNITY_DIR}/src")
    include_directories("${CMOCK_DIR}")
    
    # Add Unity and CMock sources
    set(UNITY_SOURCES
        "${UNITY_DIR}/src/unity.c"
        "${CMOCK_DIR}/cmock.c"
    )
    
    # Test sources
    set(TEST_SOURCES
        test_zoo_memory_pool.c
        test_runner.c
        ${UNITY_SOURCES}
    )
    
    # Create test executable
    add_executable(zoo_memory_pool_tests ${TEST_SOURCES})
    
    # Link against memory pool library and required system libraries
    target_link_libraries(zoo_memory_pool_tests 
        zoo_memory_pool_static
        pthread
        m
    )
    
    # Include directories for the test
    target_include_directories(zoo_memory_pool_tests PRIVATE
        ${ZOO_REPO_ROOT_DIR}/include/memory_pool
        ${ZOO_REPO_ROOT_DIR}/include/platform
        ${ZOO_REPO_ROOT_DIR}/util
        ${UNITY_DIR}
        ${UNITY_DIR}/src
        ${CMOCK_DIR}
    )
    
    # Compiler flags for tests
    target_compile_options(zoo_memory_pool_tests PRIVATE
        -Wall
        -Wextra
        -Wno-missing-prototypes
        -std=c99
        -pthread
    )
    
    # Unity and CMock configuration definitions
    target_compile_definitions(zoo_memory_pool_tests PRIVATE
        UNITY_INCLUDE_CONFIG_H=0
        UNITY_EXCLUDE_SETJMP_H=1
        CMOCK_MEM_DYNAMIC=1
    )
    
    # Set output directory
    set_target_properties(zoo_memory_pool_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
    
    # Add test targets for CTest integration
    enable_testing()
    add_test(NAME zoo_memory_pool_unit_tests COMMAND zoo_memory_pool_tests)
    
    # Custom test targets
    add_custom_target(memory_pool_quick_test
        COMMAND $<TARGET_FILE:zoo_memory_pool_tests>
        DEPENDS zoo_memory_pool_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running ZOO Memory Pool quick tests"
    )
    
    add_custom_target(memory_pool_test_verbose
        COMMAND $<TARGET_FILE:zoo_memory_pool_tests> --verbose
        DEPENDS zoo_memory_pool_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running ZOO Memory Pool tests with verbose output"
    )
    
    # Configuration summary
    message(STATUS "")
    message(STATUS "==============================================================================")
    message(STATUS "ZOO Memory Pool Tests Configuration Summary")
    message(STATUS "==============================================================================")
    message(STATUS "Test Framework: Unity C Testing Framework + CMock")
    message(STATUS "Unity Location: ${UNITY_DIR}")
    message(STATUS "CMock Location: ${CMOCK_DIR}")
    message(STATUS "Test Executable: zoo_memory_pool_tests")
    message(STATUS "Test Categories: Core, Advanced, Thread Safety")
    message(STATUS "Custom Targets: memory_pool_quick_test, memory_pool_test_verbose")
    message(STATUS "Coverage: Disabled")
    message(STATUS "Run Tests: ./zoo_memory_pool_tests or ctest")
    message(STATUS "Run Quick Tests: make memory_pool_quick_test")
    message(STATUS "==============================================================================")
    message(STATUS "")
    
else()
    message(WARNING "Unity framework or CMock not found")
    message(WARNING "Unity expected at: ${UNITY_DIR}")
    message(WARNING "CMock expected at: ${CMOCK_DIR}")
    message(WARNING "Tests will not be built. Please ensure Unity and CMock are available.")
endif()
