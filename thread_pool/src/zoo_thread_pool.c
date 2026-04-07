/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_THREAD_POOL
 * File name: zoo_thread_pool.c
 * Description: Lock-free thread pool interface for ZOO SMB (Soft Message Bus).
 *              Provides APIs for creating, submitting tasks to, destroying, and querying
 *              the status of a lock-free thread pool.
 * History recorder:
 * Version   Date           Author            Context
 * 1.0       2025-05-18     weiwang.sun      Created initial version
 * 1.1       2025-05-22     weiwang.sun      Updated to lock-free design
 ******************************************************************************/

#include "zoo_thread_pool.h"
#include "zoo_memory_pool.h"
#include "../../buffer/inc/zoo_list.h"
#include "zoo_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>


#define THREAD_POOL_MIN_THREADS 2
#define THREAD_POOL_MAX_THREADS 64
#define THREAD_POOL_LOAD_THRESHOLD 0.75 // Load threshold for expansion
#define THREAD_POOL_IDLE_THRESHOLD 0.25 // Load threshold for contraction
#define THREAD_POOL_ADJUST_INTERVAL 1   // Seconds between adjustments

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/**
 * @brief Thread pool task structure
 */
typedef struct ZOO_TASK_STRUCT
{
    const char *name;       /**< Task name */
    ZOO_TASK_FUNC function; /**< Task execution function */
    void *user_data;        /**< User data for the task */
    void *argument;         /**< Task argument */

    ZOO_TASK_FUNC callback;   /**< Callback function after task completion */
    void *callback_user_data; /**< User data for the callback */
    void *callback_argument;  /**< Argument for the callback function */
} ZOO_TASK_STRUCT;

/**
 * @brief Thread pool structure
 */
typedef struct ZOO_THREAD_POOL_STRUCT
{
    const char *name;
    ZOO_THREAD_T *threads;          /**< Array of worker threads */
    ZOO_ATOMIC_SIZE_T thread_count;     /**< Current thread count */
    ZOO_ATOMIC_SIZE_T active_threads;   /**< Number of active threads */
    ZOO_ATOMIC_SIZE_T max_thread_count; /**< Maximum allowed threads */
    ZOO_ATOMIC_SIZE_T min_thread_count; /**< Minimum required threads */
    ZOO_COND_T cond_nonempty;       /**< Condition variable for non-empty queue */
    ZOO_MUTEX_T mutex;              /**< Mutex for wait operations */
    ZOO_MUTEX_T resize_mutex;       /**< Mutex for thread resizing */
    ZOO_BOOL shutdown;              /**< Shutdown flag */
    ZOO_BOOL initialized;           /**< Initialization flag */
    ZOO_SIZE_T max_queue_size;      /**< Max queue size */
    ZOO_LIST_HANDLE tasks;          /**< Task queue */
    ZOO_TIME_T last_adjust_time;    /**< Last resize time */
} ZOO_THREAD_POOL_STRUCT;

static ZOO_THREAD_POOL_STRUCT *g_pool_instance = NULL;

/**
 * @brief Increments the count of active threads in the thread pool.
 *
 * This function updates the internal counter that tracks the number of threads
 * currently active within the specified thread pool.
 *
 * @param pool Pointer to the thread pool structure whose active thread count will be incremented.
 */
static void increment_active_threads(ZOO_THREAD_POOL_STRUCT *pool)
{
    ZOO_ATOMIC_FETCH_ADD(&pool->active_threads, 1);
}

/**
 * @brief Decrements the count of active threads in the thread pool.
 *
 * This function is typically called when a thread completes its work and
 * needs to signal that it is no longer active. It updates the internal
 * counter tracking the number of currently active threads in the given
 * thread pool structure.
 *
 * @param pool Pointer to the thread pool structure whose active thread count
 *             should be decremented.
 */
static void decrement_active_threads(ZOO_THREAD_POOL_STRUCT *pool)
{
    ZOO_ATOMIC_FETCH_SUB(&pool->active_threads, 1);
}

/**
 * @brief Wait for the thread pool to be fully initialized.
 * @param pool Pointer to the thread pool structure.
 */
static void wait_for_initialization(ZOO_THREAD_POOL_STRUCT *pool)
{
    while (!pool->initialized)
    {
        sched_yield();
    }
}

/**
 * @brief Execute a task and update active thread count.
 * @param pool Pointer to the thread pool structure.
 * @param task Pointer to the task to execute.
 */
static void thread_worker_execute_task(ZOO_THREAD_POOL_STRUCT *pool,
                                       ZOO_TASK_STRUCT *task)
{
    if (pool == NULL || task == NULL || task->function == NULL)
        return;

    increment_active_threads(pool);

    task->function(task->user_data, task->argument);
    if (task->callback)
    {
        task->callback(task->callback_user_data,
                       task->callback_argument);
    }

    decrement_active_threads(pool);

    (void)ZOO_COND_SIGNAL(&pool->cond_nonempty);
}

/**
 * @brief Makes a worker thread wait for a new task with timeout
 */
static void thread_worker_wait_for_task(ZOO_THREAD_POOL_STRUCT *pool)
{
    (void)ZOO_MUTEX_LOCK(&pool->mutex);

    while (!pool->shutdown && zoo_list_empty(pool->tasks))
    {
        (void)ZOO_COND_WAIT(&pool->cond_nonempty, &pool->mutex);
    }

    (void)ZOO_MUTEX_UNLOCK(&pool->mutex);
}

/**
 * Determines whether a thread in the thread pool should exit.
 *
 * @param pool Pointer to the thread pool structure.
 * @return true if the thread should exit, false otherwise.
 */
static ZOO_BOOL should_thread_exit(ZOO_THREAD_POOL_STRUCT *pool)
{
    if (!pool->initialized)
    {
        (void)ZOO_MUTEX_LOCK(&pool->mutex);
        while (!pool->initialized && !pool->shutdown)
        {
            (void)ZOO_COND_WAIT(&pool->cond_nonempty, &pool->mutex);
        }
        (void)ZOO_MUTEX_UNLOCK(&pool->mutex);
    }
    return pool->shutdown;
}

/**
 * @brief Retrieves the next available task from the thread pool's task queue
 *
 * @param pool Pointer to the thread pool structure containing the task queue
 * @return ZOO_TASK_STRUCT* Pointer to the next task to be executed, or NULL if queue is empty
 */
static ZOO_TASK_STRUCT *dequeue_task(ZOO_THREAD_POOL_STRUCT *pool)
{
    if (pool == NULL)
        return NULL;
    return zoo_list_pop_front(pool->tasks);
}

/**
 * @brief Main worker thread function for the thread pool.
 *        Continuously dequeues and executes tasks from the queue.
 * @param arg Pointer to the thread pool structure.
 * @return Always returns NULL.
 */
static void *thread_worker(void *arg)
{
    ZOO_THREAD_POOL_STRUCT *pool = (ZOO_THREAD_POOL_STRUCT *)arg;

    // Wait for the thread pool to be fully initialized
    wait_for_initialization(pool);

    while (1)
    {
        // 1. Check if the thread should exit
        if (should_thread_exit(pool))
            break;

        // 2. Try to dequeue and execute a task
        ZOO_TASK_STRUCT *task = dequeue_task(pool);
        if (task)
        {
            thread_worker_execute_task(pool, task);
            // Free the task after execution
            zoo_free_to_pool(task);
            continue; // After executing a task, continue to the next loop
        }

        // 3. No task available, wait for a new task or shutdown signal
        thread_worker_wait_for_task(pool);

        // 4. Check again if the thread should exit (in case shutdown occurred while waiting)
        if (should_thread_exit(pool))
            break;
    }

    return NULL;
}

/**
 * @brief Create worker threads for the thread pool.
 * @param pool Pointer to the thread pool structure.
 * @param thread_count Number of worker threads.
 * @return 0 on success, -1 on failure.
 */
static ZOO_INT32 zoo_thread_pool_create_threads(ZOO_THREAD_POOL_STRUCT *pool, ZOO_SIZE_T thread_count)
{
    pool->threads = zoo_allocate_from_pool(sizeof(ZOO_THREAD_T) * thread_count);
    if (!pool->threads)
    {
        return ZOO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    // Create threads with proper error handling
    for (ZOO_SIZE_T i = 0; i < thread_count; i++)
    {
        if (!ZOO_THREAD_CREATE(&pool->threads[i], thread_worker, pool))
        {
            // Clean up already created threads
            for (ZOO_SIZE_T j = 0; j < i; j++)
            {
                // Note: ZOO platform doesn't have thread cancellation abstraction
                // For now, rely on shutdown flag and join
                (void)ZOO_THREAD_JOIN(pool->threads[j]);
            }
            return ZOO_ERROR_THREAD_CREATE_FAILED;
        }
        ZOO_ATOMIC_FETCH_ADD(&pool->thread_count, 1);
    }

    return ZOO_OK;
}

/**
 * @brief Check the validity of thread pool parameters and function pointer.
 * @param pool Pointer to the thread pool structure.
 * @param function Task function pointer.
 * @return ZOO_OK if valid, error code otherwise.
 */
static ZOO_ERROR_TYPE check_params(
    ZOO_THREAD_POOL_STRUCT *pool,
    ZOO_TASK_FUNC function)
{
    if (NULL == pool || NULL == function)
        return ZOO_ERROR_INVALID_PARAM;
    if (pool->shutdown)
    {
        return ZOO_ERROR_NOT_STARTED;
    }
    return ZOO_OK;
}

/**
 * @brief Wait for space in the queue, block if necessary.
 * @param pool Pointer to the thread pool structure.
 * @param block Whether to block if the queue is full.
 * @return ZOO_OK if space is available, error code otherwise.
 */
static ZOO_ERROR_TYPE wait_for_space(
    ZOO_THREAD_POOL_STRUCT *pool,
    ZOO_BOOL block)
{
    ZOO_ERROR_TYPE result = ZOO_OK;
    ZOO_SIZE_T cur_q_size = zoo_list_size(pool->tasks);
    ZOO_BOOL full = cur_q_size >= pool->max_queue_size ? true : false;
    if (full)
    {
        if (!block)
        {
            result = ZOO_ERROR_AGAIN;
        }

        if (ZOO_OK == result)
        {
            (void)ZOO_MUTEX_LOCK(&pool->mutex);
            while (zoo_list_size(pool->tasks) >= pool->max_queue_size && !pool->shutdown)
                (void)ZOO_COND_WAIT(&pool->cond_nonempty, &pool->mutex);
            (void)ZOO_MUTEX_UNLOCK(&pool->mutex);
        }
    }

    return result;
}

/**
 * @brief Create a new task node for the thread pool.
 * @param function Task function pointer.
 * @param argument Task argument.
 * @return Pointer to the new task node, or NULL on failure.
 */
static ZOO_TASK_STRUCT *create_task(
    ZOO_THREAD_POOL_STRUCT *pool,
    const char *task_name,
    ZOO_TASK_FUNC function,
    void *user_data,
    void *argument,
    ZOO_TASK_FUNC callback,
    void *callback_user_data,
    void *callback_argument)
{
    ZOO_UNUSED(pool);
    ZOO_TASK_STRUCT *task = zoo_allocate_from_pool(sizeof(ZOO_TASK_STRUCT));
    if (task)
    {
        task->name = task_name;
        task->function = function;
        task->user_data = user_data;
        task->argument = argument;
        task->callback = callback;
        task->callback_user_data = callback_user_data;
        task->callback_argument = callback_argument;
    }
    return task;
}

/**
 * @brief Atomically enqueue a task node into the thread pool queue.
 * @param pool Pointer to the thread pool structure.
 * @param task Pointer to the task node.
 * @return ZOO_OK on success, error code otherwise.
 */
static ZOO_ERROR_TYPE enqueue_task(
    ZOO_THREAD_POOL_STRUCT *pool,
    const ZOO_TASK_STRUCT *task)
{
    zoo_list_push_back(pool->tasks, task);
    (void)ZOO_MUTEX_LOCK(&pool->mutex);
    (void)ZOO_COND_SIGNAL(&pool->cond_nonempty);
    (void)ZOO_MUTEX_UNLOCK(&pool->mutex);
    return ZOO_OK;
}

/**
 * @brief Set shutdown flag and wake up all waiting threads.
 * @param pool Pointer to the thread pool structure.
 */
static void zoo_thread_pool_shutdown(ZOO_THREAD_POOL_STRUCT *pool)
{
    pool->shutdown = true;
    (void)ZOO_MUTEX_LOCK(&pool->mutex);
    (void)ZOO_COND_BROADCAST(&pool->cond_nonempty);
    (void)ZOO_MUTEX_UNLOCK(&pool->mutex);
}

/**
 * @brief Wait for all worker threads to finish.
 * @param pool Pointer to the thread pool structure.
 */
static void zoo_thread_pool_join_threads(ZOO_THREAD_POOL_STRUCT *pool)
{
    for (ZOO_SIZE_T i = 0; i < pool->thread_count; i++)
    {
        (void)ZOO_THREAD_JOIN(pool->threads[i]);
    }
}

/**
 * @brief Cleanup all remaining tasks in the queue.
 * @param pool Pointer to the thread pool structure.
 */
static void zoo_thread_pool_cleanup_tasks(ZOO_THREAD_POOL_STRUCT *pool)
{
    if (pool->tasks)
    {
        zoo_list_destroy(pool->tasks);
        pool->tasks = NULL;
    }
}

/**
 * @brief Free all resources associated with the thread pool.
 * @param pool Pointer to the thread pool structure.
 */
static void zoo_thread_pool_free_resources(ZOO_THREAD_POOL_STRUCT *pool)
{
    ZOO_LOG_TRACE("free_resources[%p].", pool);
    // Free all tasks in the task pool

    // Destroy condition variable and mutex
    (void)ZOO_COND_DESTROY(&pool->cond_nonempty);
    (void)ZOO_MUTEX_DESTROY(&pool->mutex);
    (void)ZOO_MUTEX_DESTROY(&pool->resize_mutex);

    // Free threads array and pool structure
    zoo_free_to_pool(pool->threads);
    zoo_free_to_pool(pool);
}

/**
 * @brief Adjust thread pool size based on load
 */
static ZOO_ERROR_TYPE adjust_thread_pool_size(ZOO_THREAD_POOL_STRUCT *pool)
{
    ZOO_TIME_T current_time = ZOO_TIME_GET();
    if (current_time - pool->last_adjust_time < THREAD_POOL_ADJUST_INTERVAL)
    {
        return ZOO_OK;
    }

    (void)ZOO_MUTEX_LOCK(&pool->resize_mutex);

    ZOO_SIZE_T current_threads = ZOO_ATOMIC_LOAD(&pool->thread_count);
    ZOO_SIZE_T active_threads = ZOO_ATOMIC_LOAD(&pool->active_threads);
    ZOO_SIZE_T queued_tasks = zoo_list_size(pool->tasks);

    float load_factor = (float)(active_threads + queued_tasks) / (float)current_threads;
    
    // Expand pool if load is high
    if (load_factor > THREAD_POOL_LOAD_THRESHOLD)
    {
        ZOO_SIZE_T new_threads = MIN(current_threads * 2,
                                     ZOO_ATOMIC_LOAD(&pool->max_thread_count));

        if (new_threads > current_threads)
        {
            ZOO_THREAD_T *new_thread_array = zoo_allocate_from_pool(
                new_threads * sizeof(ZOO_THREAD_T));

            if (new_thread_array)
            {
                // Copy existing thread handles
                ZOO_SIZE_T copy_count = MIN(current_threads, new_threads);
                if (pool->threads && copy_count > 0)
                {
                    memcpy(new_thread_array, pool->threads, copy_count * sizeof(ZOO_THREAD_T));
                }

                // Create new threads
                for (ZOO_SIZE_T i = current_threads; i < new_threads; i++)
                {
                    if (ZOO_THREAD_CREATE(&new_thread_array[i], thread_worker, pool))
                    {
                        ZOO_ATOMIC_FETCH_ADD(&pool->thread_count, 1);
                    }
                }

                // Switch to new array
                ZOO_THREAD_T *old_array = pool->threads;
                pool->threads = new_thread_array;
                zoo_free_to_pool(old_array);
            }
        }
        pool->last_adjust_time = current_time;
    }
    // Contract pool if load is low
    else if (load_factor < THREAD_POOL_IDLE_THRESHOLD)
    {
        ZOO_SIZE_T min_threads = ZOO_ATOMIC_LOAD(&pool->min_thread_count);
        ZOO_SIZE_T target_threads = MAX(current_threads / 2, min_threads);

        if (target_threads < current_threads)
        {
            // Note: ZOO platform doesn't provide thread cancellation abstraction
            // For safe thread reduction, we rely on the shutdown mechanism
            // and natural thread termination when no work is available
            ZOO_ATOMIC_STORE(&pool->thread_count, target_threads);
        }
        pool->last_adjust_time = current_time;
    }

    (void)ZOO_MUTEX_UNLOCK(&pool->resize_mutex);
    return ZOO_OK;
}

/**
 * @brief Create a new lock-free thread pool.
 * @param thread_count Number of worker threads.
 * @param max_queue_size Maximum size of the task queue.
 * @return Pointer to the created thread pool on success, NULL on failure.
 */
ZOO_THREAD_POOL_HANDLE zoo_create_thread_pool(
    ZOO_SIZE_T thread_count,
    ZOO_SIZE_T max_queue_size)
{

    // Validate parameters
    if (thread_count == 0 || thread_count > THREAD_POOL_MAX_THREADS ||
        max_queue_size == 0)
    {
        return NULL;
    }

    if (g_pool_instance)
    {
        return g_pool_instance;
    }

    ZOO_THREAD_POOL_STRUCT *pool = zoo_allocate_from_pool(sizeof(ZOO_THREAD_POOL_STRUCT));

    if (!pool)
    {
        return NULL;
    }

    pool->max_queue_size = max_queue_size;
    pool->name = "SMB";

    // Initialize atomic variables
    ZOO_ATOMIC_INIT(&pool->thread_count, 0);
    ZOO_ATOMIC_INIT(&pool->active_threads, 0);
    ZOO_ATOMIC_INIT(&pool->max_thread_count, THREAD_POOL_MAX_THREADS);
    ZOO_ATOMIC_INIT(&pool->min_thread_count, THREAD_POOL_MIN_THREADS);

    // Initialize synchronization primitives
    if (!ZOO_MUTEX_INIT(&pool->mutex) ||
        !ZOO_MUTEX_INIT(&pool->resize_mutex) ||
        !ZOO_COND_INIT(&pool->cond_nonempty))
    {
        zoo_free_to_pool(pool);
        return NULL;
    }

    // Create task queue
    pool->tasks = zoo_list_create(max_queue_size);
    if (!pool->tasks)
    {
        zoo_free_to_pool(pool->threads);
        (void)ZOO_MUTEX_DESTROY(&pool->mutex);
        (void)ZOO_MUTEX_DESTROY(&pool->resize_mutex);
        (void)ZOO_COND_DESTROY(&pool->cond_nonempty);
        zoo_free_to_pool(pool);
        return NULL;
    }

    // Create worker threads
    ZOO_INT32 result = zoo_thread_pool_create_threads(pool, thread_count);
    if (result != ZOO_OK)
    {
        zoo_list_destroy(pool->tasks);
        (void)ZOO_MUTEX_DESTROY(&pool->mutex);
        (void)ZOO_MUTEX_DESTROY(&pool->resize_mutex);
        (void)ZOO_COND_DESTROY(&pool->cond_nonempty);
        zoo_free_to_pool(pool);
        return NULL;
    }

    pool->shutdown = false;
    pool->initialized = true;
    pool->last_adjust_time = ZOO_TIME_GET();
    g_pool_instance = pool;
    return (ZOO_THREAD_POOL_HANDLE)g_pool_instance;
}

/**
 * @brief Add a task to the thread pool.
 * @param pool Pointer to the thread pool structure.
 * @param function Task function pointer.
 * @param argument Task argument.
 * @param block Whether to block if the queue is full.
 * @return ZOO_OK on success, error code otherwise.
 */
ZOO_ERROR_TYPE zoo_thread_pool_submit_task(
    const char *task_name,
    ZOO_TASK_FUNC function,
    void *user_data,
    void *argument,
    ZOO_BOOL block)
{
    if(g_pool_instance == NULL)
        return ZOO_ERROR_INVALID_PARAM;
    ZOO_THREAD_POOL_STRUCT *tp = g_pool_instance;
    ZOO_TASK_STRUCT *task = NULL;
    ZOO_ERROR_TYPE result = check_params(tp, function);
    if (ZOO_OK == result)
    {
        result = wait_for_space(tp, block);
    }

    if (ZOO_OK == result)
    {
        task = create_task(tp, task_name, function, user_data, argument, NULL, NULL, NULL);
        if (NULL == task)
        {
            result = ZOO_ERROR_OUT_OF_MEMORY;
        }
    }

    if (ZOO_OK == result)
    {
        result = enqueue_task(tp, task);
    }

    // Check if pool needs resizing
    adjust_thread_pool_size(tp);

    return result;
}

/**
 * @brief Add a task to the thread pool
 * @param pool Thread pool handle
 * @param function Task function
 * @param argument Task argument
 * @param callback Callback function after task completion
 * @param callback_user_data User data for the callback
 * @param callback_argument Argument for the callback function
 * @param user_data User data for the task
 * @param block Whether to block if queue is full (true) or return immediately (false)
 * @return ZOO_THREAD_POOL_SUCCESS on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_thread_pool_submit_task_ex(
    const char *task_name,
    ZOO_TASK_FUNC function,
    void *f_user_data,
    void *f_argument,
    ZOO_TASK_FUNC call_after_func,
    void *caf_user_data,
    void *caf_argument,
    ZOO_BOOL block)
{
    if(g_pool_instance == NULL)
        return ZOO_ERROR_INVALID_PARAM;
    ZOO_THREAD_POOL_STRUCT *tp = g_pool_instance;
    ZOO_TASK_STRUCT *task = NULL;
    ZOO_ERROR_TYPE result = check_params(tp, function);

    if (ZOO_OK == result)
        result = wait_for_space(tp, block);

    if (ZOO_OK == result)
    {
        task = create_task(tp, task_name, function, f_user_data, f_argument, call_after_func, caf_user_data, caf_argument);
        if (NULL == task)
            result = ZOO_ERROR_OUT_OF_MEMORY;
    }

    if (ZOO_OK == result)
        result = enqueue_task(tp, task);

    // Check if pool needs resizing
    adjust_thread_pool_size(tp);
    return result;
}

/**
 * @brief Destroy the thread pool and free all resources.
 * @param pool Pointer to the thread pool structure.
 * @param graceful Whether to wait for all tasks to complete (true) or abort (false).
 */
void zoo_destroy_thread_pool(ZOO_BOOL graceful)
{
    if (g_pool_instance == NULL)
        return;
    ZOO_THREAD_POOL_STRUCT *tp = g_pool_instance;
    if (!tp->initialized)
        return;
    // Shutdown the thread pool
    zoo_thread_pool_shutdown(tp);

    // Wait for all threads to finish
    zoo_thread_pool_join_threads(tp);

    // Cleanup remaining tasks if not graceful
    if (!graceful)
        zoo_thread_pool_cleanup_tasks(tp);

    tp->initialized = false;
    // Free all other resources
    zoo_thread_pool_free_resources(tp);

    g_pool_instance = NULL;
}

/**
 * @brief Get the current status of the thread pool.
 * @param pool Pointer to the thread pool structure.
 * @param[out] active_threads Number of active threads (can be NULL).
 * @param[out] queued_tasks Number of tasks in the queue (can be NULL).
 * @return ZOO_OK on success, error code otherwise.
 */
ZOO_ERROR_TYPE zoo_thread_pool_get_status(
    ZOO_SIZE_T *active_threads,
    ZOO_SIZE_T *queued_tasks)
{
    if(g_pool_instance == NULL)
        return ZOO_ERROR_INVALID_PARAM;
    ZOO_THREAD_POOL_STRUCT *tp = g_pool_instance;
    if (active_threads)
        *active_threads = tp->active_threads;

    if (queued_tasks)
        *queued_tasks = zoo_list_size(tp->tasks);

    return ZOO_OK;
}
