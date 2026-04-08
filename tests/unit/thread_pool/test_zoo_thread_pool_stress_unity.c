/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Thread Pool
 * File name: test_zoo_thread_pool_stress_unity.c
 * Description: Stress Unity test suite for ZOO thread pool implementation
 * Features:
 * - High-load stress testing
 * - Concurrent task submission
 * - Performance validation
 * - Resource cleanup validation
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
#include <time.h>

// ==============================================================================
// TEST FIXTURES AND GLOBALS
// ==============================================================================

static volatile int stress_counter = 0;
static volatile int error_counter = 0;

// Stress test task function
static ZOO_ERROR_TYPE stress_task(void *user_data, void *argument)
{
    (void)user_data; // Suppress unused parameter warning
    
    int *task_id = (int*)argument;
    if (task_id) {
        // Simulate some work
        volatile int sum = 0;
        for (int i = 0; i < 1000; i++) {
            sum += i;
        }
        
        // Small random delay to simulate varied work
        usleep(rand() % 5000); // 0-5ms
    }
    
    __sync_fetch_and_add(&stress_counter, 1);
    return ZOO_OK;
}

// CPU intensive task
static ZOO_ERROR_TYPE cpu_intensive_task(void *user_data, void *argument)
{
    (void)user_data; // Suppress unused parameter warning
    (void)argument;  // Suppress unused parameter warning
    
    // CPU intensive computation
    volatile double result = 0.0;
    for (int i = 0; i < 10000; i++) {
        result += i * 1.5 + 0.5;
    }
    
    __sync_fetch_and_add(&stress_counter, 1);
    return ZOO_OK;
}

// Memory allocation task
static ZOO_ERROR_TYPE memory_task(void *user_data, void *argument)
{
    (void)user_data; // Suppress unused parameter warning
    (void)argument;  // Suppress unused parameter warning
    
    // Allocate and free memory
    void *ptr = malloc(1024);
    if (ptr) {
        memset(ptr, 0x42, 1024);
        free(ptr);
    }
    
    __sync_fetch_and_add(&stress_counter, 1);
    return ZOO_OK;
}

// Task that sometimes fails
static ZOO_ERROR_TYPE sometimes_fail_task(void *user_data, void *argument)
{
    (void)user_data; // Suppress unused parameter warning
    
    int *task_id = (int*)argument;
    if (task_id && (*task_id % 10 == 0)) {
        __sync_fetch_and_add(&error_counter, 1);
        return ZOO_ERROR_INVALID_PARAM;
    }
    
    __sync_fetch_and_add(&stress_counter, 1);
    return ZOO_OK;
}

// ==============================================================================
// UNITY TEST SETUP AND TEARDOWN
// ==============================================================================

void setUp(void)
{
    // Reset counters before each test
    stress_counter = 0;
    error_counter = 0;
    srand(time(NULL));
}

void tearDown(void)
{
    // Clean up after each test
    zoo_destroy_thread_pool(ZOO_TRUE);
}

// ==============================================================================
// STRESS TESTS
// ==============================================================================

void test_high_load_stress(void)
{
    printf("Testing high load stress...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(4, 100);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    const int num_tasks = 50;
    int task_ids[num_tasks];
    int successful_submissions = 0;
    
    printf("Submitting %d stress tasks...\n", num_tasks);
    
    // Submit many tasks
    for (int i = 0; i < num_tasks; i++) {
        task_ids[i] = i;
        
        ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
            "stress_task",
            stress_task,
            NULL,
            &task_ids[i],
            ZOO_FALSE  // Non-blocking
        );
        
        if (result == ZOO_OK) {
            successful_submissions++;
        }
    }
    
    printf("Successfully submitted %d tasks\n", successful_submissions);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, successful_submissions, "At least some tasks should be submitted");
    
    // Wait for tasks to complete
    usleep(2000000); // 2 seconds
    
    printf("Completed tasks: %d\n", stress_counter);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, stress_counter, "Some tasks should have completed");
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

void test_cpu_intensive_stress(void)
{
    printf("Testing CPU intensive stress...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(2, 20);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    const int num_tasks = 10;
    
    printf("Submitting %d CPU intensive tasks...\n", num_tasks);
    
    clock_t start_time = clock();
    
    // Submit CPU intensive tasks
    for (int i = 0; i < num_tasks; i++) {
        ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
            "cpu_intensive_task",
            cpu_intensive_task,
            NULL,
            NULL,
            ZOO_TRUE  // Blocking
        );
        
        TEST_ASSERT_EQUAL_MESSAGE(ZOO_OK, result, "CPU intensive task submission should succeed");
    }
    
    // Wait for completion
    usleep(3000000); // 3 seconds
    
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("CPU intensive tasks completed: %d in %.2f seconds\n", stress_counter, elapsed_time);
    TEST_ASSERT_EQUAL_MESSAGE(num_tasks, stress_counter, "All CPU intensive tasks should complete");
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

void test_memory_stress(void)
{
    printf("Testing memory allocation stress...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(3, 30);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    const int num_tasks = 25;
    
    printf("Submitting %d memory tasks...\n", num_tasks);
    
    // Submit memory allocation tasks
    for (int i = 0; i < num_tasks; i++) {
        ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
            "memory_task",
            memory_task,
            NULL,
            NULL,
            ZOO_FALSE  // Non-blocking
        );
        
        if (result != ZOO_OK) {
            printf("Task %d submission failed\n", i);
        }
    }
    
    // Wait for completion
    usleep(1000000); // 1 second
    
    printf("Memory tasks completed: %d\n", stress_counter);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, stress_counter, "Some memory tasks should complete");
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

void test_mixed_success_failure_tasks(void)
{
    printf("Testing mixed success/failure tasks...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(2, 25);
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    const int num_tasks = 20;
    int task_ids[num_tasks];
    
    printf("Submitting %d mixed tasks (some will fail)...\n", num_tasks);
    
    // Submit tasks where some will fail
    for (int i = 0; i < num_tasks; i++) {
        task_ids[i] = i;
        
        ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
            "sometimes_fail_task",
            sometimes_fail_task,
            NULL,
            &task_ids[i],
            ZOO_TRUE  // Blocking
        );
        
        TEST_ASSERT_EQUAL_MESSAGE(ZOO_OK, result, "Task submission should succeed");
    }
    
    // Wait for completion
    usleep(1000000); // 1 second
    
    printf("Successful tasks: %d, Failed tasks: %d\n", stress_counter, error_counter);
    
    // We expect some tasks to succeed and some to fail
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, stress_counter, "Some tasks should succeed");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, error_counter, "Some tasks should fail");
    TEST_ASSERT_EQUAL_MESSAGE(num_tasks, stress_counter + error_counter, "All tasks should be processed");
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

void test_rapid_creation_destruction(void)
{
    printf("Testing rapid pool creation and destruction...\n");
    
    const int num_cycles = 5;
    
    for (int cycle = 0; cycle < num_cycles; cycle++) {
        printf("Cycle %d/%d\n", cycle + 1, num_cycles);
        
        ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(2, 10);
        TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
        
        // Submit a few quick tasks
        for (int i = 0; i < 3; i++) {
            zoo_thread_pool_submit_task(
                "rapid_test_task",
                stress_task,
                NULL,
                NULL,
                ZOO_FALSE
            );
        }
        
        // Small delay
        usleep(10000); // 10ms
        
        // Destroy pool
        zoo_destroy_thread_pool(ZOO_FALSE); // Non-graceful for speed
        
        usleep(50000); // 50ms between cycles
    }
    
    printf("Rapid creation/destruction test completed\n");
    TEST_ASSERT_TRUE_MESSAGE(1, "Rapid creation/destruction should not crash");
}

void test_queue_overflow(void)
{
    printf("Testing queue overflow behavior...\n");
    
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(1, 5); // Small queue
    TEST_ASSERT_NOT_NULL_MESSAGE(pool, "Thread pool creation should succeed");
    
    const int num_tasks = 20; // More than queue size
    int successful_submissions = 0;
    int failed_submissions = 0;
    
    printf("Submitting %d tasks to small queue (size 5)...\n", num_tasks);
    
    // Try to submit more tasks than queue can hold
    for (int i = 0; i < num_tasks; i++) {
        ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
            "overflow_task",
            stress_task,
            NULL,
            NULL,
            ZOO_FALSE  // Non-blocking - should fail when queue is full
        );
        
        if (result == ZOO_OK) {
            successful_submissions++;
        } else {
            failed_submissions++;
        }
    }
    
    printf("Successful submissions: %d, Failed submissions: %d\n", 
           successful_submissions, failed_submissions);
    
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, successful_submissions, "Some tasks should be submitted");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, failed_submissions, "Some submissions should fail due to queue overflow");
    
    // Wait for queued tasks to complete
    usleep(1000000); // 1 second
    
    zoo_destroy_thread_pool(ZOO_TRUE);
}

// ==============================================================================
// UNITY TEST RUNNER
// ==============================================================================

int main(void)
{
    printf("\n==============================================================================\n");
    printf("ZOO Thread Pool Module - Stress Unity Test Suite\n");
    printf("==============================================================================\n");
    printf("Platform: Linux/POSIX\n");
    printf("Framework: Unity\n");
    printf("Test Categories: Stress testing, Performance, Resource management\n");
    printf("==============================================================================\n\n");

    UNITY_BEGIN();

    // Stress tests
    RUN_TEST(test_high_load_stress);
    RUN_TEST(test_cpu_intensive_stress);
    RUN_TEST(test_memory_stress);
    RUN_TEST(test_mixed_success_failure_tasks);
    RUN_TEST(test_rapid_creation_destruction);
    RUN_TEST(test_queue_overflow);

    return UNITY_END();
}
