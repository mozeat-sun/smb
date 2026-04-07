# Collect all source files recursively, but exclude bridge sources

option(ZOO_SMB_AUTO_INIT "Enable SMB auto init" ON)
if(ZOO_SMB_AUTO_INIT)
    add_compile_definitions(ZOO_SMB_AUTO_INIT=1)
else()
    add_compile_definitions(ZOO_SMB_AUTO_INIT=0)
endif()

set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

# Build static library (excluding bridge sources)
add_library(zoo_smb_static STATIC ${SOURCES})
set_target_properties(zoo_smb_static PROPERTIES OUTPUT_NAME zoo_smb)
target_compile_definitions(zoo_smb_static PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)
target_compile_options(zoo_smb_static PRIVATE -Wno-pedantic -Wno-unused-value)
target_include_directories(zoo_smb_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/utility
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/node
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/transport
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/qos
    ${CMAKE_CURRENT_SOURCE_DIR}/../../memory_pool/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thread_pool/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../buffer/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../socket/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../log/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../util/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../platform/inc
)
target_link_libraries(zoo_smb_static PUBLIC
    Threads::Threads
    atomic
    zoo_buffer_static
    zoo_thread_pool_static
    zoo_socket_static
    zoo_log_static
    zoo_memory_pool_static
    zoo_util_static
    zoo_platform_static
)

# Build shared library (excluding bridge sources)
add_library(zoo_smb_shared SHARED ${SOURCES})
set_target_properties(zoo_smb_shared PROPERTIES OUTPUT_NAME zoo_smb)
target_compile_definitions(zoo_smb_shared PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)
target_compile_options(zoo_smb_shared PRIVATE -Wno-pedantic -Wno-unused-value)
target_include_directories(zoo_smb_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/utility
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/node
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/transport
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/qos
    ${CMAKE_CURRENT_SOURCE_DIR}/../../memory_pool/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thread_pool/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../buffer/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../socket/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../log/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../util/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../platform/inc
)
target_link_libraries(zoo_smb_shared PUBLIC
    Threads::Threads
    atomic
    zoo_buffer_shared
    zoo_thread_pool_shared
    zoo_socket_shared
    zoo_log_shared
    zoo_memory_pool_shared
    zoo_util_shared
    zoo_platform_shared
)

# Install libraries and header files
install(TARGETS zoo_smb_static zoo_smb_shared
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)