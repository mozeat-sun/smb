cmake_minimum_required(VERSION 3.16)

project(ZooExamples C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

get_filename_component(ZOO_REPO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(EXAMPLES_INCLUDE_DIRS
    ${ZOO_REPO_ROOT_DIR}/include/smb
    ${ZOO_REPO_ROOT_DIR}/include/smb/core
    ${ZOO_REPO_ROOT_DIR}/include/smb/node
    ${ZOO_REPO_ROOT_DIR}/include/smb/qos
    ${ZOO_REPO_ROOT_DIR}/include/smb/transport
    ${ZOO_REPO_ROOT_DIR}/include/smb/utility
    ${ZOO_REPO_ROOT_DIR}/include/log
    ${ZOO_REPO_ROOT_DIR}/include/memory_pool
    ${ZOO_REPO_ROOT_DIR}/include/thread_pool
    ${ZOO_REPO_ROOT_DIR}/include/util
    ${ZOO_REPO_ROOT_DIR}/include/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

set(EXAMPLES_COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/parser.c
)

if(TARGET zoo_smb_static)
    set(ZOO_EXAMPLE_LINK_TARGET zoo_smb_static)
elseif(TARGET zoo_smb_shared)
    set(ZOO_EXAMPLE_LINK_TARGET zoo_smb_shared)
else()
    message(STATUS "ZOO examples skipped: zoo_smb targets are not available")
    return()
endif()

function(zoo_configure_example target_name)
    target_include_directories(${target_name} PRIVATE ${EXAMPLES_INCLUDE_DIRS})
    target_link_libraries(${target_name} PRIVATE ${ZOO_EXAMPLE_LINK_TARGET})
    if(TARGET zoo_util_static)
        target_link_libraries(${target_name} PRIVATE zoo_util_static)
    endif()
    if(TARGET zoo_log_static)
        target_link_libraries(${target_name} PRIVATE zoo_log_static)
    endif()
    if(TARGET zoo_platform_static)
        target_link_libraries(${target_name} PRIVATE zoo_platform_static)
    endif()
    target_compile_definitions(${target_name} PRIVATE
        _GNU_SOURCE
        _POSIX_C_SOURCE=200809L
    )
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE -Wno-pedantic)
    endif()
endfunction()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/client_server/client.c")
    add_executable(zoo_example_client
        ${CMAKE_CURRENT_SOURCE_DIR}/client_server/client.c
        ${EXAMPLES_COMMON_SOURCES}
    )
    zoo_configure_example(zoo_example_client)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/client_server/server.c")
    add_executable(zoo_example_server
        ${CMAKE_CURRENT_SOURCE_DIR}/client_server/server.c
        ${EXAMPLES_COMMON_SOURCES}
    )
    zoo_configure_example(zoo_example_server)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/client_server/client_udp.c")
    add_executable(zoo_example_udp_client
        ${CMAKE_CURRENT_SOURCE_DIR}/client_server/client_udp.c
        ${EXAMPLES_COMMON_SOURCES}
    )
    zoo_configure_example(zoo_example_udp_client)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/client_server/server_udp.c")
    add_executable(zoo_example_udp_server
        ${CMAKE_CURRENT_SOURCE_DIR}/client_server/server_udp.c
        ${EXAMPLES_COMMON_SOURCES}
    )
    zoo_configure_example(zoo_example_udp_server)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pub_sub/publisher.c")
    add_executable(zoo_example_publisher
        ${CMAKE_CURRENT_SOURCE_DIR}/pub_sub/publisher.c
    )
    zoo_configure_example(zoo_example_publisher)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pub_sub/subscriber.c")
    add_executable(zoo_example_subscriber
        ${CMAKE_CURRENT_SOURCE_DIR}/pub_sub/subscriber.c
    )
    zoo_configure_example(zoo_example_subscriber)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/transport_shm/server.c")
    add_executable(zoo_example_shm_server
        ${CMAKE_CURRENT_SOURCE_DIR}/transport_shm/server.c
    )
    zoo_configure_example(zoo_example_shm_server)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/transport_shm/client.c")
    add_executable(zoo_example_shm_client
        ${CMAKE_CURRENT_SOURCE_DIR}/transport_shm/client.c
    )
    zoo_configure_example(zoo_example_shm_client)
endif()
