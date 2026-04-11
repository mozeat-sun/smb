/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_ROUTING_ENGINE
 * File name: zoo_smb_routing_engine.c
 * Description: Implementation of the routing engine interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-30     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_routing_engine.h"
#include "zoo_types.h"
#include "zoo_memory_pool.h"
#include "zoo_thread_pool.h"
#include "zoo_smb_transport_manager.h"
#include "zoo_smb_rule_manager.h"
#include "zoo_queue.h"
#include "zoo_dispatcher.h"
#include "zoo_list.h"
#include "zoo_smb_node_observer.h"
#include "../transport/zoo_smb_protocol.h"
#include "zoo.h"
#include <string.h>
#include <stdatomic.h>

#define ROUTING_ENGINE_TASK_SUBMIT_MAX_RETRIES 100
#define ROUTING_ENGINE_TASK_SUBMIT_BACKOFF_MS 10

/**
 * @brief Mutex macros for thread safety
 */
#define LOCK_ENGINE(engine) ZOO_MUTEX_LOCK(&(engine)->mutex)
#define UNLOCK_ENGINE(engine) ZOO_MUTEX_UNLOCK(&(engine)->mutex)

typedef struct
{
    ZOO_BOOL is_running;        // Flag to indicate if the engine is running
    ZOO_MUTEX_T mutex;      // Mutex for thread safety
    ZOO_QUEUE_HANDLE message_queue;
    ZOO_DISPATCHER_HANDLE message_dispatcher;
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager;  // Transport manager for handling transports
    ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager;      // Service manager for handling services
    ZOO_SMB_RULE_MANAGER_HANDLE rule_manager;            // Manager for routing rules
    const ZOO_SMB_CONFIG_STRUCT* config;
    ZOO_U32 ingress_high_watermark;
    ZOO_U32 ingress_low_watermark;
    ZOO_BOOL telemetry_enabled;
    atomic_bool ingress_backpressure_active;
    atomic_uint ingress_in_flight;
    atomic_ullong ingress_received;
    atomic_ullong ingress_enqueued;
    atomic_ullong ingress_dequeued;
    atomic_ullong ingress_dropped_total;
    atomic_ullong ingress_drop_queue_full;
    atomic_ullong ingress_drop_backpressure;
    atomic_ullong ingress_drop_invalid_header;
    atomic_ullong ingress_drop_security_policy;
} ZOO_SMB_ROUTING_ENGINE_STRUCT;

typedef struct
{
    ZOO_SMB_RULE_HANDLE rule;     // Handle to the routing rule
    ZOO_STRING_T receiver;        // Receiver for the message
    char* receiver_owned;         // Owned receiver copy for async lifetime safety
    ZOO_BOOL delete_message_flag; // Flag to indicate if the message should be deleted after processing
    ZOO_SMB_MSG_STRUCT* message;  // Pointer to the message being routed
} ROUTING_CONTEXT_STRUCT;

/*
 * Keep one aggregate drop counter and one reason-specific counter so
 * operators can quickly detect total loss and then drill down by cause.
 * All counters are atomic because this path can be hit concurrently by
 * multiple transport callbacks.
 */
static void routing_engine_record_drop(
    ZOO_SMB_ROUTING_ENGINE_STRUCT* engine,
    ZOO_SMB_INGRESS_DROP_REASON_ENUM reason)
{
    if (!engine)
    {
        return;
    }

    if (engine->telemetry_enabled)
    {
        atomic_fetch_add(&engine->ingress_dropped_total, 1);
    }

    switch (reason)
    {
        case ZOO_SMB_INGRESS_DROP_QUEUE_FULL:
            /* Queue rejected enqueue because capacity was exhausted. */
            if (engine->telemetry_enabled)
            {
                atomic_fetch_add(&engine->ingress_drop_queue_full, 1);
            }
            break;
        case ZOO_SMB_INGRESS_DROP_BACKPRESSURE:
            /* Engine intentionally shed load due to high in-flight pressure. */
            if (engine->telemetry_enabled)
            {
                atomic_fetch_add(&engine->ingress_drop_backpressure, 1);
            }
            break;
        case ZOO_SMB_INGRESS_DROP_INVALID_HEADER:
            /* Message failed protocol/header validation. */
            if (engine->telemetry_enabled)
            {
                atomic_fetch_add(&engine->ingress_drop_invalid_header, 1);
            }
            break;
        case ZOO_SMB_INGRESS_DROP_SECURITY_POLICY:
            /* Message was rejected by security policy checks. */
            if (engine->telemetry_enabled)
            {
                atomic_fetch_add(&engine->ingress_drop_security_policy, 1);
            }
            break;
        default:
            break;
    }
}

/**
 * @brief Executes the main task for the SMB routing engine.
 *
 * This function performs the core processing logic for the SMB routing engine.
 * It is typically invoked as a task or thread entry point, handling routing
 * operations based on the provided user data and arguments.
 *
 * @param user_data Pointer to user-specific data required by the engine.
 * @param argument  Pointer to additional arguments or context for the task.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 */
static ZOO_ERROR_TYPE engine_task(void* user_data, void* argument)
{
    ZOO_SMB_UNUSED(argument);
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)user_data;
    if (!e)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    zoo_start_dispatcher(e->message_dispatcher);
    return ZOO_SMB_OK;
}

/**
 * @brief Processes the incoming message and notifies all registered observers.
 *
 * This function iterates through all observers registered for incoming data
 * on the specified rule and invokes their callback functions with the message
 * data if the message type matches the observer's expected type.
 *
 * @param rule     The routing rule handle containing observers.
 * @param message  The incoming message to be processed.
 * @param context  User-defined context passed to the observer callbacks.
 *
 * @return ZOO_ERROR_TYPE indicating success or failure of the operation.
 */
static void process_message_and_notify_observer(IN void* rule_handle,
                                                IN void* msg,
                                                IN void* context)
{
    ZOO_SMB_RULE_HANDLE rule = (ZOO_SMB_RULE_HANDLE)rule_handle;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;
    ZOO_SMB_UNUSED(context);

    ZOO_SMB_ROUTING_ENGINE_STRUCT* engine = NULL;
    if (rule)
    {
        engine = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)rule->engine;
    }

    if (!rule || !message || !rule->incoming_data_observers)
    {
        if (message)
        {
            zoo_smb_destroy_message(message);
        }
        if (engine)
        {
            if (engine->telemetry_enabled)
            {
                atomic_fetch_add(&engine->ingress_dequeued, 1);
            }
            (void)atomic_fetch_sub(&engine->ingress_in_flight, 1);
        }
        return;
    }

    ZOO_USIZE_T observer_count = zoo_list_size(rule->incoming_data_observers);
    for (ZOO_USIZE_T i = 0; i < observer_count; i++)
    {
        ZOO_SMB_NODE_OBSERVER_HANDLE observer = zoo_list_at(rule->incoming_data_observers, i);
        if (observer && observer->handler.node_msg_cb && message->header.msg_type == observer->msg_type)
        {
            observer->handler.node_msg_cb(observer->usr_data, message, message->header.payload_size);
        }
    }
    zoo_smb_destroy_message(message);

    if (engine)
    {
        ZOO_U32 remain = (ZOO_U32)atomic_fetch_sub(&engine->ingress_in_flight, 1) - 1;
        if (engine->telemetry_enabled)
        {
            atomic_fetch_add(&engine->ingress_dequeued, 1);
        }
        if (atomic_load(&engine->ingress_backpressure_active) && remain <= engine->ingress_low_watermark)
        {
            if (atomic_exchange(&engine->ingress_backpressure_active, ZOO_FALSE))
            {
                ZOO_LOG_INFO("routing ingress backpressure released: inflight=%u low=%u",
                                 remain,
                                 engine->ingress_low_watermark);
            }
        }
    }
}

/**
 * @brief Handles incoming data from the transport layer.
 *
 * This function processes data received from the transport layer,
 * performing necessary parsing, validation, and routing logic as required
 * by the SMB routing engine. It is typically invoked when new data arrives
 * on a transport connection.
 *
 * @param user_data Routing rule pointer associated with the observer callback.
 * @param message Incoming message object produced by transport layer.
 *
 * The function validates security/header constraints, applies ingress
 * backpressure policy, and enqueues accepted messages for route dispatch.
 */
static void handle_transport_incoming_data(
    IN void* user_data,
    IN const ZOO_SMB_MSG_STRUCT* message)
{
    if (!user_data || !message)
    {
        return;
    }

    ZOO_SMB_RULE_HANDLE rule = (ZOO_SMB_RULE_HANDLE)user_data;
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)rule->engine;
    if (!e || !e->is_running)
    {
        return;
    }

    if (e->telemetry_enabled)
    {
        atomic_fetch_add(&e->ingress_received, 1);
    }

    if (!zoo_smb_protocol_is_header_valid(&message->header))
    {
        routing_engine_record_drop(e, ZOO_SMB_INGRESS_DROP_INVALID_HEADER);
        ZOO_LOG_WARN("dropping invalid header message, id=%u", message->header.msg_id);
        return;
    }

    if (e->config && e->config->sys.require_sender_identity && message->header.sender[0] == '\0')
    {
        routing_engine_record_drop(e, ZOO_SMB_INGRESS_DROP_SECURITY_POLICY);
        ZOO_LOG_WARN("dropping message without sender identity, id=%u", message->header.msg_id);
        return;
    }

    if (e->config && e->config->sys.enforce_encrypted_messages &&
        ((message->header.flags & ZOO_SMB_MSG_FLAG_ENCRYPT) == 0))
    {
        routing_engine_record_drop(e, ZOO_SMB_INGRESS_DROP_SECURITY_POLICY);
        ZOO_LOG_WARN("dropping non-encrypted message due to security policy, id=%u", message->header.msg_id);
        return;
    }

    if (e->config && e->config->sys.max_timeout > 0 && message->header.timestamp > 0)
    {
        ZOO_TIME_T now = ZOO_TIME_GET();
        if (now >= 0 && (uint64_t)now > message->header.timestamp)
        {
            ZOO_TIME_T age_ms = now - (ZOO_TIME_T)message->header.timestamp;
            if (age_ms > (ZOO_TIME_T)e->config->sys.max_timeout)
            {
                routing_engine_record_drop(e, ZOO_SMB_INGRESS_DROP_SECURITY_POLICY);
                ZOO_LOG_WARN("dropping stale message due to security policy, id=%u age_ms=%llu timeout_ms=%d",
                                 message->header.msg_id,
                                 (unsigned long long)age_ms,
                                 e->config->sys.max_timeout);
                return;
            }
        }
    }

    ZOO_U32 in_flight = (ZOO_U32)atomic_fetch_add(&e->ingress_in_flight, 1) + 1;
    if (in_flight > e->ingress_high_watermark)
    {
        (void)atomic_fetch_sub(&e->ingress_in_flight, 1);
        routing_engine_record_drop(e, ZOO_SMB_INGRESS_DROP_BACKPRESSURE);
        if (!atomic_exchange(&e->ingress_backpressure_active, ZOO_TRUE))
        {
            ZOO_LOG_WARN("routing ingress backpressure active: inflight=%u high=%u low=%u",
                             in_flight,
                             e->ingress_high_watermark,
                             e->ingress_low_watermark);
        }
        return;
    }

    ZOO_LOG_DEBUG("Sender:%s, topic:%s, message ID: %u, type: %d, req ID:%llu",
                      message->header.sender,
                      message->header.topic,
                      message->header.msg_id,
                      message->header.msg_type,
                      message->header.request_id);

    ZOO_SMB_MSG_STRUCT* msg = zoo_smb_default_message();
    if (!msg)
    {
        (void)atomic_fetch_sub(&e->ingress_in_flight, 1);
        routing_engine_record_drop(e, ZOO_SMB_INGRESS_DROP_INVALID_PARAM);
        return;
    }

    zoo_smb_copy_message(message, msg);
    if (!zoo_queue_enqueue(e->message_queue, msg, NULL, process_message_and_notify_observer, rule))
    {
        (void)atomic_fetch_sub(&e->ingress_in_flight, 1);
        routing_engine_record_drop(e, ZOO_SMB_INGRESS_DROP_QUEUE_FULL);
        ZOO_LOG_WARN("routing queue full or unavailable, dropping message id=%u", message->header.msg_id);
        zoo_smb_destroy_message(msg);
        return;
    }

    if (e->telemetry_enabled)
    {
        atomic_fetch_add(&e->ingress_enqueued, 1);
    }
}

/**
 * @brief Creates and initializes a new SMB routing engine instance.
 *
 * This function allocates memory for a new routing engine structure and initializes
 * all its components including message queue, message dispatcher, and mutex.
 * The engine is created in a stopped state and requires explicit start call.
 *
 * @return ZOO_SMB_ROUTING_ENGINE_HANDLE Handle to the newly created routing engine instance, or NULL on failure.
 */
ZOO_SMB_ROUTING_ENGINE_HANDLE zoo_smb_create_routing_engine(const ZOO_SMB_CONFIG_STRUCT* config)
{
    ZOO_SMB_ROUTING_ENGINE_STRUCT* engine = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_ROUTING_ENGINE_STRUCT));
    if (engine == NULL)
    {
        ZOO_LOG_ERROR("Memory allocation failed");
        return NULL;
    }

    memset(engine, 0, sizeof(ZOO_SMB_ROUTING_ENGINE_STRUCT));

    // Initialize message queue and dispatcher
    engine->message_queue = zoo_create_queue(config->sys.max_queue_size > 0 ? config->sys.max_queue_size : ZOO_SMB_MAX_QUEUE_SIZE);
    if (engine->message_queue == NULL)
    {
        ZOO_LOG_ERROR(" Failed to create message queue");
        zoo_free_to_pool(engine);
        return NULL;
    }

    engine->message_dispatcher = zoo_create_dispatcher(engine->message_queue);
    if (engine->message_dispatcher == NULL)
    {
        ZOO_LOG_ERROR("Failed to create message dispatcher");
        zoo_destroy_queue(engine->message_queue);
        zoo_free_to_pool(engine);
        return NULL;
    }

    engine->transport_manager = NULL;
    engine->rule_manager = NULL;
    engine->config = config;
    engine->ingress_high_watermark = (config && config->sys.ingress_high_watermark > 0)
                                       ? config->sys.ingress_high_watermark
                                       : (config && config->sys.max_queue_size > 0 ? config->sys.max_queue_size : ZOO_SMB_MAX_QUEUE_SIZE);
    engine->ingress_low_watermark = (config && config->sys.ingress_low_watermark > 0)
                                      ? config->sys.ingress_low_watermark
                                      : (engine->ingress_high_watermark / 2);
    if (engine->ingress_low_watermark == 0 || engine->ingress_low_watermark >= engine->ingress_high_watermark)
    {
        engine->ingress_low_watermark = engine->ingress_high_watermark / 2;
        if (engine->ingress_low_watermark == 0)
        {
            engine->ingress_low_watermark = 1;
        }
    }
    engine->telemetry_enabled = config ? config->sys.enable_routing_telemetry : ZOO_TRUE;
    atomic_init(&engine->ingress_backpressure_active, ZOO_FALSE);
    atomic_init(&engine->ingress_in_flight, 0);
    atomic_init(&engine->ingress_received, 0);
    atomic_init(&engine->ingress_enqueued, 0);
    atomic_init(&engine->ingress_dequeued, 0);
    atomic_init(&engine->ingress_dropped_total, 0);
    atomic_init(&engine->ingress_drop_queue_full, 0);
    atomic_init(&engine->ingress_drop_backpressure, 0);
    atomic_init(&engine->ingress_drop_invalid_header, 0);
    atomic_init(&engine->ingress_drop_security_policy, 0);
    engine->is_running = ZOO_FALSE;
    ZOO_MUTEX_INIT(&engine->mutex);

    ZOO_LOG_DEBUG("SMB routing engine created successfully");
    return (ZOO_SMB_ROUTING_ENGINE_HANDLE)engine;
}

/**
 * @brief Sets the transport manager for the routing engine.
 *
 * This function assigns a transport manager instance to the routing engine,
 * which will be used for managing network transport operations.
 *
 * @param engine            Handle to the routing engine instance.
 * @param transport_manager Handle to the transport manager to be assigned.
 */
void zoo_smb_routing_engine_set_transport_manager(ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                                  ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager)
{
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;
    LOCK_ENGINE(e);
    e->transport_manager = transport_manager;
    UNLOCK_ENGINE(e);

    ZOO_LOG_DEBUG("Transport manager set");
}

/**
 * @brief Sets the rule manager for the routing engine.
 *
 * This function assigns a rule manager instance to the routing engine,
 * which will be used for managing routing rules and policies.
 *
 * @param engine       Handle to the routing engine instance.
 * @param rule_manager Handle to the rule manager to be assigned.
 */
void zoo_smb_routing_engine_set_rule_manager(ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                             ZOO_SMB_RULE_MANAGER_HANDLE rule_manager)
{
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;
    LOCK_ENGINE(e);
    e->rule_manager = rule_manager;
    UNLOCK_ENGINE(e);

    ZOO_LOG_DEBUG("Rule manager set");
}

/**
 * @brief Sets the service manager for the specified SMB routing engine.
 *
 * This function associates a service manager with the given SMB routing engine handle.
 *
 * @param engine The handle to the SMB routing engine instance.
 *
 * @note The service manager must be properly initialized before calling this function.
 */
void zoo_smb_routing_engine_set_service_manager(ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                                ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager)
{
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;
    LOCK_ENGINE(e);
    e->service_manager = service_manager;
    UNLOCK_ENGINE(e);
}

/**
 * @brief Destroys and cleans up resources used by the SMB routing engine.
 *
 * This function properly releases all memory and resources allocated by the
 * routing engine including message queue, message dispatcher, and mutex.
 * Should be called before program termination or when the engine is no longer needed.
 *
 * @param engine Handle to the routing engine to be destroyed.
 */
void zoo_smb_destroy_routing_engine(ZOO_SMB_ROUTING_ENGINE_HANDLE engine)
{
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;
    if (!e)
    {
        ZOO_LOG_ERROR("Invalid engine handle");
        return;
    }

    // Stop the engine if it's running
    if (e->is_running)
    {
        zoo_smb_stop_routing_engine(engine);
    }

    // Clean up message dispatcher and queue
    if (e->message_dispatcher)
    {
        zoo_destroy_dispatcher(e->message_dispatcher);
    }

    if (e->message_queue)
    {
        zoo_queue_exit_blocking(e->message_queue);
        zoo_destroy_queue(e->message_queue);
    }

    // Destroy mutex and free the engine structure
    ZOO_MUTEX_DESTROY(&e->mutex);
    zoo_free_to_pool(e);

    ZOO_LOG_DEBUG("SMB routing engine destroyed successfully");
}

/**
 * @brief Starts the SMB routing engine.
 *
 * This function initializes and starts the routing engine, enabling it to
 * begin processing routing tasks and messages. The engine must be started
 * before it can handle any routing operations.
 *
 * @param engine Handle to the SMB routing engine to be started.
 */
void zoo_smb_start_routing_engine(ZOO_SMB_ROUTING_ENGINE_HANDLE engine)
{
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;
    e->is_running = ZOO_TRUE;
    int retry = 0;
    while (ZOO_SMB_OK != zoo_thread_pool_submit_task("engine_task", engine_task, e, NULL, ZOO_TRUE))
    {
        retry++;
        if (retry >= ROUTING_ENGINE_TASK_SUBMIT_MAX_RETRIES)
        {
            e->is_running = ZOO_FALSE;
            ZOO_LOG_ERROR("Failed to start engine task after %d retries", retry);
            return;
        }
        ZOO_LOG_WARN("Failed to start engine task, retry=%d", retry);
        ZOO_SMB_SLEEP_MS(ROUTING_ENGINE_TASK_SUBMIT_BACKOFF_MS);
    }
    ZOO_LOG_INFO("Routing engine started successfully");
}

/**
 * @brief Stops the SMB routing engine.
 *
 * This function gracefully stops the routing engine, halting message processing
 * and routing operations. The engine can be restarted later if needed.
 *
 * @param engine Handle to the SMB routing engine to be stopped.
 */
void zoo_smb_stop_routing_engine(ZOO_SMB_ROUTING_ENGINE_HANDLE engine)
{
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;

    LOCK_ENGINE(e);
    if (!e->is_running)
    {
        UNLOCK_ENGINE(e);
        ZOO_LOG_WARN("Engine is not running");
        return;
    }

    e->is_running = ZOO_FALSE;
    UNLOCK_ENGINE(e);

    zoo_stop_dispatcher(e->message_dispatcher);
    ZOO_LOG_DEBUG("Routing engine stopped successfully");
}

/**
 * @brief Prepares a routing rule in the SMB routing engine.
 *
 * This function initializes and configures a routing rule by setting up the
 * required transport and service connections. It ensures that all necessary
 * components are available and properly configured before the rule can be used.
 *
 * @param engine            Handle to the SMB routing engine instance.
 * @param transport_manager Handle to the transport manager.
 * @param service_manager   Handle to the service manager.
 * @param rule              Handle to the routing rule to be prepared.
 * @param timeout_ms        Timeout in milliseconds for service discovery.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE zoo_smb_routing_engine_prepare_rule(ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                                       IN ZOO_SMB_RULE_HANDLE rule,
                                                       IN uint32_t timeout_ms)
{
    if (rule == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_LOG_DEBUG("Preparing routing rule: %s, timeout: %d", rule->name, timeout_ms);
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;
    rule->engine = engine;  // Associate the rule with the routing engine
    TRANSPORT_DATA_OBSERVER_HANDLE observer = create_transport_data_observer(handle_transport_incoming_data, rule);
    zoo_smb_transport_manager_register_incoming_data_observer(e->transport_manager, rule->transport, observer);
    return zoo_smb_transport_manager_start_transport(e->transport_manager, rule->transport);
}

/**
 * @brief Allocate and initialize a ROUTING_CONTEXT_STRUCT.
 *
 * @param rule Routing rule handle.
 * @param receiver Receiver string.
 * @param message Message pointer.
 * @param delete_message_after_send Whether to delete the message after sending.
 * @return Pointer to allocated ROUTING_CONTEXT_STRUCT, or NULL on failure.
 */
static ROUTING_CONTEXT_STRUCT* create_routing_context(
    ZOO_SMB_RULE_HANDLE rule,
    ZOO_STRING_T receiver,
    ZOO_SMB_MSG_STRUCT* message,
    ZOO_BOOL delete_message_after_send)
{
    ROUTING_CONTEXT_STRUCT* context = zoo_allocate_from_pool(sizeof(ROUTING_CONTEXT_STRUCT));
    if (!context)
    {
        ZOO_LOG_ERROR("Failed to allocate routing context");
        return NULL;
    }
    memset(context, 0, sizeof(ROUTING_CONTEXT_STRUCT));

    if (receiver)
    {
        ZOO_USIZE_T receiver_len = strlen(receiver) + 1;
        context->receiver_owned = (char*)zoo_allocate_from_pool(receiver_len);
        if (!context->receiver_owned)
        {
            ZOO_LOG_ERROR("Failed to allocate routing receiver");
            zoo_free_to_pool(context);
            return NULL;
        }
        memcpy(context->receiver_owned, receiver, receiver_len);
        context->receiver = context->receiver_owned;
    }

    context->rule = rule;
    context->message = message;
    context->delete_message_flag = delete_message_after_send;
    return context;
}

/**
 * @brief Destroy and free a ROUTING_CONTEXT_STRUCT.
 *
 * @param context Pointer to ROUTING_CONTEXT_STRUCT to free.
 */
static void destroy_routing_context(ROUTING_CONTEXT_STRUCT* context)
{
    if (context)
    {
        if (context->receiver_owned)
        {
            zoo_free_to_pool(context->receiver_owned);
        }
        zoo_free_to_pool(context);
    }
}

/**
 * @brief Internal helper function to handle routed messages.
 *
 * This static function processes messages that have been routed through the engine.
 * It performs the actual message handling logic based on the routing rule.
 *
 * @param engine  Handle to the routing engine.
 * @param rule    Handle to the routing rule applied to the message.
 * @param message Pointer to the message structure to be processed.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 */
static void engine_handle_route_message(void* user_data, void* msg, void* context)
{
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager = (ZOO_SMB_TRANSPORT_MANAGER_HANDLE)user_data;
    const ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;
    ROUTING_CONTEXT_STRUCT* routing_ctx = (ROUTING_CONTEXT_STRUCT*)context;
    if (routing_ctx == NULL || message == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters, routing_ctx: %p, message: %p", routing_ctx, message);
        return;
    }

    if (ZOO_SMB_OK != zoo_smb_transport_manager_send_message(
                          transport_manager, routing_ctx->rule->transport, message, routing_ctx->receiver))
    {
        ZOO_LOG_ERROR("Failed to send message");
    }

    ZOO_LOG_DEBUG("delete_message_flag: %s", routing_ctx->delete_message_flag ? "ZOO_TRUE" : "ZOO_FALSE");
    if (routing_ctx->delete_message_flag)
    {
        zoo_smb_destroy_message((ZOO_SMB_MSG_STRUCT*)message);  // Free the message payload if needed
    }

    destroy_routing_context(context);  // Free the routing context
}

/**
 * @brief Handles message routing through the specified routing engine and rule.
 *
 * This function enqueues a message for processing by the routing engine,
 * applying the specified routing rule. The message will be processed
 * asynchronously by the message dispatcher.
 *
 * @param engine  Handle to the routing engine.
 * @param rule    Handle to the routing rule to be applied.
 * @param message Pointer to the message structure to be routed.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 */
ZOO_ERROR_T zoo_smb_routing_engine_handle_route_message(ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                                               IN ZOO_SMB_RULE_HANDLE rule,
                                                               IN const ZOO_SMB_MSG_STRUCT* message,
                                                               IN ZOO_STRING_T receiver,
                                                               IN ZOO_BOOL delete_message_after_send)
{
    ZOO_LOG_DEBUG("Routing message use: %s, delete_message_after_send: %d",
                      rule->name,
                      delete_message_after_send);
    if (rule == NULL || message == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters, rule: %p, message: %p", rule, message);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;
    if (!e->is_running)
    {
        ZOO_LOG_ERROR("Engine is not running");
        return ZOO_SMB_ERROR_NOT_INITIALIZED;
    }

    ROUTING_CONTEXT_STRUCT* context = create_routing_context(rule, receiver, (ZOO_SMB_MSG_STRUCT*)message, delete_message_after_send);
    if (!context)
    {
        ZOO_LOG_ERROR("Failed to allocate routing context");
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    if (!zoo_queue_enqueue(e->message_queue, (ZOO_SMB_MSG_STRUCT*)message, context, engine_handle_route_message, e->transport_manager))
    {
        ZOO_LOG_ERROR("Failed to enqueue message for routing");
        destroy_routing_context(context);  // Free the context if enqueue fails
        return ZOO_SMB_ERROR_QUEUE_FULL;
    }
    return ZOO_SMB_OK;
}

ZOO_ERROR_TYPE zoo_smb_routing_engine_get_metrics(
    IN ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
    OUT ZOO_SMB_ROUTING_ENGINE_METRICS_STRUCT* out_metrics)
{
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;
    if (!e || !out_metrics)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    out_metrics->ingress_received = (uint64_t)atomic_load(&e->ingress_received);
    out_metrics->ingress_enqueued = (uint64_t)atomic_load(&e->ingress_enqueued);
    out_metrics->ingress_dequeued = (uint64_t)atomic_load(&e->ingress_dequeued);
    out_metrics->ingress_dropped_total = (uint64_t)atomic_load(&e->ingress_dropped_total);
    out_metrics->ingress_drop_queue_full = (uint64_t)atomic_load(&e->ingress_drop_queue_full);
    out_metrics->ingress_drop_backpressure = (uint64_t)atomic_load(&e->ingress_drop_backpressure);
    out_metrics->ingress_drop_invalid_header = (uint64_t)atomic_load(&e->ingress_drop_invalid_header);
    out_metrics->ingress_drop_security_policy = (uint64_t)atomic_load(&e->ingress_drop_security_policy);
    out_metrics->ingress_in_flight = (uint32_t)atomic_load(&e->ingress_in_flight);
    out_metrics->ingress_high_watermark = e->ingress_high_watermark;
    out_metrics->ingress_low_watermark = e->ingress_low_watermark;
    out_metrics->ingress_backpressure_active = atomic_load(&e->ingress_backpressure_active);

    return ZOO_SMB_OK;
}

ZOO_ERROR_TYPE zoo_smb_routing_engine_reset_metrics(
    IN ZOO_SMB_ROUTING_ENGINE_HANDLE engine)
{
    ZOO_SMB_ROUTING_ENGINE_STRUCT* e = (ZOO_SMB_ROUTING_ENGINE_STRUCT*)engine;
    if (!e)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    atomic_store(&e->ingress_received, 0);
    atomic_store(&e->ingress_enqueued, 0);
    atomic_store(&e->ingress_dequeued, 0);
    atomic_store(&e->ingress_dropped_total, 0);
    atomic_store(&e->ingress_drop_queue_full, 0);
    atomic_store(&e->ingress_drop_backpressure, 0);
    atomic_store(&e->ingress_drop_invalid_header, 0);
    atomic_store(&e->ingress_drop_security_policy, 0);
    return ZOO_SMB_OK;
}
