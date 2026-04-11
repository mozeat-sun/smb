cmake_minimum_required(VERSION 3.16)

project(ZooFuzzTests C)

enable_testing()

add_executable(fuzz_zoo_smb_protocol
    fuzz_zoo_smb_protocol.c
)

target_include_directories(fuzz_zoo_smb_protocol PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/smb"
    "${CMAKE_SOURCE_DIR}/include/smb/core"
    "${CMAKE_SOURCE_DIR}/include/smb/transport"
    "${CMAKE_SOURCE_DIR}/include/smb/utility"
    "${CMAKE_SOURCE_DIR}/include/memory_pool"
    "${CMAKE_SOURCE_DIR}/include/log"
    "${CMAKE_SOURCE_DIR}/include/util"
    "${CMAKE_SOURCE_DIR}/include/platform"
)

target_compile_definitions(fuzz_zoo_smb_protocol PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)

target_link_libraries(fuzz_zoo_smb_protocol PRIVATE
    zoo_smb_shared
    Threads::Threads
    atomic
)

set(_zoo_fuzz_runtime_paths
    "${ZOO_LIB_OUTPUT_DIR}"
    "${CMAKE_BINARY_DIR}/hidden_shared_libs"
)
list(JOIN _zoo_fuzz_runtime_paths ":" _zoo_fuzz_ld_path)
list(JOIN _zoo_fuzz_runtime_paths ";" _zoo_fuzz_rpath)

set_target_properties(fuzz_zoo_smb_protocol PROPERTIES
    BUILD_RPATH "${_zoo_fuzz_rpath}"
)

add_test(NAME fuzz_zoo_smb_protocol COMMAND fuzz_zoo_smb_protocol)
set_tests_properties(fuzz_zoo_smb_protocol PROPERTIES
    LABELS "fuzz;smb;protocol"
    TIMEOUT 120
    ENVIRONMENT "LD_LIBRARY_PATH=${_zoo_fuzz_ld_path}:$ENV{LD_LIBRARY_PATH}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)