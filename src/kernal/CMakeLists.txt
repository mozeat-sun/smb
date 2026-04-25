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

# Domain implementation sources are selected explicitly by profile and are
# excluded from the baseline source glob to prevent accidental cross-domain
# linkage when domain folders are introduced.
list(FILTER SOURCES EXCLUDE REGEX ".*/domain/.*\\.c$")

# Optional future profile-scoped implementation sources.
if(DEFINED ZOO_DOMAIN_PROFILE)
    file(GLOB_RECURSE DOMAIN_PROFILE_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/domain/${ZOO_DOMAIN_PROFILE}/*.c
    )
    list(APPEND SOURCES ${DOMAIN_PROFILE_SOURCES})
endif()

# Build static library (excluding bridge sources)
add_library(zoo_smb_static STATIC ${SOURCES})
set_target_properties(zoo_smb_static PROPERTIES OUTPUT_NAME zoo_smb)
target_compile_definitions(zoo_smb_static PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
    ZOO_DOMAIN_PROFILE_${ZOO_DOMAIN_PROFILE}=1
)
target_compile_options(zoo_smb_static PRIVATE -Wno-pedantic -Wno-unused-value)
target_include_directories(zoo_smb_static PUBLIC
    ${CMAKE_SOURCE_DIR}/include/kernal
    ${CMAKE_SOURCE_DIR}/include/kernal/utility
    ${CMAKE_SOURCE_DIR}/include/kernal/core
    ${CMAKE_SOURCE_DIR}/include/kernal/node
    ${CMAKE_SOURCE_DIR}/include/kernal/transport
    ${CMAKE_SOURCE_DIR}/include/kernal/qos
    ${CMAKE_SOURCE_DIR}/include/platform/dispatcher
    ${CMAKE_SOURCE_DIR}/include/platform/memory_pool
    ${CMAKE_SOURCE_DIR}/include/platform/thread_pool
    ${CMAKE_SOURCE_DIR}/include/platform/buffer
    ${CMAKE_SOURCE_DIR}/include/platform/socket
    ${CMAKE_SOURCE_DIR}/include/platform/log
    ${CMAKE_SOURCE_DIR}/include/platform/util
    ${CMAKE_SOURCE_DIR}/include/platform
    ${CMAKE_SOURCE_DIR}/include/domain
    ${CMAKE_SOURCE_DIR}/include/assurance
)
target_link_libraries(zoo_smb_static PUBLIC
    Threads::Threads
    atomic
    zoo_dispatcher_static
    zoo_buffer_static
    zoo_thread_pool_static
    zoo_socket_static
    zoo_log_static
    zoo_memory_pool_static
    zoo_util_static
    zoo_platform_static
    zoo_domain_static
    zoo_assurance_static
)

# Build shared library (excluding bridge sources)
add_library(zoo_smb_shared SHARED ${SOURCES})
set_target_properties(zoo_smb_shared PROPERTIES OUTPUT_NAME smb)
target_compile_definitions(zoo_smb_shared PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
    ZOO_DOMAIN_PROFILE_${ZOO_DOMAIN_PROFILE}=1
)
target_compile_options(zoo_smb_shared PRIVATE -Wno-pedantic -Wno-unused-value)
target_include_directories(zoo_smb_shared PUBLIC
    ${CMAKE_SOURCE_DIR}/include/kernal
    ${CMAKE_SOURCE_DIR}/include/kernal/utility
    ${CMAKE_SOURCE_DIR}/include/kernal/core
    ${CMAKE_SOURCE_DIR}/include/kernal/node
    ${CMAKE_SOURCE_DIR}/include/kernal/transport
    ${CMAKE_SOURCE_DIR}/include/kernal/qos
    ${CMAKE_SOURCE_DIR}/include/platform/dispatcher
    ${CMAKE_SOURCE_DIR}/include/platform/memory_pool
    ${CMAKE_SOURCE_DIR}/include/platform/thread_pool
    ${CMAKE_SOURCE_DIR}/include/platform/buffer
    ${CMAKE_SOURCE_DIR}/include/platform/socket
    ${CMAKE_SOURCE_DIR}/include/platform/log
    ${CMAKE_SOURCE_DIR}/include/platform/util
    ${CMAKE_SOURCE_DIR}/include/platform
    ${CMAKE_SOURCE_DIR}/include/domain
    ${CMAKE_SOURCE_DIR}/include/assurance
)
target_link_libraries(zoo_smb_shared PUBLIC
    Threads::Threads
    atomic
    zoo_dispatcher_shared
    zoo_buffer_shared
    zoo_thread_pool_shared
    zoo_socket_shared
    zoo_log_shared
    zoo_memory_pool_shared
    zoo_util_shared
    zoo_platform_shared
    zoo_domain_static
    zoo_assurance_static
)

# Install libraries and header files
install(TARGETS zoo_smb_static zoo_smb_shared
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
