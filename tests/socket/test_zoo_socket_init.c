/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: Socket Tests
 * Component ID: ZOO_SOCKET_TESTS
 * File name: test_zoo_socket_init.c
 * Description: Tests for socket initialization and cleanup
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-05     assistant       Initial creation
 ******************************************************************************/

#include "unity.h"
#include "zoo_socket.h"

void test_zoo_socket_init_success(void) {
    ZOO_ERROR_TYPE result = zoo_socket_init();
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    // Verify initialization state
    TEST_ASSERT_TRUE(zoo_socket_is_initialized());
    
    // Clean up
    zoo_socket_cleanup();
}

void test_zoo_socket_init_multiple_calls(void) {
    // First initialization
    ZOO_ERROR_TYPE result1 = zoo_socket_init();
    TEST_ASSERT_EQUAL(ZOO_OK, result1);
    TEST_ASSERT_TRUE(zoo_socket_is_initialized());
    
    // Second initialization should also succeed (idempotent)
    ZOO_ERROR_TYPE result2 = zoo_socket_init();
    TEST_ASSERT_EQUAL(ZOO_OK, result2);
    TEST_ASSERT_TRUE(zoo_socket_is_initialized());
    
    // Clean up
    zoo_socket_cleanup();
    TEST_ASSERT_FALSE(zoo_socket_is_initialized());
}

void test_zoo_socket_cleanup(void) {
    // Initialize first
    ZOO_ERROR_TYPE result = zoo_socket_init();
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_TRUE(zoo_socket_is_initialized());
    
    // Clean up
    zoo_socket_cleanup();
    TEST_ASSERT_FALSE(zoo_socket_is_initialized());
    
    // Multiple cleanups should be safe
    zoo_socket_cleanup();
    TEST_ASSERT_FALSE(zoo_socket_is_initialized());
}
