/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: buffer
 * Component id: queue_test
 * File name: zoo_memory_pool_stub.c
 * Description: Minimal stub implementation for memory pool functions
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-31     weiwang.sun         created
 ******************************************************************************/

#include <stdlib.h>
#include <stdint.h>

// Basic type definitions (avoiding zoo_memory_pool.h dependency)
typedef uint32_t ZOO_SIZE_T;
typedef int32_t ZOO_INT32;

// Forward declarations for memory pool functions
ZOO_INT32 zoo_create_memory_pool(ZOO_SIZE_T total_size);
void zoo_destroy_memory_pool(void);
void* zoo_alloc_memory(ZOO_SIZE_T size);
void zoo_free_memory(void* ptr);

// Simple stub implementation using malloc/free
static void* global_pool __attribute__((unused)) = NULL;

ZOO_INT32 zoo_create_memory_pool(ZOO_SIZE_T total_size)
{
    (void)total_size; // Suppress unused parameter warning
    return 0; // Success
}

void zoo_destroy_memory_pool(void)
{
    // No-op
}

void* zoo_alloc_memory(ZOO_SIZE_T size)
{
    return malloc(size);
}

void zoo_free_memory(void* ptr)
{
    free(ptr);
}

void* zoo_allocate_from_pool(ZOO_SIZE_T size)
{
    return malloc(size);
}

void zoo_free_to_pool(void* ptr)
{
    free(ptr);
}
