/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Platform Tests
 * Component ID: ZOO_PLATFORM_TEST_RTTHREAD
 * File name: test_platform_rtthread.c
 * Description: RT-Thread specific platform tests using Unity
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     GitHub Copilot  Initial creation
 ******************************************************************************/

// Simulate RT-Thread environment by defining RT-Thread macros before including zoo.h
#ifdef TEST_RTTHREAD_MODE
#define RT_THREAD_VERSION 0x50000
#define __RTTHREAD__
#endif

#include "zoo.h"
#include "unity.h"
#include <stdio.h>

// Mock RT-Thread types and functions for testing (only when testing RT-Thread mode)
#ifdef TEST_RTTHREAD_MODE

// Mock RT-Thread types
typedef void* rt_thread_t;
typedef void* rt_mutex_t;
typedef void* rt_sem_t;
typedef unsigned int rt_tick_t;

#define RT_NULL ((void*)0)
#define RT_EOK 0
#define RT_IPC_FLAG_PRIO 0x01
#define RT_WAITING_FOREVER 0xFFFFFFFF

// Mock RT-Thread API functions
static rt_thread_t mock_thread = (rt_thread_t)0x12345678;
static rt_mutex_t mock_mutex = (rt_mutex_t)0x87654321;
static rt_sem_t mock_sem = (rt_sem_t)0xABCDEF00;
static rt_tick_t mock_tick = 1000;
static int mock_operations_success = 1;

rt_thread_t rt_thread_create(const char* name, void (*entry)(void* parameter), 
                            void* parameter, unsigned int stack_size,
                            unsigned char priority, unsigned int tick) {
    (void)name; (void)entry; (void)parameter; (void)stack_size; (void)priority; (void)tick;
    return mock_operations_success ? mock_thread : RT_NULL;
}

int rt_thread_startup(rt_thread_t thread) {
    (void)thread;
    return mock_operations_success ? RT_EOK : -1;
}

int rt_thread_delete(rt_thread_t thread) {
    (void)thread;
    return mock_operations_success ? RT_EOK : -1;
}

rt_mutex_t rt_mutex_create(const char* name, unsigned char flag) {
    (void)name; (void)flag;
    return mock_operations_success ? mock_mutex : RT_NULL;
}

int rt_mutex_delete(rt_mutex_t mutex) {
    (void)mutex;
    return mock_operations_success ? RT_EOK : -1;
}

int rt_mutex_take(rt_mutex_t mutex, int time) {
    (void)mutex; (void)time;
    return mock_operations_success ? RT_EOK : -1;
}

int rt_mutex_release(rt_mutex_t mutex) {
    (void)mutex;
    return mock_operations_success ? RT_EOK : -1;
}

rt_sem_t rt_sem_create(const char* name, unsigned int value, unsigned char flag) {
    (void)name; (void)value; (void)flag;
    return mock_operations_success ? mock_sem : RT_NULL;
}

int rt_sem_delete(rt_sem_t sem) {
    (void)sem;
    return mock_operations_success ? RT_EOK : -1;
}

int rt_sem_take(rt_sem_t sem, int time) {
    (void)sem; (void)time;
    return mock_operations_success ? RT_EOK : -1;
}

int rt_sem_release(rt_sem_t sem) {
    (void)sem;
    return mock_operations_success ? RT_EOK : -1;
}

rt_tick_t rt_tick_get(void) {
    return mock_tick++;
}

void rt_thread_mdelay(unsigned int ms) {
    (void)ms;
    // Mock delay - in real test we might use usleep or similar
}

#endif // TEST_RTTHREAD_MODE

/**
 * Reset RT-Thread mock state for testing
 */
#ifdef TEST_RTTHREAD_MODE
static void reset_rtthread_mocks(void) {
    // Reset mock state
    mock_operations_success = 1;
    mock_tick = 1000;
}

/**
 * Test RT-Thread platform detection
 */
void test_rtthread_detection(void) {
    // Reset mock state
    reset_rtthread_mocks();
    
    // Test that RT-Thread is detected
    const char* platform_info = ZOO_GET_PLATFORM_INFO();
    TEST_ASSERT_NOT_NULL(platform_info);
    
    // Test OS name contains RT-Thread info
    const char* os_name = ZOO_OS_NAME;
    TEST_ASSERT_NOT_NULL(os_name);
    
    // Test feature flags for RT-Thread
    TEST_ASSERT_EQUAL(1, ZOO_HAS_THREADING);
    TEST_ASSERT_EQUAL(1, ZOO_IS_EMBEDDED);
    
    printf("Platform: %s\n", platform_info);
    printf("OS: %s\n", os_name);
    printf("Threading: %s\n", (ZOO_HAS_THREADING ? "Yes" : "No"));
    printf("Embedded: %s\n", (ZOO_IS_EMBEDDED ? "Yes" : "No"));
}

/**
 * Test RT-Thread time functions
 */
void test_rtthread_time_functions(void) {
    // Test RT-Thread time functions
    ZOO_TIME_T time1 = ZOO_TIME_GET();
    ZOO_TIME_T time2 = ZOO_TIME_GET();
    
    // Time should advance
    TEST_ASSERT_TRUE(time2 >= time1);
    
    printf("RT-Thread Time 1: %u\n", (unsigned int)time1);
    printf("RT-Thread Time 2: %u\n", (unsigned int)time2);

    uint64_t ns1 = ZOO_CLOCK_REALTIME_NS();
    uint64_t ns2 = ZOO_CLOCK_REALTIME_NS();

    TEST_ASSERT_TRUE(ns2 >= ns1);

    printf("RT-Thread NS 1: %llu\n", (unsigned long long)ns1);
    printf("RT-Thread NS 2: %llu\n", (unsigned long long)ns2);
}

/**
 * Test RT-Thread thread operations
 */
void test_rtthread_thread_operations(void) {
    // Test thread creation (using mock)
    ZOO_THREAD_T thread;
    
    // Mock thread function
    void thread_func(void* arg) {
        (void)arg;
        // Mock thread execution
    }
    
    // Test successful thread creation
    TEST_ASSERT_TRUE(ZOO_THREAD_CREATE(&thread, thread_func, NULL));
    TEST_ASSERT_TRUE(ZOO_THREAD_JOIN(thread));
    
    // Test failed thread creation
    mock_operations_success = 0;
    TEST_ASSERT_FALSE(ZOO_THREAD_CREATE(&thread, thread_func, NULL));
    
    // Reset for cleanup
    mock_operations_success = 1;
}

/**
 * Test RT-Thread mutex operations
 */
void test_rtthread_mutex_operations(void) {
    ZOO_MUTEX_T mutex;
    
    // Test successful mutex operations
    TEST_ASSERT_TRUE(ZOO_MUTEX_INIT(&mutex));
    TEST_ASSERT_TRUE(ZOO_MUTEX_LOCK(&mutex));
    TEST_ASSERT_TRUE(ZOO_MUTEX_UNLOCK(&mutex));
    TEST_ASSERT_TRUE(ZOO_MUTEX_TRYLOCK(&mutex));
    TEST_ASSERT_TRUE(ZOO_MUTEX_UNLOCK(&mutex));
    TEST_ASSERT_TRUE(ZOO_MUTEX_DESTROY(&mutex));
    
    // Test failed mutex operations
    mock_operations_success = 0;
    TEST_ASSERT_FALSE(ZOO_MUTEX_INIT(&mutex));
    TEST_ASSERT_FALSE(ZOO_MUTEX_LOCK(&mutex));
    TEST_ASSERT_FALSE(ZOO_MUTEX_UNLOCK(&mutex));
    TEST_ASSERT_FALSE(ZOO_MUTEX_DESTROY(&mutex));
    
    // Reset for cleanup
    mock_operations_success = 1;
}

/**
 * Test RT-Thread condition variable operations
 */
void test_rtthread_condition_operations(void) {
    ZOO_COND_T cond;
    ZOO_MUTEX_T mutex;
    
    // Test successful condition variable operations
    TEST_ASSERT_TRUE(ZOO_COND_INIT(&cond));
    TEST_ASSERT_TRUE(ZOO_MUTEX_INIT(&mutex));
    
    TEST_ASSERT_TRUE(ZOO_COND_SIGNAL(&cond));
    TEST_ASSERT_TRUE(ZOO_COND_BROADCAST(&cond));
    
    // Test wait (should return quickly in mock)
    TEST_ASSERT_TRUE(ZOO_MUTEX_LOCK(&mutex));
    int wait_result = ZOO_COND_WAIT(&cond, &mutex);
    (void)wait_result;
    TEST_ASSERT_TRUE(ZOO_MUTEX_UNLOCK(&mutex));
    
    // In mock environment, this depends on implementation
    // We mainly test that it doesn't crash
    
    TEST_ASSERT_TRUE(ZOO_COND_DESTROY(&cond));
    TEST_ASSERT_TRUE(ZOO_MUTEX_DESTROY(&mutex));
    
    // Test failed operations
    mock_operations_success = 0;
    TEST_ASSERT_FALSE(ZOO_COND_INIT(&cond));
    TEST_ASSERT_FALSE(ZOO_COND_SIGNAL(&cond));
    TEST_ASSERT_FALSE(ZOO_COND_DESTROY(&cond));
    
    // Reset for cleanup
    mock_operations_success = 1;
}

/**
 * Test RT-Thread sleep functions
 */
void test_rtthread_sleep_functions(void) {
    // Test that sleep functions exist and can be called
    // In mock environment, these won't actually sleep
    
    ZOO_TIME_T start = ZOO_TIME_GET();
    ZOO_SLEEP_MS(10);
    ZOO_TIME_T end = ZOO_TIME_GET();
    
    // In mock, time might not advance, but function should not crash
    TEST_ASSERT_TRUE(end >= start);
    
    start = ZOO_TIME_GET();
    ZOO_SLEEP_US(1000);
    end = ZOO_TIME_GET();
    
    TEST_ASSERT_TRUE(end >= start);
}

/**
 * Test RT-Thread error handling
 */
void test_rtthread_error_handling(void) {
    // Test error conditions
    mock_operations_success = 0;
    
    ZOO_THREAD_T thread;
    ZOO_MUTEX_T mutex;
    ZOO_COND_T cond;
    
    // All operations should fail gracefully
    TEST_ASSERT_FALSE(ZOO_THREAD_CREATE(&thread, NULL, NULL));
    TEST_ASSERT_FALSE(ZOO_MUTEX_INIT(&mutex));
    TEST_ASSERT_FALSE(ZOO_COND_INIT(&cond));
    
    // Even failed operations should not crash the system
    TEST_ASSERT_FALSE(ZOO_MUTEX_LOCK(&mutex));
    TEST_ASSERT_FALSE(ZOO_MUTEX_UNLOCK(&mutex));
    TEST_ASSERT_FALSE(ZOO_COND_SIGNAL(&cond));
    
    // Reset for cleanup
    mock_operations_success = 1;
}

#else // !TEST_RTTHREAD_MODE

/**
 * Test that indicates RT-Thread mode is not enabled
 */
void test_rtthread_mode_not_enabled(void) {
    printf("RT-Thread test mode not enabled. Define TEST_RTTHREAD_MODE to run RT-Thread specific tests.\n");
    
    // We can still test that the platform abstraction works on current platform
    TEST_ASSERT_NOT_NULL(ZOO_GET_PLATFORM_INFO());
    TEST_ASSERT_NOT_NULL(ZOO_OS_NAME);
    
    // Test basic operations work
    ZOO_TIME_T zoo_time = ZOO_TIME_GET();
    TEST_ASSERT_TRUE(zoo_time >= 0);

    (void)ZOO_CLOCK_REALTIME_NS();
}

#endif // TEST_RTTHREAD_MODE
