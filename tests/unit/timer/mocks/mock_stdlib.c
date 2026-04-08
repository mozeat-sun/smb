/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 * Product: ZOO
 * Module: Timer Tests
 * Component ID: TIMER_TEST
 * File name: mock_stdlib.c
 * Description: Mock implementation for standard library functions
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01     AI Assistant      Created for unit testing
 ******************************************************************************/

#include "mock_stdlib.h"
#include "unity.h"
#include <string.h>

// ==============================================================================
// Mock Data for Standard Library Functions
// ==============================================================================

// Simple memory pool for testing
#define MOCK_MEMORY_POOL_SIZE 4096
static char mock_memory_pool[MOCK_MEMORY_POOL_SIZE];
static size_t mock_memory_offset = 0;
static ZOO_BOOL mock_malloc_should_fail = ZOO_FALSE;

// ==============================================================================
// Mock malloc/free Implementation
// ==============================================================================

void* malloc(size_t size)
{
    if (mock_malloc_should_fail) {
        return NULL;
    }
    
    // Simple allocation from pool
    if (mock_memory_offset + size >= MOCK_MEMORY_POOL_SIZE) {
        return NULL; // Out of memory
    }
    
    void* ptr = &mock_memory_pool[mock_memory_offset];
    mock_memory_offset += size;
    
    // Align to 8-byte boundary
    mock_memory_offset = (mock_memory_offset + 7) & ~7;
    
    return ptr;
}

void free(void* ptr)
{
    // In a real mock, we might track this
    // For simplicity, we just ignore it
    (void)ptr;
}

// ==============================================================================
// Mock Control Functions
// ==============================================================================

void mock_stdlib_reset(void)
{
    memset(mock_memory_pool, 0, sizeof(mock_memory_pool));
    mock_memory_offset = 0;
    mock_malloc_should_fail = ZOO_FALSE;
}

void mock_stdlib_set_malloc_fail(ZOO_BOOL should_fail)
{
    mock_malloc_should_fail = should_fail;
}

size_t mock_stdlib_get_allocated_bytes(void)
{
    return mock_memory_offset;
}

ZOO_BOOL mock_stdlib_is_memory_exhausted(void)
{
    return (mock_memory_offset >= MOCK_MEMORY_POOL_SIZE) ? ZOO_TRUE : ZOO_FALSE;
}
