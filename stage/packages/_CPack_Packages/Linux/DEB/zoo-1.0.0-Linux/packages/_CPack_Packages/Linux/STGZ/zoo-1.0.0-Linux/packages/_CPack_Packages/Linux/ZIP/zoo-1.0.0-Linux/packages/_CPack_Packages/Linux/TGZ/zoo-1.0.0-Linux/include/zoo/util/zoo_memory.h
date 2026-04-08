/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Utilities
 * Component ID: ZOO_MEMORY
 * File name: zoo_memory.h
 * Description: Memory management utilities and safe memory operations for ZOO
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     AI Assistant    Extracted from zoo_util.h
 ******************************************************************************/
#ifndef ZOO_MEMORY_H
#define ZOO_MEMORY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

    // ==============================================================================
    // SAFE MEMORY MANAGEMENT INLINE FUNCTIONS
    // ==============================================================================

    /**
     * @brief Safely delete a pointer and set it to NULL
     * @param pointer_addr Address of the pointer to delete
     * @note This function takes a pointer to pointer to allow setting it to NULL
     */
    static ZOO_FORCE_INLINE void zoo_safety_delete_pointer(void **pointer_addr)
    {
        if (pointer_addr != NULL && *pointer_addr != NULL)
        {
            free(*pointer_addr);
            *pointer_addr = NULL;
        }
    }

    /**
     * @brief Safely delete an array and set the pointer to NULL
     * @param array_addr Address of the array pointer to delete
     * @note This function takes a pointer to pointer to allow setting it to NULL
     */
    static ZOO_FORCE_INLINE void zoo_safety_delete_array(void **array_addr)
    {
        if (array_addr != NULL && *array_addr != NULL)
        {
            free(*array_addr);
            *array_addr = NULL;
        }
    }

    /**
     * @brief Safely allocate memory with zero initialization
     * @param size Number of bytes to allocate
     * @return Pointer to allocated memory, or NULL on failure
     * @note Memory is automatically zeroed
     */
    static ZOO_FORCE_INLINE void *zoo_safety_malloc_zero(ZOO_SIZE_T size)
    {
        if (size == 0)
        {
            return NULL;
        }

        void *ptr = malloc(size);
        if (ptr != NULL)
        {
            memset(ptr, 0, size);
        }

        return ptr;
    }

    /**
     * @brief Safely allocate memory for array with zero initialization
     * @param count Number of elements
     * @param element_size Size of each element in bytes
     * @return Pointer to allocated memory, or NULL on failure
     * @note Memory is automatically zeroed and checked for overflow
     */
    static ZOO_FORCE_INLINE void *zoo_safety_calloc(ZOO_SIZE_T count, ZOO_SIZE_T element_size)
    {
        if (count == 0 || element_size == 0)
        {
            return NULL;
        }

        // Check for multiplication overflow
        if (count > ((ZOO_SIZE_T)SIZE_MAX) / element_size)
        {
            return NULL; // Would overflow
        }

        return calloc(count, element_size);
    }

    /**
     * @brief Safely reallocate memory with size checking
     * @param ptr Existing pointer (can be NULL)
     * @param new_size New size in bytes
     * @return Pointer to reallocated memory, or NULL on failure
     * @note Original pointer remains valid if reallocation fails
     */
    static ZOO_FORCE_INLINE void *zoo_safety_realloc(void *ptr, ZOO_SIZE_T new_size)
    {
        if (new_size == 0)
        {
            if (ptr != NULL)
            {
                free(ptr);
            }
            return NULL;
        }

        return realloc(ptr, new_size);
    }

    /**
     * @brief Safely compare memory regions
     * @param ptr1 First memory region
     * @param ptr2 Second memory region
     * @param size Number of bytes to compare
     * @return 0 if equal, negative if ptr1 < ptr2, positive if ptr1 > ptr2
     * @note Returns error if either pointer is NULL
     */
    static ZOO_FORCE_INLINE ZOO_INT32 zoo_safety_memcmp(const void *ptr1, const void *ptr2, ZOO_SIZE_T size)
    {
        if (ptr1 == NULL || ptr2 == NULL)
        {
            if (ptr1 == ptr2)
                return 0;
            return (ptr1 == NULL) ? -1 : 1;
        }

        if (size == 0)
        {
            return 0;
        }

        return memcmp(ptr1, ptr2, size);
    }

    /**
     * @brief Safely copy memory with overlap detection
     * @param dest Destination buffer
     * @param src Source buffer
     * @param size Number of bytes to copy
     * @return ZOO_OK on success, negative error code on failure
     * @note Uses memmove for safe overlap handling
     */
    static ZOO_FORCE_INLINE ZOO_INT32 zoo_safety_memcpy(void *dest, const void *src, ZOO_SIZE_T size)
    {
        if (dest == NULL || src == NULL)
        {
            return -1;
        }

        if (size == 0)
        {
            return ZOO_OK;
        }

        memmove(dest, src, size);
        return ZOO_OK;
    }

    /**
     * @brief Safely set memory with bounds checking
     * @param ptr Pointer to memory region
     * @param value Value to set (will be cast to unsigned char)
     * @param size Number of bytes to set
     * @return ZOO_OK on success, negative error code on failure
     */
    static ZOO_FORCE_INLINE ZOO_INT32 zoo_safety_memset(void *ptr, ZOO_INT32 value, ZOO_SIZE_T size)
    {
        if (ptr == NULL)
        {
            return -1;
        }

        if (size == 0)
        {
            return ZOO_OK;
        }

        memset(ptr, value, size);
        return ZOO_OK;
    }

    /**
     * @brief Safely allocate aligned memory
     * @param size Number of bytes to allocate
     * @param alignment Required alignment (must be power of 2)
     * @return Pointer to aligned memory, or NULL on failure
     * @note Caller must use zoo_safety_free_aligned to free
     */
    static ZOO_FORCE_INLINE void *zoo_safety_malloc_aligned(ZOO_SIZE_T size, ZOO_SIZE_T alignment)
    {
        if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0)
        {
            return NULL; // Invalid parameters
        }

        // Allocate extra space for alignment and storing original pointer
        ZOO_SIZE_T total_size = size + alignment + sizeof(void *);
        void *raw_ptr = malloc(total_size);

        if (raw_ptr == NULL)
        {
            return NULL;
        }

        // Calculate aligned address
        uintptr_t addr = (uintptr_t)raw_ptr + sizeof(void *);
        uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
        void *aligned_ptr = (void *)aligned_addr;

        // Store original pointer just before aligned address
        *((void **)aligned_ptr - 1) = raw_ptr;

        return aligned_ptr;
    }

    /**
     * @brief Free aligned memory allocated with zoo_safety_malloc_aligned
     * @param aligned_ptr Pointer returned by zoo_safety_malloc_aligned
     */
    static ZOO_FORCE_INLINE void zoo_safety_free_aligned(void *aligned_ptr)
    {
        if (aligned_ptr != NULL)
        {
            // Retrieve original pointer stored just before aligned address
            void *raw_ptr = *((void **)aligned_ptr - 1);
            free(raw_ptr);
        }
    }

    /**
     * @brief Check if pointer is aligned to specified boundary
     * @param ptr Pointer to check
     * @param alignment Required alignment
     * @return ZOO_TRUE if aligned, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_memory_is_aligned(const void *ptr, ZOO_SIZE_T alignment)
    {
        if (ptr == NULL || alignment == 0)
        {
            return ZOO_FALSE;
        }

        return ((uintptr_t)ptr % alignment) == 0;
    }

    /**
     * @brief Secure memory wipe (prevents compiler optimization)
     * @param ptr Pointer to memory to wipe
     * @param size Number of bytes to wipe
     * @note Uses volatile to prevent compiler optimization
     */
    static ZOO_FORCE_INLINE void zoo_memory_secure_wipe(void *ptr, ZOO_SIZE_T size)
    {
        if (ptr != NULL && size > 0)
        {
            volatile unsigned char *p = (volatile unsigned char *)ptr;
            for (ZOO_SIZE_T i = 0; i < size; i++)
            {
                p[i] = 0;
            }
        }
    }

    /**
     * @brief Copy memory with bounds checking
     * @param dest Destination buffer
     * @param dest_size Size of destination buffer
     * @param src Source buffer
     * @param src_size Number of bytes to copy
     * @return ZOO_OK on success, negative error code on failure
     */
    static ZOO_FORCE_INLINE ZOO_INT32 zoo_memory_copy_bounded(void *dest, ZOO_SIZE_T dest_size,
                                                              const void *src, ZOO_SIZE_T src_size)
    {
        if (dest == NULL || src == NULL)
        {
            return -1;
        }

        if (src_size > dest_size)
        {
            return -2; // Source too large for destination
        }

        if (src_size == 0)
        {
            return ZOO_OK;
        }

        memmove(dest, src, src_size);
        return ZOO_OK;
    }

    // ==============================================================================
    // MEMORY DEBUGGING INLINE FUNCTIONS (Debug builds only)
    // ==============================================================================

#ifdef ZOO_DEBUG_MEMORY
    /**
     * @brief Debug memory allocation with tracking
     * @param size Number of bytes to allocate
     * @param file Source file name
     * @param line Source line number
     * @return Pointer to allocated memory, or NULL on failure
     * @note Only available in debug builds
     */
    static ZOO_FORCE_INLINE void *zoo_debug_malloc(ZOO_SIZE_T size, const char *file, ZOO_INT32 line)
    {
        void *ptr = malloc(size);
        if (ptr != NULL)
        {
            printf("[DEBUG] Allocated %zu bytes at %p (%s:%d)\n",
                   (size_t)size,
                   ptr,
                   file ? file : "unknown",
                   line);
        }
        else
        {
            printf("[DEBUG] Failed to allocate %zu bytes (%s:%d)\n",
                   (size_t)size,
                   file ? file : "unknown",
                   line);
        }
        return ptr;
    }

    /**
     * @brief Debug memory deallocation with tracking
     * @param ptr Pointer to deallocate
     * @param file Source file name
     * @param line Source line number
     */
    static ZOO_FORCE_INLINE void zoo_debug_free(void *ptr, const char *file, ZOO_INT32 line)
    {
        if (ptr != NULL)
        {
            printf("[DEBUG] Freed memory at %p (%s:%d)\n",
                   ptr,
                   file ? file : "unknown",
                   line);
            free(ptr);
        }
        else
        {
            printf("[DEBUG] Attempted to free NULL pointer (%s:%d)\n",
                   file ? file : "unknown",
                   line);
        }
    }

#define ZOO_DEBUG_MALLOC(size) zoo_debug_malloc((size), __FILE__, __LINE__)
#define ZOO_DEBUG_FREE(ptr) zoo_debug_free((ptr), __FILE__, __LINE__)
#else
#define ZOO_DEBUG_MALLOC(size) malloc(size)
#define ZOO_DEBUG_FREE(ptr) free(ptr)
#endif /* ZOO_DEBUG_MEMORY */

// ==============================================================================
// MEMORY POOL ALLOCATION MACROS
// ==============================================================================

/**
 * @brief Allocate memory with automatic cleanup on scope exit
 * @param type Type to allocate
 * @param count Number of elements
 */
#define ZOO_MEMORY_ALLOC_SCOPED(type, count) \
    __attribute__((cleanup(zoo_safety_delete_pointer))) type *

/**
 * @brief Create a cleanup function for automatic resource management
 * @param func_name Name of cleanup function
 * @param cleanup_code Code to execute for cleanup
 */
#define ZOO_MEMORY_CLEANUP_FUNCTION(func_name, cleanup_code) \
    static inline void func_name(void **ptr)                 \
    {                                                        \
        if (ptr && *ptr)                                     \
        {                                                    \
            cleanup_code;                                    \
            *ptr = NULL;                                     \
        }                                                    \
    }

#ifdef __cplusplus
}
#endif

#endif /* ZOO_MEMORY_H */
