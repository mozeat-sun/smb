cmake_minimum_required(VERSION 3.16)

enable_testing()

get_filename_component(ZOO_REPO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

set(UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/thirdparty/unity")
if(NOT EXISTS "${UNITY_ROOT}")
    set(UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/thirdparty/test/unity")
endif()
set(UNITY_SRC_DIR "${UNITY_ROOT}/src")

if(NOT EXISTS "${UNITY_SRC_DIR}/unity.c")
    message(WARNING "Unity not found for performance tests; expected ${UNITY_SRC_DIR}/unity.c")
    return()
endif()

add_library(zoo_performance_unity STATIC "${UNITY_SRC_DIR}/unity.c")
target_include_directories(zoo_performance_unity PUBLIC "${UNITY_SRC_DIR}")

add_subdirectory(memory_pool)
