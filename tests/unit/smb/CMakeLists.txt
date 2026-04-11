cmake_minimum_required(VERSION 3.16)

project(ZooSmbNodeUnitTests C)

enable_testing()

if(NOT Unity_FOUND)
    message(WARNING "Unity testing framework not found. SMB node unit tests are skipped.")
    return()
endif()

add_executable(test_zoo_smb_subscription_session_manager_unit
    test_zoo_smb_subscription_session_manager_unit.c
    ${Unity_SOURCE}
)

target_include_directories(test_zoo_smb_subscription_session_manager_unit PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/smb"
    "${CMAKE_SOURCE_DIR}/include/smb/utility"
    "${CMAKE_SOURCE_DIR}/include/smb/core"
    "${CMAKE_SOURCE_DIR}/include/smb/node"
    "${CMAKE_SOURCE_DIR}/include/smb/transport"
    "${CMAKE_SOURCE_DIR}/include/smb/qos"
    "${CMAKE_SOURCE_DIR}/include/memory_pool"
    "${CMAKE_SOURCE_DIR}/include/thread_pool"
    "${CMAKE_SOURCE_DIR}/include/buffer"
    "${CMAKE_SOURCE_DIR}/include/socket"
    "${CMAKE_SOURCE_DIR}/include/log"
    "${CMAKE_SOURCE_DIR}/include/util"
    "${CMAKE_SOURCE_DIR}/include/platform"
    "${Unity_INCLUDE_DIR}"
)

target_compile_definitions(test_zoo_smb_subscription_session_manager_unit PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)

target_link_libraries(test_zoo_smb_subscription_session_manager_unit PRIVATE
    zoo_smb_shared
    Threads::Threads
    atomic
)

set(_zoo_smb_unit_runtime_paths
    "${ZOO_LIB_OUTPUT_DIR}"
    "${CMAKE_BINARY_DIR}/hidden_shared_libs"
)
list(JOIN _zoo_smb_unit_runtime_paths ":" _zoo_smb_unit_ld_path)
list(JOIN _zoo_smb_unit_runtime_paths ";" _zoo_smb_unit_rpath)

set_target_properties(test_zoo_smb_subscription_session_manager_unit PROPERTIES
    BUILD_RPATH "${_zoo_smb_unit_rpath}"
)

add_test(NAME test_zoo_smb_subscription_session_manager_unit
    COMMAND test_zoo_smb_subscription_session_manager_unit
)
set_tests_properties(test_zoo_smb_subscription_session_manager_unit PROPERTIES
    LABELS "unit;smb;node"
    TIMEOUT 120
    ENVIRONMENT "LD_LIBRARY_PATH=${_zoo_smb_unit_ld_path}:$ENV{LD_LIBRARY_PATH}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)