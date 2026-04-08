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
    ${CMAKE_SOURCE_DIR}/include/smb
    ${CMAKE_SOURCE_DIR}/include/smb/utility
    ${CMAKE_SOURCE_DIR}/include/smb/core
    ${CMAKE_SOURCE_DIR}/include/smb/node
    ${CMAKE_SOURCE_DIR}/include/smb/transport
    ${CMAKE_SOURCE_DIR}/include/smb/qos
    ${CMAKE_SOURCE_DIR}/include/memory_pool
    ${CMAKE_SOURCE_DIR}/include/thread_pool
    ${CMAKE_SOURCE_DIR}/include/buffer
    ${CMAKE_SOURCE_DIR}/include/socket
    ${CMAKE_SOURCE_DIR}/include/log
    ${CMAKE_SOURCE_DIR}/include/util
    ${CMAKE_SOURCE_DIR}/include/platform
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
set_target_properties(zoo_smb_shared PROPERTIES OUTPUT_NAME smb)
target_compile_definitions(zoo_smb_shared PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
)
target_compile_options(zoo_smb_shared PRIVATE -Wno-pedantic -Wno-unused-value)
target_include_directories(zoo_smb_shared PUBLIC
    ${CMAKE_SOURCE_DIR}/include/smb
    ${CMAKE_SOURCE_DIR}/include/smb/utility
    ${CMAKE_SOURCE_DIR}/include/smb/core
    ${CMAKE_SOURCE_DIR}/include/smb/node
    ${CMAKE_SOURCE_DIR}/include/smb/transport
    ${CMAKE_SOURCE_DIR}/include/smb/qos
    ${CMAKE_SOURCE_DIR}/include/memory_pool
    ${CMAKE_SOURCE_DIR}/include/thread_pool
    ${CMAKE_SOURCE_DIR}/include/buffer
    ${CMAKE_SOURCE_DIR}/include/socket
    ${CMAKE_SOURCE_DIR}/include/log
    ${CMAKE_SOURCE_DIR}/include/util
    ${CMAKE_SOURCE_DIR}/include/platform
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