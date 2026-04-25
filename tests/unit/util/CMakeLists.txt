# CMakeLists.txt for ZOO Util Unity Tests
cmake_minimum_required(VERSION 3.10)

# Enable testing
enable_testing()

get_filename_component(ZOO_UTIL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(ZOO_REPO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

# Unity Test Framework paths
set(UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/thirdparty/unity")
if(NOT EXISTS "${UNITY_ROOT}")
    set(UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/thirdparty/test/unity")
endif()
set(UNITY_SRC_DIR "${UNITY_ROOT}/src")

# Include directories
include_directories(${ZOO_REPO_ROOT_DIR}/include/platform/util)
include_directories(${ZOO_REPO_ROOT_DIR}/include/platform)
include_directories(${UNITY_SRC_DIR})

# Unity source files
set(UNITY_SOURCES
    ${UNITY_SRC_DIR}/unity.c
)

# Create Unity library
add_library(util_unity STATIC ${UNITY_SOURCES})
target_include_directories(util_unity PUBLIC ${UNITY_SRC_DIR})

# Individual test executables
add_executable(test_timestamp_unity
    test_zoo_util_timestamp_unity.c
    ${UNITY_SOURCES}
)

add_executable(test_string_unity
    test_zoo_util_string_unity.c
    ${UNITY_SOURCES}
)

add_executable(test_memory_unity
    test_zoo_util_memory_unity.c
    ${UNITY_SOURCES}
)

add_executable(test_math_unity
    test_zoo_util_math_unity.c
    ${UNITY_SOURCES}
)

# Set properties for all test executables
set(TEST_TARGETS 
    test_timestamp_unity
    test_string_unity
    test_memory_unity
    test_math_unity
)

foreach(TARGET ${TEST_TARGETS})
    # Link libraries
    target_link_libraries(${TARGET}
        PRIVATE
        zoo_util_static
        m  # Math library
    )
    
    # Compiler flags
    target_compile_options(${TARGET} PRIVATE
        -Wall
        -Wextra
        -Wno-missing-prototypes
        -std=c11
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O2>
    )

    target_compile_definitions(${TARGET} PRIVATE
        UNITY_INCLUDE_DOUBLE
        UNITY_SUPPORT_64
    )

    target_compile_definitions(${TARGET} PRIVATE
        UNITY_INCLUDE_DOUBLE
    )
    
    # Set language explicitly
    set_target_properties(${TARGET} PROPERTIES
        LINKER_LANGUAGE C
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
    )
endforeach()

# Add tests to CTest
add_test(NAME TimestampTests COMMAND test_timestamp_unity)
add_test(NAME StringTests COMMAND test_string_unity)
add_test(NAME MemoryTests COMMAND test_memory_unity)
add_test(NAME MathTests COMMAND test_math_unity)

# Set test properties
set_tests_properties(TimestampTests StringTests MemoryTests MathTests
    PROPERTIES
    LABELS "unit;util;core"
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Custom target to run all Unity tests
add_custom_target(run_unity_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS ${TEST_TARGETS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all Unity tests"
)

# Individual test run targets
add_custom_target(run_timestamp_tests
    COMMAND test_timestamp_unity
    DEPENDS test_timestamp_unity
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running timestamp tests"
)

add_custom_target(run_string_tests
    COMMAND test_string_unity
    DEPENDS test_string_unity
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running string tests"
)

add_custom_target(run_memory_tests
    COMMAND test_memory_unity
    DEPENDS test_memory_unity
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running memory tests"
)

add_custom_target(run_math_tests
    COMMAND test_math_unity
    DEPENDS test_math_unity
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running math tests"
)

message(STATUS "Unity Test Framework configured")
message(STATUS "Unity source: ${UNITY_SRC_DIR}")
message(STATUS "Test targets: ${TEST_TARGETS}")
