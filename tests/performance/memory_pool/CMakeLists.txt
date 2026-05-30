cmake_minimum_required(VERSION 3.16)

add_executable(test_memory_pool_performance
    test_memory_pool_performance.c
)

target_include_directories(test_memory_pool_performance PRIVATE
    "${ZOO_REPO_ROOT_DIR}/include"
    "${ZOO_REPO_ROOT_DIR}/include/platform"
    "${ZOO_REPO_ROOT_DIR}/include/platform/memory_pool"
    "${UNITY_SRC_DIR}"
)

target_compile_definitions(test_memory_pool_performance PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)

target_link_libraries(test_memory_pool_performance PRIVATE
    zoo_memory_pool_shared
    zoo_performance_unity
    Threads::Threads
)

set(_zoo_perf_runtime_paths
    "${ZOO_LIB_OUTPUT_DIR}"
    "${CMAKE_BINARY_DIR}/hidden_shared_libs"
)
list(JOIN _zoo_perf_runtime_paths ":" _zoo_perf_ld_path)
list(JOIN _zoo_perf_runtime_paths ";" _zoo_perf_rpath)

set_target_properties(test_memory_pool_performance PROPERTIES
    BUILD_RPATH "${_zoo_perf_rpath}"
)

add_test(NAME test_memory_pool_performance COMMAND test_memory_pool_performance)
set_tests_properties(test_memory_pool_performance PROPERTIES
    LABELS "performance;memory_pool;platform"
    TIMEOUT 180
    ENVIRONMENT "LD_LIBRARY_PATH=${_zoo_perf_ld_path}:$ENV{LD_LIBRARY_PATH}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
