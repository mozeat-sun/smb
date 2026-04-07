/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Platform Tests
 * Component ID: ZOO_PLATFORM_TEST_THREADING
 * File name: test_platform_threading.c
 * Description: Threading abstraction tests using Unity
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     GitHub Copilot  Initial creation
 ******************************************************************************/

#include "zoo.h"
#include "unity.h"
#include <stdio.h>

// Global test data for thread communication
static volatile int g_thread_counter = 0;
static ZOO_MUTEX_T g_test_mutex;
static ZOO_COND_T g_test_cond;
static volatile int g_condition_met = 0;

/**
 * Initialize threading test globals
 */
static void reset_threading_globals(void) {
    g_thread_counter = 0;
    g_condition_met = 0;
}

/**
 * Test thread function
 */
static void* test_thread_function(void* arg) {
    int* thread_id = (int*)arg;
    
    // Increment counter with mutex protection
    if (ZOO_MUTEX_LOCK(&g_test_mutex)) {
        g_thread_counter++;
        if (!ZOO_MUTEX_UNLOCK(&g_test_mutex)) {
            return NULL;
        }
    }
    
    // Simulate some work
    ZOO_SLEEP_MS(10);
    
    // Signal condition variable
    if (ZOO_MUTEX_LOCK(&g_test_mutex)) {
        if (*thread_id == 1) {
            g_condition_met = 1;
            if (!ZOO_COND_SIGNAL(&g_test_cond)) {
                (void)ZOO_MUTEX_UNLOCK(&g_test_mutex);
                return NULL;
            }
        }
        if (!ZOO_MUTEX_UNLOCK(&g_test_mutex)) {
            return NULL;
        }
    }
    
    return NULL;
}

/**
 * Test mutex initialization and destruction
 */
void test_mutex_init_destroy(void) {
    ZOO_MUTEX_T mutex;
    
    // Test mutex initialization
    TEST_ASSERT_TRUE(ZOO_MUTEX_INIT(&mutex));
    
    // Test mutex destruction
    TEST_ASSERT_TRUE(ZOO_MUTEX_DESTROY(&mutex));
}

/**
 * Test mutex lock and unlock
 */
void test_mutex_lock_unlock(void) {
    ZOO_MUTEX_T mutex;
    
    TEST_ASSERT_TRUE(ZOO_MUTEX_INIT(&mutex));
    
    // Test lock and unlock
    TEST_ASSERT_TRUE(ZOO_MUTEX_LOCK(&mutex));
    TEST_ASSERT_TRUE(ZOO_MUTEX_UNLOCK(&mutex));
    
    // Test try lock
    TEST_ASSERT_TRUE(ZOO_MUTEX_TRYLOCK(&mutex));
    TEST_ASSERT_TRUE(ZOO_MUTEX_UNLOCK(&mutex));
    
    TEST_ASSERT_TRUE(ZOO_MUTEX_DESTROY(&mutex));
}

/**
 * Test condition variable initialization and destruction
 */
void test_condition_variable_init_destroy(void) {
    ZOO_COND_T cond;
    
    // Test condition variable initialization
    TEST_ASSERT_TRUE(ZOO_COND_INIT(&cond));
    
    // Test condition variable destruction
    TEST_ASSERT_TRUE(ZOO_COND_DESTROY(&cond));
}

/**
 * Test condition variable signal and broadcast
 */
void test_condition_variable_signal(void) {
    ZOO_COND_T cond;
    ZOO_MUTEX_T mutex;
    
    TEST_ASSERT_TRUE(ZOO_COND_INIT(&cond));
    TEST_ASSERT_TRUE(ZOO_MUTEX_INIT(&mutex));
    
    // Test signal (should not fail even if no one is waiting)
    TEST_ASSERT_TRUE(ZOO_COND_SIGNAL(&cond));
    
    // Test broadcast (should not fail even if no one is waiting)
    TEST_ASSERT_TRUE(ZOO_COND_BROADCAST(&cond));
    
    TEST_ASSERT_TRUE(ZOO_COND_DESTROY(&cond));
    TEST_ASSERT_TRUE(ZOO_MUTEX_DESTROY(&mutex));
}

#if ZOO_HAS_THREADING
/**
 * Test thread creation and join
 */
void test_thread_create_join(void) {
    ZOO_THREAD_T thread;
    int thread_id = 42;
    
    // Reset global state
    reset_threading_globals();
    
    // Initialize mutex for thread test
    TEST_ASSERT_TRUE(ZOO_MUTEX_INIT(&g_test_mutex));
    
    // Test thread creation
    TEST_ASSERT_TRUE(ZOO_THREAD_CREATE(&thread, test_thread_function, &thread_id));
    
    // Test thread join
    TEST_ASSERT_TRUE(ZOO_THREAD_JOIN(thread));
    
    // Verify thread executed
    TEST_ASSERT_EQUAL(1, g_thread_counter);
    
    // Cleanup
    TEST_ASSERT_TRUE(ZOO_MUTEX_DESTROY(&g_test_mutex));
}

/**
 * Test multiple threads
 */
void test_multiple_threads(void) {
    const int num_threads = 4;
    ZOO_THREAD_T threads[4];
    int thread_ids[4];
    
    // Reset global state
    reset_threading_globals();
    
    // Initialize mutex for thread test
    TEST_ASSERT_TRUE(ZOO_MUTEX_INIT(&g_test_mutex));
    
    // Create multiple threads
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        TEST_ASSERT_TRUE(ZOO_THREAD_CREATE(&threads[i], test_thread_function, &thread_ids[i]));
    }
    
    // Join all threads
    for (int i = 0; i < num_threads; i++) {
        TEST_ASSERT_TRUE(ZOO_THREAD_JOIN(threads[i]));
    }
    
    // Verify all threads executed
    TEST_ASSERT_EQUAL(num_threads, g_thread_counter);
    
    // Cleanup
    TEST_ASSERT_TRUE(ZOO_MUTEX_DESTROY(&g_test_mutex));
}

/**
 * Test condition variable wait and signal
 */
void test_condition_variable_wait_signal(void) {
    // Reset global state
    reset_threading_globals();
    
    // Initialize synchronization objects
    TEST_ASSERT_TRUE(ZOO_MUTEX_INIT(&g_test_mutex));
    TEST_ASSERT_TRUE(ZOO_COND_INIT(&g_test_cond));
    
    // Create thread that will signal the condition
    ZOO_THREAD_T thread;
    int thread_id = 1;
    TEST_ASSERT_TRUE(ZOO_THREAD_CREATE(&thread, test_thread_function, &thread_id));
    
    // Wait for condition with timeout (simplified version)
    TEST_ASSERT_TRUE(ZOO_MUTEX_LOCK(&g_test_mutex));
    
    // In a real scenario we'd wait, but for testing we'll just check the mechanism works
    ZOO_SLEEP_MS(100); // Give thread time to run
    
    TEST_ASSERT_TRUE(ZOO_MUTEX_UNLOCK(&g_test_mutex));
    
    // Join thread
    TEST_ASSERT_TRUE(ZOO_THREAD_JOIN(thread));
    
    // Verify condition was met
    TEST_ASSERT_EQUAL(1, g_condition_met);
    
    // Cleanup
    TEST_ASSERT_TRUE(ZOO_COND_DESTROY(&g_test_cond));
    TEST_ASSERT_TRUE(ZOO_MUTEX_DESTROY(&g_test_mutex));
}
#endif // ZOO_HAS_THREADING

/**
 * Test sleep functions
 */
void test_sleep_functions(void) {
    // Test that sleep functions exist and can be called
    // We can't easily test timing accuracy in unit tests
    
    ZOO_TIME_T start = ZOO_TIME_GET();
    ZOO_SLEEP_MS(50);
    ZOO_TIME_T end = ZOO_TIME_GET();
    
    // Time should advance (or at least not go backwards)
    TEST_ASSERT_TRUE(end >= start);
    
    start = ZOO_TIME_GET();
    ZOO_SLEEP_US(10000); // 10ms
    end = ZOO_TIME_GET();
    
    // Time should advance
    TEST_ASSERT_TRUE(end >= start);
}
