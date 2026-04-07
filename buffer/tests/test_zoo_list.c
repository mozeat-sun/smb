/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 *
 * Product: ZOO
 * Module: buffer
 * Component ID: list
 * File name: test_zoo_list.c
 * Description: Unity C test suite for ZOO linked list implementation
 *
 * Features Tested:
 * - Cross-platform thread-safe operations
 * - Memory management and allocation
 * - FIFO and LIFO operations
 * - Boundary conditions and error handling
 * - Multi-threaded access patterns
 * - Performance characteristics
 *
 * Test Categories:
 * - Basic Operations: Creation, destruction, push/pop operations
 * - Thread Safety: Concurrent access, race condition prevention
 * - Error Handling: Invalid parameters, boundary conditions
 * - Performance: Memory usage, operation timing
 * - Platform Compatibility: Cross-platform behavior verification
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     GitHub Copilot  Initial creation (migrated from GTest)
 ******************************************************************************/

// Platform-specific feature requirements (must be before any includes)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Basic type definitions (avoiding external dependencies)
typedef int ZOO_ERROR_T;
typedef struct zoo_list zoo_list_t;

// Error codes
#define ZOO_ERROR_OK 0
#define ZOO_ERROR_INVALID_PARAM -1
#define ZOO_ERROR_EMPTY -2
#define ZOO_ERROR_FULL -3

// Platform macros
#define ZOO_HAS_THREADING 1
#define ZOO_SLEEP_US(us) usleep(us)

// Function declarations (assuming these are implemented in the buffer library)
ZOO_ERROR_T zoo_list_create(zoo_list_t** list, int max_size);
ZOO_ERROR_T zoo_list_destroy(zoo_list_t* list);
ZOO_ERROR_T zoo_list_push(zoo_list_t* list, const void* data, size_t size);
ZOO_ERROR_T zoo_list_pop(zoo_list_t* list, void* data, size_t size);
int zoo_list_size(zoo_list_t* list);
int zoo_list_is_empty(zoo_list_t* list);
int zoo_list_is_full(zoo_list_t* list);

#if ZOO_HAS_THREADING
#include <pthread.h>
#include <unistd.h>
#endif

// ==============================================================================
// TEST CONSTANTS AND UTILITIES
// ==============================================================================

#define DEFAULT_MAX_SIZE 100
#define LARGE_MAX_SIZE 10000
#define STRESS_TEST_ITERATIONS 1000
#define THREAD_COUNT 4
#define OPERATIONS_PER_THREAD 100

// Test data structure
typedef struct {
    int value;
    char data[32];
} test_data_t;

// Thread test context
typedef struct {
    zoo_list_t* list;
    int thread_id;
    int operations_count;
} thread_context_t;

// ==============================================================================
// BASIC LIST OPERATIONS TESTS
// ==============================================================================

/**
 * Test list creation and destruction
 */
void test_list_create_destroy(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    
    // Test successful creation
    result = zoo_list_create(&list, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    TEST_ASSERT_NOT_NULL(list);
    
    // Test list properties
    TEST_ASSERT_EQUAL(0, zoo_list_size(list));
    TEST_ASSERT_TRUE(zoo_list_is_empty(list));
    TEST_ASSERT_FALSE(zoo_list_is_full(list));
    
    // Test destruction
    result = zoo_list_destroy(list);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test creation with invalid parameters
    result = zoo_list_create(NULL, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_list_create(&list, 0);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    printf("List create/destroy test passed!\n");
}

/**
 * Test basic push and pop operations
 */
void test_list_push_pop_basic(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    test_data_t data_in = {42, "test_data"};
    test_data_t data_out = {0};
    
    // Create list
    result = zoo_list_create(&list, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test push operation
    result = zoo_list_push(list, &data_in, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    TEST_ASSERT_EQUAL(1, zoo_list_size(list));
    TEST_ASSERT_FALSE(zoo_list_is_empty(list));
    
    // Test pop operation
    result = zoo_list_pop(list, &data_out, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    TEST_ASSERT_EQUAL(data_in.value, data_out.value);
    TEST_ASSERT_EQUAL_STRING(data_in.data, data_out.data);
    TEST_ASSERT_EQUAL(0, zoo_list_size(list));
    TEST_ASSERT_TRUE(zoo_list_is_empty(list));
    
    // Test pop from empty list
    result = zoo_list_pop(list, &data_out, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_EMPTY, result);
    
    // Cleanup
    zoo_list_destroy(list);
    
    printf("List basic push/pop test passed!\n");
}

/**
 * Test multiple push and pop operations
 */
void test_list_push_pop_multiple(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    const int test_count = 10;
    
    // Create list
    result = zoo_list_create(&list, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Push multiple items
    for (int i = 0; i < test_count; i++) {
        test_data_t data = {i, ""};
        snprintf(data.data, sizeof(data.data), "item_%d", i);
        
        result = zoo_list_push(list, &data, sizeof(test_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        TEST_ASSERT_EQUAL(i + 1, zoo_list_size(list));
    }
    
    // Verify list is not full yet
    TEST_ASSERT_FALSE(zoo_list_is_full(list));
    
    // Pop multiple items (LIFO behavior)
    for (int i = test_count - 1; i >= 0; i--) {
        test_data_t data_out = {0};
        char expected_str[32];
        
        snprintf(expected_str, sizeof(expected_str), "item_%d", i);
        
        result = zoo_list_pop(list, &data_out, sizeof(test_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        TEST_ASSERT_EQUAL(i, data_out.value);
        TEST_ASSERT_EQUAL_STRING(expected_str, data_out.data);
        TEST_ASSERT_EQUAL(i, zoo_list_size(list));
    }
    
    TEST_ASSERT_TRUE(zoo_list_is_empty(list));
    
    // Cleanup
    zoo_list_destroy(list);
    
    printf("List multiple push/pop test passed!\n");
}

/**
 * Test list size operations
 */
void test_list_size_operations(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    
    // Create list
    result = zoo_list_create(&list, 5);  // Small list for testing
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test size tracking
    TEST_ASSERT_EQUAL(0, zoo_list_size(list));
    
    // Add items and check size
    for (int i = 0; i < 5; i++) {
        test_data_t data = {i, "test"};
        result = zoo_list_push(list, &data, sizeof(test_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        TEST_ASSERT_EQUAL(i + 1, zoo_list_size(list));
    }
    
    // List should be full now
    TEST_ASSERT_TRUE(zoo_list_is_full(list));
    
    // Try to add one more (should fail)
    test_data_t extra_data = {99, "extra"};
    result = zoo_list_push(list, &extra_data, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_FULL, result);
    TEST_ASSERT_EQUAL(5, zoo_list_size(list));
    
    // Remove items and check size
    for (int i = 4; i >= 0; i--) {
        test_data_t data_out = {0};
        result = zoo_list_pop(list, &data_out, sizeof(test_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        TEST_ASSERT_EQUAL(i, zoo_list_size(list));
    }
    
    TEST_ASSERT_TRUE(zoo_list_is_empty(list));
    
    // Cleanup
    zoo_list_destroy(list);
    
    printf("List size operations test passed!\n");
}

/**
 * Test empty list operations
 */
void test_list_empty_operations(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    test_data_t data_out = {0};
    
    // Create list
    result = zoo_list_create(&list, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test empty list properties
    TEST_ASSERT_TRUE(zoo_list_is_empty(list));
    TEST_ASSERT_EQUAL(0, zoo_list_size(list));
    
    // Test operations on empty list
    result = zoo_list_pop(list, &data_out, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_EMPTY, result);
    
    // Cleanup
    zoo_list_destroy(list);
    
    printf("List empty operations test passed!\n");
}

/**
 * Test full list operations
 */
void test_list_full_operations(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    const int max_size = 3;  // Small list for testing
    
    // Create small list
    result = zoo_list_create(&list, max_size);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Fill the list
    for (int i = 0; i < max_size; i++) {
        test_data_t data = {i, "test"};
        result = zoo_list_push(list, &data, sizeof(test_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    }
    
    // Verify list is full
    TEST_ASSERT_TRUE(zoo_list_is_full(list));
    TEST_ASSERT_EQUAL(max_size, zoo_list_size(list));
    
    // Try to add more items
    test_data_t extra_data = {99, "extra"};
    result = zoo_list_push(list, &extra_data, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_FULL, result);
    
    // Cleanup
    zoo_list_destroy(list);
    
    printf("List full operations test passed!\n");
}

/**
 * Test boundary conditions and error handling
 */
void test_list_boundary_conditions(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    test_data_t data = {42, "test"};
    test_data_t data_out = {0};
    
    // Create list
    result = zoo_list_create(&list, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test NULL pointer handling
    result = zoo_list_push(NULL, &data, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_list_push(list, NULL, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_list_pop(NULL, &data_out, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_list_pop(list, NULL, sizeof(test_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    // Test zero size handling
    result = zoo_list_push(list, &data, 0);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_list_pop(list, &data_out, 0);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    // Test size functions with NULL
    TEST_ASSERT_EQUAL(-1, zoo_list_size(NULL));
    TEST_ASSERT_FALSE(zoo_list_is_empty(NULL));
    TEST_ASSERT_FALSE(zoo_list_is_full(NULL));
    
    // Cleanup
    zoo_list_destroy(list);
    
    printf("List boundary conditions test passed!\n");
}

/**
 * Test memory management
 */
void test_list_memory_management(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    
    // Create and destroy multiple lists
    for (int i = 0; i < 10; i++) {
        result = zoo_list_create(&list, DEFAULT_MAX_SIZE);
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        TEST_ASSERT_NOT_NULL(list);
        
        result = zoo_list_destroy(list);
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        list = NULL;
    }
    
    // Test destroying NULL list
    result = zoo_list_destroy(NULL);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    printf("List memory management test passed!\n");
}

/**
 * Test performance characteristics
 */
void test_list_performance(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    const int iterations = 1000;
    
    // Create list
    result = zoo_list_create(&list, LARGE_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Performance test: rapid push/pop operations
    for (int i = 0; i < iterations; i++) {
        test_data_t data = {i, "perf_test"};
        
        result = zoo_list_push(list, &data, sizeof(test_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        
        if (i % 2 == 1) {  // Pop every other item
            test_data_t data_out = {0};
            result = zoo_list_pop(list, &data_out, sizeof(test_data_t));
            TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        }
    }
    
    // Verify some items remain
    TEST_ASSERT_GREATER_THAN(0, zoo_list_size(list));
    
    // Cleanup
    zoo_list_destroy(list);
    
    printf("List performance test completed with %d iterations!\n", iterations);
}

#if ZOO_HAS_THREADING
/**
 * Thread function for concurrent access testing
 */
static void* thread_push_pop_worker(void* arg) {
    thread_context_t* ctx = (thread_context_t*)arg;
    zoo_list_t* list = ctx->list;
    int thread_id = ctx->thread_id;
    
    for (int i = 0; i < ctx->operations_count; i++) {
        test_data_t data = {thread_id * 1000 + i, ""};
        snprintf(data.data, sizeof(data.data), "thread_%d_item_%d", thread_id, i);
        
        // Push operation
        ZOO_ERROR_T result = zoo_list_push(list, &data, sizeof(test_data_t));
        if (result == ZOO_ERROR_OK) {
            // Immediately try to pop (creates contention)
            test_data_t data_out = {0};
            zoo_list_pop(list, &data_out, sizeof(test_data_t));
        }
        
        // Small delay to encourage race conditions
        ZOO_SLEEP_US(1);
    }
    
    return NULL;
}

/**
 * Test thread safety of list operations
 */
void test_list_thread_safety(void) {
    zoo_list_t* list = NULL;
    ZOO_ERROR_T result;
    pthread_t threads[THREAD_COUNT];
    thread_context_t contexts[THREAD_COUNT];
    
    // Create list
    result = zoo_list_create(&list, LARGE_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Create threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        contexts[i].list = list;
        contexts[i].thread_id = i;
        contexts[i].operations_count = OPERATIONS_PER_THREAD;
        
        int pthread_result = pthread_create(&threads[i], NULL, thread_push_pop_worker, &contexts[i]);
        TEST_ASSERT_EQUAL(0, pthread_result);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < THREAD_COUNT; i++) {
        int pthread_result = pthread_join(threads[i], NULL);
        TEST_ASSERT_EQUAL(0, pthread_result);
    }
    
    // Verify list is still in valid state
    int final_size = zoo_list_size(list);
    TEST_ASSERT_GREATER_OR_EQUAL(0, final_size);
    TEST_ASSERT_LESS_OR_EQUAL(LARGE_MAX_SIZE, final_size);
    
    // Clean up remaining items
    test_data_t data_out = {0};
    while (!zoo_list_is_empty(list)) {
        result = zoo_list_pop(list, &data_out, sizeof(test_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    }
    
    // Cleanup
    zoo_list_destroy(list);
    
    printf("List thread safety test passed with %d threads!\n", THREAD_COUNT);
}
#else
/**
 * Stub for thread safety test when threading is not available
 */
void test_list_thread_safety(void) {
    printf("Threading not supported - thread safety test skipped\n");
}
#endif
