cmake_minimum_required(VERSION 3.16)

project(ZooSmbNodeUnitTests C)

enable_testing()

option(ZOO_ENABLE_RUNTIME_ASSURANCE_TEST
    "Enable runtime assurance unit test in CTest"
    ON)

if(NOT Unity_FOUND)
    message(WARNING "Unity testing framework not found. SMB node unit tests are skipped.")
    return()
endif()

add_executable(test_zoo_smb_subscription_session_manager_unit
    test_zoo_smb_subscription_session_manager_unit.c
    ${Unity_SOURCE}
)

add_executable(test_zoo_assurance_domain_unit
    test_zoo_assurance_domain_unit.c
    ${Unity_SOURCE}
)

add_executable(test_zoo_smb_runtime_assurance_unit
    test_zoo_smb_runtime_assurance_unit.c
    ${Unity_SOURCE}
)

target_include_directories(test_zoo_smb_subscription_session_manager_unit PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/kernal"
    "${CMAKE_SOURCE_DIR}/include/kernal/utility"
    "${CMAKE_SOURCE_DIR}/include/kernal/core"
    "${CMAKE_SOURCE_DIR}/include/kernal/node"
    "${CMAKE_SOURCE_DIR}/include/kernal/transport"
    "${CMAKE_SOURCE_DIR}/include/kernal/qos"
    "${CMAKE_SOURCE_DIR}/include/platform/memory_pool"
    "${CMAKE_SOURCE_DIR}/include/platform/thread_pool"
    "${CMAKE_SOURCE_DIR}/include/platform/buffer"
    "${CMAKE_SOURCE_DIR}/include/platform/socket"
    "${CMAKE_SOURCE_DIR}/include/platform/log"
    "${CMAKE_SOURCE_DIR}/include/platform/util"
    "${CMAKE_SOURCE_DIR}/include/platform"
    "${Unity_INCLUDE_DIR}"
)

target_include_directories(test_zoo_assurance_domain_unit PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/kernal"
    "${CMAKE_SOURCE_DIR}/include/kernal/utility"
    "${CMAKE_SOURCE_DIR}/include/kernal/core"
    "${CMAKE_SOURCE_DIR}/include/domain"
    "${CMAKE_SOURCE_DIR}/include/assurance"
    "${Unity_INCLUDE_DIR}"
)

target_include_directories(test_zoo_smb_runtime_assurance_unit PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/kernal"
    "${CMAKE_SOURCE_DIR}/include/kernal/utility"
    "${CMAKE_SOURCE_DIR}/include/kernal/core"
    "${CMAKE_SOURCE_DIR}/include/domain"
    "${CMAKE_SOURCE_DIR}/include/assurance"
    "${Unity_INCLUDE_DIR}"
)

target_compile_definitions(test_zoo_smb_subscription_session_manager_unit PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)

target_compile_definitions(test_zoo_assurance_domain_unit PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)

target_compile_definitions(test_zoo_smb_runtime_assurance_unit PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)

target_link_libraries(test_zoo_smb_subscription_session_manager_unit PRIVATE
    zoo_smb_shared
    Threads::Threads
    atomic
)

target_link_libraries(test_zoo_assurance_domain_unit PRIVATE
    zoo_assurance_static
    zoo_domain_static
    Threads::Threads
    atomic
)

target_link_libraries(test_zoo_smb_runtime_assurance_unit PRIVATE
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

set_target_properties(test_zoo_assurance_domain_unit PROPERTIES
    BUILD_RPATH "${_zoo_smb_unit_rpath}"
)

set_target_properties(test_zoo_smb_runtime_assurance_unit PROPERTIES
    BUILD_RPATH "${_zoo_smb_unit_rpath}"
)

add_test(NAME test_zoo_smb_subscription_session_manager_unit
    COMMAND test_zoo_smb_subscription_session_manager_unit
)
add_test(NAME test_zoo_assurance_domain_unit
    COMMAND test_zoo_assurance_domain_unit
)
set_tests_properties(test_zoo_smb_subscription_session_manager_unit PROPERTIES
    LABELS "unit;smb;node"
    TIMEOUT 120
    ENVIRONMENT "LD_LIBRARY_PATH=${_zoo_smb_unit_ld_path}:$ENV{LD_LIBRARY_PATH}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

set_tests_properties(test_zoo_assurance_domain_unit PROPERTIES
    LABELS "unit;smb;assurance;domain"
    TIMEOUT 120
    ENVIRONMENT "LD_LIBRARY_PATH=${_zoo_smb_unit_ld_path}:$ENV{LD_LIBRARY_PATH}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(ZOO_ENABLE_RUNTIME_ASSURANCE_TEST)
    add_test(NAME test_zoo_smb_runtime_assurance_unit
        COMMAND test_zoo_smb_runtime_assurance_unit
    )
    set_tests_properties(test_zoo_smb_runtime_assurance_unit PROPERTIES
        LABELS "unit;smb;runtime;assurance"
        TIMEOUT 120
        ENVIRONMENT "LD_LIBRARY_PATH=${_zoo_smb_unit_ld_path}:$ENV{LD_LIBRARY_PATH}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()