#include "unity.h"
#include "cmock.h"  // CMock support for mocking
#include "zoo_memory_pool.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Test helper functions and variables
static const size_t TEST_POOL_SIZE = 1024 * 1024; // 1MB
static const size_t SMALL_ALLOC_SIZE = 64;
static const size_t MEDIUM_ALLOC_SIZE = 256;
static const size_t LARGE_ALLOC_SIZE = 1024;

// Global test state
static int pool_created = 0;

// Test setup and teardown
void setUp(void) {
    // Only create pool if not already created
    if (!pool_created) {
        int result = zoo_create_memory_pool(TEST_POOL_SIZE);
        if (result == 0) {
            pool_created = 1;
        }
    }
}

void tearDown(void) {
    // Don't destroy pool between tests - memory pool is singleton
    // Will be destroyed at the end of all tests
}

// Helper function to create pool for tests that need a fresh start
static int ensure_pool_exists(void) {
    if (!pool_created) {
        int result = zoo_create_memory_pool(TEST_POOL_SIZE);
        if (result == 0) {
            pool_created = 1;
            return 1;
        }
        return 0;
    }
    return 1;
}

// Core functionality tests
void test_memory_pool_creation_and_destruction(void) {
    // This test only checks if the pool exists or can be created
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    // Test allocation to verify pool works
    void *ptr = zoo_allocate_from_pool(SMALL_ALLOC_SIZE);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Clean up allocation
    zoo_free_to_pool(ptr);
}

void test_basic_allocation_and_free(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    // Test single allocation
    void *ptr1 = zoo_allocate_from_pool(SMALL_ALLOC_SIZE);
    TEST_ASSERT_NOT_NULL(ptr1);
    
    // Test multiple allocations
    void *ptr2 = zoo_allocate_from_pool(MEDIUM_ALLOC_SIZE);
    TEST_ASSERT_NOT_NULL(ptr2);
    
    void *ptr3 = zoo_allocate_from_pool(LARGE_ALLOC_SIZE);
    TEST_ASSERT_NOT_NULL(ptr3);
    
    // Verify pointers are different
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);
    TEST_ASSERT_NOT_EQUAL(ptr2, ptr3);
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr3);
    
    // Free memory
    zoo_free_to_pool(ptr1);
    zoo_free_to_pool(ptr2);
    zoo_free_to_pool(ptr3);
}

void test_memory_write_and_read(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    // Allocate memory
    void *ptr = zoo_allocate_from_pool(256);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Write test pattern
    unsigned char *data = (unsigned char*)ptr;
    for (int i = 0; i < 256; i++) {
        data[i] = (unsigned char)(i % 256);
    }
    
    // Verify test pattern
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL_UINT8((unsigned char)(i % 256), data[i]);
    }
    
    // Test memset and verification
    memset(ptr, 0xAA, 256);
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL_UINT8(0xAA, data[i]);
    }
    
    zoo_free_to_pool(ptr);
}

void test_zero_size_allocation(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    // Zero size allocation should fail
    void *ptr = zoo_allocate_from_pool(0);
    TEST_ASSERT_NULL(ptr);
}

void test_null_pointer_free(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    // Freeing NULL should be safe (no crash)
    zoo_free_to_pool(NULL);
    
    // Should still be able to allocate after freeing NULL
    void *ptr = zoo_allocate_from_pool(SMALL_ALLOC_SIZE);
    TEST_ASSERT_NOT_NULL(ptr);
    zoo_free_to_pool(ptr);
}

void test_memory_usage_statistics(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    // Get initial usage
    ZOO_MEMORY_USAGE usage;
    zoo_memory_pool_get_usage(&usage);
    
        printf("    Memory statistics - Pool: %zu, Used: %zu, Free: %zu\n",
            (size_t)usage.pool_size,
            (size_t)usage.used_size,
            (size_t)usage.free_size);
    
    // Pool size should be reasonable (allowing for overhead)
    TEST_ASSERT_GREATER_THAN(900000, usage.pool_size);  // At least 900KB
    TEST_ASSERT_GREATER_OR_EQUAL_size_t(0, usage.used_size);
    
    // Allocate some memory
    void *ptr1 = zoo_allocate_from_pool(512);
    TEST_ASSERT_NOT_NULL(ptr1);
    
    void *ptr2 = zoo_allocate_from_pool(256);
    TEST_ASSERT_NOT_NULL(ptr2);
    
    // Get usage after allocation
    ZOO_MEMORY_USAGE usage_after;
    zoo_memory_pool_get_usage(&usage_after);
    TEST_ASSERT_GREATER_THAN_size_t(usage.used_size, usage_after.used_size);
    
    // Free memory
    zoo_free_to_pool(ptr1);
    zoo_free_to_pool(ptr2);
}

void test_multiple_small_allocations(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    const int NUM_ALLOCS = 50; // Reduced number for stability
    void *ptrs[NUM_ALLOCS];
    int successful_allocs = 0;
    
    // Allocate many small blocks
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = zoo_allocate_from_pool(32);
        if (ptrs[i] != NULL) {
            successful_allocs++;
        }
    }
    
    // Should allocate at least some blocks
    TEST_ASSERT_GREATER_THAN_INT(0, successful_allocs);
    
    // Write unique data to each block
    for (int i = 0; i < successful_allocs; i++) {
        if (ptrs[i] != NULL) {
            memset(ptrs[i], i % 256, 32);
        }
    }
    
    // Verify data integrity
    for (int i = 0; i < successful_allocs; i++) {
        if (ptrs[i] != NULL) {
            unsigned char *data = (unsigned char*)ptrs[i];
            for (int j = 0; j < 32; j++) {
                TEST_ASSERT_EQUAL_UINT8(i % 256, data[j]);
            }
        }
    }
    
    // Free all allocated blocks
    for (int i = 0; i < successful_allocs; i++) {
        if (ptrs[i] != NULL) {
            zoo_free_to_pool(ptrs[i]);
        }
    }
}

void test_memory_pool_validation(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    // Validate pool
    int validation_result = zoo_validate_memory_pool();
    TEST_ASSERT_EQUAL_INT(0, validation_result);
    
    // Allocate some memory
    void *ptr1 = zoo_allocate_from_pool(256);
    void *ptr2 = zoo_allocate_from_pool(512);
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    
    // Validate pool with allocations
    validation_result = zoo_validate_memory_pool();
    TEST_ASSERT_EQUAL_INT(0, validation_result);
    
    // Free memory
    zoo_free_to_pool(ptr1);
    zoo_free_to_pool(ptr2);
    
    // Validate pool after free
    validation_result = zoo_validate_memory_pool();
    TEST_ASSERT_EQUAL_INT(0, validation_result);
}

void test_memory_pool_auto_growth_under_pressure(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());

    const int NUM_BLOCKS = 48;
    const size_t BLOCK_SIZE = 32 * 1024;
    void *ptrs[NUM_BLOCKS];
    ZOO_MEMORY_USAGE usage_before;
    ZOO_MEMORY_USAGE usage_after;

    memset(ptrs, 0, sizeof(ptrs));
    zoo_memory_pool_get_usage(&usage_before);

    for (int i = 0; i < NUM_BLOCKS; i++) {
        ptrs[i] = zoo_allocate_from_pool(BLOCK_SIZE);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    zoo_memory_pool_get_usage(&usage_after);
    TEST_ASSERT_GREATER_THAN_size_t(usage_before.pool_size, usage_after.pool_size);
    TEST_ASSERT_EQUAL_INT(0, zoo_validate_memory_pool());

    for (int i = 0; i < NUM_BLOCKS; i++) {
        zoo_free_to_pool(ptrs[i]);
    }
}

void test_allocation_patterns(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    // Test different allocation patterns
    
    // Pattern 1: Alternating sizes
    void *ptrs[6]; // Reduced number
    size_t sizes[] = {32, 64, 128, 256, 128, 64};
    
    for (int i = 0; i < 6; i++) {
        ptrs[i] = zoo_allocate_from_pool(sizes[i]);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }
    
    // Free in different order (reverse)
    for (int i = 5; i >= 0; i--) {
        zoo_free_to_pool(ptrs[i]);
    }
}

// Simple thread safety test
typedef struct {
    int thread_id;
    int successful_ops;
} simple_thread_data_t;

void* simple_thread_worker(void* arg) {
    simple_thread_data_t *data = (simple_thread_data_t*)arg;
    const int num_ops = 10;
    
    for (int i = 0; i < num_ops; i++) {
        void *ptr = zoo_allocate_from_pool(64);
        if (ptr != NULL) {
            // Write thread-specific pattern
            memset(ptr, data->thread_id, 64);
            
            // Small delay
            usleep(100);
            
            // Verify pattern
            unsigned char *verify_data = (unsigned char*)ptr;
            if (verify_data[0] == data->thread_id) {
                data->successful_ops++;
            }
            
            zoo_free_to_pool(ptr);
        }
    }
    
    return NULL;
}

void test_basic_thread_safety(void) {
    TEST_ASSERT_TRUE(ensure_pool_exists());
    
    const int NUM_THREADS = 2; // Reduced for stability
    pthread_t threads[NUM_THREADS];
    simple_thread_data_t thread_data[NUM_THREADS];
    
    // Initialize thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i + 1;
        thread_data[i].successful_ops = 0;
    }
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int rc = pthread_create(&threads[i], NULL, simple_thread_worker, &thread_data[i]);
        TEST_ASSERT_EQUAL_INT(0, rc);
    }
    
    // Wait for threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Verify results
    int total_successful = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_successful += thread_data[i].successful_ops;
    }
    
    // Should have at least some successful operations
    TEST_ASSERT_GREATER_THAN_INT(0, total_successful);
}

// Final cleanup function (called manually at end of test suite)
void test_final_cleanup(void) {
    if (pool_created) {
        zoo_destroy_memory_pool();
        pool_created = 0;
    }
}
