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

# ==============================================================================
# Helper function to reduce boilerplate for SMB unit test targets
# ==============================================================================
function(zoo_add_smb_unit_test target_name source_file)
    add_executable(${target_name}
        ${source_file}
        test_smb_noautoinit.c
        ${Unity_SOURCE}
    )

    target_include_directories(${target_name} PRIVATE
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
        "${CMAKE_SOURCE_DIR}/include/domain"
        "${CMAKE_SOURCE_DIR}/include/assurance"
        "${CMAKE_SOURCE_DIR}/tests/unit/smb"
        "${Unity_INCLUDE_DIR}"
    )

    target_compile_definitions(${target_name} PRIVATE
        _GNU_SOURCE
        _POSIX_C_SOURCE=200809L
    )

    target_link_libraries(${target_name} PRIVATE
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

    set_target_properties(${target_name} PROPERTIES
        BUILD_RPATH "${_zoo_smb_unit_rpath}"
    )

    add_test(NAME ${target_name} COMMAND ${target_name})
    set_tests_properties(${target_name} PROPERTIES
        LABELS "unit;smb"
        TIMEOUT 120
        ENVIRONMENT "LD_LIBRARY_PATH=${_zoo_smb_unit_ld_path}:$ENV{LD_LIBRARY_PATH}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endfunction()

# ==============================================================================
# Session manager, assurance, and runtime assurance (existing registered tests)
# ==============================================================================
add_executable(test_zoo_smb_subscription_session_manager_unit
    test_zoo_smb_subscription_session_manager_unit.c
    test_smb_noautoinit.c
    ${Unity_SOURCE}
)

add_executable(test_zoo_assurance_domain_unit
    test_zoo_assurance_domain_unit.c
    test_smb_noautoinit.c
    ${Unity_SOURCE}
)

add_executable(test_zoo_smb_runtime_assurance_unit
    test_zoo_smb_runtime_assurance_unit.c
    test_smb_noautoinit.c
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

# ==============================================================================
# Previously orphaned tests — now registered (14 test executables, 81 cases)
# ==============================================================================

# Message create/destroy, copy, header validation
zoo_add_smb_unit_test(test_zoo_smb_message_unit test_zoo_smb_message.c)

# QoS entity and policy creation/validation
zoo_add_smb_unit_test(test_zoo_smb_qos_unit test_zoo_smb_qos.c)

# QoS context (request tracking, match results, message status)
zoo_add_smb_unit_test(test_zoo_smb_qos_ctx_unit test_zoo_smb_qos_ctx.c)

# Routing rule manager (rule CRUD, observer management)
zoo_add_smb_unit_test(test_zoo_smb_rule_manager_unit test_zoo_smb_rule_manager.c)

# Ring buffer create/destroy, read/write, boundary conditions
zoo_add_smb_unit_test(test_zoo_smb_ring_buffer_unit test_zoo_smb_ring_buffer.c)

# Protocol: serialization, deserialization, header validation, discovery payload
zoo_add_smb_unit_test(test_zoo_smb_protocol_unit test_zoo_smb_protocol.c)

# Consumer (transport endpoint) create/destroy, find, list management
zoo_add_smb_unit_test(test_zoo_smb_consumer_unit test_zoo_smb_consumer.c)

# Transport observer registration and notification
zoo_add_smb_unit_test(test_zoo_smb_transport_observer_unit test_zoo_smb_transport_observer.c)

# Port manager: find available port, release port resources
zoo_add_smb_unit_test(test_port_manager_unit test_port_manager.c)

# Routing rule create/destroy, get name, observer attachment
zoo_add_smb_unit_test(test_zoo_smb_routing_rule_unit test_zoo_smb_routing_rule.c)

# Service create/destroy, duplicate detection
zoo_add_smb_unit_test(test_zoo_smb_service_unit test_zoo_smb_service.c)

# Service manager: register, unregister, observer, lookup
zoo_add_smb_unit_test(test_zoo_smb_service_manager_unit test_zoo_smb_service_manager.c)

# Error code construction and string conversion
zoo_add_smb_unit_test(test_error_unit test_error.c)

# Dispatcher integration with SMB
zoo_add_smb_unit_test(test_zoo_dispatcher_smb_unit test_zoo_dispatcher_smb.c)
