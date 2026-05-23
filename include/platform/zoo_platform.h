/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: ZOO Platform
 * Component ID: ZOO_PLATFORM
 * File name: zoo_platform.h
 * Description: Platform-specific definitions, feature detection, and abstractions
 *              for different architectures and operating systems
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-05-14     weiwang.sun     Initial creation
 * 1.1       2025-08-01     assistant       Split from zoo.h
 ******************************************************************************/
#ifndef ZOO_PLATFORM_H
#define ZOO_PLATFORM_H

// Feature test macros must be defined before any system headers
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
#include <stdio.h>
#include <time.h>

#if defined(__linux__) || defined(__unix__)
#include <sys/select.h>
#include <sys/time.h>
#endif

#define ZOO_PLATFORM_VERSION "1.2.0"

// ==============================================================================
// PLATFORM DETECTION
// ==============================================================================

// Check if CMake provided platform definitions first
#ifdef __ZOO_PLATFORM_X86_64
#define ZOO_PLATFORM_X86_64
#define ZOO_PLATFORM_NAME "X86_64"
#define ZOO_PLATFORM_64BIT

#elif defined(__ZOO_PLATFORM_X86_32)
#define ZOO_PLATFORM_X86_32
#define ZOO_PLATFORM_NAME "X86_32"
#define ZOO_PLATFORM_32BIT

#elif defined(__ZOO_PLATFORM_ARM_CORTEX_A64)
#define ZOO_PLATFORM_ARM_CORTEX_A64
#define ZOO_PLATFORM_NAME "ARM_CORTEX_A64"
#define ZOO_PLATFORM_64BIT

#elif defined(__ZOO_PLATFORM_ARM_CORTEX_A32)
#define ZOO_PLATFORM_ARM_CORTEX_A32
#define ZOO_PLATFORM_NAME "ARM_CORTEX_A32"
#define ZOO_PLATFORM_32BIT

#elif defined(__ZOO_PLATFORM_ARM_CORTEX_M)
#define ZOO_PLATFORM_ARM_CORTEX_M
#define ZOO_PLATFORM_NAME "ARM_CORTEX_M"
#define ZOO_PLATFORM_32BIT

// Fallback to compiler-based detection if CMake didn't provide definitions
#elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M__)
#define ZOO_PLATFORM_ARM_CORTEX_M
#define ZOO_PLATFORM_NAME "ARM_CORTEX_M"
#define ZOO_PLATFORM_32BIT

// ARM Cortex-A series (ARM A55, A72, A78, etc.)
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ZOO_PLATFORM_ARM_CORTEX_A64
#define ZOO_PLATFORM_NAME "ARM_CORTEX_A64"
#define ZOO_PLATFORM_64BIT

#elif defined(__arm__) || defined(_M_ARM)
#define ZOO_PLATFORM_ARM_CORTEX_A32
#define ZOO_PLATFORM_NAME "ARM_CORTEX_A32"
#define ZOO_PLATFORM_32BIT

// x86/x64 platforms
#elif defined(__x86_64__) || defined(_M_X64)
#define ZOO_PLATFORM_X86_64
#define ZOO_PLATFORM_NAME "X86_64"
#define ZOO_PLATFORM_64BIT

#elif defined(__i386__) || defined(_M_IX86)
#define ZOO_PLATFORM_X86_32
#define ZOO_PLATFORM_NAME "X86_32"
#define ZOO_PLATFORM_32BIT

#else
#define ZOO_PLATFORM_UNKNOWN
#define ZOO_PLATFORM_NAME "UNKNOWN"
#warning "Unknown platform detected"
#endif

// ==============================================================================
// OPERATING SYSTEM DETECTION
// ==============================================================================

// Check if CMake provided OS definitions first
#ifdef ZOO_OS_LINUX
#ifndef ZOO_OS_NAME
#define ZOO_OS_NAME "Linux"
#endif

#elif defined(ZOO_OS_UNKNOWN)
#ifndef ZOO_OS_NAME
#define ZOO_OS_NAME "Unknown"
#endif

// Fallback to compiler-based detection if CMake didn't provide definitions
#elif defined(__linux__)
#ifndef ZOO_OS_LINUX
#define ZOO_OS_LINUX
#define ZOO_OS_NAME "Linux"
#endif

// Embedded/Bare Metal detection (typically ARM Cortex-M)
#elif defined(ZOO_PLATFORM_ARM_CORTEX_M) && !defined(__linux__)
#define ZOO_OS_BAREMETAL
#define ZOO_OS_NAME "BareMetal"

// RTOS detection (FreeRTOS, CMSIS-RTOS, RT-Thread, etc.)
#if defined(configUSE_FREERTOS) || defined(FREERTOS)
#define ZOO_OS_FREERTOS
#undef ZOO_OS_NAME
#define ZOO_OS_NAME "FreeRTOS"
#elif defined(osKernelSystemId) || defined(CMSIS_OS_H_)
#define ZOO_OS_CMSIS_RTOS
#undef ZOO_OS_NAME
#define ZOO_OS_NAME "CMSIS-RTOS"
#elif defined(RT_THREAD_VERSION) || defined(__RTTHREAD__)
#define ZOO_OS_RTTHREAD
#undef ZOO_OS_NAME
#define ZOO_OS_NAME "RT-Thread"
#endif

#else
#define ZOO_OS_UNKNOWN
#define ZOO_OS_NAME "Unknown"
#endif

// ==============================================================================
// FEATURE DETECTION MACROS
// ==============================================================================

/** @brief Check if platform supports POSIX features */
#if defined(ZOO_OS_LINUX) || (defined(__unix__) && !defined(ZOO_PLATFORM_ARM_CORTEX_M))
#define ZOO_HAS_POSIX 1
#else
#define ZOO_HAS_POSIX 0
#endif

/** @brief Check if platform supports epoll (Linux-specific) */
#if defined(ZOO_OS_LINUX) && ZOO_HAS_POSIX
#define ZOO_USE_EPOLL 1
#else
#define ZOO_USE_EPOLL 0
#endif

/** @brief Check if platform supports threading */
#if defined(ZOO_OS_LINUX) || defined(ZOO_OS_FREERTOS) || defined(ZOO_OS_CMSIS_RTOS) || defined(ZOO_OS_RTTHREAD)
#define ZOO_HAS_THREADING 1
#else
#define ZOO_HAS_THREADING 0
#endif

/** @brief Check if platform supports file system operations */
#if defined(ZOO_OS_LINUX)
#define ZOO_HAS_FILESYSTEM 1
#else
#define ZOO_HAS_FILESYSTEM 0
#endif

/** @brief Check if platform supports syslog */
#if defined(ZOO_OS_LINUX)
#define ZOO_HAS_SYSLOG 1
#else
#define ZOO_HAS_SYSLOG 0
#endif

/** @brief Check if running in embedded environment */
#if defined(ZOO_PLATFORM_ARM_CORTEX_M) && (defined(ZOO_OS_BAREMETAL) || defined(ZOO_OS_FREERTOS) || defined(ZOO_OS_CMSIS_RTOS) || defined(ZOO_OS_RTTHREAD))
#define ZOO_IS_EMBEDDED 1
#else
#define ZOO_IS_EMBEDDED 0
#endif

#if ZOO_HAS_POSIX
static inline int zoo_platform_get_realtime_timespec(struct timespec *ts)
{
    if (!ts)
    {
        return -1;
    }

#if defined(CLOCK_REALTIME)
    return clock_gettime(CLOCK_REALTIME, ts);
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        return -1;
    }

    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = (long)tv.tv_usec * 1000L;
    return 0;
#endif
}

static inline int zoo_platform_get_monotonic_timespec(struct timespec *ts)
{
    if (!ts)
    {
        return -1;
    }

#if defined(CLOCK_MONOTONIC)
    return clock_gettime(CLOCK_MONOTONIC, ts);
#else
    return zoo_platform_get_realtime_timespec(ts);
#endif
}

static inline void zoo_platform_sleep_us_impl(unsigned int delay_us)
{
    struct timeval timeout;

    timeout.tv_sec = (time_t)(delay_us / 1000000U);
    timeout.tv_usec = (long)(delay_us % 1000000U);
    (void)select(0, NULL, NULL, NULL, &timeout);
}
#endif

// ==============================================================================
// CROSS-PLATFORM THREADING AND ATOMIC OPERATIONS
// ==============================================================================

/**
 * @brief Platform-specific threading types
 * @details Cross-platform thread, mutex, and condition variable abstractions
 */
#if defined(ZOO_OS_LINUX) || ZOO_HAS_POSIX
#include <pthread.h>
#include <unistd.h>
    typedef pthread_t ZOO_THREAD_T;
    typedef pthread_mutex_t ZOO_MUTEX_T;
    typedef pthread_cond_t ZOO_COND_T;

// Thread functions
#define ZOO_THREAD_CREATE(thread, func, arg) \
    (pthread_create((thread), NULL, (func), (arg)) == 0)
#define ZOO_THREAD_JOIN(thread) (pthread_join((thread), NULL) == 0)
#define ZOO_THREAD_DETACH(thread) (pthread_detach(thread) == 0)

// Mutex functions
#define ZOO_MUTEX_INIT(mutex) (pthread_mutex_init((mutex), NULL) == 0)
#define ZOO_MUTEX_DESTROY(mutex) (pthread_mutex_destroy(mutex) == 0)
#define ZOO_MUTEX_LOCK(mutex) (pthread_mutex_lock(mutex) == 0)
#define ZOO_MUTEX_UNLOCK(mutex) (pthread_mutex_unlock(mutex) == 0)
#define ZOO_MUTEX_TRYLOCK(mutex) (pthread_mutex_trylock(mutex) == 0)

// Condition variable functions
#define ZOO_COND_INIT(cond) (pthread_cond_init((cond), NULL) == 0)
#define ZOO_COND_DESTROY(cond) (pthread_cond_destroy(cond) == 0)
#define ZOO_COND_WAIT(cond, mutex) (pthread_cond_wait((cond), (mutex)) == 0)
#define ZOO_COND_SIGNAL(cond) (pthread_cond_signal(cond) == 0)
#define ZOO_COND_BROADCAST(cond) (pthread_cond_broadcast(cond) == 0)
#define ZOO_COND_WAIT_TIMEOUT(cond, mutex, timeout_ms) \
    ({ struct timespec ts; \
           int zoo_wait_ok = 0; \
           if (zoo_platform_get_realtime_timespec(&ts) == 0) { \
               ts.tv_sec += (timeout_ms) / 1000; \
               ts.tv_nsec += ((timeout_ms) % 1000) * 1000000; \
               if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; } \
               zoo_wait_ok = (pthread_cond_timedwait((cond), (mutex), &ts) == 0); \
           } \
           zoo_wait_ok; })

#elif defined(ZOO_OS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
typedef TaskHandle_t ZOO_THREAD_T;
typedef SemaphoreHandle_t ZOO_MUTEX_T;
typedef SemaphoreHandle_t ZOO_COND_T;

// Thread functions
#define ZOO_THREAD_CREATE(thread, func, arg) \
    (xTaskCreate((func), "ZOO_Task", configMINIMAL_STACK_SIZE, (arg), tskIDLE_PRIORITY + 1, (thread)) == pdPASS)
#define ZOO_THREAD_JOIN(thread) ((void)0) // FreeRTOS doesn't support join
#define ZOO_THREAD_DETACH(thread) ((void)0)

// Mutex functions
#define ZOO_MUTEX_INIT(mutex) ((*(mutex) = xSemaphoreCreateMutex()) != NULL)
#define ZOO_MUTEX_DESTROY(mutex) (vSemaphoreDelete(*(mutex)), ZOO_TRUE)
#define ZOO_MUTEX_LOCK(mutex) (xSemaphoreTake(*(mutex), portMAX_DELAY) == pdTRUE)
#define ZOO_MUTEX_UNLOCK(mutex) (xSemaphoreGive(*(mutex)) == pdTRUE)
#define ZOO_MUTEX_TRYLOCK(mutex) (xSemaphoreTake(*(mutex), 0) == pdTRUE)

// Condition variable emulation with binary semaphore
#define ZOO_COND_INIT(cond) ((*(cond) = xSemaphoreCreateBinary()) != NULL)
#define ZOO_COND_DESTROY(cond) (vSemaphoreDelete(*(cond)), ZOO_TRUE)
#define ZOO_COND_WAIT(cond, mutex) \
    (ZOO_MUTEX_UNLOCK(mutex) && (xSemaphoreTake(*(cond), portMAX_DELAY) == pdTRUE) && ZOO_MUTEX_LOCK(mutex))
#define ZOO_COND_SIGNAL(cond) (xSemaphoreGive(*(cond)) == pdTRUE)
#define ZOO_COND_BROADCAST(cond) ZOO_COND_SIGNAL(cond) // FreeRTOS binary semaphore limitation

#elif defined(ZOO_OS_CMSIS_RTOS)
#include "cmsis_os.h"
typedef osThreadId ZOO_THREAD_T;
typedef osMutexId ZOO_MUTEX_T;
typedef osSemaphoreId ZOO_COND_T;

// Thread functions
#define ZOO_THREAD_CREATE(thread, func, arg) \
    ((*(thread) = osThreadCreate(osThread(ZOO_Task), (arg))) != NULL)
#define ZOO_THREAD_JOIN(thread) (osThreadTerminate(*(thread)) == osOK)
#define ZOO_THREAD_DETACH(thread) ((void)0)

// Mutex functions
#define ZOO_MUTEX_INIT(mutex) ((*(mutex) = osMutexCreate(osMutex(ZOO_Mutex))) != NULL)
#define ZOO_MUTEX_DESTROY(mutex) (osMutexDelete(*(mutex)) == osOK)
#define ZOO_MUTEX_LOCK(mutex) (osMutexWait(*(mutex), osWaitForever) == osOK)
#define ZOO_MUTEX_UNLOCK(mutex) (osMutexRelease(*(mutex)) == osOK)
#define ZOO_MUTEX_TRYLOCK(mutex) (osMutexWait(*(mutex), 0) == osOK)

// Condition variable emulation with semaphore
#define ZOO_COND_INIT(cond) ((*(cond) = osSemaphoreCreate(osSemaphore(ZOO_Cond), 0)) != NULL)
#define ZOO_COND_DESTROY(cond) (osSemaphoreDelete(*(cond)) == osOK)
#define ZOO_COND_WAIT(cond, mutex) \
    (ZOO_MUTEX_UNLOCK(mutex) && (osSemaphoreWait(*(cond), osWaitForever) > 0) && ZOO_MUTEX_LOCK(mutex))
#define ZOO_COND_SIGNAL(cond) (osSemaphoreRelease(*(cond)) == osOK)
#define ZOO_COND_BROADCAST(cond) ZOO_COND_SIGNAL(cond) // CMSIS-RTOS limitation

#elif defined(ZOO_OS_RTTHREAD)
#include "rtthread.h"
typedef rt_thread_t ZOO_THREAD_T;
typedef rt_mutex_t ZOO_MUTEX_T;
typedef rt_sem_t ZOO_COND_T;

// RT-Thread default configurations
#ifndef ZOO_RTTHREAD_STACK_SIZE
#define ZOO_RTTHREAD_STACK_SIZE 2048
#endif
#ifndef ZOO_RTTHREAD_PRIORITY
#define ZOO_RTTHREAD_PRIORITY (RT_THREAD_PRIORITY_MAX / 2)
#endif
#ifndef ZOO_RTTHREAD_TICK
#define ZOO_RTTHREAD_TICK 20
#endif

// Thread functions
#define ZOO_THREAD_CREATE(thread, func, arg)                                                                         \
    ((*(thread) = rt_thread_create("zoo_thread", (void (*)(void *))(func), (arg),                                    \
                                   ZOO_RTTHREAD_STACK_SIZE, ZOO_RTTHREAD_PRIORITY, ZOO_RTTHREAD_TICK)) != RT_NULL && \
     rt_thread_startup(*(thread)) == RT_EOK)
#define ZOO_THREAD_JOIN(thread) (rt_thread_delete(*(thread)) == RT_EOK)
#define ZOO_THREAD_DETACH(thread) ((void)0)

// Mutex functions
#define ZOO_MUTEX_INIT(mutex) ((*(mutex) = rt_mutex_create("zoo_mutex", RT_IPC_FLAG_PRIO)) != RT_NULL)
#define ZOO_MUTEX_DESTROY(mutex) (rt_mutex_delete(*(mutex)) == RT_EOK)
#define ZOO_MUTEX_LOCK(mutex) (rt_mutex_take(*(mutex), RT_WAITING_FOREVER) == RT_EOK)
#define ZOO_MUTEX_UNLOCK(mutex) (rt_mutex_release(*(mutex)) == RT_EOK)
#define ZOO_MUTEX_TRYLOCK(mutex) (rt_mutex_take(*(mutex), 0) == RT_EOK)

// Condition variable emulation with semaphore
#define ZOO_COND_INIT(cond) ((*(cond) = rt_sem_create("zoo_cond", 0, RT_IPC_FLAG_PRIO)) != RT_NULL)
#define ZOO_COND_DESTROY(cond) (rt_sem_delete(*(cond)) == RT_EOK)
#define ZOO_COND_WAIT(cond, mutex) \
    (ZOO_MUTEX_UNLOCK(mutex) && (rt_sem_take(*(cond), RT_WAITING_FOREVER) == RT_EOK) && ZOO_MUTEX_LOCK(mutex))
#define ZOO_COND_SIGNAL(cond) (rt_sem_release(*(cond)) == RT_EOK)
#define ZOO_COND_BROADCAST(cond) ZOO_COND_SIGNAL(cond) // RT-Thread semaphore limitation

#else
// Bare metal or unsupported platform - provide stubs
typedef int ZOO_THREAD_T;
typedef int ZOO_MUTEX_T;
typedef int ZOO_COND_T;

#define ZOO_THREAD_CREATE(thread, func, arg) ZOO_FALSE
#define ZOO_THREAD_JOIN(thread) ZOO_FALSE
#define ZOO_THREAD_DETACH(thread) ZOO_FALSE

#define ZOO_MUTEX_INIT(mutex) ZOO_TRUE
#define ZOO_MUTEX_DESTROY(mutex) ZOO_TRUE
#define ZOO_MUTEX_LOCK(mutex) ZOO_TRUE
#define ZOO_MUTEX_UNLOCK(mutex) ZOO_TRUE
#define ZOO_MUTEX_TRYLOCK(mutex) ZOO_TRUE

#define ZOO_COND_INIT(cond) ZOO_TRUE
#define ZOO_COND_DESTROY(cond) ZOO_TRUE
#define ZOO_COND_WAIT(cond, mutex) ZOO_TRUE
#define ZOO_COND_SIGNAL(cond) ZOO_TRUE
#define ZOO_COND_BROADCAST(cond) ZOO_TRUE
#endif

/**
 * @brief Platform-specific sleep and timing functions
 * @details Provides cross-platform sleep and high-resolution timing
 */
#if defined(ZOO_OS_LINUX) || ZOO_HAS_POSIX
#include <unistd.h>
#include <time.h>
#define ZOO_SLEEP_MS(ms) zoo_platform_sleep_us_impl((unsigned int)((ms) * 1000U))
#define ZOO_SLEEP_US(us) zoo_platform_sleep_us_impl((unsigned int)(us))
static inline uint64_t zoo_clock_realtime_ns_impl(void)
{
    struct timespec ts;
    if (zoo_platform_get_realtime_timespec(&ts) != 0)
    {
        return 0ULL;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static inline uint64_t zoo_clock_monotonic_ns_impl(void)
{
    struct timespec ts;
    if (zoo_platform_get_monotonic_timespec(&ts) != 0)
    {
        return 0ULL;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define ZOO_CLOCK_REALTIME_NS() zoo_clock_realtime_ns_impl()
#define ZOO_CLOCK_MONOTONIC_NS() zoo_clock_monotonic_ns_impl()
#elif defined(ZOO_OS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#define ZOO_SLEEP_MS(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define ZOO_SLEEP_US(us) vTaskDelay(pdMS_TO_TICKS((us) / 1000))
#define ZOO_CLOCK_REALTIME_NS() ((uint64_t)xTaskGetTickCount() * 1000000ULL)
#define ZOO_CLOCK_MONOTONIC_NS() ((uint64_t)xTaskGetTickCount() * 1000000ULL)
#elif defined(ZOO_OS_RTTHREAD)
#include "rtthread.h"
#define ZOO_SLEEP_MS(ms) rt_thread_mdelay(ms)
#define ZOO_SLEEP_US(us) rt_thread_mdelay((us) / 1000)
#define ZOO_CLOCK_REALTIME_NS() ((uint64_t)rt_tick_get() * 1000000ULL)
#define ZOO_CLOCK_MONOTONIC_NS() ((uint64_t)rt_tick_get() * 1000000ULL)
#else
// Fallback for platforms without sleep support
#define ZOO_SLEEP_MS(ms) ((void)0)
#define ZOO_SLEEP_US(us) ((void)0)
#define ZOO_CLOCK_REALTIME_NS() 0ULL
#define ZOO_CLOCK_MONOTONIC_NS() 0ULL
#endif

// ==============================================================================
// PLATFORM FEATURE DETECTION MACROS
// ==============================================================================

/** @brief Get platform information string */
#define ZOO_GET_PLATFORM_INFO() ZOO_PLATFORM_NAME
#define ZOO_GET_PLATFORM_VERSION() ZOO_PLATFORM_VERSION

/** @brief Platform feature detection */
#ifdef ZOO_PLATFORM_64BIT
#define ZOO_IS_64BIT() 1
#define ZOO_IS_32BIT() 0
#else
#define ZOO_IS_64BIT() 0
#define ZOO_IS_32BIT() 1
#endif

/** @brief Check if running on ARM architecture */
#if defined(ZOO_PLATFORM_ARM_CORTEX_M) ||   \
    defined(ZOO_PLATFORM_ARM_CORTEX_A32) || \
    defined(ZOO_PLATFORM_ARM_CORTEX_A64)
#define ZOO_IS_ARM() 1
#else
#define ZOO_IS_ARM() 0
#endif

/** @brief Check if running on x86 architecture */
#if defined(ZOO_PLATFORM_X86_32) || defined(ZOO_PLATFORM_X86_64)
#define ZOO_IS_X86() 1
#else
#define ZOO_IS_X86() 0
#endif

// ==============================================================================
// PLATFORM-SPECIFIC SYSTEM HEADERS
// ==============================================================================

// Platform-specific headers based on feature detection
#if ZOO_HAS_POSIX
// POSIX-compliant systems (Linux, Unix-like)
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#if ZOO_HAS_SYSLOG
#include <syslog.h>
#endif
#elif ZOO_IS_EMBEDDED
// Embedded systems headers
#if defined(ZOO_OS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#elif defined(ZOO_OS_CMSIS_RTOS)
#include "cmsis_os2.h"
#endif
// Note: Embedded systems typically don't have full file system support
#else
// Fallback for unknown platforms
#ifdef __has_include
#if __has_include(<sys/stat.h>)
#include <sys/stat.h>
#define HAVE_SYS_STAT
#endif
#endif
#endif

    // ==============================================================================
    // PLATFORM-SPECIFIC UTILITY FUNCTIONS
    // ==============================================================================

    /**
     * @brief Get current timestamp as formatted string
     *
     * Formats the current system time into a string suitable for log entries.
     *
     * @param buffer Buffer to store formatted timestamp
     * @param buffer_size Size of the buffer
     * @return Number of characters written to buffer
     */
    static inline int zoo_get_timestamp_string(char *buffer, size_t buffer_size)
    {
        if (!buffer || buffer_size == 0)
        {
            return 0;
        }

#if ZOO_HAS_POSIX
        struct timespec ts;
    if (zoo_platform_get_realtime_timespec(&ts) != 0)
        {
            return snprintf(buffer, buffer_size, "UNKNOWN_TIME");
        }

        struct tm *tm_info = localtime(&ts.tv_sec);
        if (!tm_info)
        {
            return snprintf(buffer, buffer_size, "INVALID_TIME");
        }

        return snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
                        tm_info->tm_year + 1900,
                        tm_info->tm_mon + 1,
                        tm_info->tm_mday,
                        tm_info->tm_hour,
                        tm_info->tm_min,
                        tm_info->tm_sec,
                        ts.tv_nsec / 1000000);
#elif defined(ZOO_OS_FREERTOS)
    // FreeRTOS tick-based timestamp
    TickType_t ticks = xTaskGetTickCount();
    uint32_t ms = ticks * portTICK_PERIOD_MS;
    uint32_t seconds = ms / 1000;
    uint32_t milliseconds = ms % 1000;
    return snprintf(buffer, buffer_size, "TICK:%lu.%03lu",
                    (unsigned long)seconds, (unsigned long)milliseconds);
#elif defined(ZOO_OS_CMSIS_RTOS)
    // CMSIS-RTOS tick-based timestamp
    uint32_t ticks = osKernelGetTickCount();
    uint32_t ms = ticks; // Assuming 1ms tick
    uint32_t seconds = ms / 1000;
    uint32_t milliseconds = ms % 1000;
    return snprintf(buffer, buffer_size, "TICK:%lu.%03lu",
                    (unsigned long)seconds, (unsigned long)milliseconds);
#else
    // Fallback to standard C time functions (no millisecond precision)
    time_t current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    if (!tm_info)
    {
        return snprintf(buffer, buffer_size, "INVALID_TIME");
    }

    return snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d.000",
                    tm_info->tm_year + 1900,
                    tm_info->tm_mon + 1,
                    tm_info->tm_mday,
                    tm_info->tm_hour,
                    tm_info->tm_min,
                    tm_info->tm_sec);
#endif
    }

    /**
     * @brief Get current thread ID as string
     *
     * Retrieves the current thread identifier and formats it as a string.
     *
     * @param buffer Buffer to store thread ID string
     * @param buffer_size Size of the buffer
     * @return Number of characters written to buffer
     */
    static inline int zoo_get_thread_id_string(char *buffer, size_t buffer_size)
    {
        if (!buffer || buffer_size == 0)
        {
            return 0;
        }

#if ZOO_HAS_POSIX
    pthread_t tid = pthread_self();
    return snprintf(buffer, buffer_size, "[%lu]", (unsigned long)tid);
#elif defined(ZOO_OS_FREERTOS)
    TaskHandle_t tid = xTaskGetCurrentTaskHandle();
    return snprintf(buffer, buffer_size, "[%s]", pcTaskGetName(tid));
#elif defined(ZOO_OS_CMSIS_RTOS)
    osThreadId_t tid = osThreadGetId();
    const char *name = osThreadGetName(tid);
    if (name)
    {
        return snprintf(buffer, buffer_size, "[%s]", name);
    }
    else
    {
        return snprintf(buffer, buffer_size, "[%p]", tid);
    }
#elif ZOO_IS_EMBEDDED
    // For bare metal or simple embedded systems
    return snprintf(buffer, buffer_size, "[MAIN]");
#else
    // Fallback - no threading support
    return snprintf(buffer, buffer_size, "[SINGLE]");
#endif
    }

#ifdef __cplusplus
}
#endif

#endif /* ZOO_PLATFORM_H */