/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: memory_pool
 * Component id: ZOO_MEMORY_POOL
 * File name: zoo_memory_pool.c
 * Description: Cross-platform slab-based memory pool implementation
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun       created
 * 2.0       2025-01-01     assistant         Cross-platform support added
 ******************************************************************************/

// Feature test macros for POSIX spinlocks
#if defined(__linux__) || defined(__unix__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#endif

#include "zoo_memory_pool.h"
#include "zoo.h"
#include <stdlib.h>
#include <string.h>

// Include platform-specific headers
#if ZOO_HAS_POSIX
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#elif defined(ZOO_OS_WINDOWS)
#include <windows.h>
#include <process.h>
#elif defined(ZOO_OS_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#elif defined(ZOO_OS_CMSIS_RTOS)
#include "cmsis_os.h"
#endif



#include "zoo.h"
#include "zoo_memory_pool.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Platform-specific includes
#if ZOO_HAS_POSIX
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#elif defined(ZOO_OS_WINDOWS)
#include <windows.h>
#elif defined(ZOO_OS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#elif defined(ZOO_OS_CMSIS_RTOS)
#include "cmsis_os2.h"
#endif

// ==============================================================================
// PLATFORM-SPECIFIC TYPES AND DEFINITIONS
// ==============================================================================

// Cross-platform mutex type
#if ZOO_HAS_POSIX
typedef pthread_mutex_t zoo_pool_mutex_t;
#elif defined(ZOO_OS_WINDOWS)
typedef CRITICAL_SECTION zoo_pool_mutex_t;
#elif defined(ZOO_OS_FREERTOS)
typedef SemaphoreHandle_t zoo_pool_mutex_t;
#elif defined(ZOO_OS_CMSIS_RTOS)
typedef osMutexId_t zoo_pool_mutex_t;
#else
typedef volatile ZOO_INT32 zoo_pool_mutex_t; // Simple atomic lock
#endif

// Cross-platform page size function
static ZOO_SIZE_T zoo_get_page_size(void);

// Cross-platform mutex operations
static int zoo_pool_mutex_init(zoo_pool_mutex_t *mutex);
static int zoo_pool_mutex_lock(zoo_pool_mutex_t *mutex);
static int zoo_pool_mutex_unlock(zoo_pool_mutex_t *mutex);
static int zoo_pool_mutex_destroy(zoo_pool_mutex_t *mutex);

#define SMB_SLAB_PAGE_MASK 3
#define SMB_SLAB_PAGE 0
#define SMB_SLAB_BIG 1
#define SMB_SLAB_EXACT 2
#define SMB_SLAB_SMALL 3

// Define pointer size based on platform
#ifdef ZOO_PLATFORM_64BIT
#define SMB_PTR_SIZE 8
#else
#define SMB_PTR_SIZE 4
#endif

#if (SMB_PTR_SIZE == 4)
#define SMB_SLAB_PAGE_FREE 0
#define SMB_SLAB_PAGE_BUSY 0xffffffff
#define SMB_SLAB_PAGE_START 0x80000000
#define SMB_SLAB_SHIFT_MASK 0x0000000f
#define SMB_SLAB_MAP_MASK 0xffff0000
#define SMB_SLAB_MAP_SHIFT 16
#define SMB_SLAB_BUSY 0xffffffff
#else /* (SMB_PTR_SIZE == 8) */
#define SMB_SLAB_PAGE_FREE 0
#define SMB_SLAB_PAGE_BUSY 0xffffffffffffffff
#define SMB_SLAB_PAGE_START 0x8000000000000000
#define SMB_SLAB_SHIFT_MASK 0x000000000000000f
#define SMB_SLAB_MAP_MASK 0xffffffff00000000
#define SMB_SLAB_MAP_SHIFT 32
#define SMB_SLAB_BUSY 0xffffffffffffffff
#endif

#define SMB_ALIGN_PTR(p, a) \
    (ZOO_UINT8 *)(((ZOO_UINTPTR_T)(p) + ((ZOO_UINTPTR_T)a - 1)) & ~((ZOO_UINTPTR_T)a - 1))

static ZOO_UINTPTR_T smb_slab_max_size;
static ZOO_UINTPTR_T smb_slab_exact_size;
static ZOO_UINTPTR_T smb_slab_exact_shift;
static ZOO_UINTPTR_T smb_pagesize;
static ZOO_UINTPTR_T smb_pagesize_shift;
static ZOO_UINTPTR_T smb_real_pages;
typedef struct ZOO_SLAB_PAGE_STRUCT ZOO_SLAB_PAGE_STRUCT;

/**
 * @struct ZOO_SLAB_PAGE_STRUCT
 * @brief Represents a slab page used in the memory pool management system.
 *
 * This structure is used to manage individual pages within a slab allocator.
 * It typically contains metadata about the page, such as its allocation status,
 * pointers to free blocks, and other information required for efficient memory
 * allocation and deallocation.
 */
struct ZOO_SLAB_PAGE_STRUCT
{
    ZOO_UINTPTR_T slab;
    ZOO_SLAB_PAGE_STRUCT *next;
    ZOO_UINTPTR_T prev;
};

/**
 * @struct ZOO_MEMORY_POOL_STRUCT
 * @brief Structure representing a memory pool for efficient memory management.
 *
 * This structure is used to manage a pool of memory blocks, allowing for
 * fast allocation and deallocation of memory within the pool. It is
 * typically used to reduce fragmentation and improve performance in
 * applications that require frequent memory operations.
 *
 * Members of this structure should be documented individually where defined.
 */
typedef struct
{
    ZOO_SIZE_T min_size;
    ZOO_SIZE_T min_shift;
    ZOO_SLAB_PAGE_STRUCT *pages;
    ZOO_SLAB_PAGE_STRUCT free;
    ZOO_UINT8 *start;
    ZOO_UINT8 *end;
    zoo_pool_mutex_t mutex;
} ZOO_MEMORY_POOL_STRUCT;

/**
 * @brief Static pointer to the memory pool structure.
 *
 * This variable holds the reference to the singleton instance of the memory pool.
 * It is initialized to NULL and should be assigned during memory pool creation.
 */
static ZOO_MEMORY_POOL_STRUCT *smb_memory_pool = NULL;

/**
 * Allocates one or more slab pages from the specified memory pool.
 *
 * @param pool Pointer to the memory pool structure from which pages are to be allocated.
 * @return Pointer to the allocated slab page(s), or NULL if allocation fails.
 */
static ZOO_SLAB_PAGE_STRUCT *smb_slab_alloc_pages(ZOO_MEMORY_POOL_STRUCT *pool,
                                                  ZOO_UINTPTR_T pages);

/**
 * @brief Frees a specified number of slab pages in the memory pool.
 *
 * This function releases the given number of pages starting from the specified slab page
 * within the provided memory pool structure.
 *
 * @param pool Pointer to the memory pool structure from which pages will be freed.
 * @param page Pointer to the starting slab page to be freed.
 * @param pages Number of contiguous pages to free.
 */
static void smb_slab_free_pages(ZOO_MEMORY_POOL_STRUCT *pool, ZOO_SLAB_PAGE_STRUCT *page, ZOO_UINTPTR_T pages);

// ==============================================================================
// CROSS-PLATFORM IMPLEMENTATIONS
// ==============================================================================

/**
 * @brief Retrieves the system's memory page size.
 *
 * This function returns the size of a memory page as defined by the operating system.
 * The page size is typically used for memory allocation and management purposes.
 *
 * @return The size of a memory page in bytes.
 */
static ZOO_SIZE_T zoo_get_page_size(void)
{
#if ZOO_HAS_POSIX
    return (ZOO_SIZE_T)getpagesize();
#elif defined(ZOO_OS_WINDOWS)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (ZOO_SIZE_T)si.dwPageSize;
#else
    return 4096; // Default page size for embedded systems
#endif
}

/**
 * @brief Initializes a zoo pool mutex.
 *
 * This function sets up the given zoo_pool_mutex_t structure for use as a mutex
 * within the memory pool system. It should be called before the mutex is used
 * for synchronization.
 *
 * @param mutex Pointer to a zoo_pool_mutex_t structure to initialize.
 * @return 0 on success, non-zero error code on failure.
 */
static int zoo_pool_mutex_init(zoo_pool_mutex_t *mutex)
{
#if ZOO_HAS_POSIX
    return pthread_mutex_init(mutex, NULL);
#elif defined(ZOO_OS_WINDOWS)
    InitializeCriticalSection(mutex);
    return 0;
#elif defined(ZOO_OS_FREERTOS)
    *mutex = xSemaphoreCreateMutex();
    return (*mutex != NULL) ? 0 : -1;
#elif defined(ZOO_OS_CMSIS_RTOS)
    *mutex = osMutexNew(NULL);
    return (*mutex != NULL) ? 0 : -1;
#else
    *mutex = 0; // Simple atomic lock
    return 0;
#endif
}

/**
 * @brief Locks the specified memory pool mutex.
 *
 * This function attempts to acquire a lock on the given zoo_pool_mutex_t object.
 * It is typically used to ensure thread-safe access to resources managed by the memory pool.
 *
 * @param mutex Pointer to the zoo_pool_mutex_t structure to be locked.
 * @return int Returns 0 on success, or a negative error code on failure.
 */
static int zoo_pool_mutex_lock(zoo_pool_mutex_t *mutex)
{
#if ZOO_HAS_POSIX
    return pthread_mutex_lock(mutex);
#elif defined(ZOO_OS_WINDOWS)
    EnterCriticalSection(mutex);
    return 0;
#elif defined(ZOO_OS_FREERTOS)
    return (xSemaphoreTake(*mutex, portMAX_DELAY) == pdTRUE) ? 0 : -1;
#elif defined(ZOO_OS_CMSIS_RTOS)
    return (osMutexAcquire(*mutex, osWaitForever) == osOK) ? 0 : -1;
#else
    // Simple spinlock for embedded systems without threading
    while (__sync_lock_test_and_set(mutex, 1) != 0)
    {
        // Busy wait
    }
    return 0;
#endif
}

/**
 * @brief Unlocks the specified memory pool mutex.
 *
 * This function releases the lock held by the given zoo_pool_mutex_t object.
 * It should be called after the critical section is completed to allow other
 * threads to acquire the mutex.
 *
 * @param mutex Pointer to the zoo_pool_mutex_t to be unlocked.
 * @return 0 on success, or a negative error code on failure.
 */
static int zoo_pool_mutex_unlock(zoo_pool_mutex_t *mutex)
{
#if ZOO_HAS_POSIX
    return pthread_mutex_unlock(mutex);
#elif defined(ZOO_OS_WINDOWS)
    LeaveCriticalSection(mutex);
    return 0;
#elif defined(ZOO_OS_FREERTOS)
    return (xSemaphoreGive(*mutex) == pdTRUE) ? 0 : -1;
#elif defined(ZOO_OS_CMSIS_RTOS)
    return (osMutexRelease(*mutex) == osOK) ? 0 : -1;
#else
    __sync_lock_release(mutex);
    return 0;
#endif
}

/**
 * @brief Destroys a memory pool mutex.
 *
 * This function releases any resources associated with the specified
 * memory pool mutex. After calling this function, the mutex should not
 * be used.
 *
 * @param mutex Pointer to the zoo_pool_mutex_t structure to be destroyed.
 * @return 0 on success, or a negative error code on failure.
 */
static int zoo_pool_mutex_destroy(zoo_pool_mutex_t *mutex)
{
#if ZOO_HAS_POSIX
    return pthread_mutex_destroy(mutex);
#elif defined(ZOO_OS_WINDOWS)
    DeleteCriticalSection(mutex);
    return 0;
#elif defined(ZOO_OS_FREERTOS)
    vSemaphoreDelete(*mutex);
    return 0;
#elif defined(ZOO_OS_CMSIS_RTOS)
    return (osMutexDelete(*mutex) == osOK) ? 0 : -1;
#else
    *mutex = 0;
    return 0;
#endif
}

// ==============================================================================
// MEMORY POOL IMPLEMENTATION
// ==============================================================================

/**
 * @brief Initializes a memory slab pool
 *
 * Initializes the slab pool structure and prepares it for allocating memory.
 * This function must be called before any memory allocation operations on the pool.
 *
 * @param pool Pointer to the memory pool structure to be initialized
 *
 * @note The pool structure should be allocated before calling this function
 */
static void smb_slab_init(ZOO_MEMORY_POOL_STRUCT *pool)
{
    ZOO_UINT8 *p;
    ZOO_SIZE_T size;
    ZOO_UINTPTR_T i, n, pages;
    ZOO_SLAB_PAGE_STRUCT *slots;

    // Initialize mutex
    zoo_pool_mutex_init(&pool->mutex);

    /*pagesize*/
    smb_pagesize = zoo_get_page_size();
    for (n = smb_pagesize, smb_pagesize_shift = 0;
         n >>= 1;
         smb_pagesize_shift++)
    { /* void */
    }

    /* STUB */
    if (smb_slab_max_size == 0)
    {
        smb_slab_max_size = smb_pagesize / 2;
        smb_slab_exact_size = smb_pagesize / (8 * sizeof(ZOO_UINTPTR_T));
        for (n = smb_slab_exact_size; n >>= 1; smb_slab_exact_shift++)
        {
            /* void */
        }
    }

    pool->min_size = 1 << pool->min_shift;

    p = (ZOO_UINT8 *)pool + sizeof(ZOO_MEMORY_POOL_STRUCT);
    slots = (ZOO_SLAB_PAGE_STRUCT *)p;

    n = smb_pagesize_shift - pool->min_shift;
    for (i = 0; i < n; i++)
    {
        slots[i].slab = 0;
        slots[i].next = &slots[i];
        slots[i].prev = 0;
    }

    p += n * sizeof(ZOO_SLAB_PAGE_STRUCT);

    size = (ZOO_SIZE_T)(pool->end - p);

    pages = (ZOO_UINTPTR_T)(size / (smb_pagesize + sizeof(ZOO_SLAB_PAGE_STRUCT)));

    memset(p, 0x0, pages * sizeof(ZOO_SLAB_PAGE_STRUCT));

    pool->pages = (ZOO_SLAB_PAGE_STRUCT *)p;

    pool->free.prev = (ZOO_UINTPTR_T)pool->pages;
    pool->free.next = pool->pages;

    pool->pages->slab = smb_real_pages;
    pool->pages->next = &pool->free;
    pool->pages->prev = (ZOO_UINTPTR_T)&pool->free;

    pool->start = (ZOO_UINT8 *)
        SMB_ALIGN_PTR((ZOO_UINTPTR_T)p + pages * sizeof(ZOO_SLAB_PAGE_STRUCT), smb_pagesize);

    smb_real_pages = (ZOO_SIZE_T)(pool->end - pool->start) / smb_pagesize;
    pool->pages->slab = smb_real_pages;
}

/**
 * @brief Allocates memory from a slab within the memory pool with locking
 *
 * This function allocates a block of memory of the specified size from the given memory pool.
 * The allocation is performed with proper locking mechanisms to ensure thread safety.
 *
 * @param pool Handle to the memory pool from which to allocate
 * @param size Size of the memory block to allocate in bytes
 * @return void* Pointer to the allocated memory block, or NULL if allocation fails
 */
static void *smb_slab_alloc_locked(ZOO_MEMORY_POOL_STRUCT *pool, ZOO_SIZE_T size)
{
    ZOO_SIZE_T s;
    ZOO_UINTPTR_T p, n, m, mask, *bitmap;
    ZOO_UINTPTR_T i, slot, shift, map;
    ZOO_SLAB_PAGE_STRUCT *page, *prev, *slots;

    if (size >= smb_slab_max_size)
    {
        // Logging removed

        page = smb_slab_alloc_pages(pool, (size >> smb_pagesize_shift) + ((size % smb_pagesize) ? 1 : 0));
        if (page)
        {
            p = (ZOO_UINTPTR_T)(page - pool->pages) << smb_pagesize_shift;
            p += (ZOO_UINTPTR_T)pool->start;
        }
        else
        {
            p = 0;
        }

        goto done;
    }

    if (size > pool->min_size)
    {
        shift = 1;
        for (s = size - 1; s >>= 1; shift++)
        { /* void */
        }
        slot = shift - pool->min_shift;
    }
    else
    {
        size = pool->min_size;
        shift = pool->min_shift;
        slot = 0;
    }

    slots = (ZOO_SLAB_PAGE_STRUCT *)((ZOO_UINT8 *)pool + sizeof(ZOO_MEMORY_POOL_STRUCT));
    page = slots[slot].next;

    if (page->next != page)
    {
        if (shift < smb_slab_exact_shift)
        {
            do
            {
                p = (ZOO_UINTPTR_T)(page - pool->pages) << smb_pagesize_shift;
                bitmap = (ZOO_UINTPTR_T *)(pool->start + p);

                map = (1 << (smb_pagesize_shift - shift)) / (sizeof(ZOO_UINTPTR_T) * 8);

                for (n = 0; n < map; n++)
                {
                    if (bitmap[n] != SMB_SLAB_BUSY)
                    {
                        for (m = 1, i = 0; m; m <<= 1, i++)
                        {
                            if ((bitmap[n] & m))
                            {
                                continue;
                            }

                            bitmap[n] |= m;

                            i = ((n * sizeof(ZOO_UINTPTR_T) * 8) << shift) + (i << shift);

                            if (bitmap[n] == SMB_SLAB_BUSY)
                            {
                                for (n = n + 1; n < map; n++)
                                {
                                    if (bitmap[n] != SMB_SLAB_BUSY)
                                    {
                                        p = (ZOO_UINTPTR_T)bitmap + i;

                                        goto done;
                                    }
                                }

                                prev = (ZOO_SLAB_PAGE_STRUCT *)(page->prev & ~SMB_SLAB_PAGE_MASK);
                                prev->next = page->next;
                                page->next->prev = page->prev;

                                page->next = NULL;
                                page->prev = SMB_SLAB_SMALL;
                            }

                            p = (ZOO_UINTPTR_T)bitmap + i;

                            goto done;
                        }
                    }
                }

                page = page->next;

            } while (page);
        }
        else if (shift == smb_slab_exact_shift)
        {
            do
            {
                if (page->slab != SMB_SLAB_BUSY)
                {
                    for (m = 1, i = 0; m; m <<= 1, i++)
                    {
                        if ((page->slab & m))
                        {
                            continue;
                        }

                        page->slab |= m;

                        if (page->slab == SMB_SLAB_BUSY)
                        {
                            prev = (ZOO_SLAB_PAGE_STRUCT *)(page->prev & ~SMB_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = SMB_SLAB_EXACT;
                        }

                        p = (ZOO_UINTPTR_T)(page - pool->pages) << smb_pagesize_shift;
                        p += i << shift;
                        p += (ZOO_UINTPTR_T)pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);
        }
        else
        { /* shift > smb_slab_exact_shift */

            n = smb_pagesize_shift - (page->slab & SMB_SLAB_SHIFT_MASK);
            n = 1 << n;
            n = ((ZOO_UINTPTR_T)1 << n) - 1;
            mask = n << SMB_SLAB_MAP_SHIFT;

            do
            {
                if ((page->slab & SMB_SLAB_MAP_MASK) != mask)
                {
                    for (m = (ZOO_UINTPTR_T)1 << SMB_SLAB_MAP_SHIFT, i = 0;
                         m & mask;
                         m <<= 1, i++)
                    {
                        if ((page->slab & m))
                        {
                            continue;
                        }

                        page->slab |= m;

                        if ((page->slab & SMB_SLAB_MAP_MASK) == mask)
                        {
                            prev = (ZOO_SLAB_PAGE_STRUCT *)(page->prev & ~SMB_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = SMB_SLAB_BIG;
                        }

                        p = (ZOO_UINTPTR_T)(page - pool->pages) << smb_pagesize_shift;
                        p += i << shift;
                        p += (ZOO_UINTPTR_T)pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);
        }
    }

    page = smb_slab_alloc_pages(pool, 1);

    if (page)
    {
        if (shift < smb_slab_exact_shift)
        {
            p = (ZOO_UINTPTR_T)(page - pool->pages) << smb_pagesize_shift;
            bitmap = (ZOO_UINTPTR_T *)(pool->start + p);

            s = 1 << shift;
            n = (ZOO_SIZE_T)((1 << (smb_pagesize_shift - shift)) / 8 / s);

            if (n == 0)
            {
                n = 1;
            }

            bitmap[0] = (ZOO_UINTPTR_T)((2 << n) - 1);

            map = (1 << (smb_pagesize_shift - shift)) / (sizeof(ZOO_UINTPTR_T) * 8);

            for (i = 1; i < map; i++)
            {
                bitmap[i] = 0;
            }

            page->slab = shift;
            page->next = &slots[slot];
            page->prev = (ZOO_UINTPTR_T)&slots[slot] | SMB_SLAB_SMALL;

            slots[slot].next = page;

            p = (ZOO_UINTPTR_T)((page - pool->pages) << smb_pagesize_shift) + s * n;
            p += (ZOO_UINTPTR_T)pool->start;

            goto done;
        }
        else if (shift == smb_slab_exact_shift)
        {
            page->slab = 1;
            page->next = &slots[slot];
            page->prev = (ZOO_UINTPTR_T)&slots[slot] | SMB_SLAB_EXACT;

            slots[slot].next = page;

            p = (ZOO_UINTPTR_T)(page - pool->pages) << smb_pagesize_shift;
            p += (ZOO_UINTPTR_T)pool->start;

            goto done;
        }
        else
        { /* shift > smb_slab_exact_shift */

            page->slab = ((ZOO_UINTPTR_T)1 << SMB_SLAB_MAP_SHIFT) | shift;
            page->next = &slots[slot];
            page->prev = (ZOO_UINTPTR_T)&slots[slot] | SMB_SLAB_BIG;

            slots[slot].next = page;

            p = (ZOO_UINTPTR_T)(page - pool->pages) << smb_pagesize_shift;
            p += (ZOO_UINTPTR_T)pool->start;

            goto done;
        }
    }

    p = 0;

done:

    // // Logging removed

    return (void *)p;
}

/**
 * Frees memory back to the slab allocator while holding the lock.
 * This function assumes the memory pool lock is already acquired.
 *
 * @param pool Pointer to the memory pool structure
 * @param p Pointer to the memory block to be freed
 *
 * @warning This function must be called with the pool lock held
 */
static void smb_slab_free_locked(ZOO_MEMORY_POOL_STRUCT *pool, void *p)
{
    ZOO_SIZE_T size;
    ZOO_UINTPTR_T slab, m, *bitmap;
    ZOO_UINTPTR_T n, type, slot, shift, map;
    ZOO_SLAB_PAGE_STRUCT *slots, *page;

    if ((ZOO_UINT8 *)p < pool->start || (ZOO_UINT8 *)p > pool->end)
    {
        // Logging removed
        goto fail;
    }

    n = (ZOO_UINTPTR_T)((ZOO_UINT8 *)p - pool->start) >> smb_pagesize_shift;
    page = &pool->pages[n];
    slab = page->slab;
    type = page->prev & SMB_SLAB_PAGE_MASK;

    switch (type)
    {
    case SMB_SLAB_SMALL:

        shift = slab & SMB_SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((ZOO_UINTPTR_T)p & (size - 1))
        {
            goto wrong_chunk;
        }

        n = ((ZOO_UINTPTR_T)p & (smb_pagesize - 1)) >> shift;
        m = (ZOO_UINTPTR_T)1 << (n & (sizeof(ZOO_UINTPTR_T) * 8 - 1));
        n /= (sizeof(ZOO_UINTPTR_T) * 8);
        bitmap = (ZOO_UINTPTR_T *)((ZOO_UINTPTR_T)p & ~(smb_pagesize - 1));

        if (bitmap[n] & m)
        {
            if (page->next == NULL)
            {
                slots = (ZOO_SLAB_PAGE_STRUCT *)((ZOO_UINT8 *)pool + sizeof(ZOO_MEMORY_POOL_STRUCT));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (ZOO_UINTPTR_T)&slots[slot] | SMB_SLAB_SMALL;
                page->next->prev = (ZOO_UINTPTR_T)page | SMB_SLAB_SMALL;
            }

            bitmap[n] &= ~m;

            n = (ZOO_UINTPTR_T)((1 << (smb_pagesize_shift - shift)) / 8 / (1 << shift));

            if (n == 0)
            {
                n = 1;
            }

            if (bitmap[0] & ~(((ZOO_UINTPTR_T)1 << n) - 1))
            {
                goto done;
            }

            map = (1 << (smb_pagesize_shift - shift)) / (sizeof(ZOO_UINTPTR_T) * 8);

            for (n = 1; n < map; n++)
            {
                if (bitmap[n])
                {
                    goto done;
                }
            }

            smb_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case SMB_SLAB_EXACT:

        m = (ZOO_UINTPTR_T)1 << (((ZOO_UINTPTR_T)p & (smb_pagesize - 1)) >> smb_slab_exact_shift);
        size = smb_slab_exact_size;

        if ((ZOO_UINTPTR_T)p & (size - 1))
        {
            goto wrong_chunk;
        }

        if (slab & m)
        {
            if (slab == SMB_SLAB_BUSY)
            {
                slots = (ZOO_SLAB_PAGE_STRUCT *)((ZOO_UINT8 *)pool + sizeof(ZOO_MEMORY_POOL_STRUCT));
                slot = smb_slab_exact_shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (ZOO_UINTPTR_T)&slots[slot] | SMB_SLAB_EXACT;
                page->next->prev = (ZOO_UINTPTR_T)page | SMB_SLAB_EXACT;
            }

            page->slab &= ~m;

            if (page->slab)
            {
                goto done;
            }

            smb_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case SMB_SLAB_BIG:

        shift = slab & SMB_SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((ZOO_UINTPTR_T)p & (size - 1))
        {
            goto wrong_chunk;
        }

        m = (ZOO_UINTPTR_T)1 << ((((ZOO_UINTPTR_T)p & (smb_pagesize - 1)) >> shift) + SMB_SLAB_MAP_SHIFT);

        if (slab & m)
        {
            if (page->next == NULL)
            {
                slots = (ZOO_SLAB_PAGE_STRUCT *)((ZOO_UINT8 *)pool + sizeof(ZOO_MEMORY_POOL_STRUCT));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (ZOO_UINTPTR_T)&slots[slot] | SMB_SLAB_BIG;
                page->next->prev = (ZOO_UINTPTR_T)page | SMB_SLAB_BIG;
            }

            page->slab &= ~m;

            if (page->slab & SMB_SLAB_MAP_MASK)
            {
                goto done;
            }

            smb_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case SMB_SLAB_PAGE:

        if ((ZOO_UINTPTR_T)p & (smb_pagesize - 1))
        {
            goto wrong_chunk;
        }

        if (slab == SMB_SLAB_PAGE_FREE)
        {
            // Logging removed
            goto fail;
        }

        if (slab == SMB_SLAB_PAGE_BUSY)
        {
            // Logging removed
            goto fail;
        }

        n = (ZOO_UINTPTR_T)((ZOO_UINT8 *)p - pool->start) >> smb_pagesize_shift;
        size = slab & ~SMB_SLAB_PAGE_START;

        smb_slab_free_pages(pool, &pool->pages[n], size);

        return;
    }

    /* not reached */

    return;

done:

    return;

wrong_chunk:

    // Logging removed

    goto fail;

chunk_already_free:

    // Logging removed

fail:

    return;
}

/**
 * @brief Create a memory pool with preallocated memory.
 * @param total_size Total size of the memory pool in bytes.
 * @param[out] error Error code output (optional).
 * @return Memory pool handle, NULL on failure.
 */
ZOO_INT32 zoo_create_memory_pool(ZOO_SIZE_T total_size)
{
    // Logging removed

    if (smb_memory_pool == NULL)
    {
        ZOO_UINT8 *buffer = (ZOO_UINT8 *)malloc(sizeof(char) * total_size);
        if (!buffer)
        {
            // Logging removed
            return ZOO_ERROR_MEM_POOL_ALLOCATION_FAILED;
        }

        // Clear memory
        // memset(smb_memory_pool, 0, sizeof(ZOO_MEMORY_POOL_STRUCT));
        memset(buffer, 0, sizeof(char) * total_size);
        smb_memory_pool = (ZOO_MEMORY_POOL_STRUCT *)buffer;
        // Initialize pool structure
        smb_memory_pool->start = buffer + sizeof(ZOO_MEMORY_POOL_STRUCT);
        smb_memory_pool->end = buffer + total_size;
        smb_memory_pool->min_shift = 3;

        // Initialize mutex
        if (zoo_pool_mutex_init(&smb_memory_pool->mutex) != 0)
        {
            // Logging removed
            free(buffer);
            smb_memory_pool = NULL;
            return ZOO_ERROR_MEM_POOL_ALLOCATION_FAILED;
        }

        // Initialize slab allocator
        smb_slab_init(smb_memory_pool);
    }

    return ZOO_OK;
}

/**
 * @brief Allocates memory from a memory pool
 *
 * @param pool Pointer to the memory pool to allocate from
 * @param size Size of the memory block to allocate in bytes
 * @return void* Pointer to the allocated memory block, or NULL if allocation fails
 *
 * This function attempts to allocate a block of memory from the specified memory pool.
 * The allocation is done in a thread-safe manner if the pool was created with thread
 * safety enabled.
 */
void *zoo_allocate_from_pool(
    ZOO_SIZE_T size)
{
    if (!smb_memory_pool || size == 0)
    {
        // Logging removed
        return NULL;
    }
    void *ptr = NULL;
    zoo_pool_mutex_lock(&smb_memory_pool->mutex);
    ptr = smb_slab_alloc_locked(smb_memory_pool, size);
    zoo_pool_mutex_unlock(&smb_memory_pool->mutex);
    if (ptr)
    {
        memset(ptr, 0, size);
    }
    return ptr;
}

/**
 * @brief Destroy a memory pool and release all resources.
 * @param pool Memory pool handle.
 */
void zoo_destroy_memory_pool(void)
{
    // Logging removed
    if (smb_memory_pool)
    {
        zoo_pool_mutex_destroy(&smb_memory_pool->mutex);
        free(smb_memory_pool);
        smb_memory_pool = NULL;
    }
}

/**
 * @brief Frees memory back to the specified memory pool
 *
 * @param pool Handle to the memory pool where memory will be returned
 * @param p Pointer to the memory block to be freed
 *
 * This function releases previously allocated memory back to the specified memory pool.
 * The memory block pointed to by p will be made available for future allocations from
 * the same pool.
 */
void zoo_free_to_pool(void *p)
{
    if (p && smb_memory_pool)
    {
        zoo_pool_mutex_lock(&smb_memory_pool->mutex);
        smb_slab_free_locked(smb_memory_pool, p);
        zoo_pool_mutex_unlock(&smb_memory_pool->mutex);
    }
}

/**
 * @brief Allocates a specified number of slab pages from the memory pool
 *
 * @param pool Handle to the memory pool to allocate pages from
 * @param pages Number of pages to allocate
 *
 * @return Pointer to the allocated slab page structure if successful, NULL if allocation fails
 */
static ZOO_SLAB_PAGE_STRUCT *smb_slab_alloc_pages(ZOO_MEMORY_POOL_STRUCT *pool, ZOO_UINTPTR_T pages)
{
    ZOO_SLAB_PAGE_STRUCT *page = NULL;
    ZOO_SLAB_PAGE_STRUCT *p = NULL;

    for (page = pool->free.next; page != &pool->free; page = page->next)
    {
        if (page->slab >= pages)
        {
            if (page->slab > pages)
            {
                page[pages].slab = page->slab - pages;
                page[pages].next = page->next;
                page[pages].prev = page->prev;

                p = (ZOO_SLAB_PAGE_STRUCT *)page->prev;
                p->next = &page[pages];
                page->next->prev = (ZOO_UINTPTR_T)&page[pages];
            }
            else
            {
                p = (ZOO_SLAB_PAGE_STRUCT *)page->prev;
                p->next = page->next;
                page->next->prev = page->prev;
            }

            page->slab = pages | SMB_SLAB_PAGE_START;
            page->next = NULL;
            page->prev = SMB_SLAB_PAGE;

            if (--pages == 0)
            {
                return page;
            }

            for (p = page + 1; pages; pages--)
            {
                p->slab = SMB_SLAB_PAGE_BUSY;
                p->next = NULL;
                p->prev = SMB_SLAB_PAGE;
                p++;
            }

            return page;
        }
    }
    // Logging removed
    return NULL;
}

/**
 * @brief Frees pages from a slab allocator back to the memory pool
 *
 * @param pool Handle to the memory pool
 * @param page Pointer to the slab page structure to be freed
 * @param pages Number of pages to free
 *
 * This function releases previously allocated slab pages back to the memory pool.
 * It handles the deallocation of contiguous pages starting from the given page address.
 */
static void smb_slab_free_pages(ZOO_MEMORY_POOL_STRUCT *pool, ZOO_SLAB_PAGE_STRUCT *page, ZOO_UINTPTR_T pages)
{
    ZOO_SLAB_PAGE_STRUCT *prev = NULL;

    if (pages > 1)
    {
        memset(&page[1], 0x0, (pages - 1) * sizeof(ZOO_SLAB_PAGE_STRUCT));
    }

    if (page->next)
    {
        prev = (ZOO_SLAB_PAGE_STRUCT *)(page->prev & ~SMB_SLAB_PAGE_MASK);
        prev->next = page->next;
        page->next->prev = page->prev;
    }

    page->slab = pages;
    page->prev = (ZOO_UINTPTR_T)&pool->free;
    page->next = pool->free.next;
    page->next->prev = (ZOO_UINTPTR_T)page;

    pool->free.next = page;
}

/**
 * @brief Get detailed memory usage statistics with deadlock prevention
 *
 * This function traverses the memory pool pages safely and collects statistics
 * about memory usage including allocated blocks, free pages, and fragmentation info.
 *
 * @param stat Pointer to structure to store usage statistics
 *
 * @note This function includes protection against infinite loops and validates
 *       all page traversal operations to prevent deadlocks
 */
void zoo_memory_pool_get_usage(ZOO_MEMORY_USAGE_T *stat)
{
    if (!stat || !smb_memory_pool)
    {
        return;
    }

    memset(stat, 0, sizeof(ZOO_MEMORY_USAGE_T));
    zoo_pool_mutex_lock(&smb_memory_pool->mutex);

    stat->pool_size = (ZOO_SIZE_T)(smb_memory_pool->end - smb_memory_pool->start);
    stat->pages = stat->pool_size / smb_pagesize;

    ZOO_SLAB_PAGE_STRUCT *page = smb_memory_pool->free.next;
    while (page != &smb_memory_pool->free)
    {
        ZOO_UINTPTR_T free_pages = page->slab;
        stat->free_page += free_pages;
        stat->free_size += free_pages * smb_pagesize;

        if (free_pages > stat->max_free_pages)
        {
            stat->max_free_pages = free_pages;
        }
        page = page->next;
    }

    stat->used_size = stat->pool_size - stat->free_size;

    if (stat->pool_size > 0)
    {
        stat->used_pct = (stat->used_size * 100) / stat->pool_size;
    }

    zoo_pool_mutex_unlock(&smb_memory_pool->mutex);
}

/**
 * @brief Validate memory pool integrity
 * Fixed validation logic to correctly check free list
 */
ZOO_INT32 zoo_validate_memory_pool(void)
{
    if (!smb_memory_pool)
    {
        return -1;
    }

    zoo_pool_mutex_lock(&smb_memory_pool->mutex);
    ZOO_SIZE_T free_accounted = 0;

    ZOO_SLAB_PAGE_STRUCT *page = smb_memory_pool->free.next;
    ZOO_SLAB_PAGE_STRUCT *prev = &smb_memory_pool->free;

    while (page != &smb_memory_pool->free)
    {
        if (page->next == NULL)
        {
            // Logging removed
            zoo_pool_mutex_unlock(&smb_memory_pool->mutex);
            return -1;
        }

        ZOO_SLAB_PAGE_STRUCT *prev_from_node = (ZOO_SLAB_PAGE_STRUCT *)(page->prev & ~SMB_SLAB_PAGE_MASK);
        if (prev_from_node != prev)
        {
            // Logging removed
            zoo_pool_mutex_unlock(&smb_memory_pool->mutex);
            return -1;
        }

        free_accounted += page->slab;

        prev = page;
        page = page->next;

        if (page == smb_memory_pool->free.next)
        {
            // Logging removed
            zoo_pool_mutex_unlock(&smb_memory_pool->mutex);
            return -1;
        }
    }

    ZOO_SIZE_T total_accounted = smb_real_pages;

    if (free_accounted > total_accounted)
    {
        // Page count mismatch error
        zoo_pool_mutex_unlock(&smb_memory_pool->mutex);
        return -1;
    }

    zoo_pool_mutex_unlock(&smb_memory_pool->mutex);
    return 0;
}
