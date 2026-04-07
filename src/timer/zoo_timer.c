/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 * Product: ZOO
 * Module: Timer
 * Component ID: TIMER
 * File name: zoo_timer.c
 * Description: Cross-platform timer utility implementation for ZOO
 *              Supports Windows, Linux, FreeRTOS, and bare-metal platforms
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-30     weiwang.sun       Created with cross-platform support
 ******************************************************************************/

#include "zoo_timer.h"
#include "zoo.h"
#include "zoo_error.h"
#include <stdlib.h>
#include <string.h>

// Platform-specific includes
#ifdef ZOO_OS_WINDOWS
    #include <windows.h>
    #include <process.h>
#elif defined(ZOO_HAS_POSIX)
    #include <pthread.h>
    #include <unistd.h>
    #include <time.h>
#elif defined(ZOO_OS_FREERTOS)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "timers.h"
#elif defined(ZOO_OS_CMSIS_RTOS)
    #include "cmsis_os.h"
#endif

/* POSIX epoll/timerfd based singleton timer manager implementation */
#if defined(ZOO_HAS_POSIX)
    #include <sys/epoll.h>
    #include <sys/timerfd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <stdint.h>
    #include <unistd.h>

    typedef struct TIMER_ENTRY_STRUCT
    {
        uint64_t id;
        int tfd;
        uint64_t interval_ms; /* 0 for one-shot */
        ZOO_TIMER_CALLBACK callback;
        void *user_data;
        struct TIMER_ENTRY_STRUCT *next;
    } TIMER_ENTRY_STRUCT;

    typedef struct TIMER_MANAGER_TYPE
    {
        int epfd;
        pthread_t thread;
        pthread_mutex_t lock;
        TIMER_ENTRY_STRUCT *head;
        uint64_t next_id;
        int running;
    } TIMER_MANAGER_TYPE;

    static TIMER_MANAGER_TYPE g_timer_mgr = {.epfd = -1, .thread = 0, .lock = PTHREAD_MUTEX_INITIALIZER, .head = NULL, .next_id = 1, .running = 0};

    /**
     * @brief Cleanup all timer entries. Caller must hold manager lock.
     *
     * This closes any open timer fds and frees the linked-list of timer entries.
     */
    static void timer_manager_cleanup_locked(void)
    {
    TIMER_ENTRY_STRUCT *e = g_timer_mgr.head;
        while (e)
        {
            TIMER_ENTRY_STRUCT *n = e->next;
            if (e->tfd >= 0)
                close(e->tfd);
            free(e);
            e = n;
        }
        g_timer_mgr.head = NULL;
    }

    /**
     * @brief Background thread that waits on epoll and dispatches timer callbacks.
     *
     * @param[in] arg Unused argument.
     * @return NULL
     */
    static void *timer_manager_thread(void *arg)
    {
        (void)arg;
        struct epoll_event events[8];

        while (g_timer_mgr.running)
        {
            int n = epoll_wait(g_timer_mgr.epfd, events, 8, -1);
            if (n < 0)
            {
                if (errno == EINTR)
                    continue;
                /* epoll fd closed or error, break out */
                break;
            }

            for (int i = 0; i < n; ++i)
            {
                TIMER_ENTRY_STRUCT *entry = (TIMER_ENTRY_STRUCT *)events[i].data.ptr;
                if (!entry)
                    continue;

                /* consume timerfd expirations */
                uint64_t expirations = 0;
                ssize_t r = read(entry->tfd, &expirations, sizeof(expirations));
                (void)r;

                if (entry->callback)
                {
                    entry->callback(entry->user_data);
                }

                /* one-shot timers are removed after firing */
                if (entry->interval_ms == 0)
                {
                    /* remove (may re-lock) */
                    zoo_timer_remove(entry->id);
                }
            }
        }

        /* cleanup: close timers and free entries */
        pthread_mutex_lock(&g_timer_mgr.lock);
        timer_manager_cleanup_locked();
        pthread_mutex_unlock(&g_timer_mgr.lock);
        return NULL;
    }

    /**
     * @brief Ensure the singleton timer manager is started.
     *
     * Creates the epoll instance and background thread if not already running.
     *
     * @return 0 on success, -1 on failure.
     */
    static int ensure_timer_manager_started(void)
    {
        if (g_timer_mgr.running)
            return 0;

        g_timer_mgr.epfd = epoll_create1(EPOLL_CLOEXEC);
        if (g_timer_mgr.epfd < 0)
            return -1;

        g_timer_mgr.running = 1;
        int rc = pthread_create(&g_timer_mgr.thread, NULL, timer_manager_thread, NULL);
        if (rc != 0)
        {
            close(g_timer_mgr.epfd);
            g_timer_mgr.epfd = -1;
            g_timer_mgr.running = 0;
            return -1;
        }
        return 0;
    }

    /**
     * @brief Add a timer to the singleton timer manager.
     *
     * @param[in] delay_ms Initial delay in milliseconds before the timer fires.
     * @param[in] interval_ms Interval in milliseconds for periodic timers. Set 0 for one-shot.
     * @param[in] callback Callback invoked when the timer expires.
     * @param[in] user_data User data pointer passed back to the callback.
     * @return Non-zero 64-bit timer id on success; 0 on failure.
     */
    uint64_t zoo_timer_add(uint64_t delay_ms, uint64_t interval_ms, ZOO_TIMER_CALLBACK callback, void *user_data)
    {
        if (callback == NULL || delay_ms == 0)
            return 0;

        if (ensure_timer_manager_started() != 0)
            return 0;

        int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (tfd < 0)
            return 0;

        struct itimerspec its;
        its.it_value.tv_sec = (time_t)(delay_ms / 1000);
        its.it_value.tv_nsec = (long)((delay_ms % 1000) * 1000000);
        if (interval_ms == 0)
        {
            its.it_interval.tv_sec = 0;
            its.it_interval.tv_nsec = 0;
        }
        else
        {
            its.it_interval.tv_sec = (time_t)(interval_ms / 1000);
            its.it_interval.tv_nsec = (long)((interval_ms % 1000) * 1000000);
        }

        if (timerfd_settime(tfd, 0, &its, NULL) != 0)
        {
            close(tfd);
            return 0;
        }

    TIMER_ENTRY_STRUCT *entry = (TIMER_ENTRY_STRUCT *)malloc(sizeof(TIMER_ENTRY_STRUCT));
        if (!entry)
        {
            close(tfd);
            return 0;
        }

        entry->tfd = tfd;
        entry->interval_ms = interval_ms;
        entry->callback = callback;
        entry->user_data = user_data;

        pthread_mutex_lock(&g_timer_mgr.lock);
        entry->id = g_timer_mgr.next_id++;
        entry->next = g_timer_mgr.head;
        g_timer_mgr.head = entry;

    struct epoll_event ev = {0};
    ev.events = EPOLLIN;
    ev.data.ptr = entry;
        if (epoll_ctl(g_timer_mgr.epfd, EPOLL_CTL_ADD, tfd, &ev) != 0)
        {
            /* rollback */
            g_timer_mgr.head = entry->next;
            pthread_mutex_unlock(&g_timer_mgr.lock);
            close(tfd);
            free(entry);
            return 0;
        }

        uint64_t id = entry->id;
        pthread_mutex_unlock(&g_timer_mgr.lock);
        return id;
    }

    /**
     * @brief Remove a previously added timer.
     *
     * @param[in] timer_id Timer id returned by `zoo_timer_add`.
     * @return 0 on success, -1 on failure.
     */
    int zoo_timer_remove(uint64_t timer_id)
    {
        if (timer_id == 0)
            return -1;

        if (!g_timer_mgr.running)
            return -1;

        pthread_mutex_lock(&g_timer_mgr.lock);
    TIMER_ENTRY_STRUCT *prev = NULL;
    TIMER_ENTRY_STRUCT *cur = g_timer_mgr.head;
        while (cur)
        {
            if (cur->id == timer_id)
            {
                /* remove from epoll and close */
                epoll_ctl(g_timer_mgr.epfd, EPOLL_CTL_DEL, cur->tfd, NULL);
                close(cur->tfd);

                if (prev)
                    prev->next = cur->next;
                else
                    g_timer_mgr.head = cur->next;

                free(cur);
                pthread_mutex_unlock(&g_timer_mgr.lock);
                return 0; /* success */
            }
            prev = cur;
            cur = cur->next;
        }

        pthread_mutex_unlock(&g_timer_mgr.lock);
        return -1; /* not found */
    }

    /**
     * @brief Shutdown the global timer manager and free resources.
     *
     * Stops the background thread, closes the epoll fd and frees timer entries.
     */
    void zoo_timer_shutdown(void)
    {
        if (!g_timer_mgr.running)
            return;

        g_timer_mgr.running = 0;
        /* closing epfd will break epoll_wait with EBADF */
        if (g_timer_mgr.epfd >= 0)
        {
            close(g_timer_mgr.epfd);
            g_timer_mgr.epfd = -1;
        }

        /* join thread if running */
        pthread_join(g_timer_mgr.thread, NULL);
        g_timer_mgr.thread = 0;

        /* remaining cleanup done inside thread or here in case */
        pthread_mutex_lock(&g_timer_mgr.lock);
        timer_manager_cleanup_locked();
        pthread_mutex_unlock(&g_timer_mgr.lock);
    }
#else
    /* Non-POSIX platforms: stubs. Caller will get failure/unsupported.
     * We intentionally keep symbols so callers compile on other platforms.
     */
    uint64_t zoo_timer_add(uint64_t delay_ms, uint64_t interval_ms, ZOO_TIMER_CALLBACK callback, void *user_data)
    {
        (void)delay_ms; (void)interval_ms; (void)callback; (void)user_data;
        return 0;
    }

    int zoo_timer_remove(uint64_t timer_id)
    {
        (void)timer_id;
        return -1;
    }

    void zoo_timer_shutdown(void)
    {
        return;
    }
#endif

/**
 * @brief Timer structure definition for cross-platform support
 */
typedef struct ZOO_TIMER_STRUCT
{
    ZOO_UINT32 interval_ms;                /**< Timer ZOO_INT32erval in milliseconds */
    ZOO_BOOL repeat;                       /**< Whether timer should repeat */
    ZOO_TIMER_CALLBACK callback;          /**< User callback function */
    void *user_data;                       /**< User data passed to callback */
    ZOO_THREAD_T thread;                   /**< Platform-specific thread handle */
    ZOO_ATOMIC_BOOL running;               /**< Atomic flag for timer state */
    ZOO_UINT32 timer_id;                   /**< Unique timer identifier */
    
#ifdef ZOO_OS_WINDOWS
    HANDLE stop_event;                     /**< Windows event for stopping timer */
#elif defined(ZOO_OS_LINUX) || defined(ZOO_OS_FREERTOS)
    pthread_mutex_t stop_mutex;            /**< Mutex for stop condition */
    pthread_cond_t stop_cond;              /**< Condition variable for stopping */
    ZOO_BOOL should_stop;                  /**< Flag to signal stop request */
#elif defined(ZOO_OS_FREERTOS)
    TimerHandle_t freertos_timer;          /**< FreeRTOS timer handle */
#elif defined(ZOO_OS_CMSIS_RTOS)
    osTimerId cmsis_timer;                 /**< CMSIS-RTOS timer handle */
#endif
} ZOO_TIMER_STRUCT;

// ==============================================================================
// PLATFORM-SPECIFIC FUNCTION DECLARATIONS
// ==============================================================================

/**
 * @brief Platform-specific sleep function
 * @param[in] milliseconds Number of milliseconds to sleep
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_INT32 zoo_timer_sleep_ms(ZOO_UINT32 milliseconds);

/**
 * @brief Platform-specific thread creation function
 * @param[out] thread PoZOO_INT32er to thread handle
 * @param[in] func Thread function
 * @param[in] arg Thread argument
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_INT32 zoo_timer_create_thread(ZOO_THREAD_T *thread, void *(*func)(void *), void *arg);

/**
 * @brief Platform-specific thread join function
 * @param[in] thread Thread handle to join
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_INT32 zoo_timer_join_thread(ZOO_THREAD_T thread);

/**
 * @brief Get next unique timer ID
 * @return Unique timer ID
 */
static ZOO_UINT32 zoo_timer_get_next_id(void);

// ==============================================================================
// GLOBAL VARIABLES
// ==============================================================================

// ==============================================================================
// PLATFORM-SPECIFIC FUNCTION IMPLEMENTATIONS
// ==============================================================================

/**
 * @brief Platform-specific sleep function
 * @param[in] milliseconds Number of milliseconds to sleep
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_INT32 zoo_timer_sleep_ms(ZOO_UINT32 milliseconds)
{
#ifdef ZOO_OS_WINDOWS
    Sleep(milliseconds);
    return ZOO_OK;
#elif defined(ZOO_HAS_POSIX)
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
    return ZOO_OK;
#elif defined(ZOO_OS_FREERTOS)
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
    return ZOO_OK;
#elif defined(ZOO_OS_CMSIS_RTOS)
    osDelay(milliseconds);
    return ZOO_OK;
#else
    // Fallback busy wait - not efficient but works
    volatile ZOO_UINT32 i;
    for (i = 0; i < milliseconds * 1000; i++)
    {
        // Busy wait
    }
    return ZOO_OK;
#endif
}

/**
 * @brief Platform-specific thread creation function
 * @param[out] thread PoZOO_INT32er to thread handle
 * @param[in] func Thread function
 * @param[in] arg Thread argument
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_INT32 zoo_timer_create_thread(ZOO_THREAD_T *thread, void *(*func)(void *), void *arg)
{
    if (thread == NULL || func == NULL)
    {
        return ZOO_ERROR_NULL_POINTER;
    }

#ifdef ZOO_OS_WINDOWS
    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
    return (*thread != NULL) ? ZOO_OK : ZOO_ERROR_GENERAL;
    
#elif defined(ZOO_HAS_POSIX)
    ZOO_INT32 result = pthread_create(thread, NULL, func, arg);
    return (result == 0) ? ZOO_OK : ZOO_ERROR_GENERAL;
    
#elif defined(ZOO_OS_FREERTOS)
    BaseType_t result = xTaskCreate((TaskFunction_t)func, "zoo_timer", 
                                   configMINIMAL_STACK_SIZE * 2, arg, 
                                   tskIDLE_PRIORITY + 1, thread);
    return (result == pdPASS) ? ZOO_OK : ZOO_ERROR_GENERAL;
    
#elif defined(ZOO_OS_CMSIS_RTOS)
    osThreadDef_t thread_def = {0};
    thread_def.pthread = (os_pthread)func;
    thread_def.tpriority = osPriorityNormal;
    thread_def.stacksize = 512;
    *thread = osThreadCreate(&thread_def, arg);
    return (*thread != NULL) ? ZOO_OK : ZOO_ERROR_GENERAL;
    
#else
    // No threading support
    ZOO_UNUSED(thread);
    ZOO_UNUSED(func);
    ZOO_UNUSED(arg);
    return ZOO_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Platform-specific thread join function
 * @param[in] thread Thread handle to join
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_INT32 zoo_timer_join_thread(ZOO_THREAD_T thread)
{
#ifdef ZOO_OS_WINDOWS
    if (thread != NULL)
    {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
    return ZOO_OK;
    
#elif defined(ZOO_HAS_POSIX)
    ZOO_INT32 result = pthread_join(thread, NULL);
    return (result == 0) ? ZOO_OK : ZOO_ERROR_GENERAL;
    
#elif defined(ZOO_OS_FREERTOS)
    if (thread != NULL)
    {
        vTaskDelete(thread);
    }
    return ZOO_OK;
    
#elif defined(ZOO_OS_CMSIS_RTOS)
    if (thread != NULL)
    {
        osThreadTerminate(thread);
    }
    return ZOO_OK;
    
#else
    ZOO_UNUSED(thread);
    return ZOO_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Get next unique timer ID
 * @return Unique timer ID
 */
static ZOO_UINT32 zoo_timer_get_next_id(void)
{
    static ZOO_UINT32 timer_counter = 1;
    return timer_counter++;
}

/**
 * @brief Cross-platform timer thread function
 * @param[in] arg PoZOO_INT32er to ZOO_TIMER_STRUCT
 * @return Platform-specific return value
 * @note This function runs in a separate thread and handles timer operations
 */
#ifdef ZOO_OS_WINDOWS
static DWORD WINAPI timer_thread_func(LPVOID arg)
#elif defined(ZOO_HAS_POSIX)
static void* timer_thread_func(void *arg)
#elif defined(ZOO_OS_FREERTOS)
static void timer_thread_func(void *arg)
#else
static void timer_thread_func(void *arg)
#endif
{
    ZOO_TIMER_STRUCT *timer = (ZOO_TIMER_STRUCT *)arg;
    if (timer == NULL)
    {
#ifdef ZOO_OS_WINDOWS
        return 1;
#else
        return NULL;
#endif
    }

    do
    {
        ZOO_UINT32 ZOO_INT32erval = timer->interval_ms;
        ZOO_UINT32 slept = 0;
        
        // Sleep in small chunks to allow for responsive stopping
        while (slept < ZOO_INT32erval && ZOO_ATOMIC_LOAD(&timer->running))
        {
            ZOO_UINT32 sleep_chunk = (ZOO_INT32erval - slept > 10) ? 10 : (ZOO_INT32erval - slept);
            zoo_timer_sleep_ms(sleep_chunk);
            slept += sleep_chunk;
        }
        
        if (!ZOO_ATOMIC_LOAD(&timer->running))
            break;
            
        // Execute callback if timer is still running
        if (timer->callback)
        {
            timer->callback(timer->user_data);
        }
        
    } while (timer->repeat && ZOO_ATOMIC_LOAD(&timer->running));

#ifdef ZOO_OS_WINDOWS
    return 0;
#elif defined(ZOO_HAS_POSIX)
    return NULL;
#else
    return;
#endif
}

// ==============================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// ==============================================================================

/**
 * @brief Creates a new cross-platform timer handle
 * @param[in] ZOO_INT32erval_ms Timer ZOO_INT32erval in milliseconds
 * @param[in] repeat Whether the timer should repeat
 * @param[in] callback Callback function to execute on timer expiry
 * @param[in] user_data User data passed to callback
 * @return Timer handle on success, NULL on failure
 * @note This function is thread-safe and works across all supported platforms
 */
ZOO_TIMER_HANDLE zoo_timer_create(ZOO_UINT32 interval_ms,
                                  ZOO_BOOL repeat,
                                  ZOO_TIMER_CALLBACK callback,
                                  void *user_data)
{
    // Validate parameters
    if (interval_ms == 0 || callback == NULL)
    {
        return NULL;
    }

    // Allocate timer structure
    ZOO_TIMER_STRUCT *timer = (ZOO_TIMER_STRUCT *)malloc(sizeof(ZOO_TIMER_STRUCT));
    if (timer == NULL)
    {
        return NULL;
    }

    // Initialize timer structure
    memset(timer, 0, sizeof(ZOO_TIMER_STRUCT));
    timer->interval_ms = interval_ms;
    timer->repeat = repeat;
    timer->callback = callback;
    timer->user_data = user_data;
    timer->timer_id = zoo_timer_get_next_id();
    ZOO_ATOMIC_INIT(&timer->running, ZOO_FALSE);

#ifdef ZOO_OS_WINDOWS
    // Create stop event for Windows
    timer->stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (timer->stop_event == NULL)
    {
        free(timer);
        return NULL;
    }
#elif defined(ZOO_OS_LINUX) || defined(ZOO_OS_FREERTOS)
    // Initialize mutex and condition variable for POSIX systems
    if (pthread_mutex_init(&timer->stop_mutex, NULL) != 0)
    {
        free(timer);
        return NULL;
    }
    if (pthread_cond_init(&timer->stop_cond, NULL) != 0)
    {
        pthread_mutex_destroy(&timer->stop_mutex);
        free(timer);
        return NULL;
    }
    timer->should_stop = ZOO_FALSE;
#elif defined(ZOO_OS_FREERTOS)
    // For FreeRTOS, we'll use the thread-based approach instead of native timers
    // to maZOO_INT32ain consistency across platforms
#elif defined(ZOO_OS_CMSIS_RTOS)
    // For CMSIS-RTOS, we'll use the thread-based approach
#endif

    return (ZOO_TIMER_HANDLE)timer;
}

/**
 * @brief Starts the specified timer
 * @param[in] timer Timer handle to start
 * @return ZOO_TRUE if successfully started, ZOO_FALSE otherwise
 * @note This function is thread-safe and works across all supported platforms
 * @note If timer is already running, this function returns ZOO_FALSE
 */
ZOO_BOOL zoo_timer_start(ZOO_TIMER_HANDLE timer)
{
    ZOO_TIMER_STRUCT *tm = (ZOO_TIMER_STRUCT *)timer;
    if (tm == NULL)
    {
        return ZOO_FALSE;
    }

    // Check if already running
    if (ZOO_ATOMIC_LOAD(&tm->running))
    {
        return ZOO_FALSE; // Already running
    }

    // Set running flag
    ZOO_ATOMIC_STORE(&tm->running, ZOO_TRUE);

#ifdef ZOO_OS_WINDOWS
    // Reset stop event
    ResetEvent(tm->stop_event);
#endif

    // Create and start timer thread
    ZOO_INT32 result = zoo_timer_create_thread(&tm->thread, timer_thread_func, tm);
    if (result != ZOO_OK)
    {
        ZOO_ATOMIC_STORE(&tm->running, ZOO_FALSE);
        return ZOO_FALSE;
    }

    return ZOO_TRUE;
}

/**
 * @brief Stops the specified timer
 * @param[in] timer Timer handle to stop
 * @note This function is thread-safe and works across all supported platforms
 * @note If timer is not running, this function returns immediately
 * @note Function waits for timer thread to complete before returning
 */
void zoo_timer_stop(ZOO_TIMER_HANDLE timer)
{
    ZOO_TIMER_STRUCT *tm = (ZOO_TIMER_STRUCT *)timer;
    if (tm == NULL)
    {
        return;
    }

    // Check if not running
    if (!ZOO_ATOMIC_LOAD(&tm->running))
    {
        return; // Not running
    }

    // Signal stop
    ZOO_ATOMIC_STORE(&tm->running, ZOO_FALSE);

#ifdef ZOO_OS_WINDOWS
    // Signal stop event
    SetEvent(tm->stop_event);
#elif defined(ZOO_OS_LINUX) || defined(ZOO_OS_FREERTOS)
    // Use condition variable to wake up sleeping thread
    pthread_mutex_lock(&tm->stop_mutex);
    tm->should_stop = ZOO_TRUE;
    pthread_cond_signal(&tm->stop_cond);
    pthread_mutex_unlock(&tm->stop_mutex);
#endif

    // Wait for thread to finish
    zoo_timer_join_thread(tm->thread);
}

/**
 * @brief Destroys a timer instance and releases associated resources
 * @param[in] timer Timer handle to be destroyed
 * @note This function stops the timer if it's running and frees all resources
 * @note After calling this function, the timer handle should not be used
 * @note This function is safe to call with NULL timer handle
 */
void zoo_timer_destroy(ZOO_TIMER_HANDLE timer)
{
    ZOO_TIMER_STRUCT *tm = (ZOO_TIMER_STRUCT *)timer;
    if (tm == NULL)
    {
        return;
    }

    // Stop timer if running
    zoo_timer_stop(timer);

    // Clean up platform-specific resources
#ifdef ZOO_OS_WINDOWS
    if (tm->stop_event != NULL)
    {
        CloseHandle(tm->stop_event);
    }
#elif defined(ZOO_OS_LINUX) || defined(ZOO_OS_FREERTOS)
    pthread_mutex_destroy(&tm->stop_mutex);
    pthread_cond_destroy(&tm->stop_cond);
#endif

    // Free timer structure
    free(tm);
}