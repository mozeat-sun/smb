/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 *
 * Product: ZOO
 * Module: buffer
 * Component ID: test_runner
 * File name: test_runner.c
 * Description: Unity test runner for ZOO buffer module tests
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     GitHub Copilot  Initial creation (migrated from GTest)
 ******************************************************************************/

#include "unity.h"
#include <stdio.h>

// Platform information (simplified for testing)
#define ZOO_GET_PLATFORM_INFO() "X86_64"
#define ZOO_GET_PLATFORM_VERSION() "1.2.0"
#define ZOO_OS_NAME "Linux"
#define ZOO_HAS_THREADING 1
#define ZOO_CACHE_LINE_SIZE 64

/**
 * Global setUp function called before each test
 */
void setUp(void) {
    // Global setup code for each test
}

/**
 * Global tearDown function called after each test
 */
void tearDown(void) {
    // Global cleanup code for each test
}

// External test function declarations

// List tests
extern void test_list_create_destroy(void);
extern void test_list_push_pop_basic(void);
extern void test_list_push_pop_multiple(void);
extern void test_list_size_operations(void);
extern void test_list_empty_operations(void);
extern void test_list_full_operations(void);
extern void test_list_thread_safety(void);
extern void test_list_boundary_conditions(void);
extern void test_list_memory_management(void);
extern void test_list_performance(void);

// Queue tests
extern void test_queue_create_destroy(void);
extern void test_queue_enqueue_dequeue_basic(void);
extern void test_queue_enqueue_dequeue_multiple(void);
extern void test_queue_size_operations(void);
extern void test_queue_empty_operations(void);
extern void test_queue_full_operations(void);
extern void test_queue_thread_safety(void);
extern void test_queue_boundary_conditions(void);

int main(void) {
    printf("=== ZOO Buffer Test Suite ===\n");
    printf("Platform: %s\n", ZOO_GET_PLATFORM_INFO());
    printf("OS: %s\n", ZOO_OS_NAME);
    printf("Version: %s\n", ZOO_GET_PLATFORM_VERSION());
    printf("Threading Support: %s\n", ZOO_HAS_THREADING ? "Yes" : "No");
    printf("Cache Line Size: %d bytes\n", ZOO_CACHE_LINE_SIZE);
    printf("====================================\n\n");
    
    UNITY_BEGIN();
    
    printf("=== Running List Tests ===\n");
    RUN_TEST(test_list_create_destroy);
    RUN_TEST(test_list_push_pop_basic);
    RUN_TEST(test_list_push_pop_multiple);
    RUN_TEST(test_list_size_operations);
    RUN_TEST(test_list_empty_operations);
    RUN_TEST(test_list_full_operations);
    RUN_TEST(test_list_boundary_conditions);
    RUN_TEST(test_list_memory_management);
    
    printf("\n=== Running Queue Tests ===\n");
    RUN_TEST(test_queue_create_destroy);
    RUN_TEST(test_queue_enqueue_dequeue_basic);
    RUN_TEST(test_queue_enqueue_dequeue_multiple);
    RUN_TEST(test_queue_size_operations);
    RUN_TEST(test_queue_empty_operations);
    RUN_TEST(test_queue_full_operations);
    RUN_TEST(test_queue_boundary_conditions);
    
#if ZOO_HAS_THREADING
    printf("\n=== Running Thread Safety Tests ===\n");
    RUN_TEST(test_list_thread_safety);
    RUN_TEST(test_queue_thread_safety);
#else
    printf("\nThreading not supported - skipping thread safety tests\n");
#endif

    printf("\n=== Running Performance Tests ===\n");
    RUN_TEST(test_list_performance);
    
    printf("\n=== Test Summary ===\n");
    return UNITY_END();
}
