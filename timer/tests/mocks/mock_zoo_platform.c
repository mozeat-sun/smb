/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 * Product: ZOO
 * Module: Timer Tests
 * Component ID: TIMER_TEST
 * File name: mock_zoo_platform.c
 * Description: Mock implementation for platform-specific functions
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01     AI Assistant      Created for unit testing
 ******************************************************************************/

#include "mock_zoo_platform.h"
#include "unity.h"
#include <string.h>

// ==============================================================================
// Mock Data Structures
// ==============================================================================

typedef struct {
    ZOO_BOOL is_called;
    ZOO_UINT32 call_count;
    ZOO_UINT32 last_milliseconds;
    ZOO_INT32 return_value;
} mock_sleep_data_t;

typedef struct {
    ZOO_BOOL is_called;
    ZOO_UINT32 call_count;
    ZOO_THREAD_T* last_thread;
    void* (*last_func)(void*);
    void* last_arg;
    ZOO_INT32 return_value;
} mock_create_thread_data_t;

typedef struct {
    ZOO_BOOL is_called;
    ZOO_UINT32 call_count;
    ZOO_THREAD_T last_thread;
    ZOO_INT32 return_value;
} mock_join_thread_data_t;

typedef struct {
    ZOO_BOOL is_called;
    ZOO_UINT32 call_count;
    void* last_ptr;
    size_t last_size;
    void* return_value;
} mock_malloc_data_t;

typedef struct {
    ZOO_BOOL is_called;
    ZOO_UINT32 call_count;
    void* last_ptr;
} mock_free_data_t;

// ==============================================================================
// Mock State Variables
// ==============================================================================

static mock_sleep_data_t mock_sleep_data = {0};
static mock_create_thread_data_t mock_create_thread_data = {0};
static mock_join_thread_data_t mock_join_thread_data = {0};
static mock_malloc_data_t mock_malloc_data = {0};
static mock_free_data_t mock_free_data = {0};

// ==============================================================================
// Mock Function Implementations
// ==============================================================================

void mock_zoo_platform_reset(void)
{
    memset(&mock_sleep_data, 0, sizeof(mock_sleep_data));
    memset(&mock_create_thread_data, 0, sizeof(mock_create_thread_data));
    memset(&mock_join_thread_data, 0, sizeof(mock_join_thread_data));
    memset(&mock_malloc_data, 0, sizeof(mock_malloc_data));
    memset(&mock_free_data, 0, sizeof(mock_free_data));
}

// Platform sleep mock
ZOO_INT32 mock_zoo_timer_sleep_ms(ZOO_UINT32 milliseconds)
{
    mock_sleep_data.is_called = ZOO_TRUE;
    mock_sleep_data.call_count++;
    mock_sleep_data.last_milliseconds = milliseconds;
    return mock_sleep_data.return_value;
}

void mock_zoo_timer_sleep_ms_expect(ZOO_UINT32 expected_milliseconds, ZOO_INT32 return_value)
{
    (void)expected_milliseconds; // Suppress unused parameter warning
    mock_sleep_data.return_value = return_value;
}

void mock_zoo_timer_sleep_ms_verify(ZOO_UINT32 expected_milliseconds, ZOO_UINT32 expected_call_count)
{
    TEST_ASSERT_TRUE_MESSAGE(mock_sleep_data.is_called, "Sleep function was not called");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_call_count, mock_sleep_data.call_count, "Sleep call count mismatch");
    if (expected_call_count > 0) {
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_milliseconds, mock_sleep_data.last_milliseconds, "Sleep milliseconds mismatch");
    }
}

// Thread creation mock
ZOO_INT32 mock_zoo_timer_create_thread(ZOO_THREAD_T *thread, void *(*func)(void *), void *arg)
{
    mock_create_thread_data.is_called = ZOO_TRUE;
    mock_create_thread_data.call_count++;
    mock_create_thread_data.last_thread = thread;
    mock_create_thread_data.last_func = func;
    mock_create_thread_data.last_arg = arg;
    
    // Simulate successful thread creation
    if (thread && mock_create_thread_data.return_value == ZOO_OK) {
        *thread = (ZOO_THREAD_T)0x12345678; // Mock thread handle
    }
    
    return mock_create_thread_data.return_value;
}

void mock_zoo_timer_create_thread_expect(ZOO_INT32 return_value)
{
    mock_create_thread_data.return_value = return_value;
}

void mock_zoo_timer_create_thread_verify(ZOO_UINT32 expected_call_count)
{
    TEST_ASSERT_TRUE_MESSAGE(mock_create_thread_data.is_called, "Create thread function was not called");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_call_count, mock_create_thread_data.call_count, "Create thread call count mismatch");
}

// Thread join mock
ZOO_INT32 mock_zoo_timer_join_thread(ZOO_THREAD_T thread)
{
    mock_join_thread_data.is_called = ZOO_TRUE;
    mock_join_thread_data.call_count++;
    mock_join_thread_data.last_thread = thread;
    return mock_join_thread_data.return_value;
}

void mock_zoo_timer_join_thread_expect(ZOO_INT32 return_value)
{
    mock_join_thread_data.return_value = return_value;
}

void mock_zoo_timer_join_thread_verify(ZOO_UINT32 expected_call_count)
{
    TEST_ASSERT_TRUE_MESSAGE(mock_join_thread_data.is_called, "Join thread function was not called");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_call_count, mock_join_thread_data.call_count, "Join thread call count mismatch");
}

// Memory allocation mocks
void* mock_malloc(size_t size)
{
    mock_malloc_data.is_called = ZOO_TRUE;
    mock_malloc_data.call_count++;
    mock_malloc_data.last_size = size;
    return mock_malloc_data.return_value;
}

void mock_malloc_expect(void* return_value)
{
    mock_malloc_data.return_value = return_value;
}

void mock_malloc_verify(size_t expected_size, ZOO_UINT32 expected_call_count)
{
    TEST_ASSERT_TRUE_MESSAGE(mock_malloc_data.is_called, "Malloc function was not called");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_call_count, mock_malloc_data.call_count, "Malloc call count mismatch");
    if (expected_call_count > 0) {
        TEST_ASSERT_EQUAL_size_t_MESSAGE(expected_size, mock_malloc_data.last_size, "Malloc size mismatch");
    }
}

void mock_free(void* ptr)
{
    mock_free_data.is_called = ZOO_TRUE;
    mock_free_data.call_count++;
    mock_free_data.last_ptr = ptr;
}

void mock_free_verify(ZOO_UINT32 expected_call_count)
{
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_call_count, mock_free_data.call_count, "Free call count mismatch");
}

// ==============================================================================
// Mock State Query Functions
// ==============================================================================

ZOO_BOOL mock_zoo_platform_was_sleep_called(void)
{
    return mock_sleep_data.is_called;
}

ZOO_UINT32 mock_zoo_platform_get_sleep_call_count(void)
{
    return mock_sleep_data.call_count;
}

ZOO_UINT32 mock_zoo_platform_get_last_sleep_milliseconds(void)
{
    return mock_sleep_data.last_milliseconds;
}

ZOO_BOOL mock_zoo_platform_was_create_thread_called(void)
{
    return mock_create_thread_data.is_called;
}

ZOO_UINT32 mock_zoo_platform_get_create_thread_call_count(void)
{
    return mock_create_thread_data.call_count;
}

ZOO_BOOL mock_zoo_platform_was_join_thread_called(void)
{
    return mock_join_thread_data.is_called;
}

ZOO_UINT32 mock_zoo_platform_get_join_thread_call_count(void)
{
    return mock_join_thread_data.call_count;
}

ZOO_BOOL mock_zoo_platform_was_malloc_called(void)
{
    return mock_malloc_data.is_called;
}

ZOO_UINT32 mock_zoo_platform_get_malloc_call_count(void)
{
    return mock_malloc_data.call_count;
}

ZOO_BOOL mock_zoo_platform_was_free_called(void)
{
    return mock_free_data.is_called;
}

ZOO_UINT32 mock_zoo_platform_get_free_call_count(void)
{
    return mock_free_data.call_count;
}
