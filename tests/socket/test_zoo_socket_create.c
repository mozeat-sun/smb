/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: Socket Tests
 * Component ID: ZOO_SOCKET_TESTS
 * File name: test_zoo_socket_create.c
 * Description: Tests for socket creation and closing
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-05     assistant       Initial creation
 ******************************************************************************/

#include "unity.h"
#include "zoo_socket.h"

static bool is_initialized = false;

// setUp and tearDown are defined in test_runner.c

void test_zoo_socket_create_tcp_ipv4(void) {
    ZOO_SOCKET_INFO_STRUCT socket_info;
    
    ZOO_ERROR_TYPE result = zoo_socket_create(
        ZOO_AF_INET,
        ZOO_SOCK_STREAM,
        ZOO_IPPROTO_TCP,
        &socket_info
    );
    
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_NOT_EQUAL(ZOO_INVALID_SOCKET, socket_info.fd);
    TEST_ASSERT_EQUAL(ZOO_AF_INET, socket_info.family);
    TEST_ASSERT_EQUAL(ZOO_SOCK_STREAM, socket_info.type);
    TEST_ASSERT_EQUAL(ZOO_IPPROTO_TCP, socket_info.protocol);
    TEST_ASSERT_TRUE(socket_info.is_blocking);
    TEST_ASSERT_FALSE(socket_info.is_connected);
    TEST_ASSERT_FALSE(socket_info.is_bound);
    TEST_ASSERT_FALSE(socket_info.is_listening);
    
    // Clean up
    zoo_socket_close(&socket_info);
}

void test_zoo_socket_create_udp_ipv4(void) {
    ZOO_SOCKET_INFO_STRUCT socket_info;
    
    ZOO_ERROR_TYPE result = zoo_socket_create(
        ZOO_AF_INET,
        ZOO_SOCK_DGRAM,
        ZOO_IPPROTO_UDP,
        &socket_info
    );
    
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_NOT_EQUAL(ZOO_INVALID_SOCKET, socket_info.fd);
    TEST_ASSERT_EQUAL(ZOO_AF_INET, socket_info.family);
    TEST_ASSERT_EQUAL(ZOO_SOCK_DGRAM, socket_info.type);
    TEST_ASSERT_EQUAL(ZOO_IPPROTO_UDP, socket_info.protocol);
    
    // Clean up
    zoo_socket_close(&socket_info);
}

void test_zoo_socket_create_tcp_ipv6(void) {
    ZOO_SOCKET_INFO_STRUCT socket_info;
    
    ZOO_ERROR_TYPE result = zoo_socket_create(
        ZOO_AF_INET6,
        ZOO_SOCK_STREAM,
        ZOO_IPPROTO_TCP,
        &socket_info
    );
    
    // IPv6 may not be available on all systems
    if (result == ZOO_OK) {
        TEST_ASSERT_NOT_EQUAL(ZOO_INVALID_SOCKET, socket_info.fd);
        TEST_ASSERT_EQUAL(ZOO_AF_INET6, socket_info.family);
        TEST_ASSERT_EQUAL(ZOO_SOCK_STREAM, socket_info.type);
        TEST_ASSERT_EQUAL(ZOO_IPPROTO_TCP, socket_info.protocol);
        
        // Clean up
        zoo_socket_close(&socket_info);
    } else {
        // IPv6 not supported - this is acceptable
        TEST_ASSERT_TRUE(result != ZOO_OK);
    }
}

void test_zoo_socket_create_invalid_params(void) {
    ZOO_SOCKET_INFO_STRUCT socket_info;
    
    // NULL socket_info pointer
    ZOO_ERROR_TYPE result = zoo_socket_create(
        ZOO_AF_INET,
        ZOO_SOCK_STREAM,
        ZOO_IPPROTO_TCP,
        NULL
    );
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    // Test with uninitialized socket system
    zoo_socket_cleanup();
    result = zoo_socket_create(
        ZOO_AF_INET,
        ZOO_SOCK_STREAM,
        ZOO_IPPROTO_TCP,
        &socket_info
    );
    TEST_ASSERT_EQUAL(ZOO_ERROR_NOT_INITIALIZED, result);
    
    // Re-initialize for other tests
    zoo_socket_init();
    is_initialized = true;
}

void test_zoo_socket_close(void) {
    ZOO_SOCKET_INFO_STRUCT socket_info;
    
    // Create a socket first
    ZOO_ERROR_TYPE result = zoo_socket_create(
        ZOO_AF_INET,
        ZOO_SOCK_STREAM,
        ZOO_IPPROTO_TCP,
        &socket_info
    );
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    // Close the socket
    result = zoo_socket_close(&socket_info);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_EQUAL(ZOO_INVALID_SOCKET, socket_info.fd);
    TEST_ASSERT_FALSE(socket_info.is_connected);
    TEST_ASSERT_FALSE(socket_info.is_bound);
    TEST_ASSERT_FALSE(socket_info.is_listening);
    
    // Test invalid parameters
    result = zoo_socket_close(NULL);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    // Test closing already closed socket
    result = zoo_socket_close(&socket_info);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
}
