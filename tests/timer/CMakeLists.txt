# ==============================================================================
# ZOO Timer Module Tests CMakeLists.txt
# Unity and CMock testing framework
# ==============================================================================

cmake_minimum_required(VERSION 3.15)
project(zoo_timer_tests VERSION 1.0.0 LANGUAGES C)

get_filename_component(ZOO_TIMER_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(ZOO_REPO_ROOT_DIR "${ZOO_TIMER_ROOT_DIR}/.." ABSOLUTE)

# ==============================================================================
# Test Configuration
# ==============================================================================

# Enable testing
enable_testing()

# Set test binary directory
set(TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${TEST_BINARY_DIR})

# ==============================================================================
# Unity Test Framework Setup
# ==============================================================================

# Add Unity source files
set(UNITY_ROOT ${ZOO_REPO_ROOT_DIR}/thirdparty/test/unity)
if(EXISTS ${UNITY_ROOT})
    set(UNITY_SOURCES
        ${UNITY_ROOT}/src/unity.c
    )
    
    set(UNITY_INCLUDES
        ${UNITY_ROOT}/src
    )
    
    message(STATUS "Unity framework found at: ${UNITY_ROOT}")
else()
    message(FATAL_ERROR "Unity framework not found. Please ensure Unity is in thirdparty/test/unity")
endif()

# ==============================================================================
# CMock Setup
# ==============================================================================

set(CMOCK_ROOT ${ZOO_REPO_ROOT_DIR}/thirdparty/test/CMock)
if(EXISTS ${CMOCK_ROOT})
    set(CMOCK_SOURCES
        ${CMOCK_ROOT}/src/cmock.c
    )
    
    set(CMOCK_INCLUDES
        ${CMOCK_ROOT}/src
    )
    
    message(STATUS "CMock framework found at: ${CMOCK_ROOT}")
else()
    message(STATUS "CMock framework not found. Mock generation will be manual.")
endif()

# ==============================================================================
# Test Include Directories
# ==============================================================================

include_directories(
    ${ZOO_REPO_ROOT_DIR}/include/timer
    ${ZOO_REPO_ROOT_DIR}/src/timer
    ${ZOO_REPO_ROOT_DIR}/include/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${UNITY_INCLUDES}
    ${CMOCK_INCLUDES}
)

# ==============================================================================
# Test Compiler Flags
# ==============================================================================

# Add test-specific compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUNIT_TEST")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")  # Debug symbols, no optimization
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# ==============================================================================
# Mock Generation (Manual for now)
# ==============================================================================

# Create mock files for platform-specific functions
set(MOCK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_zoo_platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_stdlib.c
)

# ==============================================================================
# Test Source Files
# ==============================================================================

set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_zoo_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_zoo_timer_platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_runner.c
)

# ==============================================================================
# Source Under Test
# ==============================================================================

set(SOURCE_UNDER_TEST
    ${ZOO_REPO_ROOT_DIR}/src/timer/zoo_timer.c
)

# ==============================================================================
# Test Executable
# ==============================================================================

add_executable(zoo_timer_tests
    ${TEST_SOURCES}
    ${SOURCE_UNDER_TEST}
    ${UNITY_SOURCES}
    ${CMOCK_SOURCES}
    ${MOCK_SOURCES}
)

target_compile_options(zoo_timer_tests PRIVATE -Wno-missing-prototypes)

# Link libraries
target_link_libraries(zoo_timer_tests
    pthread
)

# ==============================================================================
# Test Registration
# ==============================================================================

add_test(
    NAME zoo_timer_unit_tests
    COMMAND zoo_timer_tests
    WORKING_DIRECTORY ${TEST_BINARY_DIR}
)

# ==============================================================================
# Coverage Target
# ==============================================================================

find_program(GCOV_EXECUTABLE gcov)
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(GCOV_EXECUTABLE AND LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    add_custom_target(timer_coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory coverage
        COMMAND ${LCOV_EXECUTABLE} --capture --directory . --output-file coverage/coverage.info
        COMMAND ${LCOV_EXECUTABLE} --remove coverage/coverage.info '/usr/*' --output-file coverage/coverage.info
        COMMAND ${LCOV_EXECUTABLE} --remove coverage/coverage.info '*/thirdparty/*' --output-file coverage/coverage.info
        COMMAND ${LCOV_EXECUTABLE} --remove coverage/coverage.info '*/tests/*' --output-file coverage/coverage.info
        COMMAND ${GENHTML_EXECUTABLE} coverage/coverage.info --output-directory coverage/html
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating code coverage report"
    )
    
    message(STATUS "Coverage reporting enabled")
else()
    message(STATUS "Coverage tools not found. Coverage reporting disabled.")
endif()

# ==============================================================================
# Custom Test Targets
# ==============================================================================

# Clean test artifacts
add_custom_target(timer_test_clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/coverage
    COMMAND find ${CMAKE_CURRENT_BINARY_DIR} -name "*.gcda" -delete
    COMMAND find ${CMAKE_CURRENT_BINARY_DIR} -name "*.gcno" -delete
    COMMENT "Cleaning test artifacts"
)

# Run tests with verbose output
add_custom_target(timer_test_verbose
    COMMAND zoo_timer_tests -v
    DEPENDS zoo_timer_tests
    WORKING_DIRECTORY ${TEST_BINARY_DIR}
    COMMENT "Running tests with verbose output"
)
