/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Thread Pool
 * Component id: ZOO_THREAD_POOL
 * File name: zoo_thread_pool.h
 * Description: Lock-free thread pool interface for ZOO.
 *              Provides APIs for creating, submitting tasks to, destroying, and querying
 *              the status of a lock-free thread pool.
 * History recorder:
 * Version   Date           Author            Context
 * 1.0       2025-05-18     weiwang.sun      Created initial version
 * 1.1       2025-05-22     weiwang.sun      Updated to lock-free design
 ******************************************************************************/
#ifndef ZOO_THREAD_POOL_H
#define ZOO_THREAD_POOL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo.h"
#include "zoo_error.h"
#include <stddef.h>
#include <stdbool.h>
    /**
     * @defgroup ZOO_SMB_THREAD_POOL Thread Pool Module
     * @brief Lock-free thread pool implementation for ZOO SMB
     * @{
     */
    typedef ZOO_ERROR_T (*ZOO_TASK_FUNC)(void *user_data, void *argument);

    /**
     * @brief Thread pool handle type
     */
    typedef struct ZOO_THREAD_POOL_STRUCT *ZOO_THREAD_POOL_HANDLE;

    /**
     * @brief Create a new lock-free thread pool
     * @param thread_count Number of worker threads
     * @param max_queue_size Maximum size of the task queue
     * @return Thread pool handle on success, NULL on failure
     * @note errno is set appropriately on failure
     */
    ZOO_THREAD_POOL_HANDLE zoo_create_thread_pool(ZOO_SIZE thread_count,
                                                  ZOO_SIZE max_queue_size);

    /**
     * @brief Add a task to the thread pool
     * @param pool Thread pool handle
     * @param function Task function
     * @param argument Task argument
     * @param block Whether to block if queue is full (true) or return immediately (false)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_T zoo_thread_pool_submit_task(
        const char *task_name,
        ZOO_TASK_FUNC function,
        void *user_data,
        void *argument,
        ZOO_BOOL block);

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
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_T zoo_thread_pool_submit_task_ex(
        const char *task_name,
        ZOO_TASK_FUNC function,
        void *f_user_data,
        void *f_argument,
        ZOO_TASK_FUNC call_after_func,
        void *caf_user_data,
        void *caf_argument,
        ZOO_BOOL block);

    /**
     * @brief Destroy the thread pool
     * @param pool Thread pool handle
     * @param graceful Whether to wait for all tasks to complete (true) or abort (false)
     */
    void zoo_destroy_thread_pool(ZOO_BOOL graceful);

    /**
     * @brief Get the current status of the thread pool
     * @param pool Thread pool handle
     * @param[out] active_threads Number of active threads (can be NULL)
     * @param[out] queued_tasks Number of tasks in the queue (can be NULL)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_thread_pool_get_status(
        ZOO_SIZE *active_threads,
        ZOO_SIZE *queued_tasks);

    /** @} */ // end of ZOO_SMB_THREAD_POOL group

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_THREAD_POOL_H */