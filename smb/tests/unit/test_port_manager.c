/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_port_manager_
 * File name: test_port_manager.c
 * Description: Unity-based port manager tests (converted from GTest)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                converted from GTest
 ******************************************************************************/

#include "test_utils.h"
#include "zoo_smb_port_manager.h"
#include <stdlib.h>

// Global test state
static int* allocated_ports = NULL;
static size_t allocated_count = 0;
static size_t allocated_capacity = 0;

// Test constants (fallback if not defined in header)
#ifndef MIN_TRANSPORT_PORT
#define MIN_TRANSPORT_PORT 8000
#endif
#ifndef MAX_TRANSPORT_PORT  
#define MAX_TRANSPORT_PORT 9000
#endif

void setUp(void)
{
    // Setup before each test
    allocated_ports = NULL;
    allocated_count = 0;
    allocated_capacity = 0;
}

void tearDown(void)
{
    // Release all allocated ports
    for (size_t i = 0; i < allocated_count; i++) {
        zoo_smb_release_port_resources(allocated_ports[i]);
    }
    
    if (allocated_ports) {
        free(allocated_ports);
        allocated_ports = NULL;
    }
    allocated_count = 0;
    allocated_capacity = 0;
}

static void add_allocated_port(int port)
{
    if (allocated_count >= allocated_capacity) {
        allocated_capacity = allocated_capacity == 0 ? 4 : allocated_capacity * 2;
        allocated_ports = realloc(allocated_ports, allocated_capacity * sizeof(int));
        TEST_ASSERT_NOT_NULL(allocated_ports);
    }
    allocated_ports[allocated_count++] = port;
}

void test_find_and_release_port(void)
{
    int port = zoo_smb_find_available_port(MIN_TRANSPORT_PORT, MAX_TRANSPORT_PORT);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(MIN_TRANSPORT_PORT, port);
    TEST_ASSERT_LESS_OR_EQUAL_INT(MAX_TRANSPORT_PORT, port);
    add_allocated_port(port);
}

void test_multiple_port_allocation(void)
{
    const int PORT_COUNT = 5;
    int ports[PORT_COUNT];
    
    // Allocate multiple ports
    for (int i = 0; i < PORT_COUNT; i++) {
        int port = zoo_smb_find_available_port(MIN_TRANSPORT_PORT, MAX_TRANSPORT_PORT);
        TEST_ASSERT_GREATER_OR_EQUAL_INT(MIN_TRANSPORT_PORT, port);
        TEST_ASSERT_LESS_OR_EQUAL_INT(MAX_TRANSPORT_PORT, port);
        ports[i] = port;
        add_allocated_port(port);
    }
    
    // Verify all ports are unique
    for (int i = 0; i < PORT_COUNT; i++) {
        for (int j = i + 1; j < PORT_COUNT; j++) {
            TEST_ASSERT_NOT_EQUAL_INT(ports[i], ports[j]);
        }
    }
}

void test_invalid_range_handling(void)
{
    // Test invalid port range
    int port = zoo_smb_find_available_port(MAX_TRANSPORT_PORT, MIN_TRANSPORT_PORT);
    TEST_ASSERT_EQUAL_INT(-1, port);
    
    // Test out of bounds range
    port = zoo_smb_find_available_port(0, 100);
    TEST_ASSERT_EQUAL_INT(-1, port);
}

void test_port_resource_management(void)
{
    int port = zoo_smb_find_available_port(MIN_TRANSPORT_PORT, MAX_TRANSPORT_PORT);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(MIN_TRANSPORT_PORT, port);
    
    // Test releasing port resources (void function)
    zoo_smb_release_port_resources(port);
    // No return value to check for void function
}

void test_port_range_boundary(void)
{
    // Test minimum port boundary
    int port = zoo_smb_find_available_port(MIN_TRANSPORT_PORT, MIN_TRANSPORT_PORT);
    if (port != -1) {
        TEST_ASSERT_EQUAL_INT(MIN_TRANSPORT_PORT, port);
        add_allocated_port(port);
    }
    
    // Test maximum port boundary  
    port = zoo_smb_find_available_port(MAX_TRANSPORT_PORT, MAX_TRANSPORT_PORT);
    if (port != -1) {
        TEST_ASSERT_EQUAL_INT(MAX_TRANSPORT_PORT, port);
        add_allocated_port(port);
    }
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_find_and_release_port);
    RUN_TEST(test_multiple_port_allocation);
    RUN_TEST(test_invalid_range_handling);
    RUN_TEST(test_port_resource_management);
    RUN_TEST(test_port_range_boundary);
    
    return UNITY_END();
}
