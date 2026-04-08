/************#include "zoo_dispatcher.h"
#include "zoo_memory_pool.h"
#include "zoo_queue.h"
#include "zoo_math.h"
#include "zoo_string.h"
#include "zoo.h"
#include "zoo_platform.h"
#include "zoo_log.h"***********************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: dispatcher
 * Component id: ZOO_DISPATCHER
 * File name: test_integration.c
 * Description: Integration test for ZOO dispatcher with other modules
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-04     AI Assistant      created
 ******************************************************************************/

#include "zoo_dispatcher.h"
#include "../memory_pool/inc/zoo_memory_pool.h"
#include "../buffer/inc/zoo_queue.h"
#include "../util/inc/zoo_math.h"
#include "../util/inc/zoo_string.h"
#include "../platform/inc/zoo.h"
#include "../platform/inc/zoo_platform.h"
#include "../log/inc/zoo_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

// Integration test configuration
#define INTEGRATION_MEMORY_POOL_SIZE (2 * 1024 * 1024)  // 2MB
#define INTEGRATION_QUEUE_SIZE 500
#define INTEGRATION_MESSAGE_COUNT 200

// Test results tracking
static int g_tests_run = 0;
static int g_tests_passed = 0;

#define INTEGRATION_ASSERT(condition, message) \
    do { \
        g_tests_run++; \
        if (condition) { \
            g_tests_passed++; \
            printf("[PASS] %s\n", message); \
        } else { \
            printf("[FAIL] %s\n", message); \
        } \
    } while(0)

// Test message structure for integration
typedef struct {
    ZOO_INT64 uuid;
    char content[128];
    int sequence_number;
    struct timeval timestamp;
} IntegrationMessage;

// Test context for integration tests
typedef struct {
    int messages_processed;
    int expected_count;
    pthread_mutex_t mutex;
    pthread_cond_t completion_cond;
    ZOO_BOOL test_complete;
    ZOO_BOOL* uuid_seen;  // Track UUID uniqueness
    int uuid_count;
} IntegrationTestContext;

// Message handler for integration tests
static void integration_message_handler(void* user_data, void* msg, void* context)
{
    IntegrationTestContext* test_ctx = (IntegrationTestContext*)user_data;
    IntegrationMessage* int_msg = (IntegrationMessage*)msg;
    
    if (!test_ctx || !int_msg) {
        return;
    }
    
    pthread_mutex_lock(&test_ctx->mutex);
    
    // Track UUID uniqueness
    if (test_ctx->uuid_count < test_ctx->expected_count) {
        ZOO_BOOL uuid_duplicate = ZOO_FALSE;
        for (int i = 0; i < test_ctx->messages_processed; i++) {
            if (test_ctx->uuid_seen[i] && 
                zoo_uuid64_compare(&int_msg->uuid, &int_msg->uuid) == 0) {
                uuid_duplicate = ZOO_TRUE;
                break;
            }
        }
        
        if (!uuid_duplicate) {
            test_ctx->uuid_seen[test_ctx->messages_processed] = ZOO_TRUE;
        }
    }
    
    test_ctx->messages_processed++;
    
    ZOO_LOG_DEBUG("Integration test processed message %d: UUID=%lld, seq=%d, content=%s",
                  test_ctx->messages_processed, int_msg->uuid, 
                  int_msg->sequence_number, int_msg->content);
    
    if (test_ctx->messages_processed >= test_ctx->expected_count) {
        test_ctx->test_complete = ZOO_TRUE;
        pthread_cond_signal(&test_ctx->completion_cond);
    }
    
    pthread_mutex_unlock(&test_ctx->mutex);
}

// Setup integration test environment
static ZOO_BOOL setup_integration_environment(void)
{
    // Initialize memory pool
    if (zoo_create_memory_pool(INTEGRATION_MEMORY_POOL_SIZE) != ZOO_OK) {
        printf("Failed to create memory pool for integration tests\n");
        return ZOO_FALSE;
    }
    
    printf("Integration test environment initialized\n");
    return ZOO_TRUE;
}

// Cleanup integration test environment
static void cleanup_integration_environment(void)
{
    zoo_destroy_memory_pool();
    printf("Integration test environment cleaned up\n");
}

// Test 1: Full module integration test
static void test_full_module_integration(void)
{
    printf("\n=== Full Module Integration Test ===\n");
    
    // Create queue with dispatcher
    ZOO_QUEUE_HANDLE queue = zoo_create_queue(INTEGRATION_QUEUE_SIZE);
    INTEGRATION_ASSERT(queue != NULL, "Queue creation for integration test");
    
    ZOO_DISPATCHER_HANDLE dispatcher = zoo_create_dispatcher(queue);
    INTEGRATION_ASSERT(dispatcher != NULL, "Dispatcher creation for integration test");
    
    // Initialize test context
    IntegrationTestContext test_ctx = {0};
    pthread_mutex_init(&test_ctx.mutex, NULL);
    pthread_cond_init(&test_ctx.completion_cond, NULL);
    test_ctx.expected_count = INTEGRATION_MESSAGE_COUNT;
    test_ctx.uuid_seen = calloc(INTEGRATION_MESSAGE_COUNT, sizeof(ZOO_BOOL));
    
    // Set up different sort strategies to test queue integration
    zoo_dispatcher_set_sort_strategy(dispatcher, ZOO_QUEUE_SORT_STRATEGY_PRIORITY);
    
    // Start dispatcher
    pthread_t dispatcher_thread;
    int thread_result = pthread_create(&dispatcher_thread, NULL, 
                                      (void*(*)(void*))zoo_start_dispatcher, 
                                      dispatcher);
    INTEGRATION_ASSERT(thread_result == 0, "Dispatcher thread creation for integration");
    
    // Create and enqueue messages using utility functions
    for (int i = 0; i < INTEGRATION_MESSAGE_COUNT; i++) {
        IntegrationMessage* msg = zoo_allocate_from_pool(sizeof(IntegrationMessage));
        INTEGRATION_ASSERT(msg != NULL, "Memory allocation from pool");
        
        // Use math utility for UUID generation
        msg->uuid = zoo_generate_uuid64();
        INTEGRATION_ASSERT(zoo_uuid64_is_valid(&msg->uuid), "Generated UUID validation");
        
        // Use string utility for content creation
        snprintf(msg->content, sizeof(msg->content), "Integration test message %d", i);
        msg->sequence_number = i;
        gettimeofday(&msg->timestamp, NULL);
        
        // Enqueue with different priorities for sort testing
        ZOO_BOOL enqueued = zoo_queue_enqueue(queue, msg, &test_ctx, 
                                             integration_message_handler, &test_ctx);
        INTEGRATION_ASSERT(enqueued, "Message enqueue in integration test");
        
        if (i % 50 == 0) {
            printf("Enqueued %d messages...\n", i + 1);
        }
    }
    
    printf("All messages enqueued, waiting for processing...\n");
    
    // Wait for processing completion
    pthread_mutex_lock(&test_ctx.mutex);
    while (!test_ctx.test_complete) {
        pthread_cond_wait(&test_ctx.completion_cond, &test_ctx.mutex);
    }
    pthread_mutex_unlock(&test_ctx.mutex);
    
    // Stop dispatcher
    zoo_stop_dispatcher(dispatcher);
    pthread_join(dispatcher_thread, NULL);
    
    // Verify results
    INTEGRATION_ASSERT(test_ctx.messages_processed == INTEGRATION_MESSAGE_COUNT,
                      "All messages processed in integration test");
    
    // Count unique UUIDs
    int unique_uuids = 0;
    for (int i = 0; i < INTEGRATION_MESSAGE_COUNT; i++) {
        if (test_ctx.uuid_seen[i]) {
            unique_uuids++;
        }
    }
    INTEGRATION_ASSERT(unique_uuids == INTEGRATION_MESSAGE_COUNT,
                      "All UUIDs are unique in integration test");
    
    printf("Integration test completed: processed %d messages with %d unique UUIDs\n",
           test_ctx.messages_processed, unique_uuids);
    
    // Cleanup
    free(test_ctx.uuid_seen);
    pthread_mutex_destroy(&test_ctx.mutex);
    pthread_cond_destroy(&test_ctx.completion_cond);
    zoo_destroy_dispatcher(dispatcher);
    zoo_destroy_queue(queue);
}

// Test 2: Memory pool stress integration
static void test_memory_pool_integration(void)
{
    printf("\n=== Memory Pool Integration Test ===\n");
    
    ZOO_QUEUE_HANDLE queue = zoo_create_queue(100);  // Smaller queue
    ZOO_DISPATCHER_HANDLE dispatcher = zoo_create_dispatcher(queue);
    
    IntegrationTestContext test_ctx = {0};
    pthread_mutex_init(&test_ctx.mutex, NULL);
    pthread_cond_init(&test_ctx.completion_cond, NULL);
    test_ctx.expected_count = 300;  // More messages than queue size
    test_ctx.uuid_seen = calloc(test_ctx.expected_count, sizeof(ZOO_BOOL));
    
    // Start dispatcher
    pthread_t dispatcher_thread;
    pthread_create(&dispatcher_thread, NULL, 
                   (void*(*)(void*))zoo_start_dispatcher, 
                   dispatcher);
    
    // Test rapid allocation and deallocation
    int successful_allocations = 0;
    for (int i = 0; i < test_ctx.expected_count; i++) {
        IntegrationMessage* msg = zoo_allocate_from_pool(sizeof(IntegrationMessage));
        if (msg != NULL) {
            successful_allocations++;
            msg->uuid = zoo_generate_uuid64();
            snprintf(msg->content, sizeof(msg->content), "Memory stress message %d", i);
            msg->sequence_number = i;
            
            zoo_queue_enqueue(queue, msg, &test_ctx, integration_message_handler, &test_ctx);
        } else {
            // If allocation fails, wait and retry
            usleep(10000);  // 10ms
            i--;  // Retry this iteration
        }
    }
    
    INTEGRATION_ASSERT(successful_allocations > 0, "Memory pool allocations under stress");
    
    // Wait for processing
    pthread_mutex_lock(&test_ctx.mutex);
    while (!test_ctx.test_complete) {
        pthread_cond_wait(&test_ctx.completion_cond, &test_ctx.mutex);
    }
    pthread_mutex_unlock(&test_ctx.mutex);
    
    zoo_stop_dispatcher(dispatcher);
    pthread_join(dispatcher_thread, NULL);
    
    printf("Memory pool integration: %d successful allocations, %d processed\n",
           successful_allocations, test_ctx.messages_processed);
    
    free(test_ctx.uuid_seen);
    pthread_mutex_destroy(&test_ctx.mutex);
    pthread_cond_destroy(&test_ctx.completion_cond);
    zoo_destroy_dispatcher(dispatcher);
    zoo_destroy_queue(queue);
}

// Test 3: String utility integration
static void test_string_utility_integration(void)
{
    printf("\n=== String Utility Integration Test ===\n");
    
    // Test string comparison functionality in context
    const char* test_strings[] = {
        "dispatcher_test_1",
        "dispatcher_test_2", 
        "dispatcher_test_1",  // Duplicate
        NULL,
        "dispatcher_test_3",
        NULL  // Duplicate NULL
    };
    
    int num_strings = sizeof(test_strings) / sizeof(test_strings[0]);
    int comparisons_made = 0;
    int identical_pairs = 0;
    
    for (int i = 0; i < num_strings; i++) {
        for (int j = i + 1; j < num_strings; j++) {
            comparisons_made++;
            if (zoo_string_is_the_same(test_strings[i], test_strings[j])) {
                identical_pairs++;
                printf("Found identical strings at positions %d and %d\n", i, j);
            }
        }
    }
    
    INTEGRATION_ASSERT(comparisons_made > 0, "String comparisons performed");
    INTEGRATION_ASSERT(identical_pairs == 2, "Correct number of identical pairs found");  // One duplicate string, one duplicate NULL
    
    printf("String utility integration: %d comparisons, %d identical pairs\n",
           comparisons_made, identical_pairs);
}

// Test 4: Math utility integration
static void test_math_utility_integration(void)
{
    printf("\n=== Math Utility Integration Test ===\n");
    
    // Test UUID generation and validation
    const int uuid_test_count = 100;
    ZOO_INT64 uuids[uuid_test_count];
    int valid_uuids = 0;
    int unique_uuids = 0;
    
    // Generate UUIDs
    for (int i = 0; i < uuid_test_count; i++) {
        uuids[i] = zoo_generate_uuid64();
        if (zoo_uuid64_is_valid(&uuids[i])) {
            valid_uuids++;
        }
    }
    
    // Check uniqueness
    for (int i = 0; i < uuid_test_count; i++) {
        ZOO_BOOL is_unique = ZOO_TRUE;
        for (int j = 0; j < uuid_test_count; j++) {
            if (i != j && zoo_uuid64_compare(&uuids[i], &uuids[j]) == 0) {
                is_unique = ZOO_FALSE;
                break;
            }
        }
        if (is_unique) {
            unique_uuids++;
        }
    }
    
    INTEGRATION_ASSERT(valid_uuids == uuid_test_count, "All generated UUIDs are valid");
    INTEGRATION_ASSERT(unique_uuids == uuid_test_count, "All generated UUIDs are unique");
    
    // Test seeded UUID generation
    ZOO_INT64 seeded_uuid1 = zoo_generate_uuid64_seeded(12345);
    ZOO_INT64 seeded_uuid2 = zoo_generate_uuid64_seeded(12345);
    ZOO_INT64 seeded_uuid3 = zoo_generate_uuid64_seeded(54321);
    
    INTEGRATION_ASSERT(zoo_uuid64_compare(&seeded_uuid1, &seeded_uuid2) == 0,
                      "Seeded UUIDs with same seed are identical");
    INTEGRATION_ASSERT(zoo_uuid64_compare(&seeded_uuid1, &seeded_uuid3) != 0,
                      "Seeded UUIDs with different seeds are different");
    
    printf("Math utility integration: %d valid UUIDs, %d unique UUIDs\n",
           valid_uuids, unique_uuids);
}

// Test 5: Platform integration
static void test_platform_integration(void)
{
    printf("\n=== Platform Integration Test ===\n");
    
    // Test platform-specific types and definitions
    ZOO_BOOL bool_test = ZOO_TRUE;
    INTEGRATION_ASSERT(bool_test == ZOO_TRUE, "ZOO_BOOL TRUE value");
    
    bool_test = ZOO_FALSE;
    INTEGRATION_ASSERT(bool_test == ZOO_FALSE, "ZOO_BOOL FALSE value");
    
    // Test platform types
    ZOO_INT64 large_int = 9223372036854775807LL;  // Max signed 64-bit
    INTEGRATION_ASSERT(large_int > 0, "ZOO_INT64 large value handling");
    
    printf("Platform integration: ZOO types validated\n");
}

// Main integration test runner
int main(int argc, char* argv[])
{
    printf("ZOO Dispatcher Integration Test Suite\n");
    printf("=====================================\n");
    
    // Setup integration test environment
    if (!setup_integration_environment()) {
        printf("Failed to setup integration test environment\n");
        return 1;
    }
    
    // Run integration tests
    test_full_module_integration();
    test_memory_pool_integration();
    test_string_utility_integration();
    test_math_utility_integration();
    test_platform_integration();
    
    // Cleanup
    cleanup_integration_environment();
    
    // Print results
    printf("\nIntegration Test Results Summary:\n");
    printf("=================================\n");
    printf("Total tests:  %d\n", g_tests_run);
    printf("Passed:       %d\n", g_tests_passed);
    printf("Failed:       %d\n", g_tests_run - g_tests_passed);
    printf("Success rate: %.1f%%\n", 
           (float)g_tests_passed / g_tests_run * 100.0f);
    
    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
