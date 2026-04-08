/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: buffer
 * Component id: queue
 * File name: zoo_queue.h
 * Description: Cross-platform message queue implementation for ZOO
 *              Supports Windows, Linux, FreeRTOS, CMSIS-RTOS, and bare metal
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 * 1.1       2025-07-31     weiwang.sun         added cross-platform support
 ******************************************************************************/

#ifndef ZOO_QUEUE_H
#define ZOO_QUEUE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo.h"
#include "zoo_error.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_QUEUE_OBSERVER_NAME_LENGTH 64
    /**
     * @defgroup ZOO_QUEUE Message Queue
     * @brief Cross-platform message queue implementation for decoupling message reception and processing.
     * @{
     */
    typedef enum
    {
        ZOO_QUEUE_SORT_STRATEGY_NONE = 0,
        ZOO_QUEUE_SORT_STRATEGY_PRIORITY,
        ZOO_QUEUE_SORT_STRATEGY_TIMESTAMP,
        ZOO_QUEUE_SORT_STRATEGY_FIFO
    } ZOO_QUEUE_SORT_STRATEGY_ENUM;

    /** Forward declaration of the message queue handle. */
    typedef struct ZOO_QUEUE_STRUCT* ZOO_QUEUE_HANDLE;

    /**
     * @brief Callback function type for handling queue changes.
     *
     * This function is called whenever the message queue changes.
     *
     * @param user_data Pointer to user-defined data passed to the handler.
     * @param msg Pointer to the message associated with the queue change.
     * @param context Additional context data for the message.
     */
    typedef void (*ZOO_QUEUED_HANDLER)(void* user_data, void* msg, void* context);
    typedef void (*ON_QUEUE_CHANGED_HANDLER)(void* queue, void* user_data, void* msg, void* context, ZOO_QUEUED_HANDLER handler);
    typedef void (*ZOO_ENQUEUED_HANDLER)(void* user_data, void* msg, void* context);

    /**
     * @brief Creates a new message queue.
     *
     * @param max_queue_size Maximum number of messages the queue can hold
     * @return Handle to the created message queue, or NULL on error.
     */
    ZOO_QUEUE_HANDLE zoo_create_queue(
        ZOO_SIZE_T max_queue_size);

    /**
     * @brief Adds an observer to the message queue.
     *
     * @param queue_handle Handle to the message queue.
     * @param observer_handler Callback function to be called when the queue changes.
     * @param observer Pointer to the observer to be added.
     *
     * @note The observer must remain valid during registration.
     */
    void zoo_queue_add_observer(
        ZOO_QUEUE_HANDLE queue_handle,
        ON_QUEUE_CHANGED_HANDLER observer_handler,
        void* observer,
        const char* name);

    /**
     * @brief Destroys a message queue and frees all resources.
     *
     * @param queue_handle Handle to the message queue to destroy.
     */
    void zoo_destroy_queue(
        ZOO_QUEUE_HANDLE queue_handle);

    /**
     * @brief Enqueues a message into the message queue.
     *
     * This function adds a new message to the end of the specified message queue.
     *
     * @param queue_handle Handle to the message queue.
     * @param msg Pointer to the message to be enqueued.
     * @param context Additional context data for the message.
     * @param handler Callback handler for processing the message.
     * @param user_data User-defined data to pass to the handler.
     * @return true if the message was successfully enqueued, false otherwise (e.g., if the queue is full or on error).
     */
    ZOO_BOOL zoo_queue_enqueue(
        ZOO_QUEUE_HANDLE queue_handle,
        void* msg,
        void* context,
        ZOO_QUEUED_HANDLER handler,
        void* user_data);

    /**
     * @brief Dequeues a message from the message queue.
     *
     * Removes the next message from the queue and provides it to the caller.
     *
     * @param queue_handle Handle to the message queue.
     * @param[out] msg Pointer to store the dequeued message.
     * @param[out] context Pointer to store the message context.
     * @param[out] handler Pointer to store the message handler.
     * @param[out] user_data Pointer to store the user data.
     * @param block_if_empty Whether to block if the queue is empty.
     * @return true if a message was successfully dequeued, false if the queue was empty or an error occurred.
     */
    ZOO_BOOL zoo_queue_dequeue(
        ZOO_QUEUE_HANDLE queue_handle,
        void** msg,
        void** context,
        ZOO_QUEUED_HANDLER* handler,
        void** user_data,
        ZOO_BOOL block_if_empty);

    /**
     * @brief Exits the blocking state of the message queue.
     *
     * This function is used to signal or trigger the exit from a blocking
     * operation within the message queue, allowing any waiting threads
     * or processes to continue execution.
     *
     * Usage of this function is typically required when shutting down the
     * message queue or when an external event necessitates interruption of
     * a blocking wait.
     */
    void zoo_queue_exit_blocking(
        ZOO_QUEUE_HANDLE queue_handle);

    /**
     * @brief Sets the priority for a message in the message queue.
     *
     * This function allows the user to assign a specific priority level to a message
     * within the message queue, which may affect the order in which messages are processed.
     *
     * @param queue_handle Handle to the message queue.
     * @param msg Pointer to the message whose priority is to be set.
     * @param priority The new priority level to assign to the message.
     * @return true if the priority was successfully set, false otherwise.
     */
    ZOO_BOOL zoo_queue_set_priority(
        ZOO_QUEUE_HANDLE queue_handle,
        void* msg,
        ZOO_INT32 priority);
    
    /**
     * @brief Retrieves the priority of a message from the message queue.
     *
     * This function returns the priority level associated with a message in the queue.
     *
     * @param queue_handle Handle to the message queue.
     * @param msg Pointer to the message to query.
     * @return ZOO_INT32 The priority value of the specified message, -1 if not found.
     */
    ZOO_INT32 zoo_queue_get_priority(
        ZOO_QUEUE_HANDLE queue_handle,
        void* msg);
        
    /**
     * @brief Sorts the messages in the specified message queue.
     *
     * This function organizes the messages within the queue referenced by
     * queue_handle according to the specified sorting strategy.
     *
     * @param queue_handle Handle to the message queue to be sorted.
     * @param strategy Sorting strategy to use.
     */
    void zoo_queue_sort(
        ZOO_QUEUE_HANDLE queue_handle,
        ZOO_INT32 strategy);

    /** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZOO_QUEUE_H */