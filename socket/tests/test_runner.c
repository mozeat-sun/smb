/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: Socket Tests
 * Component ID: ZOO_SOCKET_TESTS
 * File name: test_runner.c
 * Description: Unity test runner for ZOO Socket library
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-05     assistant       Initial creation
 ******************************************************************************/

#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

// ==============================================================================
// TEST FUNCTION DECLARATIONS
// ==============================================================================

// Initialization tests
void test_zoo_socket_init_success(void);
void test_zoo_socket_init_multiple_calls(void);
void test_zoo_socket_cleanup(void);

// Socket creation tests
void test_zoo_socket_create_tcp_ipv4(void);
void test_zoo_socket_create_udp_ipv4(void);
void test_zoo_socket_create_tcp_ipv6(void);
void test_zoo_socket_create_invalid_params(void);
void test_zoo_socket_close(void);

// Address utility tests
void test_zoo_sockaddr_inet_init(void);
void test_zoo_sockaddr_inet6_init(void);
void test_zoo_sockaddr_any_init(void);
void test_zoo_sockaddr_loopback_init(void);
void test_zoo_sockaddr_equal(void);

// Connection tests
void test_zoo_socket_connect_success(void);
void test_zoo_socket_connect_invalid_params(void);
void test_zoo_socket_connect_timeout(void);

// Bind and listen tests
void test_zoo_socket_bind_success(void);
void test_zoo_socket_bind_invalid_params(void);
void test_zoo_socket_listen_success(void);
void test_zoo_socket_listen_invalid_params(void);
void test_zoo_socket_accept_invalid_params(void);

// Send/receive tests
void test_zoo_socket_send_success(void);
void test_zoo_socket_send_invalid_params(void);
void test_zoo_socket_recv_invalid_params(void);
void test_zoo_socket_sendto_success(void);
void test_zoo_socket_recvfrom_success(void);

// Socket options tests
void test_zoo_socket_setsockopt_success(void);
void test_zoo_socket_getsockopt_success(void);
void test_zoo_socket_set_blocking(void);

// Select tests
void test_zoo_socket_select_read(void);
void test_zoo_socket_select_write(void);
void test_zoo_socket_select_timeout(void);

// Error handling tests
void test_zoo_socket_get_last_error(void);
void test_zoo_socket_error_string(void);
void test_zoo_socket_is_initialized(void);

// ==============================================================================
// TEST SETUP AND TEARDOWN
// ==============================================================================

void setUp(void) {
    // This function is called before each test
    // Initialize socket subsystem if needed
    if (!zoo_socket_is_initialized()) {
        zoo_socket_init();
    }
}

void tearDown(void) {
    // This function is called after each test
    // Clean up any resources
}

// ==============================================================================
// TEST SUITE RUNNER
// ==============================================================================

int main(void) {
    UNITY_BEGIN();

    printf("\n");
    printf("================================================================================\n");
    printf("ZOO Socket Library Unit Tests\n");
    printf("================================================================================\n");
    printf("\n");

    // Initialization tests
    printf("Running initialization tests...\n");
    RUN_TEST(test_zoo_socket_init_success);
    RUN_TEST(test_zoo_socket_init_multiple_calls);
    RUN_TEST(test_zoo_socket_cleanup);

    // Socket creation tests
    printf("Running socket creation tests...\n");
    RUN_TEST(test_zoo_socket_create_tcp_ipv4);
    RUN_TEST(test_zoo_socket_create_udp_ipv4);
    RUN_TEST(test_zoo_socket_create_tcp_ipv6);
    RUN_TEST(test_zoo_socket_create_invalid_params);
    RUN_TEST(test_zoo_socket_close);

    // Address utility tests
    printf("Running address utility tests...\n");
    RUN_TEST(test_zoo_sockaddr_inet_init);
    RUN_TEST(test_zoo_sockaddr_inet6_init);
    RUN_TEST(test_zoo_sockaddr_any_init);
    RUN_TEST(test_zoo_sockaddr_loopback_init);
    RUN_TEST(test_zoo_sockaddr_equal);

    // Connection tests
    printf("Running connection tests...\n");
    RUN_TEST(test_zoo_socket_connect_success);
    RUN_TEST(test_zoo_socket_connect_invalid_params);
    RUN_TEST(test_zoo_socket_connect_timeout);

    // Bind and listen tests
    printf("Running bind and listen tests...\n");
    RUN_TEST(test_zoo_socket_bind_success);
    RUN_TEST(test_zoo_socket_bind_invalid_params);
    RUN_TEST(test_zoo_socket_listen_success);
    RUN_TEST(test_zoo_socket_listen_invalid_params);
    RUN_TEST(test_zoo_socket_accept_invalid_params);

    // Send/receive tests
    printf("Running send/receive tests...\n");
    RUN_TEST(test_zoo_socket_send_success);
    RUN_TEST(test_zoo_socket_send_invalid_params);
    RUN_TEST(test_zoo_socket_recv_invalid_params);
    RUN_TEST(test_zoo_socket_sendto_success);
    RUN_TEST(test_zoo_socket_recvfrom_success);

    // Socket options tests
    printf("Running socket options tests...\n");
    RUN_TEST(test_zoo_socket_setsockopt_success);
    RUN_TEST(test_zoo_socket_getsockopt_success);
    RUN_TEST(test_zoo_socket_set_blocking);

    // Select tests
    printf("Running select tests...\n");
    RUN_TEST(test_zoo_socket_select_read);
    RUN_TEST(test_zoo_socket_select_write);
    RUN_TEST(test_zoo_socket_select_timeout);

    // Error handling tests
    printf("Running error handling tests...\n");
    RUN_TEST(test_zoo_socket_get_last_error);
    RUN_TEST(test_zoo_socket_error_string);
    RUN_TEST(test_zoo_socket_is_initialized);

    printf("\n");
    printf("================================================================================\n");
    printf("Test Summary\n");
    printf("================================================================================\n");

    return UNITY_END();
}
