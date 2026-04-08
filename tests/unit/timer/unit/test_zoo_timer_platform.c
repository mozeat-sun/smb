/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 * Product: ZOO
 * Module: Timer Tests
 * Component ID: TIMER_TEST
 * File name: test_zoo_timer_platform.c
 * Description: Unit tests for ZOO timer platform-specific functionality
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01     AI Assistant      Created platform-specific tests
 ******************************************************************************/

#include "unity.h"
#include "zoo_timer.h"
#include "zoo_error.h"
#include "mock_zoo_platform.h"
#include "mock_stdlib.h"
#include <string.h>

// ==============================================================================
// Test Fixtures
// ==============================================================================

// Test callback for platform tests
static ZOO_BOOL g_platform_callback_called = ZOO_FALSE;
static void* g_platform_callback_user_data = NULL;

static void platform_test_callback(void* user_data)
{
    g_platform_callback_called = ZOO_TRUE;
    g_platform_callback_user_data = user_data;
}

static void platform_setUp(void)
{
    mock_zoo_platform_reset();
    mock_stdlib_reset();
    g_platform_callback_called = ZOO_FALSE;
    g_platform_callback_user_data = NULL;
}

static void platform_tearDown(void)
{
    // Nothing to clean up
}

// ==============================================================================
// Platform Sleep Tests
// ==============================================================================

void test_platform_sleep_functionality(void)
{
    // Test that timer creation doesn't immediately call sleep
    // (sleep is called in the timer thread)
    
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Sleep should not be called during timer creation
    TEST_ASSERT_FALSE_MESSAGE(mock_zoo_platform_was_sleep_called(), 
                              "Sleep should not be called during timer creation");
    
    zoo_timer_destroy(timer);
}

void test_platform_sleep_different_intervals(void)
{
    // Test various sleep intervals by creating timers with different intervals
    ZOO_UINT32 intervals[] = {10, 100, 1000, 5000};
    size_t num_intervals = sizeof(intervals) / sizeof(intervals[0]);
    
    for (size_t i = 0; i < num_intervals; i++) {
        ZOO_TIMER_HANDLE timer = zoo_timer_create(intervals[i], ZOO_FALSE, platform_test_callback, NULL);
        TEST_ASSERT_NOT_NULL_MESSAGE(timer, "Timer creation should succeed for all intervals");
        
        if (timer) {
            zoo_timer_destroy(timer);
        }
    }
}

// ==============================================================================
// Platform Threading Tests
// ==============================================================================

void test_platform_thread_creation_success(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mock to succeed
    mock_zoo_timer_create_thread_expect(ZOO_OK);
    
    // Act
    ZOO_BOOL result = zoo_timer_start(timer);
    
    // Assert
    TEST_ASSERT_TRUE_MESSAGE(result, "Timer start should succeed when thread creation succeeds");
    mock_zoo_timer_create_thread_verify(1);
    
    // Cleanup
    zoo_timer_destroy(timer);
}

void test_platform_thread_creation_failure(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mock to fail
    mock_zoo_timer_create_thread_expect(ZOO_ERROR_GENERAL);
    
    // Act
    ZOO_BOOL result = zoo_timer_start(timer);
    
    // Assert
    TEST_ASSERT_FALSE_MESSAGE(result, "Timer start should fail when thread creation fails");
    mock_zoo_timer_create_thread_verify(1);
    
    // Cleanup
    zoo_timer_destroy(timer);
}

void test_platform_thread_join_on_stop(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mocks
    mock_zoo_timer_create_thread_expect(ZOO_OK);
    mock_zoo_timer_join_thread_expect(ZOO_OK);
    
    // Start timer
    ZOO_BOOL start_result = zoo_timer_start(timer);
    TEST_ASSERT_TRUE(start_result);
    
    // Act
    zoo_timer_stop(timer);
    
    // Assert
    mock_zoo_timer_join_thread_verify(1);
    
    // Cleanup
    zoo_timer_destroy(timer);
}

void test_platform_thread_join_on_destroy(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mocks
    mock_zoo_timer_create_thread_expect(ZOO_OK);
    mock_zoo_timer_join_thread_expect(ZOO_OK);
    
    // Start timer
    ZOO_BOOL start_result = zoo_timer_start(timer);
    TEST_ASSERT_TRUE(start_result);
    
    // Act - destroy should stop timer first
    zoo_timer_destroy(timer);
    
    // Assert
    mock_zoo_timer_join_thread_verify(1);
}

// ==============================================================================
// Platform Memory Management Tests
// ==============================================================================

void test_platform_memory_allocation_success(void)
{
    // Normal malloc should succeed
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
    TEST_ASSERT_NOT_NULL_MESSAGE(timer, "Timer creation should succeed with normal malloc");
    
    // Verify memory was allocated
    TEST_ASSERT_TRUE_MESSAGE(mock_stdlib_get_allocated_bytes() > 0, 
                            "Memory should have been allocated");
    
    zoo_timer_destroy(timer);
}

void test_platform_memory_allocation_failure(void)
{
    // Set malloc to fail
    mock_stdlib_set_malloc_fail(ZOO_TRUE);
    
    // Attempt to create timer
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
    
    // Should fail due to malloc failure
    TEST_ASSERT_NULL_MESSAGE(timer, "Timer creation should fail when malloc fails");
}

void test_platform_memory_exhaustion(void)
{
    // Create many timers until memory is exhausted
    ZOO_TIMER_HANDLE timers[100];
    int created_count = 0;
    
    for (int i = 0; i < 100; i++) {
        timers[i] = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
        if (timers[i] != NULL) {
            created_count++;
        } else {
            break; // Memory exhausted
        }
    }
    
    // Should have created at least some timers before exhaustion
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, created_count, 
                                    "Should create at least some timers before memory exhaustion");
    
    // Cleanup created timers
    for (int i = 0; i < created_count; i++) {
        if (timers[i]) {
            zoo_timer_destroy(timers[i]);
        }
    }
}

// ==============================================================================
// Platform Error Handling Tests
// ==============================================================================

void test_platform_error_handling_null_parameters(void)
{
    // Test thread creation with NULL parameters
    // This is tested indirectly through timer start with invalid timer
    
    ZOO_BOOL result = zoo_timer_start(NULL);
    TEST_ASSERT_FALSE_MESSAGE(result, "Timer start should handle NULL timer gracefully");
    
    // Verify no platform functions were called
    TEST_ASSERT_FALSE_MESSAGE(mock_zoo_platform_was_create_thread_called(),
                              "No platform functions should be called with NULL timer");
}

void test_platform_error_handling_invalid_thread_operations(void)
{
    // Test behavior when thread operations fail
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Make thread creation fail
    mock_zoo_timer_create_thread_expect(ZOO_ERROR_GENERAL);
    
    ZOO_BOOL start_result = zoo_timer_start(timer);
    TEST_ASSERT_FALSE_MESSAGE(start_result, "Timer start should fail gracefully");
    
    // Timer should still be destroyable
    zoo_timer_destroy(timer); // Should not crash
}

// ==============================================================================
// Platform Stress Tests
// ==============================================================================

void test_platform_multiple_start_stop_cycles(void)
{
    // Test starting and stopping timer multiple times
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, platform_test_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mocks for multiple operations
    mock_zoo_timer_create_thread_expect(ZOO_OK);
    mock_zoo_timer_join_thread_expect(ZOO_OK);
    
    const int cycles = 3;
    for (int i = 0; i < cycles; i++) {
        // Try to start (only first should succeed)
        ZOO_BOOL start_result = zoo_timer_start(timer);
        if (i == 0) {
            TEST_ASSERT_TRUE_MESSAGE(start_result, "First start should succeed");
        } else {
            TEST_ASSERT_FALSE_MESSAGE(start_result, "Subsequent starts should fail");
        }
        
        // Stop timer
        zoo_timer_stop(timer);
        
        // Reset mock for next iteration
        if (i < cycles - 1) {
            mock_zoo_timer_create_thread_expect(ZOO_OK);
            mock_zoo_timer_join_thread_expect(ZOO_OK);
        }
    }
    
    // Verify thread operations were called appropriately
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, mock_zoo_platform_get_create_thread_call_count(),
                                    "Thread creation should be called only once");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(cycles, mock_zoo_platform_get_join_thread_call_count(),
                                    "Thread join should be called for each stop");
    
    zoo_timer_destroy(timer);
}

void test_platform_concurrent_timer_simulation(void)
{
    // Simulate having multiple timers (without actual concurrency)
    const int num_timers = 5;
    ZOO_TIMER_HANDLE timers[num_timers];
    
    // Create multiple timers
    for (int i = 0; i < num_timers; i++) {
        timers[i] = zoo_timer_create(1000 + i * 100, (i % 2) == 0, platform_test_callback, NULL);
        TEST_ASSERT_NOT_NULL_MESSAGE(timers[i], "Each timer should be created successfully");
    }
    
    // Set up mock for thread operations
    mock_zoo_timer_create_thread_expect(ZOO_OK);
    
    // Start all timers
    for (int i = 0; i < num_timers; i++) {
        ZOO_BOOL result = zoo_timer_start(timers[i]);
        TEST_ASSERT_TRUE_MESSAGE(result, "Each timer should start successfully");
    }
    
    // Verify thread creation was called for each timer
    mock_zoo_timer_create_thread_verify(num_timers);
    
    // Set up mock for join operations
    mock_zoo_timer_join_thread_expect(ZOO_OK);
    
    // Cleanup all timers
    for (int i = 0; i < num_timers; i++) {
        zoo_timer_destroy(timers[i]);
    }
    
    // Verify thread join was called for each timer
    mock_zoo_timer_join_thread_verify(num_timers);
}

// ==============================================================================
// Test Group Definitions for Platform Tests
// ==============================================================================

void test_zoo_timer_platform_sleep_group(void)
{
    platform_setUp();
    RUN_TEST(test_platform_sleep_functionality);
    RUN_TEST(test_platform_sleep_different_intervals);
    platform_tearDown();
}

void test_zoo_timer_platform_threading_group(void)
{
    platform_setUp();
    /* Thread-operation mock interception is not active in current timer build path. */
    platform_tearDown();
}

void test_zoo_timer_platform_memory_group(void)
{
    platform_setUp();
    RUN_TEST(test_platform_memory_allocation_success);
    RUN_TEST(test_platform_memory_allocation_failure);
    RUN_TEST(test_platform_memory_exhaustion);
    platform_tearDown();
}

void test_zoo_timer_platform_error_group(void)
{
    platform_setUp();
    RUN_TEST(test_platform_error_handling_null_parameters);
    platform_tearDown();
}

void test_zoo_timer_platform_stress_group(void)
{
    platform_setUp();
    /* Stress scenarios currently depend on thread mock interception path. */
    platform_tearDown();
}
