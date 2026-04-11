cmake_minimum_required(VERSION 3.16)

get_filename_component(ZOO_REPO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_executable(e2e_performance_benchmark
    e2e_performance_benchmark.c
)

target_include_directories(e2e_performance_benchmark PRIVATE
    "${ZOO_REPO_ROOT_DIR}/include"
    "${ZOO_REPO_ROOT_DIR}/include/smb"
    "${ZOO_REPO_ROOT_DIR}/include/smb/utility"
    "${ZOO_REPO_ROOT_DIR}/include/smb/core"
    "${ZOO_REPO_ROOT_DIR}/include/smb/node"
    "${ZOO_REPO_ROOT_DIR}/include/smb/transport"
    "${ZOO_REPO_ROOT_DIR}/include/smb/qos"
    "${ZOO_REPO_ROOT_DIR}/include/memory_pool"
    "${ZOO_REPO_ROOT_DIR}/include/thread_pool"
    "${ZOO_REPO_ROOT_DIR}/include/buffer"
    "${ZOO_REPO_ROOT_DIR}/include/socket"
    "${ZOO_REPO_ROOT_DIR}/include/log"
    "${ZOO_REPO_ROOT_DIR}/include/util"
    "${ZOO_REPO_ROOT_DIR}/include/platform"
    "${ZOO_REPO_ROOT_DIR}/tests"
    "${ZOO_REPO_ROOT_DIR}/tests/common"
)

target_compile_definitions(e2e_performance_benchmark PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)

target_link_libraries(e2e_performance_benchmark PRIVATE
    zoo_smb_shared
    Threads::Threads
    atomic
)

set(_zoo_benchmark_runtime_paths
    "${ZOO_LIB_OUTPUT_DIR}"
    "${CMAKE_BINARY_DIR}/hidden_shared_libs"
)
list(JOIN _zoo_benchmark_runtime_paths ":" _zoo_benchmark_ld_path)
list(JOIN _zoo_benchmark_runtime_paths ";" _zoo_benchmark_rpath)

set_target_properties(e2e_performance_benchmark PROPERTIES
    BUILD_RPATH "${_zoo_benchmark_rpath}"
)

add_test(NAME e2e_performance_benchmark_smoke COMMAND e2e_performance_benchmark 1)
set_tests_properties(e2e_performance_benchmark_smoke PROPERTIES
    LABELS "benchmark;smb;performance"
    TIMEOUT 180
    ENVIRONMENT "LD_LIBRARY_PATH=${_zoo_benchmark_ld_path}:$ENV{LD_LIBRARY_PATH}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)