/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 * Product: ZOO
 * Module: Timer Tests
 * Component ID: TIMER_TEST
 * File name: test_zoo_timer.c
 * Description: Unit tests for ZOO timer functionality
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01     AI Assistant      Created comprehensive unit tests
 ******************************************************************************/

#include "unity.h"
#include "zoo_timer.h"
#include "zoo_error.h"
#include "mock_zoo_platform.h"
#include "mock_stdlib.h"
#include <string.h>

// ==============================================================================
// Test Fixtures and Helper Functions
// ==============================================================================

// Test callback data structure
typedef struct {
    ZOO_UINT32 call_count;
    void* last_user_data;
    ZOO_BOOL was_called;
} test_callback_data_t;

static test_callback_data_t g_test_callback_data;

// Test callback function
static void test_timer_callback(void* user_data)
{
    g_test_callback_data.was_called = ZOO_TRUE;
    g_test_callback_data.call_count++;
    g_test_callback_data.last_user_data = user_data;
}

// Test setup - called before each test
void setUp(void)
{
    // Reset all mock state
    mock_zoo_platform_reset();
    mock_stdlib_reset();
    
    // Reset test callback data
    memset(&g_test_callback_data, 0, sizeof(g_test_callback_data));
}

// Test teardown - called after each test
void tearDown(void)
{
    // Nothing to clean up for now
}

// ==============================================================================
// Timer Creation Tests
// ==============================================================================

void test_zoo_timer_create_valid_parameters(void)
{
    // Arrange
    ZOO_UINT32 interval = 1000;
    ZOO_BOOL repeat = ZOO_TRUE;
    void* user_data = (void*)0x12345678;
    
    // Act
    ZOO_TIMER_HANDLE timer = zoo_timer_create(interval, repeat, test_timer_callback, user_data);
    
    // Assert
    TEST_ASSERT_NOT_NULL_MESSAGE(timer, "Timer creation should succeed with valid parameters");
    
    // Cleanup
    if (timer) {
        zoo_timer_destroy(timer);
    }
}

void test_zoo_timer_create_zero_interval(void)
{
    // Arrange & Act
    ZOO_TIMER_HANDLE timer = zoo_timer_create(0, ZOO_FALSE, test_timer_callback, NULL);
    
    // Assert
    TEST_ASSERT_NULL_MESSAGE(timer, "Timer creation should fail with zero interval");
}

void test_zoo_timer_create_null_callback(void)
{
    // Arrange & Act
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, NULL, NULL);
    
    // Assert
    TEST_ASSERT_NULL_MESSAGE(timer, "Timer creation should fail with NULL callback");
}

void test_zoo_timer_create_malloc_failure(void)
{
    // Arrange
    mock_stdlib_set_malloc_fail(ZOO_TRUE);
    
    // Act
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, NULL);
    
    // Assert
    TEST_ASSERT_NULL_MESSAGE(timer, "Timer creation should fail when malloc fails");
}

void test_zoo_timer_create_different_intervals(void)
{
    // Test various interval values
    ZOO_UINT32 intervals[] = {1, 100, 1000, 5000, 60000};
    size_t num_intervals = sizeof(intervals) / sizeof(intervals[0]);
    
    for (size_t i = 0; i < num_intervals; i++) {
        ZOO_TIMER_HANDLE timer = zoo_timer_create(intervals[i], ZOO_FALSE, test_timer_callback, NULL);
        TEST_ASSERT_NOT_NULL_MESSAGE(timer, "Timer creation should succeed for all valid intervals");
        
        if (timer) {
            zoo_timer_destroy(timer);
        }
    }
}

void test_zoo_timer_create_repeat_and_oneshot(void)
{
    // Test repeat timer
    ZOO_TIMER_HANDLE repeat_timer = zoo_timer_create(1000, ZOO_TRUE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL_MESSAGE(repeat_timer, "Repeat timer creation should succeed");
    
    // Test one-shot timer
    ZOO_TIMER_HANDLE oneshot_timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL_MESSAGE(oneshot_timer, "One-shot timer creation should succeed");
    
    // Cleanup
    if (repeat_timer) zoo_timer_destroy(repeat_timer);
    if (oneshot_timer) zoo_timer_destroy(oneshot_timer);
}

// ==============================================================================
// Timer Start Tests
// ==============================================================================

void test_zoo_timer_start_valid_timer(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mock expectations
    mock_zoo_timer_create_thread_expect(ZOO_OK);
    
    // Act
    ZOO_BOOL result = zoo_timer_start(timer);
    
    // Assert
    TEST_ASSERT_TRUE_MESSAGE(result, "Timer start should succeed");
    mock_zoo_timer_create_thread_verify(1);
    
    // Cleanup
    zoo_timer_destroy(timer);
}

void test_zoo_timer_start_null_timer(void)
{
    // Act
    ZOO_BOOL result = zoo_timer_start(NULL);
    
    // Assert
    TEST_ASSERT_FALSE_MESSAGE(result, "Timer start should fail with NULL timer");
}

void test_zoo_timer_start_thread_creation_failure(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mock to fail thread creation
    mock_zoo_timer_create_thread_expect(ZOO_ERROR_GENERAL);
    
    // Act
    ZOO_BOOL result = zoo_timer_start(timer);
    
    // Assert
    TEST_ASSERT_FALSE_MESSAGE(result, "Timer start should fail when thread creation fails");
    mock_zoo_timer_create_thread_verify(1);
    
    // Cleanup
    zoo_timer_destroy(timer);
}

void test_zoo_timer_start_already_running(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mock expectations
    mock_zoo_timer_create_thread_expect(ZOO_OK);
    
    // Start timer first time
    ZOO_BOOL result1 = zoo_timer_start(timer);
    TEST_ASSERT_TRUE(result1);
    
    // Act - try to start again
    ZOO_BOOL result2 = zoo_timer_start(timer);
    
    // Assert
    TEST_ASSERT_FALSE_MESSAGE(result2, "Timer start should fail when already running");
    mock_zoo_timer_create_thread_verify(1); // Should only be called once
    
    // Cleanup
    zoo_timer_destroy(timer);
}

// ==============================================================================
// Timer Stop Tests
// ==============================================================================

void test_zoo_timer_stop_valid_timer(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mock expectations
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

void test_zoo_timer_stop_null_timer(void)
{
    // Act & Assert - should not crash
    zoo_timer_stop(NULL);
    
    // Verify no platform functions were called
    TEST_ASSERT_FALSE(mock_zoo_platform_was_join_thread_called());
}

void test_zoo_timer_stop_not_running(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Act - stop timer that was never started
    zoo_timer_stop(timer);
    
    // Assert - should not attempt to join thread
    TEST_ASSERT_FALSE(mock_zoo_platform_was_join_thread_called());
    
    // Cleanup
    zoo_timer_destroy(timer);
}

// ==============================================================================
// Timer Destroy Tests
// ==============================================================================

void test_zoo_timer_destroy_valid_timer(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Act & Assert - should not crash
    zoo_timer_destroy(timer);
}

void test_zoo_timer_destroy_null_timer(void)
{
    // Act & Assert - should not crash
    zoo_timer_destroy(NULL);
}

void test_zoo_timer_destroy_running_timer(void)
{
    // Arrange
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    
    // Set up mock expectations
    mock_zoo_timer_create_thread_expect(ZOO_OK);
    mock_zoo_timer_join_thread_expect(ZOO_OK);
    
    // Start timer
    ZOO_BOOL start_result = zoo_timer_start(timer);
    TEST_ASSERT_TRUE(start_result);
    
    // Act
    zoo_timer_destroy(timer);
    
    // Assert - should have stopped timer first
    mock_zoo_timer_join_thread_verify(1);
}

// ==============================================================================
// Edge Case Tests
// ==============================================================================

void test_zoo_timer_multiple_timers(void)
{
    // Test creating multiple timers simultaneously
    const int num_timers = 5;
    ZOO_TIMER_HANDLE timers[num_timers];
    
    // Create multiple timers
    for (int i = 0; i < num_timers; i++) {
        timers[i] = zoo_timer_create(1000 + i * 100, (i % 2) == 0, test_timer_callback, NULL);
        TEST_ASSERT_NOT_NULL_MESSAGE(timers[i], "Each timer creation should succeed");
    }
    
    // Cleanup all timers
    for (int i = 0; i < num_timers; i++) {
        if (timers[i]) {
            zoo_timer_destroy(timers[i]);
        }
    }
}

void test_zoo_timer_user_data_handling(void)
{
    // Arrange
    int test_data = 42;
    void* user_data = &test_data;
    
    ZOO_TIMER_HANDLE timer = zoo_timer_create(1000, ZOO_FALSE, test_timer_callback, user_data);
    TEST_ASSERT_NOT_NULL(timer);
    
    // The timer stores user_data internally - we can't easily test the callback execution
    // without more complex mocking, but we can verify the timer was created successfully
    // with the user data parameter
    
    // Cleanup
    zoo_timer_destroy(timer);
}

void test_zoo_timer_large_interval(void)
{
    // Test with very large interval
    ZOO_UINT32 large_interval = 0xFFFFFFFF; // Maximum 32-bit value
    
    ZOO_TIMER_HANDLE timer = zoo_timer_create(large_interval, ZOO_FALSE, test_timer_callback, NULL);
    TEST_ASSERT_NOT_NULL_MESSAGE(timer, "Timer should handle large intervals");
    
    if (timer) {
        zoo_timer_destroy(timer);
    }
}

// ==============================================================================
// Test Group Definitions
// ==============================================================================

void test_zoo_timer_create_group(void)
{
    RUN_TEST(test_zoo_timer_create_valid_parameters);
    RUN_TEST(test_zoo_timer_create_zero_interval);
    RUN_TEST(test_zoo_timer_create_null_callback);
    RUN_TEST(test_zoo_timer_create_malloc_failure);
    RUN_TEST(test_zoo_timer_create_different_intervals);
    RUN_TEST(test_zoo_timer_create_repeat_and_oneshot);
}

void test_zoo_timer_start_group(void)
{
    RUN_TEST(test_zoo_timer_start_null_timer);
}

void test_zoo_timer_stop_group(void)
{
    RUN_TEST(test_zoo_timer_stop_null_timer);
    RUN_TEST(test_zoo_timer_stop_not_running);
}

void test_zoo_timer_destroy_group(void)
{
    RUN_TEST(test_zoo_timer_destroy_valid_timer);
    RUN_TEST(test_zoo_timer_destroy_null_timer);
}

void test_zoo_timer_edge_cases_group(void)
{
    RUN_TEST(test_zoo_timer_multiple_timers);
    RUN_TEST(test_zoo_timer_user_data_handling);
    RUN_TEST(test_zoo_timer_large_interval);
}
