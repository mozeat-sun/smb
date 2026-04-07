/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 * Product: ZOO
 * Module: Timer Tests
 * Component ID: TIMER_TEST
 * File name: mock_zoo_platform.h
 * Description: Mock implementation headers for platform-specific functions
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01     AI Assistant      Created for unit testing
 ******************************************************************************/

#ifndef MOCK_ZOO_PLATFORM_H
#define MOCK_ZOO_PLATFORM_H

#include "zoo.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// Mock Control Functions
// ==============================================================================

/**
 * @brief Reset all mock state and expectations
 */
void mock_zoo_platform_reset(void);

// ==============================================================================
// Platform Function Mocks
// ==============================================================================

/**
 * @brief Mock implementation of platform sleep function
 * @param[in] milliseconds Number of milliseconds to sleep
 * @return ZOO_OK on success, error code on failure
 */
ZOO_INT32 mock_zoo_timer_sleep_ms(ZOO_UINT32 milliseconds);

/**
 * @brief Set expectation for sleep function call
 * @param[in] expected_milliseconds Expected sleep duration
 * @param[in] return_value Value to return from mock function
 */
void mock_zoo_timer_sleep_ms_expect(ZOO_UINT32 expected_milliseconds, ZOO_INT32 return_value);

/**
 * @brief Verify sleep function was called correctly
 * @param[in] expected_milliseconds Expected sleep duration
 * @param[in] expected_call_count Expected number of calls
 */
void mock_zoo_timer_sleep_ms_verify(ZOO_UINT32 expected_milliseconds, ZOO_UINT32 expected_call_count);

/**
 * @brief Mock implementation of thread creation
 * @param[out] thread Pointer to thread handle
 * @param[in] func Thread function
 * @param[in] arg Thread argument
 * @return ZOO_OK on success, error code on failure
 */
ZOO_INT32 mock_zoo_timer_create_thread(ZOO_THREAD_T *thread, void *(*func)(void *), void *arg);

/**
 * @brief Set expectation for create thread function
 * @param[in] return_value Value to return from mock function
 */
void mock_zoo_timer_create_thread_expect(ZOO_INT32 return_value);

/**
 * @brief Verify create thread function was called correctly
 * @param[in] expected_call_count Expected number of calls
 */
void mock_zoo_timer_create_thread_verify(ZOO_UINT32 expected_call_count);

/**
 * @brief Mock implementation of thread join
 * @param[in] thread Thread handle to join
 * @return ZOO_OK on success, error code on failure
 */
ZOO_INT32 mock_zoo_timer_join_thread(ZOO_THREAD_T thread);

/**
 * @brief Set expectation for join thread function
 * @param[in] return_value Value to return from mock function
 */
void mock_zoo_timer_join_thread_expect(ZOO_INT32 return_value);

/**
 * @brief Verify join thread function was called correctly
 * @param[in] expected_call_count Expected number of calls
 */
void mock_zoo_timer_join_thread_verify(ZOO_UINT32 expected_call_count);

// ==============================================================================
// Memory Allocation Mocks
// ==============================================================================

/**
 * @brief Mock implementation of malloc
 * @param[in] size Size to allocate
 * @return Pointer to allocated memory or NULL
 */
void* mock_malloc(size_t size);

/**
 * @brief Set expectation for malloc function
 * @param[in] return_value Value to return from mock function
 */
void mock_malloc_expect(void* return_value);

/**
 * @brief Verify malloc function was called correctly
 * @param[in] expected_size Expected allocation size
 * @param[in] expected_call_count Expected number of calls
 */
void mock_malloc_verify(size_t expected_size, ZOO_UINT32 expected_call_count);

/**
 * @brief Mock implementation of free
 * @param[in] ptr Pointer to free
 */
void mock_free(void* ptr);

/**
 * @brief Verify free function was called correctly
 * @param[in] expected_call_count Expected number of calls
 */
void mock_free_verify(ZOO_UINT32 expected_call_count);

// ==============================================================================
// Mock State Query Functions
// ==============================================================================

ZOO_BOOL mock_zoo_platform_was_sleep_called(void);
ZOO_UINT32 mock_zoo_platform_get_sleep_call_count(void);
ZOO_UINT32 mock_zoo_platform_get_last_sleep_milliseconds(void);

ZOO_BOOL mock_zoo_platform_was_create_thread_called(void);
ZOO_UINT32 mock_zoo_platform_get_create_thread_call_count(void);

ZOO_BOOL mock_zoo_platform_was_join_thread_called(void);
ZOO_UINT32 mock_zoo_platform_get_join_thread_call_count(void);

ZOO_BOOL mock_zoo_platform_was_malloc_called(void);
ZOO_UINT32 mock_zoo_platform_get_malloc_call_count(void);

ZOO_BOOL mock_zoo_platform_was_free_called(void);
ZOO_UINT32 mock_zoo_platform_get_free_call_count(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_ZOO_PLATFORM_H */
