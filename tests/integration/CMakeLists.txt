cmake_minimum_required(VERSION 3.16)

enable_testing()

get_filename_component(ZOO_REPO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

set(UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/thirdparty/unity")
if(NOT EXISTS "${UNITY_ROOT}")
    set(UNITY_ROOT "${ZOO_REPO_ROOT_DIR}/thirdparty/test/unity")
endif()
set(UNITY_SRC_DIR "${UNITY_ROOT}/src")

if(NOT EXISTS "${UNITY_SRC_DIR}/unity.c")
    message(WARNING "Unity not found for integration tests; expected ${UNITY_SRC_DIR}/unity.c")
    return()
endif()

add_library(zoo_integration_unity STATIC "${UNITY_SRC_DIR}/unity.c")
target_include_directories(zoo_integration_unity PUBLIC "${UNITY_SRC_DIR}")

function(zoo_add_smb_integration_test target_name source_file)
    add_executable(${target_name} "${source_file}")

    target_include_directories(${target_name} PRIVATE
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
        "${ZOO_REPO_ROOT_DIR}/tests/integration"
        "${UNITY_SRC_DIR}"
    )

    target_compile_definitions(${target_name} PRIVATE
        _GNU_SOURCE
        _POSIX_C_SOURCE=200809L
    )

    target_link_libraries(${target_name} PRIVATE
        zoo_smb_shared
        zoo_integration_unity
        Threads::Threads
        atomic
    )

    add_test(NAME ${target_name} COMMAND ${target_name})
    set_tests_properties(${target_name} PROPERTIES
        LABELS "integration;smb"
        TIMEOUT 120
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endfunction()

zoo_add_smb_integration_test(test_zoo_smb_integration smb/test_zoo_smb_integration.c)
zoo_add_smb_integration_test(test_service_discovery smb/test_service_discovery.c)
zoo_add_smb_integration_test(test_backpressure_security smb/test_backpressure_security.c)
zoo_add_smb_integration_test(test_zoo_smb_transport_udp smb/test_zoo_smb_transport_udp.c)
zoo_add_smb_integration_test(test_zoo_smb_transport_shm smb/test_zoo_smb_transport_shm.c)

target_sources(test_zoo_smb_transport_shm PRIVATE
    "${ZOO_REPO_ROOT_DIR}/tests/common/test_utils.c"
)
