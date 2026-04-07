/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights r// Note: Consumer functions are provided by actual SMB implementation when neededd.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_utils_
 * File name: test_utils.h
 * Description: Common test utilities for Unity-based tests
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                created
 ******************************************************************************/

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "unity.h"
#include "zoo_smb_error.h"
#include "zoo_smb_message.h"  // For ZOO_SMB_MSG_STRUCT
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

// Use the canonical list type from root-level include layout.
#include "zoo_list.h"

// Consumer structure - define only if not already defined
#ifndef ZOO_SMB_CONSUMER_HANDLE
// This will be defined by the actual header, so we don't need our own definition
// typedef struct ZOO_SMB_CONSUMER_STRUCT {
//     char name[256];
//     int fd;
//     struct sockaddr_in addr;
// } *ZOO_SMB_CONSUMER_HANDLE;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Test fixture setup/teardown
void test_setup_memory_pool(void);
void test_teardown_memory_pool(void);

// Memory utilities
void* test_malloc(size_t size);
void test_free(void* ptr);

// String utilities
void test_assert_string_equal(const char* expected, const char* actual);
void test_assert_string_not_null(const char* str);

// Error utilities
void test_assert_error_ok(ZOO_ERROR_TYPE error);
void test_assert_error_not_ok(ZOO_ERROR_TYPE error);

// Memory utilities with cleanup tracking
typedef struct {
    void** ptrs;
    size_t count;
    size_t capacity;
} test_memory_tracker_t;

extern test_memory_tracker_t g_test_memory_tracker;

void test_memory_tracker_init(void);
void test_memory_tracker_cleanup(void);
void* test_tracked_malloc(size_t size);
void test_tracked_free(void* ptr);

// Test macros for easier migration from gtest
#define TEST_ASSERT_STRING_EQUAL(expected, actual) test_assert_string_equal((expected), (actual))
#define TEST_ASSERT_ERROR_OK(error) test_assert_error_ok((error))
#define TEST_ASSERT_ERROR_NOT_OK(error) test_assert_error_not_ok((error))

// Custom assertion macros
#define TEST_ASSERT_STREQ(expected, actual) TEST_ASSERT_EQUAL_STRING((expected), (actual))
#define TEST_ASSERT_STRNE(not_expected, actual) TEST_ASSERT_NOT_EQUAL_STRING((not_expected), (actual))

// Test fixture macros
#define TEST_SETUP() \
    void setUp(void) { \
        test_setup_memory_pool(); \
        test_memory_tracker_init(); \
    }

#define TEST_TEARDOWN() \
    void tearDown(void) { \
        test_memory_tracker_cleanup(); \
        test_teardown_memory_pool(); \
    }

// Main test runner macro
#define RUN_TEST_SUITE(test_func) \
    int main(void) { \
        UNITY_BEGIN(); \
        test_func(); \
        return UNITY_END(); \
    }

//=============================================================================
// ZOO SMB Function Stubs for Testing (Declaration)
//=============================================================================

// Error handling function declarations
uint32_t zoo_set_last_error(uint32_t error_code);
uint32_t zoo_get_last_error(void);
const char* zoo_error_string(uint32_t error_code);

// Memory pool function declarations
ZOO_ERROR_TYPE zoo_create_memory_pool(size_t size);
ZOO_INT32_T zoo_validate_memory_pool(void);
void zoo_destroy_memory_pool(void);
void* zoo_allocate_from_pool(size_t size);
void zoo_free_to_pool(void* ptr);

// Note: List functions are provided by the ZOO library, don't redeclare

// Logging function declarations - with correct signatures
void zoo_log_error(const char* file, int line_num, const char* func_name, const char* format, ...);
void zoo_log_debug(const char* file, int line_num, const char* func_name, const char* format, ...);
void zoo_log_trace(const char* file, int line_num, const char* func_name, const char* format, ...);
void zoo_log_warn(const char* file, int line_num, const char* func_name, const char* format, ...);
void zoo_log_info(const char* file, int line_num, const char* func_name, const char* format, ...);

// CRC calculation function declaration  
uint32_t zoo_calc_crc32(const void* data, size_t size);

// Message function declarations
ZOO_SMB_MSG_STRUCT* zoo_smb_copy_message_shallow(const ZOO_SMB_MSG_STRUCT* original);

// Ring buffer function declarations
size_t zoo_smb_ring_buffer_get_free_space(void* ring_buffer);
size_t zoo_smb_ring_buffer_get_used_space(void* ring_buffer);

// Consumer function declarations removed due to type conflicts
ZOO_LIST_HANDLE zoo_smb_create_list(size_t capacity);
void zoo_smb_destroy_list(ZOO_LIST_HANDLE list);
// Don't redeclare zoo_list_size if it's already declared

// Port manager function declarations (remove duplicates - they exist in the library)
// int zoo_smb_find_available_port(int min_port, int max_port);
// void zoo_smb_release_port_resources(int port);

// UDP transport initialization
void zoo_smb_transport_udp_init(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_UTILS_H
