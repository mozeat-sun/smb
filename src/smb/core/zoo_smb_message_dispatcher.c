/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_MESSAGE_DISPATCHER
 * File name: zoo_smb_message_dispatcher.c
 * Description: Minimal message dispatcher implementation for SMB runtime wiring
 ******************************************************************************/

#include "zoo_smb_message_dispatcher.h"
#include "zoo_memory_pool.h"
#include <stdatomic.h>

/**
 * @brief Minimal dispatcher state used by the routing engine.
 *
 * The dispatcher runs as the routing engine's worker loop. It drains queued
 * messages and invokes the handler/user-data tuple captured at enqueue time.
 */
typedef struct ZOO_SMB_MESSAGE_DISPATCHER_STRUCT
{
    ZOO_QUEUE_HANDLE queue;
    atomic_bool running;
    atomic_bool worker_active;
} ZOO_SMB_MESSAGE_DISPATCHER_STRUCT;

/**
 * @brief Executes one queued work item.
 *
 * @param handler Queue callback captured at enqueue time.
 * @param user_data User data captured at enqueue time.
 * @param msg Message payload dequeued from the queue.
 * @param context Context payload dequeued from the queue.
 */
static void dispatcher_invoke(
    ZOO_QUEUED_HANDLER handler,
    void* user_data,
    void* msg,
    void* context)
{
    if (handler)
    {
        handler(user_data, msg, context);
    }
}

/**
 * @brief Creates a message dispatcher bound to the given queue.
 *
 * @param queue Queue used by the routing engine for inbound messages.
 * @return Dispatcher handle on success, or NULL if the queue is invalid or
 *         memory allocation fails.
 */
ZOO_SMB_MESSAGE_DISPATCHER_HANDLE zoo_smb_create_message_dispatcher(ZOO_QUEUE_HANDLE queue)
{
    if (!queue)
    {
        return NULL;
    }

    ZOO_SMB_MESSAGE_DISPATCHER_STRUCT* dispatcher =
        (ZOO_SMB_MESSAGE_DISPATCHER_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_MESSAGE_DISPATCHER_STRUCT));
    if (!dispatcher)
    {
        return NULL;
    }

    dispatcher->queue = queue;
    atomic_init(&dispatcher->running, false);
    atomic_init(&dispatcher->worker_active, false);
    return (ZOO_SMB_MESSAGE_DISPATCHER_HANDLE)dispatcher;
}

/**
 * @brief Destroys a previously created message dispatcher.
 *
 * @param dispatcher Dispatcher instance to destroy. NULL is ignored.
 */
void zoo_smb_destroy_message_dispatcher(ZOO_SMB_MESSAGE_DISPATCHER_HANDLE dispatcher)
{
    if (!dispatcher)
    {
        return;
    }

    (void)zoo_smb_stop_message_dispatcher(dispatcher);

    zoo_free_to_pool(dispatcher);
}

/**
 * @brief Marks the dispatcher as started.
 *
 * This stub does not spawn a worker or drain the queue yet; it only records
 * lifecycle state for runtime wiring.
 *
 * @param dispatcher Dispatcher instance to start.
 * @return ZOO_OK on success, or ZOO_ERROR_INVALID_PARAM when the handle is NULL.
 */
ZOO_ERROR_TYPE zoo_smb_start_message_dispatcher(ZOO_SMB_MESSAGE_DISPATCHER_HANDLE dispatcher)
{
    if (!dispatcher)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_MESSAGE_DISPATCHER_STRUCT* state = (ZOO_SMB_MESSAGE_DISPATCHER_STRUCT*)dispatcher;
    bool expected = false;
    if (!atomic_compare_exchange_strong(&state->running, &expected, true))
    {
        return ZOO_SMB_ERROR_ALREADY_STARTED;
    }

    atomic_store(&state->worker_active, true);

    while (atomic_load(&state->running))
    {
        void* msg = NULL;
        void* context = NULL;
        void* user_data = NULL;
        ZOO_QUEUED_HANDLER handler = NULL;

        if (!zoo_queue_dequeue(state->queue, &msg, &context, &handler, &user_data, ZOO_FALSE))
        {
            ZOO_SLEEP_MS(1);
            continue;
        }

        dispatcher_invoke(handler, user_data, msg, context);
    }

    atomic_store(&state->worker_active, false);
    return ZOO_OK;
}

/**
 * @brief Marks the dispatcher as stopped.
 *
 * @param dispatcher Dispatcher instance to stop.
 * @return ZOO_OK on success, or ZOO_ERROR_INVALID_PARAM when the handle is NULL.
 */
ZOO_ERROR_TYPE zoo_smb_stop_message_dispatcher(ZOO_SMB_MESSAGE_DISPATCHER_HANDLE dispatcher)
{
    if (!dispatcher)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_MESSAGE_DISPATCHER_STRUCT* state = (ZOO_SMB_MESSAGE_DISPATCHER_STRUCT*)dispatcher;
    atomic_store(&state->running, false);

    for (int wait_count = 0; wait_count < 100 && atomic_load(&state->worker_active); ++wait_count)
    {
        ZOO_SLEEP_MS(1);
    }

    return ZOO_OK;
}
