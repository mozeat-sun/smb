/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 * Product: ZOO
 * Module: Timer Tests
 * Component ID: TIMER_TEST
 * File name: mock_stdlib.h
 * Description: Mock implementation headers for standard library functions
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01     AI Assistant      Created for unit testing
 ******************************************************************************/

#ifndef MOCK_STDLIB_H
#define MOCK_STDLIB_H

#include "zoo.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// Mock Control Functions
// ==============================================================================

/**
 * @brief Reset mock stdlib state
 */
void mock_stdlib_reset(void);

/**
 * @brief Configure malloc to fail
 * @param[in] should_fail ZOO_TRUE to make malloc fail, ZOO_FALSE for normal operation
 */
void mock_stdlib_set_malloc_fail(ZOO_BOOL should_fail);

/**
 * @brief Get total allocated bytes
 * @return Number of bytes allocated from mock pool
 */
size_t mock_stdlib_get_allocated_bytes(void);

/**
 * @brief Check if mock memory pool is exhausted
 * @return ZOO_TRUE if no more memory available, ZOO_FALSE otherwise
 */
ZOO_BOOL mock_stdlib_is_memory_exhausted(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_STDLIB_H */
