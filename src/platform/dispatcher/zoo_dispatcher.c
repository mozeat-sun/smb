/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: dispatcher
 * Component id: ZOO_DISPATCHER
 * File name: zoo_dispatcher.c
 * Description: Message dispatcher module for the ZOO framework
 *              Provides functions for creating, managing and processing
 *              message dispatchers with queue-based message handling.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-19     weiwang.sun       created
 * 1.1       2025-08-04     AI Assistant      optimized comments and dependencies
 ******************************************************************************/

#include "zoo_dispatcher.h"
#include "zoo_memory_pool.h"
#include "zoo_list.h"
#include "zoo_queue.h"
#include "zoo_math.h"
#include "zoo.h"
#include "zoo_platform.h"
#include "zoo_error.h"
#include "zoo_log.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Structure representing the ZOO message dispatcher.
 *
 * This structure contains all the necessary components for managing
 * message processing including queue handling, threading synchronization,
 * and sorting strategies.
 */
typedef struct ZOO_DISPATCHER_STRUCT
{
    ZOO_QUEUE_HANDLE queue;                     /**< Message queue handle */
    volatile ZOO_BOOL is_running;               /**< Dispatcher running state */
    ZOO_MUTEX_T mutex;                          /**< Mutex for thread synchronization */
    ZOO_COND_T cond;                            /**< Condition variable for signaling */
    ZOO_QUEUE_SORT_STRATEGY_ENUM sort_strategy; /**< Message sorting strategy */
    ZOO_INT64 dispatcher_id;                    /**< Unique dispatcher identifier */
} ZOO_DISPATCHER_STRUCT;

/**
 * @brief Processes an incoming message from the dispatcher queue.
 *
 * This function dequeues a message from the dispatcher's queue and
 * calls the associated handler with the message data and context.
 *
 * @param dispatcher Pointer to the dispatcher structure
 */
static void process_message(ZOO_DISPATCHER_STRUCT *dispatcher)
{
    void *msg = NULL;
    void *context = NULL;
    ZOO_QUEUED_HANDLER handler = NULL;
    void *user_data = NULL;

    if (zoo_queue_dequeue(dispatcher->queue, &msg, &context, &handler, &user_data, ZOO_TRUE))
    {
        ZOO_LOG_DEBUG("Calling handler: (%p)(user_data:%p, msg:%p, context:%p)",
                      handler, user_data, msg, context);
        if (handler)
        {
            handler(user_data, msg, context);
        }
    }
}

/**
 * @brief Creates and initializes a new ZOO message dispatcher handle.
 *
 * This function creates a new dispatcher instance that manages message
 * processing through the provided queue. Each dispatcher gets a unique
 * identifier for tracking purposes.
 *
 * @param queue Message queue handle to be managed by this dispatcher
 * @return A handle to the newly created message dispatcher, or NULL if creation fails
 */
ZOO_DISPATCHER_HANDLE zoo_create_dispatcher(
    ZOO_QUEUE_HANDLE queue)
{
    if (!queue)
    {
        ZOO_LOG_ERROR("%s", "Cannot create dispatcher: queue is NULL");
        return NULL;
    }

    ZOO_DISPATCHER_STRUCT *dispatcher =
        zoo_allocate_from_pool(sizeof(ZOO_DISPATCHER_STRUCT));
    if (!dispatcher)
    {
        ZOO_LOG_ERROR("%s", "Failed to allocate memory for dispatcher");
        return NULL;
    }

    // Initialize dispatcher fields
    dispatcher->queue = queue;
    dispatcher->is_running = ZOO_FALSE;
    dispatcher->sort_strategy = ZOO_QUEUE_SORT_STRATEGY_NONE;
    dispatcher->dispatcher_id = zoo_generate_uuid64();

    // Initialize threading primitives
    if (!ZOO_MUTEX_INIT(&dispatcher->mutex))
    {
        ZOO_LOG_ERROR("%s", "Failed to initialize dispatcher mutex");
        zoo_free_to_pool(dispatcher);
        return NULL;
    }

    if (!ZOO_COND_INIT(&dispatcher->cond))
    {
        ZOO_LOG_ERROR("%s", "Failed to initialize dispatcher condition variable");
        (void)ZOO_MUTEX_DESTROY(&dispatcher->mutex);
        zoo_free_to_pool(dispatcher);
        return NULL;
    }

    // Add observer to the queue
    zoo_queue_add_observer(queue,
                           zoo_dispatcher_on_queue_changed,
                           dispatcher,
                           "dispatcher_observer");

    return (ZOO_DISPATCHER_HANDLE)dispatcher;
}

/**
 * @brief Sets the sorting strategy for the message dispatcher.
 *
 * This function configures the sorting strategy used by the specified
 * dispatcher when managing its internal message queue. The change takes
 * effect immediately and is thread-safe.
 *
 * @param dispatcher_handle Handle to the dispatcher instance
 * @param sort_strategy The sorting strategy to be applied from ZOO_QUEUE_SORT_STRATEGY_ENUM
 */
void zoo_dispatcher_set_sort_strategy(
    ZOO_DISPATCHER_HANDLE dispatcher_handle,
    ZOO_QUEUE_SORT_STRATEGY_ENUM sort_strategy)
{
    if (!dispatcher_handle)
    {
        ZOO_LOG_ERROR("%s", "Cannot set sort strategy: dispatcher handle is NULL");
        return;
    }

    ZOO_DISPATCHER_STRUCT *dispatcher = (ZOO_DISPATCHER_STRUCT *)dispatcher_handle;

    if (!ZOO_MUTEX_LOCK(&dispatcher->mutex))
    {
        ZOO_LOG_WARN("%s", "Failed to lock dispatcher mutex while setting sort strategy");
        return;
    }
    dispatcher->sort_strategy = sort_strategy;
    if (!ZOO_MUTEX_UNLOCK(&dispatcher->mutex))
    {
        ZOO_LOG_WARN("%s", "Failed to unlock dispatcher mutex while setting sort strategy");
    }
}

/**
 * @brief Retrieves the message queue handle from a message dispatcher.
 *
 * This function returns the message queue handle associated with the given
 * message dispatcher handle, allowing access to the underlying message queue
 * for direct queue operations.
 *
 * @param dispatcher_handle Handle to the message dispatcher from which to
 *                         retrieve the queue handle
 *
 * @return ZOO_QUEUE_HANDLE The message queue handle associated
 *         with the dispatcher, or NULL if the dispatcher handle is invalid
 *
 * @note The returned queue handle should not be freed directly as it is
 *       managed by the dispatcher
 */
ZOO_QUEUE_HANDLE zoo_dispatcher_get_queue(ZOO_DISPATCHER_HANDLE dispatcher_handle)
{
    if (!dispatcher_handle)
        return NULL;

    ZOO_DISPATCHER_STRUCT *dispatcher = (ZOO_DISPATCHER_STRUCT *)dispatcher_handle;
    return dispatcher->queue;
}

/**
 * @brief Starts the ZOO message dispatcher.
 *
 * This function starts the message processing loop for the dispatcher.
 * The dispatcher will continue processing messages until stopped.
 *
 * @param dispatcher_handle Handle to the dispatcher instance
 */
void zoo_start_dispatcher(
    ZOO_DISPATCHER_HANDLE dispatcher_handle)
{
    if (!dispatcher_handle)
    {
        ZOO_LOG_ERROR("%s", "Cannot start dispatcher: handle is NULL");
        return;
    }

    ZOO_DISPATCHER_STRUCT *dispatcher = (ZOO_DISPATCHER_STRUCT *)dispatcher_handle;
    if (dispatcher->is_running)
    {
        ZOO_LOG_WARN("Dispatcher %lld is already running", dispatcher->dispatcher_id);
        return;
    }

    dispatcher->is_running = ZOO_TRUE;

    while (dispatcher->is_running)
    {
        process_message(dispatcher);
    }
}

/**
 * @brief Stops the message dispatcher.
 *
 * This function gracefully stops the dispatcher by setting the running
 * flag to false and signaling any waiting threads to wake up.
 *
 * @param dispatcher_handle Handle to the dispatcher instance
 */
void zoo_stop_dispatcher(
    ZOO_DISPATCHER_HANDLE dispatcher_handle)
{
    if (!dispatcher_handle)
    {
        ZOO_LOG_ERROR("%s", "Cannot stop dispatcher: handle is NULL");
        return;
    }

    ZOO_DISPATCHER_STRUCT *dispatcher = (ZOO_DISPATCHER_STRUCT *)dispatcher_handle;

    dispatcher->is_running = ZOO_FALSE;

    /* Signal the queue to exit blocking dequeue so the dispatcher loop can
     * observe is_running == ZOO_FALSE and return.  The dispatcher thread
     * waits on queue->cond, not dispatcher->cond, so we must signal the
     * queue. */
    zoo_queue_exit_blocking(dispatcher->queue);

    if (ZOO_MUTEX_LOCK(&dispatcher->mutex))
    {
        if (!ZOO_COND_BROADCAST(&dispatcher->cond))
        {
            ZOO_LOG_WARN("%s", "Failed to broadcast dispatcher condition variable");
        }
        if (!ZOO_MUTEX_UNLOCK(&dispatcher->mutex))
        {
            ZOO_LOG_WARN("%s", "Failed to unlock dispatcher mutex while stopping");
        }
    }
    else
    {
        ZOO_LOG_WARN("%s", "Failed to lock dispatcher mutex while stopping");
    }
}

/**
 * @brief Destroys the dispatcher instance and frees all resources.
 *
 * This function completely destroys the dispatcher, cleaning up all
 * resources including threading primitives and allocated memory.
 *
 * @param dispatcher_handle Handle to the dispatcher instance to be destroyed
 */
void zoo_destroy_dispatcher(
    ZOO_DISPATCHER_HANDLE dispatcher_handle)
{
    if (!dispatcher_handle)
    {
        ZOO_LOG_WARN("%s", "Attempt to destroy NULL dispatcher handle");
        return;
    }

    ZOO_DISPATCHER_STRUCT *dispatcher = (ZOO_DISPATCHER_STRUCT *)dispatcher_handle;

    // Ensure dispatcher is stopped
    if (dispatcher->is_running)
    {
        zoo_stop_dispatcher(dispatcher_handle);
    }

    // Cleanup threading primitives
    (void)ZOO_MUTEX_DESTROY(&dispatcher->mutex);
    (void)ZOO_COND_DESTROY(&dispatcher->cond);

    // Free memory
    zoo_free_to_pool(dispatcher);
}

/**
 * @brief Callback function for queue change notifications.
 *
 * This function is called whenever the message queue changes state.
 * It sorts the queue according to the configured strategy and signals
 * waiting threads that new messages are available.
 *
 * @param queue_handle Handle to the message queue
 * @param observer Pointer to the dispatcher observer
 * @param msg Pointer to the message that caused the change
 * @param context Message context data
 * @param handler Message handler function
 */
void zoo_dispatcher_on_queue_changed(void *queue_handle, void *observer, void *msg, void *context, ZOO_QUEUED_HANDLER handler)
{
    (void)msg;
    (void)context;
    (void)handler;

    // Only queue and dispatcher observer are required for sort/signal behavior.
    // msg/context/handler can be NULL for valid enqueue paths.
    if (!queue_handle || !observer)
    {
        ZOO_LOG_WARN("%s", "Invalid parameters in queue change notification");
        return;
    }

    ZOO_QUEUE_HANDLE queue = (ZOO_QUEUE_HANDLE)queue_handle;
    ZOO_DISPATCHER_STRUCT *dispatcher = (ZOO_DISPATCHER_STRUCT *)observer;

    if (dispatcher->sort_strategy == ZOO_QUEUE_SORT_STRATEGY_PRIORITY ||
        dispatcher->sort_strategy == ZOO_QUEUE_SORT_STRATEGY_TIMESTAMP)
    {
        zoo_queue_sort(queue, dispatcher->sort_strategy);
    }
}