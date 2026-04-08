/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_utils_
 * File name: test_utils.c
 * Description: Common test utilities implementation for Unity-based tests
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                created
 ******************************************************************************/

#include "test_utils.h"
#include "zoo_smb_message.h"
#include "zoo_smb_qos.h"
#include "zoo_smb_service.h"
#include "zoo_smb_ring_buffer.h"
#include "zoo_list.h"
#include <stdio.h>

// Global memory tracker
test_memory_tracker_t g_test_memory_tracker = {0};

/**
 * @brief Setup memory pool for tests (stub)
 */
void test_setup_memory_pool(void)
{
    // Stub implementation - no actual memory pool needed for basic tests
}

/**
 * @brief Teardown memory pool for tests (stub)
 */
void test_teardown_memory_pool(void)
{
    // Stub implementation - no actual memory pool cleanup needed
}

/**
 * @brief Test malloc wrapper
 */
void* test_malloc(size_t size)
{
    return malloc(size);
}

/**
 * @brief Test free wrapper
 */
void test_free(void* ptr)
{
    free(ptr);
}

/**
 * @brief Assert string equality with better error messages
 */
void test_assert_string_equal(const char* expected, const char* actual)
{
    if (expected == NULL && actual == NULL) {
        return; // Both NULL, considered equal
    }
    
    if (expected == NULL || actual == NULL) {
        char msg[256];
        snprintf(msg, sizeof(msg), "String comparison failed: expected '%s', actual '%s'", 
                expected ? expected : "NULL", actual ? actual : "NULL");
        TEST_FAIL_MESSAGE(msg);
    }
    
    if (strcmp(expected, actual) != 0) {
        char msg[512];
        snprintf(msg, sizeof(msg), "String comparison failed: expected '%s', actual '%s'", expected, actual);
        TEST_FAIL_MESSAGE(msg);
    }
}

/**
 * @brief Assert string is not null
 */
void test_assert_string_not_null(const char* str)
{
    if (str == NULL) {
        TEST_FAIL_MESSAGE("String should not be NULL");
    }
}

/**
 * @brief Assert error is OK
 */
void test_assert_error_ok(ZOO_ERROR_TYPE error)
{
    if (error != ZOO_ERROR_OK) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Expected ZOO_ERROR_OK, got error code: %d", error);
        TEST_FAIL_MESSAGE(msg);
    }
}

/**
 * @brief Assert error is not OK
 */
void test_assert_error_not_ok(ZOO_ERROR_TYPE error)
{
    if (error == ZOO_ERROR_OK) {
        TEST_FAIL_MESSAGE("Expected error, but got ZOO_ERROR_OK");
    }
}

/**
 * @brief Initialize memory tracker
 */
void test_memory_tracker_init(void)
{
    g_test_memory_tracker.ptrs = NULL;
    g_test_memory_tracker.count = 0;
    g_test_memory_tracker.capacity = 0;
}

/**
 * @brief Cleanup memory tracker and free all tracked memory
 */
void test_memory_tracker_cleanup(void)
{
    for (size_t i = 0; i < g_test_memory_tracker.count; i++) {
        if (g_test_memory_tracker.ptrs[i]) {
            free(g_test_memory_tracker.ptrs[i]);
        }
    }
    
    if (g_test_memory_tracker.ptrs) {
        free(g_test_memory_tracker.ptrs);
    }
    
    test_memory_tracker_init();
}

/**
 * @brief Malloc with automatic tracking for cleanup
 */
void* test_tracked_malloc(size_t size)
{
    void* ptr = malloc(size);
    if (!ptr) {
        return NULL;
    }
    
    // Expand tracker if needed
    if (g_test_memory_tracker.count >= g_test_memory_tracker.capacity) {
        size_t new_capacity = g_test_memory_tracker.capacity == 0 ? 16 : g_test_memory_tracker.capacity * 2;
        void** new_ptrs = realloc(g_test_memory_tracker.ptrs, new_capacity * sizeof(void*));
        if (!new_ptrs) {
            free(ptr);
            return NULL;
        }
        g_test_memory_tracker.ptrs = new_ptrs;
        g_test_memory_tracker.capacity = new_capacity;
    }
    
    // Track the pointer
    g_test_memory_tracker.ptrs[g_test_memory_tracker.count++] = ptr;
    return ptr;
}

/**
 * @brief Free tracked memory
 */
void test_tracked_free(void* ptr)
{
    if (!ptr) {
        return;
    }
    
    // Remove from tracker
    for (size_t i = 0; i < g_test_memory_tracker.count; i++) {
        if (g_test_memory_tracker.ptrs[i] == ptr) {
            // Shift remaining pointers
            for (size_t j = i; j < g_test_memory_tracker.count - 1; j++) {
                g_test_memory_tracker.ptrs[j] = g_test_memory_tracker.ptrs[j + 1];
            }
            g_test_memory_tracker.count--;
            break;
        }
    }
    
    free(ptr);
}

//=============================================================================
// ZOO SMB Function Stubs for Testing (minimal set)
//=============================================================================

// Global error variable  
static uint32_t last_error = ZOO_ERROR_OK;

// Error handling stubs
/**
 * @brief Test or example function zoo_set_last_error.
 */
uint32_t zoo_set_last_error(uint32_t error_code) {
    last_error = error_code;
    return error_code;
}

/**
 * @brief Test or example function zoo_get_last_error.
 */
uint32_t zoo_get_last_error(void) {
    return last_error;
}

/**
 * @brief Test or example function zoo_error_string.
 */
const char* zoo_error_string(uint32_t error_code) {
    switch (error_code) {
        case ZOO_ERROR_OK: return "No error";
        case ZOO_ERROR_INVALID_PARAM: return "Invalid parameter";
        case ZOO_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case ZOO_ERROR_NOT_FOUND: return "Not found";
        case ZOO_ERROR_ALREADY_EXISTS: return "Already exists";
        case ZOO_ERROR_OPERATION_FAILED: return "Operation failed";
        case ZOO_ERROR_TIMEOUT: return "Timeout";
        case ZOO_ERROR_INVALID_STATE: return "Invalid state";
        case ZOO_ERROR_PERMISSION_DENIED: return "Permission denied";
        case ZOO_ERROR_NOT_SUPPORTED: return "Not supported";
        case ZOO_ERROR_RESOURCE_BUSY: return "Resource busy";
        default: return "Unknown error";
    }
}

// Memory pool stubs
/**
 * @brief Test or example function zoo_allocate_from_pool.
 */
void* zoo_allocate_from_pool(size_t size) {
    return malloc(size);
}

/**
 * @brief Test or example function zoo_free_to_pool.
 */
void zoo_free_to_pool(void* ptr) {
    free(ptr);
}

// Logging stubs - with correct signatures matching zoo_log.h
// CRC calculation stub
/**
 * @brief Test or example function zoo_calc_crc32.
 */
uint32_t zoo_calc_crc32(const void* data, size_t size) {
    (void)data;
    (void)size;
    // Simple stub - return a dummy CRC value
    return 0x12345678;
}

// Message function stubs
ZOO_SMB_MSG_STRUCT* zoo_smb_copy_message_shallow(const ZOO_SMB_MSG_STRUCT* original)
{
    if (!original) return NULL;
    
    ZOO_SMB_MSG_STRUCT* copy = malloc(sizeof(ZOO_SMB_MSG_STRUCT));
    if (!copy) return NULL;
    
    // Shallow copy - copy header and reference same payload
    copy->header = original->header;
    copy->payload = original->payload; // Shallow copy - same pointer
    
    return copy;
}

// Ring buffer function stubs
size_t zoo_smb_ring_buffer_get_free_space(void* ring_buffer)
{
    if (!ring_buffer)
    {
        return 0;
    }
    size_t used = zoo_smb_ring_buffer_get_available_data_size((ZOO_SMB_RING_BUFFER_HANDLE)ring_buffer);
    const size_t synthetic_capacity = 65536;
    return (used < synthetic_capacity) ? (synthetic_capacity - used) : 0;
}

size_t zoo_smb_ring_buffer_get_used_space(void* ring_buffer)
{
    if (!ring_buffer)
    {
        return 0;
    }
    return zoo_smb_ring_buffer_get_available_data_size((ZOO_SMB_RING_BUFFER_HANDLE)ring_buffer);
}

// Note: Consumer functions removed due to type conflicts with ZOO headers

// Memory pool function implementations
/**
 * @brief Test or example function zoo_create_memory_pool.
 */
ZOO_ERROR_TYPE zoo_create_memory_pool(size_t size) {
    (void)size;
    return ZOO_ERROR_OK;
}

ZOO_INT32_T zoo_validate_memory_pool(void)
{
    return ZOO_ERROR_OK;
}

void zoo_destroy_memory_pool(void)
{
}

// Note: List functions are provided by ZOO library, don't implement them

ZOO_LIST_HANDLE zoo_smb_create_list(size_t capacity)
{
    return zoo_list_create(capacity);
}

void zoo_smb_destroy_list(ZOO_LIST_HANDLE list)
{
    if (list)
    {
        zoo_list_destroy(list);
    }
}

// Don't implement zoo_list_size if it's already implemented elsewhere

// Port manager functions are implemented in the main library, so don't duplicate them
// int zoo_smb_find_available_port(int min_port, int max_port) { ... }
// void zoo_smb_release_port_resources(int port) { ... }


