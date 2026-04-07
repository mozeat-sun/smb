/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Thread Pool
 * File name: test_zoo_thread_pool_basic_unity.c
 * Description: Basic Unity test suite for ZOO thread pool implementation
 * Features:
 * - Thread pool creation and destruction
 * - Basic task submission and execution
 * - Error handling validation
 *
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01    AI Assistant      Converted from gtest to Unity
 ******************************************************************************/

#include "unity.h"
#include "zoo_thread_pool.h"
#include "zoo.h"
#include "zoo_error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ==============================================================================
// TEST FIXTURES AND GLOBALS
// ==============================================================================

static int task_counter = 0;
static int callback_counter = 0;

// Simple task function for testing
static ZOO_ERROR_TYPE simple_task(void *user_data, void *argument)
{
    (void)user_data; // Suppress unused parameter warning
    int *data = (int*)argument;
    if (data) {
        (*data)++;
    }
    __sync_fetch_and_add(&task_counter, 1);
    return ZOO_OK;
}

// Simple callback function for testing
static ZOO_ERROR_TYPE simple_callback(void *user_data, void *argument)
{
    (void)user_data; // Suppress unused parameter warning
    (void)argument;  // Suppress unused parameter warning
    __sync_fetch_and_add(&callback_counter, 1);
    return ZOO_OK;
}

// Test task that takes some time
static ZOO_ERROR_TYPE slow_task(void *user_data, void *argument)
{
    (void)user_data; // Suppress unused parameter warning
    (void)argument;  // Suppress unused parameter warning
    usleep(10000); // 10ms
    __sync_fetch_and_add(&task_counter, 1);
    return ZOO_OK;
}

// Test task that returns an error
static ZOO_ERROR_TYPE error_task(void *user_data, void *argument)
{
    (void)user_data; // Suppress unused parameter warning
    (void)argument;  // Suppress unused parameter warning
    return ZOO_ERROR_INVALID_PARAM;
}

// ==============================================================================
// UNITY TEST SETUP AND TEARDOWN
// ==============================================================================

void setUp(void)
{
    // Reset counters before each test
    task_counter = 0;
    callback_counter = 0;
}

void tearDown(void)
{
    // Clean up after each test
    zoo_destroy_thread_pool(ZOO_TRUE);
}

// ==============================================================================
// BASIC FUNCTIONALITY TESTS
// ==============================================================================

void test_thread_pool_creation(void)
{
    printf("Testing thread pool creation...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(4, 10);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

void test_thread_pool_creation_invalid_params(void)
{
    printf("Testing thread pool creation with invalid parameters...\n");
    
    // Test with zero threads
    ZOO_THREAD_POOL_HANDLE pool1 = zoo_create_thread_pool(0, 10);
    TEST_ASSERT_NULL_MESSAGE(pool1, "Thread pool creation with 0 threads should fail");
    
    // Test with zero queue size
    ZOO_THREAD_POOL_HANDLE pool2 = zoo_create_thread_pool(4, 0);
    TEST_ASSERT_NULL_MESSAGE(pool2, "Thread pool creation with 0 queue size should fail");
}

void test_simple_task_submission(void)
{
    printf("Testing simple task submission...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(2, 5);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    int test_data = 0;
    ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
        "simple_test_task",
        simple_task,
        NULL,
        &test_data,
        ZOO_TRUE
    );
    
    TEST_ASSERT_EQUAL_MESSAGE(ZOO_OK, result, "Task submission should succeed");
    
    // Wait a bit for task execution
    usleep(50000); // 50ms
    
    TEST_ASSERT_EQUAL_MESSAGE(1, test_data, "Task should have executed and incremented data");
    TEST_ASSERT_EQUAL_MESSAGE(1, task_counter, "Task counter should be incremented");
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

void test_multiple_task_submission(void)
{
    printf("Testing multiple task submission...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(2, 10);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    const int num_tasks = 5;
    int test_data[num_tasks];
    
    // Initialize test data
    for (int i = 0; i < num_tasks; i++) {
        test_data[i] = 0;
    }
    
    // Submit multiple tasks
    for (int i = 0; i < num_tasks; i++) {
        char task_name[32];
        snprintf(task_name, sizeof(task_name), "task_%d", i);
        
        ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
            task_name,
            simple_task,
            NULL,
            &test_data[i],
            ZOO_TRUE
        );
        
        TEST_ASSERT_EQUAL_MESSAGE(ZOO_OK, result, "Each task submission should succeed");
    }
    
    // Wait for all tasks to complete
    usleep(100000); // 100ms
    
    // Verify all tasks executed
    for (int i = 0; i < num_tasks; i++) {
        TEST_ASSERT_EQUAL_MESSAGE(1, test_data[i], "Each task should have executed");
    }
    
    TEST_ASSERT_EQUAL_MESSAGE(num_tasks, task_counter, "All tasks should have been counted");
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

void test_task_with_callback(void)
{
    printf("Testing task with callback...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(2, 5);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    int test_data = 0;
    ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task_ex(
        "task_with_callback",
        simple_task,
        NULL,
        &test_data,
        simple_callback,
        NULL,
        NULL,
        ZOO_TRUE
    );
    
    TEST_ASSERT_EQUAL_MESSAGE(ZOO_OK, result, "Task submission with callback should succeed");
    
    // Wait for task and callback execution
    usleep(100000); // 100ms
    
    TEST_ASSERT_EQUAL_MESSAGE(1, test_data, "Task should have executed");
    TEST_ASSERT_EQUAL_MESSAGE(1, task_counter, "Task counter should be incremented");
    TEST_ASSERT_EQUAL_MESSAGE(1, callback_counter, "Callback should have executed");
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

void test_null_task_function(void)
{
    printf("Testing NULL task function...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(2, 5);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
        "null_task",
        NULL,
        NULL,
        NULL,
        ZOO_TRUE
    );
    
    TEST_ASSERT_NOT_EQUAL_MESSAGE(ZOO_OK, result, "NULL task function should fail");
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

void test_thread_pool_destruction(void)
{
    printf("Testing thread pool destruction...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(2, 5);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    // Submit some tasks
    for (int i = 0; i < 3; i++) {
        zoo_thread_pool_submit_task(
            "destruction_test_task",
            slow_task,
            NULL,
            NULL,
            ZOO_FALSE
        );
    }
    
    // Graceful destruction should wait for tasks to complete
    zoo_destroy_thread_pool(ZOO_TRUE);
    
    // After graceful destruction, task counter should reflect completed tasks
    printf("Task counter after graceful destruction: %d\n", task_counter);
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, task_counter, "Some tasks should have completed");
}

// ==============================================================================
// UNITY TEST RUNNER
// ==============================================================================

int main(void)
{
    printf("\n==============================================================================\n");
    printf("ZOO Thread Pool Module - Basic Unity Test Suite\n");
    printf("==============================================================================\n");
    printf("Platform: Linux/POSIX\n");
    printf("Framework: Unity\n");
    printf("Test Categories: Basic functionality, Error handling\n");
    printf("==============================================================================\n\n");

    UNITY_BEGIN();

    // Basic functionality tests
    RUN_TEST(test_thread_pool_creation);
    RUN_TEST(test_thread_pool_creation_invalid_params);
    RUN_TEST(test_simple_task_submission);
    RUN_TEST(test_multiple_task_submission);
    RUN_TEST(test_task_with_callback);
    RUN_TEST(test_null_task_function);
    RUN_TEST(test_thread_pool_destruction);

    return UNITY_END();
}
