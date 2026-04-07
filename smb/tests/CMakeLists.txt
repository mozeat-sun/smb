cmake_minimum_required(VERSION 3.14)
project(ZooSMBTests LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
enable_testing()

get_filename_component(ZOO_SMB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(ZOO_REPO_ROOT_DIR "${ZOO_SMB_ROOT_DIR}/.." ABSOLUTE)

# ----------------------
# Unity Test Framework
# ----------------------
set(UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/third_party/test/unity")
include_directories(${UNITY_ROOT}/src)

# Create Unity library
add_library(unity STATIC
    ${UNITY_ROOT}/src/unity.c
)

target_include_directories(unity PUBLIC
    ${UNITY_ROOT}/src
)

# ----------------------
# Test Configuration
# ----------------------
set(TEST_INCLUDE_DIRS
    ${ZOO_SMB_ROOT_DIR}/inc
    ${ZOO_SMB_ROOT_DIR}/inc/utility
    ${ZOO_SMB_ROOT_DIR}/inc/core
    ${ZOO_SMB_ROOT_DIR}/inc/node
    ${ZOO_SMB_ROOT_DIR}/inc/transport
    ${ZOO_SMB_ROOT_DIR}/inc/qos
    ${ZOO_REPO_ROOT_DIR}/platform/inc
    ${ZOO_REPO_ROOT_DIR}/log/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
    ${CMAKE_CURRENT_SOURCE_DIR}/integration
    ${CMAKE_CURRENT_SOURCE_DIR}/performance
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# Common test libraries
set(TEST_LIBS
    zoo_smb_static
    unity
    pthread
    m
)

# ----------------------
# Test Utilities
# ----------------------
add_library(test_utils STATIC
    common/test_utils.c
)

target_include_directories(test_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${TEST_INCLUDE_DIRS}
)

target_compile_definitions(test_utils PRIVATE _GNU_SOURCE _POSIX_C_SOURCE=200809L)
target_compile_options(test_utils PRIVATE -Wno-unused-parameter)

target_link_libraries(test_utils
    zoo_smb_static
    pthread
    m
)

# ----------------------
# Unit Tests
# ----------------------
file(GLOB UNIT_TEST_SOURCES "unit/test_*.c")

list(REMOVE_ITEM UNIT_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/test_zoo_smb_qos.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/test_zoo_smb_reactor.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/test_zoo_smb_routing_rule.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/test_zoo_smb_rule_manager.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/test_zoo_smb_runtime_ha.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/test_zoo_smb_service_manager.c"
)

foreach(test_source ${UNIT_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    
    target_include_directories(${test_name} PRIVATE ${TEST_INCLUDE_DIRS})
    target_compile_definitions(${test_name} PRIVATE _GNU_SOURCE _POSIX_C_SOURCE=200809L)
    target_link_libraries(${test_name} ${TEST_LIBS} test_utils)
    
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 30)
endforeach()

# ----------------------
# Integration Tests
# ----------------------
file(GLOB INTEGRATION_TEST_SOURCES "integration/test_*.c")

foreach(test_source ${INTEGRATION_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    
    target_include_directories(${test_name} PRIVATE ${TEST_INCLUDE_DIRS})
    target_compile_definitions(${test_name} PRIVATE _GNU_SOURCE _POSIX_C_SOURCE=200809L)
    target_link_libraries(${test_name} ${TEST_LIBS} test_utils)
    
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()

# ----------------------
# Performance Tests  
# ----------------------
file(GLOB PERFORMANCE_TEST_SOURCES "performance/test_*.c")

foreach(test_source ${PERFORMANCE_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    
    target_include_directories(${test_name} PRIVATE ${TEST_INCLUDE_DIRS})
    target_compile_definitions(${test_name} PRIVATE _GNU_SOURCE _POSIX_C_SOURCE=200809L)
    target_link_libraries(${test_name} ${TEST_LIBS} test_utils)
    
    # Performance tests are not added to CTest by default
    # add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

message(STATUS "Unity test framework configured")
message(STATUS "Unit tests: ${UNIT_TEST_SOURCES}")
message(STATUS "Integration tests: ${INTEGRATION_TEST_SOURCES}")
message(STATUS "Performance tests: ${PERFORMANCE_TEST_SOURCES}")