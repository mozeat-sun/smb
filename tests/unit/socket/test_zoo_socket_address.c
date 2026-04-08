/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: Socket Tests
 * Component ID: ZOO_SOCKET_TESTS
 * File name: test_zoo_socket_address.c
 * Description: Tests for socket address utility functions
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-05     assistant       Initial creation
 ******************************************************************************/

#include "unity.h"
#include "zoo_socket.h"
#include <string.h>

void test_zoo_sockaddr_inet_init(void) {
    ZOO_SOCKADDR_UNION addr;
    
    ZOO_ERROR_TYPE result = zoo_sockaddr_inet_init(&addr, 0x7F000001, 8080);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    TEST_ASSERT_EQUAL(ZOO_AF_INET, addr.family);
    TEST_ASSERT_EQUAL(ZOO_AF_INET, addr.in.family);
    TEST_ASSERT_EQUAL(0x7F000001, addr.in.addr);  // 127.0.0.1
    TEST_ASSERT_EQUAL(8080, addr.in.port);
    
    // Test with NULL pointer
    result = zoo_sockaddr_inet_init(NULL, 0x7F000001, 8080);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
}

void test_zoo_sockaddr_inet6_init(void) {
    ZOO_SOCKADDR_UNION addr;
    uint8_t ipv6_addr[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}; // ::1
    
    ZOO_ERROR_TYPE result = zoo_sockaddr_inet6_init(&addr, ipv6_addr, 8080, 0, 0);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    TEST_ASSERT_EQUAL(ZOO_AF_INET6, addr.family);
    TEST_ASSERT_EQUAL(ZOO_AF_INET6, addr.in6.family);
    TEST_ASSERT_EQUAL_MEMORY(ipv6_addr, addr.in6.addr, 16);
    TEST_ASSERT_EQUAL(8080, addr.in6.port);
    TEST_ASSERT_EQUAL(0, addr.in6.flowinfo);
    TEST_ASSERT_EQUAL(0, addr.in6.scope_id);
    
    // Test with NULL pointers
    result = zoo_sockaddr_inet6_init(NULL, ipv6_addr, 8080, 0, 0);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_sockaddr_inet6_init(&addr, NULL, 8080, 0, 0);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
}

void test_zoo_sockaddr_any_init(void) {
    ZOO_SOCKADDR_UNION addr;
    
    // Test IPv4 any address
    ZOO_ERROR_TYPE result = zoo_sockaddr_any_init(&addr, ZOO_AF_INET, 8080);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    TEST_ASSERT_EQUAL(ZOO_AF_INET, addr.family);
    TEST_ASSERT_EQUAL(ZOO_AF_INET, addr.in.family);
    TEST_ASSERT_EQUAL(ZOO_INADDR_ANY, addr.in.addr);
    TEST_ASSERT_EQUAL(8080, addr.in.port);
    
    // Test IPv6 any address
    result = zoo_sockaddr_any_init(&addr, ZOO_AF_INET6, 8080);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    TEST_ASSERT_EQUAL(ZOO_AF_INET6, addr.family);
    TEST_ASSERT_EQUAL(ZOO_AF_INET6, addr.in6.family);
    TEST_ASSERT_EQUAL(8080, addr.in6.port);
    TEST_ASSERT_EQUAL(0, addr.in6.flowinfo);
    TEST_ASSERT_EQUAL(0, addr.in6.scope_id);
    
    // Verify IPv6 any address (all zeros)
    uint8_t zero_addr[16] = {0};
    TEST_ASSERT_EQUAL_MEMORY(zero_addr, addr.in6.addr, 16);
    
    // Test invalid parameters
    result = zoo_sockaddr_any_init(NULL, ZOO_AF_INET, 8080);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_sockaddr_any_init(&addr, (ZOO_SOCKET_FAMILY_ENUM)999, 8080);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
}

void test_zoo_sockaddr_loopback_init(void) {
    ZOO_SOCKADDR_UNION addr;
    
    // Test IPv4 loopback address
    ZOO_ERROR_TYPE result = zoo_sockaddr_loopback_init(&addr, ZOO_AF_INET, 8080);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    TEST_ASSERT_EQUAL(ZOO_AF_INET, addr.family);
    TEST_ASSERT_EQUAL(ZOO_AF_INET, addr.in.family);
    TEST_ASSERT_EQUAL(ZOO_INADDR_LOOPBACK, addr.in.addr);
    TEST_ASSERT_EQUAL(8080, addr.in.port);
    
    // Test IPv6 loopback address
    result = zoo_sockaddr_loopback_init(&addr, ZOO_AF_INET6, 8080);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    TEST_ASSERT_EQUAL(ZOO_AF_INET6, addr.family);
    TEST_ASSERT_EQUAL(ZOO_AF_INET6, addr.in6.family);
    TEST_ASSERT_EQUAL(8080, addr.in6.port);
    TEST_ASSERT_EQUAL(0, addr.in6.flowinfo);
    TEST_ASSERT_EQUAL(0, addr.in6.scope_id);
    
    // Verify IPv6 loopback address (::1)
    uint8_t loopback_addr[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    TEST_ASSERT_EQUAL_MEMORY(loopback_addr, addr.in6.addr, 16);
    
    // Test invalid parameters
    result = zoo_sockaddr_loopback_init(NULL, ZOO_AF_INET, 8080);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_sockaddr_loopback_init(&addr, (ZOO_SOCKET_FAMILY_ENUM)999, 8080);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
}

void test_zoo_sockaddr_equal(void) {
    ZOO_SOCKADDR_UNION addr1, addr2;
    
    // Test IPv4 equality
    zoo_sockaddr_inet_init(&addr1, 0x7F000001, 8080);
    zoo_sockaddr_inet_init(&addr2, 0x7F000001, 8080);
    TEST_ASSERT_TRUE(zoo_sockaddr_equal(&addr1, &addr2));
    
    // Test IPv4 inequality (different address)
    zoo_sockaddr_inet_init(&addr2, 0x7F000002, 8080);
    TEST_ASSERT_FALSE(zoo_sockaddr_equal(&addr1, &addr2));
    
    // Test IPv4 inequality (different port)
    zoo_sockaddr_inet_init(&addr2, 0x7F000001, 8081);
    TEST_ASSERT_FALSE(zoo_sockaddr_equal(&addr1, &addr2));
    
    // Test IPv6 equality
    uint8_t ipv6_addr[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    zoo_sockaddr_inet6_init(&addr1, ipv6_addr, 8080, 0, 0);
    zoo_sockaddr_inet6_init(&addr2, ipv6_addr, 8080, 0, 0);
    TEST_ASSERT_TRUE(zoo_sockaddr_equal(&addr1, &addr2));
    
    // Test IPv6 inequality (different address)
    ipv6_addr[15] = 2;
    zoo_sockaddr_inet6_init(&addr2, ipv6_addr, 8080, 0, 0);
    TEST_ASSERT_FALSE(zoo_sockaddr_equal(&addr1, &addr2));
    
    // Test different families
    zoo_sockaddr_inet_init(&addr1, 0x7F000001, 8080);
    zoo_sockaddr_inet6_init(&addr2, ipv6_addr, 8080, 0, 0);
    TEST_ASSERT_FALSE(zoo_sockaddr_equal(&addr1, &addr2));
    
    // Test NULL parameters
    TEST_ASSERT_FALSE(zoo_sockaddr_equal(NULL, &addr2));
    TEST_ASSERT_FALSE(zoo_sockaddr_equal(&addr1, NULL));
    TEST_ASSERT_FALSE(zoo_sockaddr_equal(NULL, NULL));
}
