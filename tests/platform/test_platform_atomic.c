/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Platform Tests
 * Component ID: ZOO_PLATFORM_TEST_ATOMIC
 * File name: test_platform_atomic.c
 * Description: Atomic operations tests using Unity
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     GitHub Copilot  Initial creation
 ******************************************************************************/

#include "zoo.h"
#include "unity.h"
#include <stdio.h>

/**
 * Test atomic boolean operations
 */
void test_atomic_boolean_operations(void) {
    ZOO_ATOMIC_BOOL atomic_bool;
    
    // Test initialization
    ZOO_ATOMIC_INIT(&atomic_bool, ZOO_FALSE);
    TEST_ASSERT_EQUAL(ZOO_FALSE, ZOO_ATOMIC_LOAD(&atomic_bool));
    
    // Test store and load
    ZOO_ATOMIC_STORE(&atomic_bool, ZOO_TRUE);
    TEST_ASSERT_EQUAL(ZOO_TRUE, ZOO_ATOMIC_LOAD(&atomic_bool));
    
    ZOO_ATOMIC_STORE(&atomic_bool, ZOO_FALSE);
    TEST_ASSERT_EQUAL(ZOO_FALSE, ZOO_ATOMIC_LOAD(&atomic_bool));
    
    // Test exchange
    int old_value = ZOO_ATOMIC_EXCHANGE(&atomic_bool, ZOO_TRUE);
    TEST_ASSERT_EQUAL(ZOO_FALSE, old_value);
    TEST_ASSERT_EQUAL(ZOO_TRUE, ZOO_ATOMIC_LOAD(&atomic_bool));
    
    // Test compare and swap
    ZOO_ATOMIC_BOOL expected_bool = ZOO_TRUE;
    int success = ZOO_ATOMIC_COMPARE_EXCHANGE(&atomic_bool, &expected_bool, ZOO_FALSE);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(ZOO_FALSE, ZOO_ATOMIC_LOAD(&atomic_bool));
    
    // Test failed compare and swap
    expected_bool = ZOO_TRUE; // Wrong expected value
    success = ZOO_ATOMIC_COMPARE_EXCHANGE(&atomic_bool, &expected_bool, ZOO_TRUE);
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL(ZOO_FALSE, expected_bool); // Should be updated with actual value
    TEST_ASSERT_EQUAL(ZOO_FALSE, ZOO_ATOMIC_LOAD(&atomic_bool)); // Should remain unchanged
}

/**
 * Test atomic size_t operations
 */
void test_atomic_size_t_operations(void) {
    ZOO_ATOMIC_SIZE_T atomic_size;
    
    // Test initialization
    ZOO_ATOMIC_INIT(&atomic_size, 0);
    TEST_ASSERT_EQUAL(0, ZOO_ATOMIC_LOAD(&atomic_size));
    
    // Test store and load
    ZOO_ATOMIC_STORE(&atomic_size, 42);
    TEST_ASSERT_EQUAL(42, ZOO_ATOMIC_LOAD(&atomic_size));
    
    // Test fetch and add
    size_t old_value = ZOO_ATOMIC_FETCH_ADD(&atomic_size, 8);
    TEST_ASSERT_EQUAL(42, old_value);
    TEST_ASSERT_EQUAL(50, ZOO_ATOMIC_LOAD(&atomic_size));
    
    // Test fetch and sub
    old_value = ZOO_ATOMIC_FETCH_SUB(&atomic_size, 10);
    TEST_ASSERT_EQUAL(50, old_value);
    TEST_ASSERT_EQUAL(40, ZOO_ATOMIC_LOAD(&atomic_size));
    
    // Test exchange
    old_value = ZOO_ATOMIC_EXCHANGE(&atomic_size, 100);
    TEST_ASSERT_EQUAL(40, old_value);
    TEST_ASSERT_EQUAL(100, ZOO_ATOMIC_LOAD(&atomic_size));
    
    // Test compare and swap
    ZOO_ATOMIC_SIZE_T expected_size = 100;
    int success = ZOO_ATOMIC_COMPARE_EXCHANGE(&atomic_size, &expected_size, 200);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(200, ZOO_ATOMIC_LOAD(&atomic_size));
    
    // Test failed compare and swap
    expected_size = 150; // Wrong expected value
    success = ZOO_ATOMIC_COMPARE_EXCHANGE(&atomic_size, &expected_size, 300);
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL(200, expected_size); // Should be updated with actual value
    TEST_ASSERT_EQUAL(200, ZOO_ATOMIC_LOAD(&atomic_size)); // Should remain unchanged
}

/**
 * Test atomic integer operations
 */
void test_atomic_integer_operations(void) {
    ZOO_ATOMIC_INT atomic_int;
    
    // Test initialization
    ZOO_ATOMIC_INIT(&atomic_int, -5);
    TEST_ASSERT_EQUAL(-5, ZOO_ATOMIC_LOAD(&atomic_int));
    
    // Test store and load
    ZOO_ATOMIC_STORE(&atomic_int, 15);
    TEST_ASSERT_EQUAL(15, ZOO_ATOMIC_LOAD(&atomic_int));
    
    // Test fetch and add
    int old_value = ZOO_ATOMIC_FETCH_ADD(&atomic_int, 10);
    TEST_ASSERT_EQUAL(15, old_value);
    TEST_ASSERT_EQUAL(25, ZOO_ATOMIC_LOAD(&atomic_int));
    
    // Test fetch and sub  
    old_value = ZOO_ATOMIC_FETCH_SUB(&atomic_int, 30);
    TEST_ASSERT_EQUAL(25, old_value);
    TEST_ASSERT_EQUAL(-5, ZOO_ATOMIC_LOAD(&atomic_int));
    
    // Test exchange
    old_value = ZOO_ATOMIC_EXCHANGE(&atomic_int, 0);
    TEST_ASSERT_EQUAL(-5, old_value);
    TEST_ASSERT_EQUAL(0, ZOO_ATOMIC_LOAD(&atomic_int));
}

#if ZOO_HAS_THREADING
/**
 * Thread function for concurrent atomic operations test
 */
static void* atomic_increment_thread(void* arg) {
    ZOO_ATOMIC_SIZE_T* counter = (ZOO_ATOMIC_SIZE_T*)arg;
    
    // Perform many atomic increments
    for (int i = 0; i < 1000; i++) {
        ZOO_ATOMIC_FETCH_ADD(counter, 1);
    }
    
    return NULL;
}

/**
 * Test atomic operations under concurrent access
 */
void test_concurrent_atomic_operations(void) {
    ZOO_ATOMIC_SIZE_T shared_counter;
    ZOO_ATOMIC_INIT(&shared_counter, 0);
    
    const int num_threads = 4;
    const int increments_per_thread = 1000;
    ZOO_THREAD_T threads[4];
    
    // Create threads that increment the counter
    for (int i = 0; i < num_threads; i++) {
        TEST_ASSERT_TRUE(ZOO_THREAD_CREATE(&threads[i], atomic_increment_thread, &shared_counter));
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        TEST_ASSERT_TRUE(ZOO_THREAD_JOIN(threads[i]));
    }
    
    // Verify the final count
    size_t final_count = ZOO_ATOMIC_LOAD(&shared_counter);
    TEST_ASSERT_EQUAL(num_threads * increments_per_thread, final_count);
}

/**
 * Thread function for atomic compare-exchange test
 */
static void* atomic_cas_thread(void* arg) {
    ZOO_ATOMIC_SIZE_T* counter = (ZOO_ATOMIC_SIZE_T*)arg;
    
    // Try to increment using compare-and-swap
    for (int i = 0; i < 100; i++) {
        size_t current = ZOO_ATOMIC_LOAD(counter);
        size_t desired = current + 1;
        
        // Keep trying until we succeed
        while (!ZOO_ATOMIC_COMPARE_EXCHANGE(counter, &current, desired)) {
            desired = current + 1;
        }
        
        // Small delay to increase contention
        ZOO_SLEEP_US(1);
    }
    
    return NULL;
}

/**
 * Test atomic compare-and-swap under concurrent access
 */
void test_concurrent_compare_and_swap(void) {
    ZOO_ATOMIC_SIZE_T shared_counter;
    ZOO_ATOMIC_INIT(&shared_counter, 0);
    
    const int num_threads = 3;
    const int increments_per_thread = 100;
    ZOO_THREAD_T threads[3];
    
    // Create threads that use compare-and-swap to increment
    for (int i = 0; i < num_threads; i++) {
        TEST_ASSERT_TRUE(ZOO_THREAD_CREATE(&threads[i], atomic_cas_thread, &shared_counter));
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        TEST_ASSERT_TRUE(ZOO_THREAD_JOIN(threads[i]));
    }
    
    // Verify the final count
    size_t final_count = ZOO_ATOMIC_LOAD(&shared_counter);
    TEST_ASSERT_EQUAL(num_threads * increments_per_thread, final_count);
}
#endif // ZOO_HAS_THREADING

/**
 * Test atomic memory ordering (if supported)
 */
void test_memory_ordering(void) {
    ZOO_ATOMIC_SIZE_T atomic1, atomic2;
    
    ZOO_ATOMIC_INIT(&atomic1, 0);
    ZOO_ATOMIC_INIT(&atomic2, 0);
    
    // Test basic ordering - store should be visible after load
    ZOO_ATOMIC_STORE(&atomic1, 1);
    ZOO_ATOMIC_STORE(&atomic2, 2);
    
    TEST_ASSERT_EQUAL(1, ZOO_ATOMIC_LOAD(&atomic1));
    TEST_ASSERT_EQUAL(2, ZOO_ATOMIC_LOAD(&atomic2));
    
    // Test exchange preserves ordering
    size_t old1 = ZOO_ATOMIC_EXCHANGE(&atomic1, 10);
    size_t old2 = ZOO_ATOMIC_EXCHANGE(&atomic2, 20);
    
    TEST_ASSERT_EQUAL(1, old1);
    TEST_ASSERT_EQUAL(2, old2);
    TEST_ASSERT_EQUAL(10, ZOO_ATOMIC_LOAD(&atomic1));
    TEST_ASSERT_EQUAL(20, ZOO_ATOMIC_LOAD(&atomic2));
}
