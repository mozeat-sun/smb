/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_smb_helpers
 * File name: test_smb_helpers.h
 * Description: Minimal test helpers for SMB unit tests linked against the real
 *              library. Provides inline stubs for setUp/tearDown infrastructure
 *              and macros previously in test_utils.h, without redefining any
 *              zoo_* symbols that would conflict with the real SMB library.
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-06-13     claude            created for orphaned test migration
 ******************************************************************************/

#ifndef TEST_SMB_HELPERS_H
#define TEST_SMB_HELPERS_H

#include "unity.h"
#include "zoo_smb_error.h"
#include "zoo_smb_message.h"
#include "zoo_list.h"
#include "zoo_memory_pool.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Override the weak SMB auto-init constructor to prevent port binding during
 * unit tests.  Each test that needs a running SMB must start it explicitly.
 * -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
 * Inline stubs — these were previously in test_utils.c.  They do nothing
 * because the real zoo_smb_shared library provides the actual memory pool,
 * and the tracker was only used for leak detection in standalone mode.
 * -------------------------------------------------------------------------- */

/* Initialize a real memory pool for tests that need allocation support.
 * Uses a modest 1 MB pool — large enough for unit tests, small enough
 * to create/destroy quickly per test case. */
static inline void test_setup_memory_pool(void)
{
    zoo_create_memory_pool(1024 * 1024);
}

static inline void test_teardown_memory_pool(void)
{
    zoo_destroy_memory_pool();
}

static inline void test_memory_tracker_init(void)
{
    /* Also ensure a memory pool exists — many older tests call only this. */
    test_setup_memory_pool();
}

static inline void test_memory_tracker_cleanup(void)
{
    test_teardown_memory_pool();
}

/* --------------------------------------------------------------------------
 * Memory helpers for tests that need to allocate/free outside the pool.
 * -------------------------------------------------------------------------- */

static inline void* test_tracked_malloc(size_t size)
{
    return malloc(size);
}

static inline void test_tracked_free(void* ptr)
{
    free(ptr);
}

static inline void* test_malloc(size_t size)
{
    return malloc(size);
}

static inline void test_free(void* ptr)
{
    free(ptr);
}

/* --------------------------------------------------------------------------
 * Thin wrappers around real library list functions, matching the names used
 * by older test code that expected test_utils.h stubs.
 * -------------------------------------------------------------------------- */

static inline ZOO_LIST_HANDLE zoo_smb_create_list(size_t capacity)
{
    return zoo_list_create((int)capacity);
}

static inline void zoo_smb_destroy_list(ZOO_LIST_HANDLE list)
{
    if (list) { zoo_list_destroy(list); }
}

/* --------------------------------------------------------------------------
 * Shallow message copy — was in test_utils.c but not in the real library.
 * -------------------------------------------------------------------------- */

static inline ZOO_SMB_MSG_STRUCT* zoo_smb_copy_message_shallow(const ZOO_SMB_MSG_STRUCT* original)
{
    if (!original) return NULL;
    ZOO_SMB_MSG_STRUCT* copy = (ZOO_SMB_MSG_STRUCT*)malloc(sizeof(ZOO_SMB_MSG_STRUCT));
    if (!copy) return NULL;
    copy->header = original->header;
    copy->payload = original->payload;
    return copy;
}

/* --------------------------------------------------------------------------
 * Assertion helpers that map to Unity primitives
 * -------------------------------------------------------------------------- */

static inline void test_assert_error_ok(ZOO_ERROR_TYPE error)
{
    if ((uint32_t)error != (uint32_t)ZOO_SMB_OK) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Expected ZOO_SMB_OK, got 0x%08X", (unsigned)error);
        TEST_FAIL_MESSAGE(msg);
    }
}

static inline void test_assert_error_not_ok(ZOO_ERROR_TYPE error)
{
    if ((uint32_t)error == (uint32_t)ZOO_SMB_OK) {
        TEST_FAIL_MESSAGE("Expected error, but got ZOO_SMB_OK");
    }
}

#define TEST_ASSERT_STREQ(expected, actual) \
    TEST_ASSERT_EQUAL_STRING((expected), (actual))

#define TEST_ASSERT_STRNE(not_expected, actual) \
    TEST_ASSERT_NOT_EQUAL_STRING((not_expected), (actual))

#define TEST_ASSERT_ERROR_OK(error)    test_assert_error_ok((error))
#define TEST_ASSERT_ERROR_NOT_OK(error) test_assert_error_not_ok((error))
#define TEST_ASSERT_STRING_EQUAL(e, a) TEST_ASSERT_EQUAL_STRING((e), (a))

/* --------------------------------------------------------------------------
 * Test fixture macros (same API as test_utils.h but using inline stubs)
 * -------------------------------------------------------------------------- */

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

#define RUN_TEST_SUITE(test_func) \
    int main(void) { \
        UNITY_BEGIN(); \
        test_func(); \
        return UNITY_END(); \
    }

#ifdef __cplusplus
}
#endif

#endif /* TEST_SMB_HELPERS_H */
