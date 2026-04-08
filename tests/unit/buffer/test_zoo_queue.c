/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 *
 * Product: ZOO
 * Module: buffer
 * Component ID: queue
 * File name: test_zoo_queue.c
 * Description: Unity C test suite for ZOO queue implementation
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
typedef struct zoo_queue zoo_queue_t;

// Error codes
#define ZOO_ERROR_OK 0
#define ZOO_ERROR_INVALID_PARAM -1
#define ZOO_ERROR_EMPTY -2
#define ZOO_ERROR_FULL -3

// Platform macros
#define ZOO_HAS_THREADING 1
#define ZOO_SLEEP_US(us) usleep(us)

// Function declarations (assuming these are implemented in the buffer library)
ZOO_ERROR_T zoo_queue_create(zoo_queue_t** queue, int max_size);
ZOO_ERROR_T zoo_queue_destroy(zoo_queue_t* queue);
ZOO_ERROR_T zoo_queue_enqueue(zoo_queue_t* queue, const void* data, size_t size);
ZOO_ERROR_T zoo_queue_dequeue(zoo_queue_t* queue, void* data, size_t size);
int zoo_queue_size(zoo_queue_t* queue);
int zoo_queue_is_empty(zoo_queue_t* queue);
int zoo_queue_is_full(zoo_queue_t* queue);

#if ZOO_HAS_THREADING
#include <pthread.h>
#include <unistd.h>
#endif

// ==============================================================================
// TEST CONSTANTS AND UTILITIES
// ==============================================================================

#define DEFAULT_MAX_SIZE 100
#define THREAD_COUNT 4
#define OPERATIONS_PER_THREAD 100

// Test data structure
typedef struct {
    int value;
    char data[32];
} test_queue_data_t;

// Thread test context
typedef struct {
    zoo_queue_t* queue;
    int thread_id;
    int operations_count;
} queue_thread_context_t;

// ==============================================================================
// BASIC QUEUE OPERATIONS TESTS
// ==============================================================================

/**
 * Test queue creation and destruction
 */
void test_queue_create_destroy(void) {
    zoo_queue_t* queue = NULL;
    ZOO_ERROR_T result;
    
    // Test successful creation
    result = zoo_queue_create(&queue, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Test queue properties
    TEST_ASSERT_EQUAL(0, zoo_queue_size(queue));
    TEST_ASSERT_TRUE(zoo_queue_is_empty(queue));
    TEST_ASSERT_FALSE(zoo_queue_is_full(queue));
    
    // Test destruction
    result = zoo_queue_destroy(queue);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test creation with invalid parameters
    result = zoo_queue_create(NULL, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_queue_create(&queue, 0);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    printf("Queue create/destroy test passed!\n");
}

/**
 * Test basic enqueue and dequeue operations
 */
void test_queue_enqueue_dequeue_basic(void) {
    zoo_queue_t* queue = NULL;
    ZOO_ERROR_T result;
    test_queue_data_t data_in = {42, "test_data"};
    test_queue_data_t data_out = {0};
    
    // Create queue
    result = zoo_queue_create(&queue, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test enqueue operation
    result = zoo_queue_enqueue(queue, &data_in, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    TEST_ASSERT_EQUAL(1, zoo_queue_size(queue));
    TEST_ASSERT_FALSE(zoo_queue_is_empty(queue));
    
    // Test dequeue operation
    result = zoo_queue_dequeue(queue, &data_out, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    TEST_ASSERT_EQUAL(data_in.value, data_out.value);
    TEST_ASSERT_EQUAL_STRING(data_in.data, data_out.data);
    TEST_ASSERT_EQUAL(0, zoo_queue_size(queue));
    TEST_ASSERT_TRUE(zoo_queue_is_empty(queue));
    
    // Test dequeue from empty queue
    result = zoo_queue_dequeue(queue, &data_out, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_EMPTY, result);
    
    // Cleanup
    zoo_queue_destroy(queue);
    
    printf("Queue basic enqueue/dequeue test passed!\n");
}

/**
 * Test multiple enqueue and dequeue operations (FIFO behavior)
 */
void test_queue_enqueue_dequeue_multiple(void) {
    zoo_queue_t* queue = NULL;
    ZOO_ERROR_T result;
    const int test_count = 10;
    
    // Create queue
    result = zoo_queue_create(&queue, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Enqueue multiple items
    for (int i = 0; i < test_count; i++) {
        test_queue_data_t data = {i, ""};
        snprintf(data.data, sizeof(data.data), "item_%d", i);
        
        result = zoo_queue_enqueue(queue, &data, sizeof(test_queue_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        TEST_ASSERT_EQUAL(i + 1, zoo_queue_size(queue));
    }
    
    // Verify queue is not full yet
    TEST_ASSERT_FALSE(zoo_queue_is_full(queue));
    
    // Dequeue multiple items (FIFO behavior - first in, first out)
    for (int i = 0; i < test_count; i++) {
        test_queue_data_t data_out = {0};
        char expected_str[32];
        
        snprintf(expected_str, sizeof(expected_str), "item_%d", i);
        
        result = zoo_queue_dequeue(queue, &data_out, sizeof(test_queue_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        TEST_ASSERT_EQUAL(i, data_out.value);
        TEST_ASSERT_EQUAL_STRING(expected_str, data_out.data);
        TEST_ASSERT_EQUAL(test_count - i - 1, zoo_queue_size(queue));
    }
    
    TEST_ASSERT_TRUE(zoo_queue_is_empty(queue));
    
    // Cleanup
    zoo_queue_destroy(queue);
    
    printf("Queue multiple enqueue/dequeue test passed!\n");
}

/**
 * Test queue size operations
 */
void test_queue_size_operations(void) {
    zoo_queue_t* queue = NULL;
    ZOO_ERROR_T result;
    
    // Create queue
    result = zoo_queue_create(&queue, 5);  // Small queue for testing
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test size tracking
    TEST_ASSERT_EQUAL(0, zoo_queue_size(queue));
    
    // Add items and check size
    for (int i = 0; i < 5; i++) {
        test_queue_data_t data = {i, "test"};
        result = zoo_queue_enqueue(queue, &data, sizeof(test_queue_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        TEST_ASSERT_EQUAL(i + 1, zoo_queue_size(queue));
    }
    
    // Queue should be full now
    TEST_ASSERT_TRUE(zoo_queue_is_full(queue));
    
    // Try to add one more (should fail)
    test_queue_data_t extra_data = {99, "extra"};
    result = zoo_queue_enqueue(queue, &extra_data, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_FULL, result);
    TEST_ASSERT_EQUAL(5, zoo_queue_size(queue));
    
    // Remove items and check size
    for (int i = 4; i >= 0; i--) {
        test_queue_data_t data_out = {0};
        result = zoo_queue_dequeue(queue, &data_out, sizeof(test_queue_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
        TEST_ASSERT_EQUAL(i, zoo_queue_size(queue));
    }
    
    TEST_ASSERT_TRUE(zoo_queue_is_empty(queue));
    
    // Cleanup
    zoo_queue_destroy(queue);
    
    printf("Queue size operations test passed!\n");
}

/**
 * Test empty queue operations
 */
void test_queue_empty_operations(void) {
    zoo_queue_t* queue = NULL;
    ZOO_ERROR_T result;
    test_queue_data_t data_out = {0};
    
    // Create queue
    result = zoo_queue_create(&queue, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test empty queue properties
    TEST_ASSERT_TRUE(zoo_queue_is_empty(queue));
    TEST_ASSERT_EQUAL(0, zoo_queue_size(queue));
    
    // Test operations on empty queue
    result = zoo_queue_dequeue(queue, &data_out, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_EMPTY, result);
    
    // Cleanup
    zoo_queue_destroy(queue);
    
    printf("Queue empty operations test passed!\n");
}

/**
 * Test full queue operations
 */
void test_queue_full_operations(void) {
    zoo_queue_t* queue = NULL;
    ZOO_ERROR_T result;
    const int max_size = 3;  // Small queue for testing
    
    // Create small queue
    result = zoo_queue_create(&queue, max_size);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Fill the queue
    for (int i = 0; i < max_size; i++) {
        test_queue_data_t data = {i, "test"};
        result = zoo_queue_enqueue(queue, &data, sizeof(test_queue_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    }
    
    // Verify queue is full
    TEST_ASSERT_TRUE(zoo_queue_is_full(queue));
    TEST_ASSERT_EQUAL(max_size, zoo_queue_size(queue));
    
    // Try to add more items
    test_queue_data_t extra_data = {99, "extra"};
    result = zoo_queue_enqueue(queue, &extra_data, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_FULL, result);
    
    // Cleanup
    zoo_queue_destroy(queue);
    
    printf("Queue full operations test passed!\n");
}

/**
 * Test boundary conditions and error handling
 */
void test_queue_boundary_conditions(void) {
    zoo_queue_t* queue = NULL;
    ZOO_ERROR_T result;
    test_queue_data_t data = {42, "test"};
    test_queue_data_t data_out = {0};
    
    // Create queue
    result = zoo_queue_create(&queue, DEFAULT_MAX_SIZE);
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Test NULL pointer handling
    result = zoo_queue_enqueue(NULL, &data, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_queue_enqueue(queue, NULL, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_queue_dequeue(NULL, &data_out, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_queue_dequeue(queue, NULL, sizeof(test_queue_data_t));
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    // Test zero size handling
    result = zoo_queue_enqueue(queue, &data, 0);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_queue_dequeue(queue, &data_out, 0);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    // Test size functions with NULL
    TEST_ASSERT_EQUAL(-1, zoo_queue_size(NULL));
    TEST_ASSERT_FALSE(zoo_queue_is_empty(NULL));
    TEST_ASSERT_FALSE(zoo_queue_is_full(NULL));
    
    // Cleanup
    zoo_queue_destroy(queue);
    
    printf("Queue boundary conditions test passed!\n");
}

#if ZOO_HAS_THREADING
/**
 * Thread function for concurrent queue access testing
 */
static void* thread_queue_worker(void* arg) {
    queue_thread_context_t* ctx = (queue_thread_context_t*)arg;
    zoo_queue_t* queue = ctx->queue;
    int thread_id = ctx->thread_id;
    
    for (int i = 0; i < ctx->operations_count; i++) {
        test_queue_data_t data = {thread_id * 1000 + i, ""};
        snprintf(data.data, sizeof(data.data), "thread_%d_item_%d", thread_id, i);
        
        // Enqueue operation
        ZOO_ERROR_T result = zoo_queue_enqueue(queue, &data, sizeof(test_queue_data_t));
        if (result == ZOO_ERROR_OK) {
            // Immediately try to dequeue (creates contention)
            test_queue_data_t data_out = {0};
            zoo_queue_dequeue(queue, &data_out, sizeof(test_queue_data_t));
        }
        
        // Small delay to encourage race conditions
        ZOO_SLEEP_US(1);
    }
    
    return NULL;
}

/**
 * Test thread safety of queue operations
 */
void test_queue_thread_safety(void) {
    zoo_queue_t* queue = NULL;
    ZOO_ERROR_T result;
    pthread_t threads[THREAD_COUNT];
    queue_thread_context_t contexts[THREAD_COUNT];
    
    // Create queue
    result = zoo_queue_create(&queue, 10000);  // Large queue for threading test
    TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    
    // Create threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        contexts[i].queue = queue;
        contexts[i].thread_id = i;
        contexts[i].operations_count = OPERATIONS_PER_THREAD;
        
        int pthread_result = pthread_create(&threads[i], NULL, thread_queue_worker, &contexts[i]);
        TEST_ASSERT_EQUAL(0, pthread_result);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < THREAD_COUNT; i++) {
        int pthread_result = pthread_join(threads[i], NULL);
        TEST_ASSERT_EQUAL(0, pthread_result);
    }
    
    // Verify queue is still in valid state
    int final_size = zoo_queue_size(queue);
    TEST_ASSERT_GREATER_OR_EQUAL(0, final_size);
    
    // Clean up remaining items
    test_queue_data_t data_out = {0};
    while (!zoo_queue_is_empty(queue)) {
        result = zoo_queue_dequeue(queue, &data_out, sizeof(test_queue_data_t));
        TEST_ASSERT_EQUAL(ZOO_ERROR_OK, result);
    }
    
    // Cleanup
    zoo_queue_destroy(queue);
    
    printf("Queue thread safety test passed with %d threads!\n", THREAD_COUNT);
}
#else
/**
 * Stub for thread safety test when threading is not available
 */
void test_queue_thread_safety(void) {
    printf("Threading not supported - queue thread safety test skipped\n");
}
#endif
