/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: Socket Tests
 * Component ID: ZOO_SOCKET_TESTS
 * File name: test_zoo_socket_connect.c
 * Description: Tests for socket connection functionality
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

void test_zoo_socket_connect_success(void) {
    // Note: This test may fail if no server is running on localhost:80
    // It's primarily testing the function interface and parameter validation
    ZOO_SOCKET_INFO_STRUCT socket_info;
    ZOO_SOCKADDR_UNION addr;
    
    // Create a TCP socket
    ZOO_ERROR_TYPE result = zoo_socket_create(
        ZOO_AF_INET,
        ZOO_SOCK_STREAM,
        ZOO_IPPROTO_TCP,
        &socket_info
    );
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    // Try to connect to localhost:80 (this may fail, but should not crash)
    zoo_sockaddr_inet_init(&addr, 0x7F000001, 80);  // 127.0.0.1:80
    
    result = zoo_socket_connect(&socket_info, &addr);
    // We don't assert success here as there may be no server running
    // But the function should return a valid error code
    TEST_ASSERT_TRUE(result == ZOO_OK || 
                     result == ZOO_ERROR_CONNECTION_REFUSED ||
                     result == ZOO_ERROR_TIMEOUT ||
                     result == ZOO_ERROR_NETWORK_UNREACHABLE);
    
    // Clean up
    zoo_socket_close(&socket_info);
}

void test_zoo_socket_connect_invalid_params(void) {
    ZOO_SOCKET_INFO_STRUCT socket_info;
    ZOO_SOCKADDR_UNION addr;
    
    // Create a valid socket
    ZOO_ERROR_TYPE result = zoo_socket_create(
        ZOO_AF_INET,
        ZOO_SOCK_STREAM,
        ZOO_IPPROTO_TCP,
        &socket_info
    );
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    zoo_sockaddr_inet_init(&addr, 0x7F000001, 8080);
    
    // Test NULL socket_info
    result = zoo_socket_connect(NULL, &addr);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    // Test NULL address
    result = zoo_socket_connect(&socket_info, NULL);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    // Test with invalid socket
    socket_info.fd = ZOO_INVALID_SOCKET;
    result = zoo_socket_connect(&socket_info, &addr);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    // Clean up (restore valid socket first)
    zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &socket_info);
    zoo_socket_close(&socket_info);
}

void test_zoo_socket_connect_timeout(void) {
    // This test verifies that connection attempts to unreachable addresses
    // return appropriate error codes
    ZOO_SOCKET_INFO_STRUCT socket_info;
    ZOO_SOCKADDR_UNION addr;
    
    // Create a TCP socket
    ZOO_ERROR_TYPE result = zoo_socket_create(
        ZOO_AF_INET,
        ZOO_SOCK_STREAM,
        ZOO_IPPROTO_TCP,
        &socket_info
    );
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    // Try to connect to a non-routable address (should timeout or fail quickly)
    zoo_sockaddr_inet_init(&addr, 0x0A000001, 12345);  // 10.0.0.1:12345
    
    result = zoo_socket_connect(&socket_info, &addr);
    
    // Should fail with timeout or network unreachable
    TEST_ASSERT_TRUE(result == ZOO_ERROR_TIMEOUT ||
                     result == ZOO_ERROR_NETWORK_UNREACHABLE ||
                     result == ZOO_ERROR_CONNECTION_REFUSED);
    
    // Clean up
    zoo_socket_close(&socket_info);
}
