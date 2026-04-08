/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: ZOO Platform
 * Component ID: ZOO_TYPES
 * File name: zoo_types.h
 * Description: Common type definitions for ZOO platform abstraction layer
 *              Provides standard data types, platform-specific types, and
 *              utility macros used across all ZOO components
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     system          Extracted from zoo.h for modularity
 ******************************************************************************/

#ifndef ZOO_TYPES_H
#define ZOO_TYPES_H

// Feature test macros for POSIX/GNU extensions (must be before any includes)
#if defined(__linux__) || defined(__unix__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include "zoo_platform.h"

    // ==============================================================================
    // BASIC TYPE DEFINITIONS
    // ==============================================================================

    /**
     * @brief Platform-specific integer types
     * @details Standardized integer types for cross-platform compatibility
     */
    typedef int8_t ZOO_INT8_T;
    typedef uint8_t ZOO_UINT8_T;
    typedef int16_t ZOO_INT16_T;
    typedef uint16_t ZOO_UINT16_T;
    typedef int32_t ZOO_INT32_T;
    typedef uint32_t ZOO_UINT32_T;
    typedef int64_t ZOO_INT64_T;
    typedef uint64_t ZOO_UINT64_T;
    typedef float ZOO_FLOAT_T;
    typedef double ZOO_DOUBLE_T;

    /**
     * @brief Platform-specific boolean and pointer types
     */
    typedef bool ZOO_BOOL_T;

    // Short type aliases for convenience - following ZOO naming convention with _T suffix
    typedef int8_t ZOO_S8_T;
    typedef uint8_t ZOO_U8_T;
    typedef int16_t ZOO_S16_T;
    typedef uint16_t ZOO_U16_T;
    typedef int32_t ZOO_S32_T;
    typedef uint32_t ZOO_U32_T;
    typedef int64_t ZOO_S64_T;
    typedef uint64_t ZOO_U64_T;
    typedef size_t ZOO_USIZE_T;
    typedef ssize_t ZOO_SSIZE_T;
    typedef void *ZOO_PVOID_T;
    typedef const void *ZOO_CPVOID_T;
    typedef char *ZOO_PCHAR_T;
    typedef const char *ZOO_CPCHAR_T;

    // Legacy type aliases (for backward compatibility - without _T suffix)
    typedef int8_t ZOO_INT8;
    typedef uint8_t ZOO_UINT8;
    typedef int16_t ZOO_INT16;
    typedef uint16_t ZOO_UINT16;
    typedef int32_t ZOO_INT32;
    typedef uint32_t ZOO_UINT32;
    typedef int64_t ZOO_INT64;
    typedef uint64_t ZOO_UINT64;
    typedef float ZOO_FLOAT;
    typedef double ZOO_DOUBLE;
    typedef bool ZOO_BOOL;
    typedef int8_t ZOO_S8;
    typedef uint8_t ZOO_U8;
    typedef int16_t ZOO_S16;
    typedef uint16_t ZOO_U16;
    typedef int32_t ZOO_S32;
    typedef uint32_t ZOO_U32;
    typedef int64_t ZOO_S64;
    typedef uint64_t ZOO_U64;
    typedef size_t ZOO_USIZE;
    typedef ssize_t ZOO_SSIZE;
    typedef void *ZOO_PVOID;
    typedef const void *ZOO_CPVOID;
    typedef char *ZOO_PCHAR;
    typedef const char *ZOO_CPCHAR;

#ifdef __cplusplus
#define ZOO_NULL nullptr
#else
#define ZOO_NULL NULL
#endif

    /**
     * @brief Platform-specific string and error types
     */
    typedef const char *ZOO_STRING_T;
    typedef const char *ZOO_STRING; // Legacy compatibility
    // ZOO_ERROR_T is defined in zoo_error.h

    /**
     * @brief Platform-specific size and pointer types
     * @details Size types depend on platform architecture (32-bit vs 64-bit)
     */
#ifdef ZOO_PLATFORM_64BIT
    typedef uint64_t ZOO_SIZE_T;
    typedef int64_t ZOO_SSIZE_T;
    typedef uint64_t ZOO_UINTPTR_T;
    typedef int64_t ZOO_INTPTR_T;
    // Legacy compatibility
    typedef uint64_t ZOO_SIZE;
    typedef int64_t ZOO_SSIZE;
    typedef uint64_t ZOO_UINTPTR;
    typedef int64_t ZOO_INTPTR;
#else
    typedef uint32_t ZOO_SIZE_T;
    typedef int32_t ZOO_SSIZE_T;
    typedef uint32_t ZOO_UINTPTR_T;
    typedef int32_t ZOO_INTPTR_T;
    // Legacy compatibility
    typedef uint32_t ZOO_SIZE;
    typedef int32_t ZOO_SSIZE;
    typedef uint32_t ZOO_UINTPTR;
    typedef int32_t ZOO_INTPTR;
#endif

// ==============================================================================
// PLATFORM-SPECIFIC TIME TYPES
// ==============================================================================

/**
 * @brief Platform-specific time type
 * @details Time representation varies by operating system
 */
#if defined(ZOO_OS_WINDOWS) || defined(ZOO_OS_LINUX)
    #include <time.h>
    typedef time_t ZOO_TIME_T;
    typedef time_t ZOO_TIME; // Legacy compatibility
    #define ZOO_TIME_GET() time(NULL)
#elif defined(ZOO_OS_FREERTOS)
    #include "FreeRTOS.h"
    #include "task.h"
    typedef TickType_t ZOO_TIME_T;
    typedef TickType_t ZOO_TIME; // Legacy compatibility
    #define ZOO_TIME_GET() xTaskGetTickCount()
#elif defined(ZOO_OS_CMSIS_RTOS)
    #include "cmsis_os.h"
    typedef uint32_t ZOO_TIME_T;
    typedef uint32_t ZOO_TIME; // Legacy compatibility
    #define ZOO_TIME_GET() osKernelSysTick()
#elif defined(ZOO_OS_RTTHREAD)
    #include "rtthread.h"
    typedef rt_tick_t ZOO_TIME_T;
    typedef rt_tick_t ZOO_TIME; // Legacy compatibility
    #define ZOO_TIME_GET() rt_tick_get()
#else
    typedef ZOO_UINT32_T ZOO_TIME_T;
    typedef ZOO_UINT32 ZOO_TIME; // Legacy compatibility
#define ZOO_TIME_GET() 0
#endif

// ==============================================================================
// PLATFORM-SPECIFIC THREAD TYPES
// ==============================================================================

/**
 * @brief Platform-specific thread types
 * @details Thread handle and ID abstractions for different platforms
 */
#ifdef ZOO_OS_WINDOWS
    #include <windows.h>
    typedef HANDLE ZOO_THREAD;
    typedef DWORD ZOO_THREAD_ID;
#elif defined(ZOO_HAS_POSIX)
    #include <pthread.h>
    typedef pthread_t ZOO_THREAD;
    typedef pthread_t ZOO_THREAD_ID;
#elif defined(ZOO_OS_FREERTOS)
    #include "FreeRTOS.h"
    #include "task.h"
    typedef TaskHandle_t ZOO_THREAD;
    typedef TaskHandle_t ZOO_THREAD_ID;
#elif defined(ZOO_OS_CMSIS_RTOS)
    #include "cmsis_os.h"
    typedef osThreadId ZOO_THREAD;
    typedef osThreadId ZOO_THREAD_ID;
#elif defined(ZOO_OS_RTTHREAD)
    #include "rtthread.h"
    typedef rt_thread_t ZOO_THREAD;
    typedef rt_thread_t ZOO_THREAD_ID;
#else
    typedef void *ZOO_THREAD;
    typedef void *ZOO_THREAD_ID;
#endif

// ==============================================================================
// PLATFORM-SPECIFIC ATOMIC TYPES
// ==============================================================================

/**
 * @brief Platform-specific atomic types
 * @details Atomic operations support for thread-safe programming
 */
#ifdef ZOO_OS_WINDOWS
    #include <intrin.h>
    typedef volatile LONG ZOO_ATOMIC_BOOL_T;
    typedef volatile LONG ZOO_ATOMIC_SIZE_T;
    typedef volatile LONG ZOO_ATOMIC_INT_T;
    // Legacy compatibility
    typedef volatile LONG ZOO_ATOMIC_BOOL;
    typedef volatile LONG ZOO_ATOMIC_SIZE;
    typedef volatile LONG ZOO_ATOMIC_INT;

// Atomic operation macros for Windows
#define ZOO_ATOMIC_LOAD(ptr) InterlockedOr((ptr), 0)
#define ZOO_ATOMIC_STORE(ptr, val) InterlockedExchange((ptr), (val))
#define ZOO_ATOMIC_INIT(ptr, val) (*(ptr) = (val))
#define ZOO_ATOMIC_FETCH_ADD(ptr, val) InterlockedExchangeAdd((ptr), (val))
#define ZOO_ATOMIC_FETCH_SUB(ptr, val) InterlockedExchangeAdd((ptr), -(val))
#define ZOO_ATOMIC_EXCHANGE(ptr, val) InterlockedExchange((ptr), (val))
#define ZOO_ATOMIC_COMPARE_EXCHANGE(ptr, expected, desired) \
    (InterlockedCompareExchange((ptr), (desired), *(expected)) == *(expected) ? ZOO_TRUE : (*(expected) = InterlockedOr((ptr), 0), ZOO_FALSE))

#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    #include <stdatomic.h>
    typedef atomic_bool ZOO_ATOMIC_BOOL_T;
    typedef atomic_size_t ZOO_ATOMIC_SIZE_T;
    typedef atomic_int ZOO_ATOMIC_INT_T;
    // Legacy compatibility
    typedef atomic_bool ZOO_ATOMIC_BOOL;
    typedef atomic_size_t ZOO_ATOMIC_SIZE;
    typedef atomic_int ZOO_ATOMIC_INT;

// Atomic operation macros for C11
#define ZOO_ATOMIC_LOAD(ptr) atomic_load(ptr)
#define ZOO_ATOMIC_STORE(ptr, val) atomic_store((ptr), (val))
#define ZOO_ATOMIC_INIT(ptr, val) atomic_init((ptr), (val))
#define ZOO_ATOMIC_FETCH_ADD(ptr, val) atomic_fetch_add((ptr), (val))
#define ZOO_ATOMIC_FETCH_SUB(ptr, val) atomic_fetch_sub((ptr), (val))
#define ZOO_ATOMIC_EXCHANGE(ptr, val) atomic_exchange((ptr), (val))
#define ZOO_ATOMIC_COMPARE_EXCHANGE(ptr, expected, desired) \
    atomic_compare_exchange_weak((ptr), (expected), (desired))

#else
    // Fallback for platforms without atomic support
    typedef volatile ZOO_BOOL_T ZOO_ATOMIC_BOOL_T;
    typedef volatile ZOO_SIZE_T ZOO_ATOMIC_SIZE_T;
    typedef volatile int ZOO_ATOMIC_INT_T;
    // Legacy compatibility
    typedef volatile ZOO_BOOL ZOO_ATOMIC_BOOL;
    typedef volatile ZOO_SIZE ZOO_ATOMIC_SIZE;
    typedef volatile int ZOO_ATOMIC_INT;

// Fallback atomic operation macros
#define ZOO_ATOMIC_LOAD(ptr) (*(ptr))
#define ZOO_ATOMIC_STORE(ptr, val) (*(ptr) = (val))
#define ZOO_ATOMIC_INIT(ptr, val) (*(ptr) = (val))
#define ZOO_ATOMIC_FETCH_ADD(ptr, val) (++(*(ptr)))
#define ZOO_ATOMIC_FETCH_SUB(ptr, val) (--(*(ptr)))
#define ZOO_ATOMIC_EXCHANGE(ptr, val) ((*(ptr)) = (val))
#define ZOO_ATOMIC_COMPARE_EXCHANGE(ptr, expected, desired) \
    ((*(ptr) == *(expected)) ? (*(ptr) = (desired), ZOO_TRUE) : (*(expected) = *(ptr), ZOO_FALSE))
#endif

    // ==============================================================================
    // PLATFORM-SPECIFIC MAXIMUM VALUES
    // ==============================================================================

#define ZOO_MAX_INT8 INT8_MAX
#define ZOO_MAX_UINT8 UINT8_MAX
#define ZOO_MAX_INT16 INT16_MAX
#define ZOO_MAX_UINT16 UINT16_MAX
#define ZOO_MAX_INT32 INT32_MAX
#define ZOO_MAX_UINT32 UINT32_MAX
#define ZOO_MAX_INT64 INT64_MAX
#define ZOO_MAX_UINT64 UINT64_MAX

#ifdef ZOO_PLATFORM_64BIT
#define ZOO_MAX_SIZE UINT64_MAX
#define ZOO_MAX_SSIZE INT64_MAX
#define ZOO_SIZE_FMT "%" PRIu64
#define ZOO_SSIZE_FMT "%" PRId64
#define ZOO_UINTPTR_FMT "%" PRIu64
#define ZOO_INTPTR_FMT "%" PRId64
#else
#define ZOO_MAX_SIZE UINT32_MAX
#define ZOO_MAX_SSIZE INT32_MAX
#define ZOO_SIZE_FMT "%" PRIu32
#define ZOO_SSIZE_FMT "%" PRId32
#define ZOO_UINTPTR_FMT "%" PRIu32
#define ZOO_INTPTR_FMT "%" PRId32
#endif

    // ==============================================================================
    // PLATFORM-SPECIFIC COMPILER ATTRIBUTES
    // ==============================================================================

#ifdef __GNUC__
#define ZOO_ALIGNED(n) __attribute__((aligned(n)))
#define ZOO_PACKED __attribute__((packed))
#define ZOO_LIKELY(x) __builtin_expect(!!(x), 1)
#define ZOO_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define ZOO_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define ZOO_ALIGNED(n) __declspec(align(n))
#define ZOO_PACKED
#define ZOO_LIKELY(x) (x)
#define ZOO_UNLIKELY(x) (x)
#define ZOO_FORCE_INLINE __forceinline
#else
#define ZOO_ALIGNED(n)
#define ZOO_PACKED
#define ZOO_LIKELY(x) (x)
#define ZOO_UNLIKELY(x) (x)
#define ZOO_FORCE_INLINE inline
#endif

// ==============================================================================
// MEMORY ALIGNMENT DEFINITIONS
// ==============================================================================

/**
 * @brief Platform-specific cache line sizes
 * @details Different architectures have different cache line sizes:
 *          - ARM Cortex-M: 32 bytes
 *          - ARM Cortex-A: 64 bytes
 *          - x86/x64: 64 bytes
 */
#ifdef ZOO_PLATFORM_ARM_CORTEX_M
#define ZOO_CACHE_LINE_SIZE 32
#elif defined(ZOO_PLATFORM_ARM_CORTEX_A64) || defined(ZOO_PLATFORM_ARM_CORTEX_A32)
#define ZOO_CACHE_LINE_SIZE 64
#else // x86/x64
#define ZOO_CACHE_LINE_SIZE 64
#endif

/** @brief Align data structure to cache line boundary */
#define ZOO_CACHE_ALIGNED ZOO_ALIGNED(ZOO_CACHE_LINE_SIZE)

// ==============================================================================
// UTILITY MACROS
// ==============================================================================

/** @brief Get the number of elements in an array */
#define ZOO_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/** @brief Get the offset of a member in a structure */
#define ZOO_OFFSET_OF(type, member) ((ZOO_SIZE) & ((type *)0)->member)

/** @brief Get the container structure from a member pointer */
#define ZOO_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - ZOO_OFFSET_OF(type, member)))

/** @brief Align value up to the nearest multiple of alignment */
#define ZOO_ALIGN_UP(value, alignment) \
    (((value) + (alignment) - 1) & ~((alignment) - 1))

/** @brief Align value down to the nearest multiple of alignment */
#define ZOO_ALIGN_DOWN(value, alignment) \
    ((value) & ~((alignment) - 1))

/** @brief Check if a value is aligned to the specified boundary */
#define ZOO_IS_ALIGNED(value, alignment) \
    (((value) & ((alignment) - 1)) == 0)

/** @brief Minimum of two values */
#define ZOO_MIN(a, b) ((a) < (b) ? (a) : (b))

/** @brief Maximum of two values */
#define ZOO_MAX(a, b) ((a) > (b) ? (a) : (b))

/** @brief Clamp value between min and max */
#define ZOO_CLAMP(value, min_val, max_val) \
    ZOO_MAX((min_val), ZOO_MIN((value), (max_val)))

/** @brief Suppress unused variable warnings */
#ifndef ZOO_UNUSED
#define ZOO_UNUSED(x) (void)(x)
#endif

// ==============================================================================
// STATIC ASSERTIONS FOR COMPILE-TIME CHECKS
// ==============================================================================

/** @brief Compile-time assertion macro */
#ifdef __cplusplus
#define ZOO_STATIC_ASSERT(condition, message) static_assert(condition, message)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define ZOO_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#else
#define ZOO_STATIC_ASSERT(condition, message) \
    typedef char ZOO_STATIC_ASSERT_##__LINE__[(condition) ? 1 : -1]
#endif

    // Compile-time checks for type sizes
    ZOO_STATIC_ASSERT(sizeof(ZOO_INT8) == 1, "ZOO_INT8 must be 1 byte");
    ZOO_STATIC_ASSERT(sizeof(ZOO_INT16) == 2, "ZOO_INT16 must be 2 bytes");
    ZOO_STATIC_ASSERT(sizeof(ZOO_INT32) == 4, "ZOO_INT32 must be 4 bytes");
    ZOO_STATIC_ASSERT(sizeof(ZOO_INT64) == 8, "ZOO_INT64 must be 8 bytes");

// Verify pointer size consistency
#ifdef ZOO_PLATFORM_64BIT
    ZOO_STATIC_ASSERT(sizeof(ZOO_UINTPTR) == 8, "ZOO_UINTPTR must be 8 bytes on 64-bit platforms");
    ZOO_STATIC_ASSERT(sizeof(ZOO_SIZE) == 8, "ZOO_SIZE must be 8 bytes on 64-bit platforms");
#else
    ZOO_STATIC_ASSERT(sizeof(ZOO_UINTPTR) == 4, "ZOO_UINTPTR must be 4 bytes on 32-bit platforms");
    ZOO_STATIC_ASSERT(sizeof(ZOO_SIZE) == 4, "ZOO_SIZE must be 4 bytes on 32-bit platforms");
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZOO_TYPES_H */
