/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_MEMORY_POOL
 * File name: zoo_memory_pool.h
 * Description: Memory pool implementation for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_MEMORY_POOL_H
#define ZOO_MEMORY_POOL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo.h"
#include "zoo_error.h"
#include <stdio.h>

// Error code definitions
#define ZOO_ERROR_MEM_POOL_ALLOCATION_FAILED -1
#define ZOO_ERROR_MEM_POOL_INVALID_PARAM -2
#define ZOO_ERROR_MEM_POOL_OUT_OF_MEMORY -3

    /**
     * @defgroup MEMORY_POOL Memory Pool Module
     * @brief Thread-safe memory pool with usage statistics.
     * @{
     */

    /**
     * @brief Memory pool usage statistics structure.
     */
    typedef struct
    {
        ZOO_SIZE_T pool_size, used_size, used_pct, free_size; /* 内存池总大小、已用大小、使用率、空闲大小 */
        ZOO_SIZE_T pages, free_page;
        ZOO_SIZE_T p_small, p_exact, p_big, p_page; /* 四种slab占用的page数 */
        ZOO_SIZE_T b_small, b_exact, b_big, b_page; /* 四种slab占用的byte数 */
        ZOO_SIZE_T max_free_pages;                  /* 最大的连续可用page数 */
    } ZOO_MEMORY_USAGE_T;

    /* Backward compatibility for older call sites/tests. */
    typedef ZOO_MEMORY_USAGE_T ZOO_MEMORY_USAGE;

    /**
     * @brief Create a memory pool with preallocated memory.
     * @param total_size Total size of the memory pool in bytes.
     * @param[out] error Error code output (optional).
     * @return Memory pool handle, NULL on failure.
     */
    ZOO_INT32_T zoo_create_memory_pool(
        ZOO_SIZE_T total_size);

    /**
     * @brief Destroy a memory pool and release all resources.
     * @param pool Memory pool handle.
     */
    void zoo_destroy_memory_pool(void);

    /**
     * @brief Allocate memory from the pool.
     * @param pool Memory pool handle.
     * @param size Requested memory size in bytes.
     * @param[out] error Error code output (optional).
     * @return Pointer to allocated memory, NULL on failure.
     */
    void *zoo_allocate_from_pool(ZOO_SIZE_T size);

    /**
     * @brief Free memory back to the pool.
     * @param pool Memory pool handle.
     * @param ptr Pointer to memory to free.
     */
    void zoo_free_to_pool(void *ptr);

    /**
     * @brief Validate memory pool integrity
     * Fixed validation logic to correctly check free list
     */
    ZOO_INT32_T zoo_validate_memory_pool(void);

    /**
     * @brief Get detailed memory usage statistics.
     * @param pool Memory pool handle.
     * @param[out] usage Structure to store usage data.
     * @return Error code indicating success or failure.
     */
    void zoo_memory_pool_get_usage(ZOO_MEMORY_USAGE_T *usage);
#ifdef __cplusplus
}
#endif

#endif /* ZOO_MEMORY_POOL_H */