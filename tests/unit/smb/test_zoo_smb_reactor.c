/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: UNIT_TEST_REACTOR
 * File name: test_zoo_smb_reactor.c
 * Description: Unit tests for Reactor abstraction layer
 * Traceability coverage:
 * - REQ-REL-001: event reactor readiness and watch lifecycle validation.
 * - REQ-SAFE-001: deterministic fd watch registration behavior.
 * - REQ-SAFE-003: requirement-linked unit evidence for reactor primitives.
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "unity.h"
#include "zoo_smb_reactor.h"
#include "zoo_log.h"

// ==============================================================================
// TEST FIXTURES
// ==============================================================================

static int g_reactor_fd = -1;
static int g_test_socket = -1;

void setUp(void)
{
    // Create reactor (epoll on Linux)
    g_reactor_fd = epoll_create1(0);
    TEST_ASSERT_GREATER_THAN(-1, g_reactor_fd);
    
    // Create test socket
    g_test_socket = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_GREATER_THAN(-1, g_test_socket);
}

void tearDown(void)
{
    if (g_test_socket >= 0)
    {
        close(g_test_socket);
        g_test_socket = -1;
    }
    
    if (g_reactor_fd >= 0)
    {
        close(g_reactor_fd);
        g_reactor_fd = -1;
    }
}

// ==============================================================================
// REACTOR API TESTS
// ==============================================================================

void test_reactor_watch_fd(void)
{
    ZOO_ERROR_TYPE result = zoo_smb_reactor_watch_fd(g_reactor_fd, g_test_socket, EPOLLIN);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
}

void test_reactor_watch_fd_multiple(void)
{
    int sock2 = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_GREATER_THAN(-1, sock2);
    
    ZOO_ERROR_TYPE result1 = zoo_smb_reactor_watch_fd(g_reactor_fd, g_test_socket, EPOLLIN);
    ZOO_ERROR_TYPE result2 = zoo_smb_reactor_watch_fd(g_reactor_fd, sock2, EPOLLIN);
    
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result1);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result2);
    
    close(sock2);
}

void test_reactor_unwatch_fd(void)
{
    zoo_smb_reactor_watch_fd(g_reactor_fd, g_test_socket, EPOLLIN);
    
    ZOO_ERROR_TYPE result = zoo_smb_reactor_unwatch_fd(g_reactor_fd, g_test_socket);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
}

void test_reactor_wait_timeout(void)
{
    struct epoll_event events[10];
    int event_count = 0;
    
    ZOO_ERROR_TYPE result = zoo_smb_reactor_wait(
        g_reactor_fd,
        events,
        10,
        100,  // 100ms timeout
        &event_count);
    
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    TEST_ASSERT_EQUAL(0, event_count);  // No events should be ready
}

void test_reactor_watch_modify(void)
{
    // Watch with EPOLLIN
    zoo_smb_reactor_watch_fd(g_reactor_fd, g_test_socket, EPOLLIN);
    
    // Modify to EPOLLOUT
    ZOO_ERROR_TYPE result = zoo_smb_reactor_watch_fd(g_reactor_fd, g_test_socket, EPOLLOUT);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
}

void test_reactor_invalid_fd(void)
{
    // Try to watch invalid file descriptor
    ZOO_ERROR_TYPE result = zoo_smb_reactor_watch_fd(-1, g_test_socket, EPOLLIN);
    // Should handle gracefully or return error
    TEST_ASSERT_NOT_EQUAL(ZOO_SMB_OK, result);
}

void test_reactor_wait_invalid_params(void)
{
    struct epoll_event events[10];
    int event_count = 0;
    
    // Invalid max_events
    ZOO_ERROR_TYPE result = zoo_smb_reactor_wait(
        g_reactor_fd,
        events,
        0,  // Invalid
        100,
        &event_count);
    
    TEST_ASSERT_EQUAL(ZOO_SMB_ERROR_INVALID_PARAM, result);
}

// ==============================================================================
// REACTOR STRESS TESTS
// ==============================================================================

void test_reactor_many_fds(void)
{
    int sockets[100];
    
    // Create and watch 100 file descriptors
    for (int i = 0; i < 100; i++)
    {
        sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
        TEST_ASSERT_GREATER_THAN(-1, sockets[i]);
        
        ZOO_ERROR_TYPE result = zoo_smb_reactor_watch_fd(g_reactor_fd, sockets[i], EPOLLIN);
        TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    }
    
    // Wait for events
    struct epoll_event events[128];
    int event_count = 0;
    ZOO_ERROR_TYPE result = zoo_smb_reactor_wait(
        g_reactor_fd,
        events,
        128,
        100,
        &event_count);
    
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    
    // Cleanup
    for (int i = 0; i < 100; i++)
    {
        close(sockets[i]);
    }
}

void test_reactor_fd_reuse(void)
{
    // Watch fd
    zoo_smb_reactor_watch_fd(g_reactor_fd, g_test_socket, EPOLLIN);
    
    // Unwatch fd
    zoo_smb_reactor_unwatch_fd(g_reactor_fd, g_test_socket);
    
    // Watch same fd again
    ZOO_ERROR_TYPE result = zoo_smb_reactor_watch_fd(g_reactor_fd, g_test_socket, EPOLLOUT);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
}

// ==============================================================================
// CROSS-PLATFORM COMPATIBILITY TESTS
// ==============================================================================

void test_reactor_event_masks(void)
{
    // Test different event masks
    uint32_t masks[] = {EPOLLIN, EPOLLOUT, EPOLLIN | EPOLLOUT, EPOLLERR};
    
    for (int i = 0; i < 4; i++)
    {
        zoo_smb_reactor_unwatch_fd(g_reactor_fd, g_test_socket);
        
        ZOO_ERROR_TYPE result = zoo_smb_reactor_watch_fd(g_reactor_fd, g_test_socket, masks[i]);
        TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    }
}
