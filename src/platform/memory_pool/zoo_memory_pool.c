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
typedef struct ZOO_SLAB_PAGE_STRUCT ZOO_SLAB_PAGE_STRUCT;
typedef struct ZOO_MEMORY_POOL_STRUCT ZOO_MEMORY_POOL_STRUCT;

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
struct ZOO_MEMORY_POOL_STRUCT
{
    ZOO_SIZE_T min_size;
    ZOO_SIZE_T min_shift;
    ZOO_SLAB_PAGE_STRUCT *pages;
    ZOO_SLAB_PAGE_STRUCT free;
    ZOO_UINT8 *start;
    ZOO_UINT8 *end;
    zoo_pool_mutex_t mutex;
    ZOO_SIZE_T total_size;
    ZOO_SIZE_T real_pages;
    ZOO_MEMORY_POOL_STRUCT *next;
};

/**
 * @brief Static pointer to the memory pool structure.
 *
 * This variable holds the reference to the singleton instance of the memory pool.
 * It is initialized to NULL and should be assigned during memory pool creation.
 */
static ZOO_MEMORY_POOL_STRUCT *smb_memory_pool = NULL;
static ZOO_MEMORY_POOL_STRUCT *smb_memory_pool_tail = NULL;
static zoo_pool_mutex_t smb_memory_pool_list_mutex;
static ZOO_BOOL smb_memory_pool_list_mutex_ready = ZOO_FALSE;

static ZOO_MEMORY_POOL_STRUCT *smb_memory_pool_create_segment(ZOO_SIZE_T total_size);
static ZOO_MEMORY_POOL_STRUCT *smb_memory_pool_find_segment_for_ptr(ZOO_MEMORY_POOL_STRUCT *pool, const void *ptr);
static ZOO_SIZE_T smb_memory_pool_segment_size(ZOO_SIZE_T request_size, ZOO_SIZE_T reference_size);
static void *smb_memory_pool_alloc_locked(ZOO_MEMORY_POOL_STRUCT *pool, ZOO_SIZE_T size);
static void smb_memory_pool_free_locked(ZOO_MEMORY_POOL_STRUCT *pool, void *p);
static void *smb_slab_alloc_locked(ZOO_MEMORY_POOL_STRUCT *pool, ZOO_SIZE_T size);
static void smb_slab_free_locked(ZOO_MEMORY_POOL_STRUCT *pool, void *p);

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
 * @brief Returns the OS page size used to lay out slab metadata and payload pages.
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
 * @brief Initializes one allocator mutex using the active platform backend.
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
 * @brief Acquires one allocator mutex.
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
 * @brief Releases one allocator mutex.
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
 * @brief Destroys one allocator mutex.
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
 * @brief Initializes slab metadata for a single pool segment.
 *
 * This lays out slot headers, page descriptors, the segment free-page list,
 * and the aligned payload start for one growable segment.
 */
static void smb_slab_init(ZOO_MEMORY_POOL_STRUCT *pool)
{
    ZOO_UINT8 *p;
    ZOO_SIZE_T size;
    ZOO_UINTPTR_T i, n, pages;
    ZOO_SLAB_PAGE_STRUCT *slots;

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

    pool->pages->next = &pool->free;
    pool->pages->prev = (ZOO_UINTPTR_T)&pool->free;

    pool->start = (ZOO_UINT8 *)
        SMB_ALIGN_PTR((ZOO_UINTPTR_T)p + pages * sizeof(ZOO_SLAB_PAGE_STRUCT), smb_pagesize);

    pool->real_pages = (ZOO_SIZE_T)(pool->end - pool->start) / smb_pagesize;
    pool->pages->slab = pool->real_pages;
}

/**
 * @brief Allocates and initializes one pool segment.
 *
 * Each segment owns its own slab metadata, backing storage, and mutex while
 * remaining linked into the global segment chain.
 */
static ZOO_MEMORY_POOL_STRUCT *smb_memory_pool_create_segment(ZOO_SIZE_T total_size)
{
    ZOO_UINT8 *buffer;
    ZOO_MEMORY_POOL_STRUCT *pool;

    if (total_size <= sizeof(ZOO_MEMORY_POOL_STRUCT))
    {
        return NULL;
    }

    buffer = (ZOO_UINT8 *)malloc(total_size);
    if (!buffer)
    {
        return NULL;
    }

    memset(buffer, 0, total_size);
    pool = (ZOO_MEMORY_POOL_STRUCT *)buffer;
    pool->start = buffer + sizeof(ZOO_MEMORY_POOL_STRUCT);
    pool->end = buffer + total_size;
    pool->min_shift = 3;
    pool->total_size = total_size;
    pool->next = NULL;

    if (zoo_pool_mutex_init(&pool->mutex) != 0)
    {
        free(buffer);
        return NULL;
    }

    smb_slab_init(pool);
    return pool;
}

/**
 * @brief Returns the segment that owns a previously allocated pointer.
 */
static ZOO_MEMORY_POOL_STRUCT *smb_memory_pool_find_segment_for_ptr(ZOO_MEMORY_POOL_STRUCT *pool, const void *ptr)
{
    ZOO_MEMORY_POOL_STRUCT *segment;
    const ZOO_UINT8 *address = (const ZOO_UINT8 *)ptr;

    for (segment = pool; segment != NULL; segment = segment->next)
    {
        if (address >= segment->start && address < segment->end)
        {
            return segment;
        }
    }

    return NULL;
}

/**
 * @brief Chooses the size of a new segment for a growth allocation.
 *
 * The result is large enough for the requested block plus the segment's slab
 * bookkeeping, and grows from the previous segment size when possible.
 */
static ZOO_SIZE_T smb_memory_pool_segment_size(ZOO_SIZE_T request_size, ZOO_SIZE_T reference_size)
{
    ZOO_SIZE_T page_size = smb_pagesize ? (ZOO_SIZE_T)smb_pagesize : zoo_get_page_size();
    ZOO_UINTPTR_T page_shift = 0;
    ZOO_UINTPTR_T slot_count;
    ZOO_SIZE_T requested_pages;
    ZOO_SIZE_T min_total;
    ZOO_SIZE_T candidate_size;

    for (ZOO_SIZE_T n = page_size; n >>= 1; page_shift++)
    {
        /* void */
    }

    slot_count = page_shift > 3 ? page_shift - 3 : 1;
    requested_pages = (request_size + page_size - 1) / page_size;
    if (requested_pages == 0)
    {
        requested_pages = 1;
    }

    min_total = sizeof(ZOO_MEMORY_POOL_STRUCT) +
                (slot_count + requested_pages + 2) * sizeof(ZOO_SLAB_PAGE_STRUCT) +
                (requested_pages + 2) * page_size;

    candidate_size = reference_size > 0 ? reference_size : min_total;
    while (candidate_size < min_total)
    {
        if (candidate_size > (SIZE_MAX / 2))
        {
            candidate_size = min_total;
            break;
        }
        candidate_size *= 2;
    }

    if (candidate_size % page_size != 0)
    {
        candidate_size += page_size - (candidate_size % page_size);
    }

    return candidate_size;
}

/**
 * @brief Allocates from the segment chain while coordinating list and segment locks.
 *
 * The caller enters with the global segment-list mutex held. This function may
 * walk existing segments, grow the chain with a new segment, and always releases
 * the list mutex before it returns.
 */
static void *smb_memory_pool_alloc_locked(ZOO_MEMORY_POOL_STRUCT *pool, ZOO_SIZE_T size)
{
    ZOO_MEMORY_POOL_STRUCT *segment;
    ZOO_MEMORY_POOL_STRUCT *next_segment;
    ZOO_MEMORY_POOL_STRUCT *new_segment;
    void *ptr;

    segment = pool;
    while (segment != NULL)
    {
        next_segment = segment->next;
        zoo_pool_mutex_lock(&segment->mutex);
        zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
        ptr = smb_slab_alloc_locked(segment, size);
        zoo_pool_mutex_unlock(&segment->mutex);
        if (ptr)
        {
            return ptr;
        }

        zoo_pool_mutex_lock(&smb_memory_pool_list_mutex);
        segment = next_segment;
    }

    if (!smb_memory_pool_tail)
    {
        zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
        return NULL;
    }

    new_segment = smb_memory_pool_create_segment(
        smb_memory_pool_segment_size(size, smb_memory_pool_tail->total_size));
    if (!new_segment)
    {
        zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
        return NULL;
    }

    smb_memory_pool_tail->next = new_segment;
    smb_memory_pool_tail = new_segment;

    zoo_pool_mutex_lock(&smb_memory_pool_tail->mutex);
    zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
    ptr = smb_slab_alloc_locked(smb_memory_pool_tail, size);
    zoo_pool_mutex_unlock(&smb_memory_pool_tail->mutex);

    return ptr;
}

/**
 * @brief Returns a pointer to its owning segment and frees it there.
 *
 * The caller enters with the global segment-list mutex held. This function
 * locates the owning segment, hands off to the segment mutex, and releases the
 * list mutex before it returns.
 */
static void smb_memory_pool_free_locked(ZOO_MEMORY_POOL_STRUCT *pool, void *p)
{
    ZOO_MEMORY_POOL_STRUCT *segment = smb_memory_pool_find_segment_for_ptr(pool, p);

    if (segment)
    {
        zoo_pool_mutex_lock(&segment->mutex);
        zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
        smb_slab_free_locked(segment, p);
        zoo_pool_mutex_unlock(&segment->mutex);
        return;
    }

    zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
}

/**
 * @brief Returns the payload base address mapped by a page descriptor.
 */
static ZOO_UINTPTR_T smb_slab_page_addr(const ZOO_MEMORY_POOL_STRUCT *pool,
                                        const ZOO_SLAB_PAGE_STRUCT *page)
{
    ZOO_UINTPTR_T address = (ZOO_UINTPTR_T)(page - pool->pages) << smb_pagesize_shift;
    return address + (ZOO_UINTPTR_T)pool->start;
}

/**
 * @brief Removes a fully consumed slab page from its slot list.
 */
static void smb_slab_detach_full_page(ZOO_SLAB_PAGE_STRUCT *page, ZOO_UINTPTR_T slab_type)
{
    ZOO_SLAB_PAGE_STRUCT *prev = (ZOO_SLAB_PAGE_STRUCT *)(page->prev & ~SMB_SLAB_PAGE_MASK);

    prev->next = page->next;
    page->next->prev = page->prev;
    page->next = NULL;
    page->prev = slab_type;
}

/**
 * @brief Returns the number of bitmap words required for one small-slab page.
 */
static ZOO_UINTPTR_T smb_slab_small_map_words(ZOO_UINTPTR_T shift)
{
    return (1 << (smb_pagesize_shift - shift)) / (sizeof(ZOO_UINTPTR_T) * 8);
}

/**
 * @brief Returns the summary bit that tracks availability for one bitmap word.
 */
static ZOO_UINTPTR_T smb_slab_small_summary_bit(ZOO_UINTPTR_T word_index)
{
    return (ZOO_UINTPTR_T)1 << (SMB_SLAB_MAP_SHIFT + word_index);
}

/**
 * @brief Returns the mask of all summary bits used by one small-slab page.
 */
static ZOO_UINTPTR_T smb_slab_small_summary_mask(ZOO_UINTPTR_T shift)
{
    ZOO_UINTPTR_T map_words = smb_slab_small_map_words(shift);
    return (((ZOO_UINTPTR_T)1 << map_words) - 1) << SMB_SLAB_MAP_SHIFT;
}

/**
 * @brief Allocates from an existing small-slab page list.
 *
 * Small slabs store summary bits in page->slab so allocation can skip bitmap
 * words that are already full and detach a page without rescanning the whole map.
 */
static ZOO_UINTPTR_T smb_slab_try_alloc_small(ZOO_MEMORY_POOL_STRUCT *pool,
                                              ZOO_SLAB_PAGE_STRUCT *page,
                                              ZOO_UINTPTR_T shift)
{
    ZOO_UINTPTR_T n;
    ZOO_UINTPTR_T m;
    ZOO_UINTPTR_T i;
    ZOO_UINTPTR_T map = smb_slab_small_map_words(shift);
    ZOO_UINTPTR_T summary_mask = smb_slab_small_summary_mask(shift);
    ZOO_UINTPTR_T *bitmap;

    do
    {
        if ((page->slab & summary_mask) == 0)
        {
            page = page->next;
            continue;
        }

        bitmap = (ZOO_UINTPTR_T *)(pool->start + ((ZOO_UINTPTR_T)(page - pool->pages) << smb_pagesize_shift));

        for (n = 0; n < map; n++)
        {
            ZOO_UINTPTR_T summary_bit = smb_slab_small_summary_bit(n);

            if ((page->slab & summary_bit) == 0)
            {
                continue;
            }

            for (m = 1, i = 0; m; m <<= 1, i++)
            {
                if (bitmap[n] & m)
                {
                    continue;
                }

                bitmap[n] |= m;
                i = ((n * sizeof(ZOO_UINTPTR_T) * 8) << shift) + (i << shift);

                if (bitmap[n] == SMB_SLAB_BUSY)
                {
                    page->slab &= ~summary_bit;

                    if ((page->slab & summary_mask) == 0)
                    {
                        smb_slab_detach_full_page(page, SMB_SLAB_SMALL);
                    }
                }

                return (ZOO_UINTPTR_T)bitmap + i;
            }
        }

        page = page->next;
    } while (page);

    return 0;
}

/**
 * @brief Allocates from an existing exact-size slab page list.
 */
static ZOO_UINTPTR_T smb_slab_try_alloc_exact(ZOO_MEMORY_POOL_STRUCT *pool,
                                              ZOO_SLAB_PAGE_STRUCT *page,
                                              ZOO_UINTPTR_T shift)
{
    ZOO_UINTPTR_T m;
    ZOO_UINTPTR_T i;

    do
    {
        if (page->slab != SMB_SLAB_BUSY)
        {
            for (m = 1, i = 0; m; m <<= 1, i++)
            {
                if (page->slab & m)
                {
                    continue;
                }

                page->slab |= m;
                if (page->slab == SMB_SLAB_BUSY)
                {
                    smb_slab_detach_full_page(page, SMB_SLAB_EXACT);
                }

                return smb_slab_page_addr(pool, page) + (i << shift);
            }
        }

        page = page->next;
    } while (page);

    return 0;
}

/**
 * @brief Allocates from an existing big-slab page list.
 */
static ZOO_UINTPTR_T smb_slab_try_alloc_big(ZOO_MEMORY_POOL_STRUCT *pool,
                                            ZOO_SLAB_PAGE_STRUCT *page,
                                            ZOO_UINTPTR_T shift)
{
    ZOO_UINTPTR_T n = smb_pagesize_shift - (page->slab & SMB_SLAB_SHIFT_MASK);
    ZOO_UINTPTR_T mask;
    ZOO_UINTPTR_T m;
    ZOO_UINTPTR_T i;

    n = 1 << n;
    n = ((ZOO_UINTPTR_T)1 << n) - 1;
    mask = n << SMB_SLAB_MAP_SHIFT;

    do
    {
        if ((page->slab & SMB_SLAB_MAP_MASK) != mask)
        {
            for (m = (ZOO_UINTPTR_T)1 << SMB_SLAB_MAP_SHIFT, i = 0; m & mask; m <<= 1, i++)
            {
                if (page->slab & m)
                {
                    continue;
                }

                page->slab |= m;
                if ((page->slab & SMB_SLAB_MAP_MASK) == mask)
                {
                    smb_slab_detach_full_page(page, SMB_SLAB_BIG);
                }

                return smb_slab_page_addr(pool, page) + (i << shift);
            }
        }

        page = page->next;
    } while (page);

    return 0;
}

/**
 * @brief Initializes a fresh page as a small-slab page and returns its first chunk.
 */
static ZOO_UINTPTR_T smb_slab_init_small_page(ZOO_MEMORY_POOL_STRUCT *pool,
                                              ZOO_SLAB_PAGE_STRUCT *page,
                                              ZOO_SLAB_PAGE_STRUCT *slot,
                                              ZOO_UINTPTR_T shift)
{
    ZOO_UINTPTR_T p = smb_slab_page_addr(pool, page);
    ZOO_UINTPTR_T *bitmap = (ZOO_UINTPTR_T *)p;
    ZOO_SIZE_T unit_size = 1 << shift;
    ZOO_SIZE_T reserved_words = (ZOO_SIZE_T)((1 << (smb_pagesize_shift - shift)) / 8 / unit_size);
    ZOO_UINTPTR_T map = smb_slab_small_map_words(shift);
    ZOO_UINTPTR_T summary = smb_slab_small_summary_mask(shift);
    ZOO_UINTPTR_T i;

    if (reserved_words == 0)
    {
        reserved_words = 1;
    }

    bitmap[0] = (ZOO_UINTPTR_T)((2 << reserved_words) - 1);
    for (i = 1; i < map; i++)
    {
        bitmap[i] = 0;
    }

    if (bitmap[0] == SMB_SLAB_BUSY)
    {
        summary &= ~smb_slab_small_summary_bit(0);
    }

    page->slab = shift | summary;
    page->next = slot;
    page->prev = (ZOO_UINTPTR_T)slot | SMB_SLAB_SMALL;
    slot->next = page;

    return p + unit_size * reserved_words;
}

/**
 * @brief Initializes a fresh page as an exact-size slab page and returns its first chunk.
 */
static ZOO_UINTPTR_T smb_slab_init_exact_page(ZOO_MEMORY_POOL_STRUCT *pool,
                                              ZOO_SLAB_PAGE_STRUCT *page,
                                              ZOO_SLAB_PAGE_STRUCT *slot)
{
    page->slab = 1;
    page->next = slot;
    page->prev = (ZOO_UINTPTR_T)slot | SMB_SLAB_EXACT;
    slot->next = page;

    return smb_slab_page_addr(pool, page);
}

/**
 * @brief Initializes a fresh page as a big-slab page and returns its first chunk.
 */
static ZOO_UINTPTR_T smb_slab_init_big_page(ZOO_MEMORY_POOL_STRUCT *pool,
                                            ZOO_SLAB_PAGE_STRUCT *page,
                                            ZOO_SLAB_PAGE_STRUCT *slot,
                                            ZOO_UINTPTR_T shift)
{
    page->slab = ((ZOO_UINTPTR_T)1 << SMB_SLAB_MAP_SHIFT) | shift;
    page->next = slot;
    page->prev = (ZOO_UINTPTR_T)slot | SMB_SLAB_BIG;
    slot->next = page;

    return smb_slab_page_addr(pool, page);
}

/**
 * @brief Allocates one block from a single segment.
 *
 * Large requests are satisfied from whole pages. Smaller requests are served
 * from the slot list for the computed shift, or by converting a new page into
 * the required slab class when no reusable page exists.
 */
static void *smb_slab_alloc_locked(ZOO_MEMORY_POOL_STRUCT *pool, ZOO_SIZE_T size)
{
    ZOO_SIZE_T s;
    ZOO_UINTPTR_T p;
    ZOO_UINTPTR_T slot;
    ZOO_UINTPTR_T shift;
    ZOO_SLAB_PAGE_STRUCT *page;
    ZOO_SLAB_PAGE_STRUCT *slots;

    if (size >= smb_slab_max_size)
    {
        page = smb_slab_alloc_pages(pool, (size >> smb_pagesize_shift) + ((size % smb_pagesize) ? 1 : 0));
        return page ? (void *)smb_slab_page_addr(pool, page) : NULL;
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
            p = smb_slab_try_alloc_small(pool, page, shift);
        }
        else if (shift == smb_slab_exact_shift)
        {
            p = smb_slab_try_alloc_exact(pool, page, shift);
        }
        else
        {
            p = smb_slab_try_alloc_big(pool, page, shift);
        }

        if (p != 0)
        {
            return (void *)p;
        }
    }

    page = smb_slab_alloc_pages(pool, 1);

    if (page)
    {
        if (shift < smb_slab_exact_shift)
        {
            return (void *)smb_slab_init_small_page(pool, page, &slots[slot], shift);
        }
        else if (shift == smb_slab_exact_shift)
        {
            return (void *)smb_slab_init_exact_page(pool, page, &slots[slot]);
        }
        else
        {
            return (void *)smb_slab_init_big_page(pool, page, &slots[slot], shift);
        }
    }

    return NULL;
}

/**
 * @brief Frees one allocation back into a single segment.
 *
 * This restores slot-list membership when a previously full slab page becomes
 * reusable and returns whole pages to the free-page list when a slab page becomes empty.
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
            ZOO_UINTPTR_T summary_bit = smb_slab_small_summary_bit(n);

            if (page->next == NULL)
            {
                slots = (ZOO_SLAB_PAGE_STRUCT *)((ZOO_UINT8 *)pool + sizeof(ZOO_MEMORY_POOL_STRUCT));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (ZOO_UINTPTR_T)&slots[slot] | SMB_SLAB_SMALL;
                page->next->prev = (ZOO_UINTPTR_T)page | SMB_SLAB_SMALL;
            }

            if (bitmap[n] == SMB_SLAB_BUSY)
            {
                page->slab |= summary_bit;
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
 * @brief Creates the root memory-pool segment and shared list mutex.
 */
ZOO_INT32 zoo_create_memory_pool(ZOO_SIZE_T total_size)
{
    // Logging removed

    if (smb_memory_pool == NULL)
    {
        if (!smb_memory_pool_list_mutex_ready)
        {
            if (zoo_pool_mutex_init(&smb_memory_pool_list_mutex) != 0)
            {
                return ZOO_ERROR_MEM_POOL_ALLOCATION_FAILED;
            }
            smb_memory_pool_list_mutex_ready = ZOO_TRUE;
        }

        smb_memory_pool = smb_memory_pool_create_segment(total_size);
        if (!smb_memory_pool)
        {
            return ZOO_ERROR_MEM_POOL_ALLOCATION_FAILED;
        }
        smb_memory_pool_tail = smb_memory_pool;
    }

    return ZOO_OK;
}

/**
 * @brief Allocates zeroed memory from the segmented pool.
 *
 * The allocator may satisfy the request from an existing segment or grow the
 * pool by appending a new segment when existing segments cannot satisfy it.
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
    zoo_pool_mutex_lock(&smb_memory_pool_list_mutex);
    ptr = smb_memory_pool_alloc_locked(smb_memory_pool, size);
    if (ptr)
    {
        memset(ptr, 0, size);
    }
    return ptr;
}

/**
 * @brief Destroys all segments and the shared list mutex.
 */
void zoo_destroy_memory_pool(void)
{
    // Logging removed
    if (smb_memory_pool)
    {
        ZOO_MEMORY_POOL_STRUCT *segment = smb_memory_pool;

        smb_memory_pool = NULL;
        smb_memory_pool_tail = NULL;
        while (segment)
        {
            ZOO_MEMORY_POOL_STRUCT *next = segment->next;
            zoo_pool_mutex_destroy(&segment->mutex);
            free(segment);
            segment = next;
        }

        if (smb_memory_pool_list_mutex_ready)
        {
            zoo_pool_mutex_destroy(&smb_memory_pool_list_mutex);
            smb_memory_pool_list_mutex_ready = ZOO_FALSE;
        }

        smb_memory_pool = NULL;
    }
}

/**
 * @brief Returns a previously allocated block to the segmented pool.
 */
void zoo_free_to_pool(void *p)
{
    if (p && smb_memory_pool)
    {
        zoo_pool_mutex_lock(&smb_memory_pool_list_mutex);
        smb_memory_pool_free_locked(smb_memory_pool, p);
    }
}

/**
 * @brief Carves a contiguous run of pages from one segment's free-page list.
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
 * @brief Returns a contiguous page run to one segment's free-page list.
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
 * @brief Aggregates usage statistics across all pool segments.
 */
void zoo_memory_pool_get_usage(ZOO_MEMORY_USAGE_T *stat)
{
    if (!stat || !smb_memory_pool)
    {
        return;
    }

    memset(stat, 0, sizeof(ZOO_MEMORY_USAGE_T));
    zoo_pool_mutex_lock(&smb_memory_pool_list_mutex);

    for (ZOO_MEMORY_POOL_STRUCT *segment = smb_memory_pool; segment != NULL; segment = segment->next)
    {
        ZOO_SLAB_PAGE_STRUCT *page = segment->free.next;

        zoo_pool_mutex_lock(&segment->mutex);

        stat->pool_size += (ZOO_SIZE_T)(segment->end - segment->start);
        stat->pages += segment->real_pages;

        while (page != &segment->free)
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

        zoo_pool_mutex_unlock(&segment->mutex);
    }

    stat->used_size = stat->pool_size - stat->free_size;

    if (stat->pool_size > 0)
    {
        stat->used_pct = (stat->used_size * 100) / stat->pool_size;
    }

    zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
}

/**
 * @brief Validates free-list linkage and free-page accounting for every segment.
 */
ZOO_INT32 zoo_validate_memory_pool(void)
{
    if (!smb_memory_pool)
    {
        return -1;
    }

    zoo_pool_mutex_lock(&smb_memory_pool_list_mutex);

    for (ZOO_MEMORY_POOL_STRUCT *segment = smb_memory_pool; segment != NULL; segment = segment->next)
    {
        ZOO_SIZE_T free_accounted = 0;
        ZOO_SLAB_PAGE_STRUCT *page = segment->free.next;
        ZOO_SLAB_PAGE_STRUCT *prev = &segment->free;

        zoo_pool_mutex_lock(&segment->mutex);

        while (page != &segment->free)
        {
            if (page->next == NULL)
            {
                zoo_pool_mutex_unlock(&segment->mutex);
                zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
                return -1;
            }

            ZOO_SLAB_PAGE_STRUCT *prev_from_node = (ZOO_SLAB_PAGE_STRUCT *)(page->prev & ~SMB_SLAB_PAGE_MASK);
            if (prev_from_node != prev)
            {
                zoo_pool_mutex_unlock(&segment->mutex);
                zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
                return -1;
            }

            free_accounted += page->slab;

            prev = page;
            page = page->next;

            if (page == segment->free.next)
            {
                zoo_pool_mutex_unlock(&segment->mutex);
                zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
                return -1;
            }
        }

        if (free_accounted > segment->real_pages)
        {
            zoo_pool_mutex_unlock(&segment->mutex);
            zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
            return -1;
        }

        zoo_pool_mutex_unlock(&segment->mutex);
    }

    zoo_pool_mutex_unlock(&smb_memory_pool_list_mutex);
    return 0;
}
