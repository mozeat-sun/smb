################################################################################
# Copyright (C) 2025, ZOO Ltd
#
# Product: ZOO
# Module: Socket Examples
# Component ID: ZOO_SOCKET_EXAMPLES
# File name: CMakeLists.txt
# Description: CMake build configuration for ZOO Socket examples
#
# Change History:
# Version   Date           Author          Description
# -------   ----------     -----------     ---------------------------------
# 1.0       2025-08-05     assistant       Initial creation
################################################################################

cmake_minimum_required(VERSION 3.16)

# ==============================================================================
# EXAMPLE PROGRAMS
# ==============================================================================

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${ZOO_PLATFORM_INCLUDE}
    ${ZOO_UTIL_INCLUDE}
)

# Simple TCP client example
add_executable(tcp_client_example tcp_client.c)
target_link_libraries(tcp_client_example zoo_socket_static)

# Simple TCP server example
add_executable(tcp_server_example tcp_server.c)
target_link_libraries(tcp_server_example zoo_socket_static)

# UDP example
add_executable(udp_example udp_example.c)
target_link_libraries(udp_example zoo_socket_static)

# Address utility example
add_executable(address_example address_example.c)
target_link_libraries(address_example zoo_socket_static)

# Platform-specific libraries
if(UNIX)
    target_link_libraries(tcp_client_example pthread)
    target_link_libraries(tcp_server_example pthread)
    target_link_libraries(udp_example pthread)
    target_link_libraries(address_example pthread)
endif()

if(WIN32)
    target_link_libraries(tcp_client_example ws2_32 wsock32)
    target_link_libraries(tcp_server_example ws2_32 wsock32)
    target_link_libraries(udp_example ws2_32 wsock32)
    target_link_libraries(address_example ws2_32 wsock32)
endif()

# ==============================================================================
# INSTALLATION
# ==============================================================================

install(TARGETS 
    tcp_client_example 
    tcp_server_example 
    udp_example 
    address_example
    DESTINATION bin/examples
)
