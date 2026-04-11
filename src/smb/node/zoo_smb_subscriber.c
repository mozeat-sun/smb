/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SUBSCRIBER
 * File name: zoo_smb_subscriber.c
 * Description: Implementation of subscriber node for ZOO Soft Message Bus (SMB)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-28     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_subscriber.h"
#include "zoo_smb_node.h"
#include "zoo_smb_qos.h"
#include "zoo_smb.h"
#include "zoo_smb_service_observer.h"
#include "zoo_smb_subscription_session.h"
#include "zoo_smb_subscription_session_manager.h"
#include "zoo_thread_pool.h"
#include "zoo.h"
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>

#define SUBSCRIBER_RECONCILE_SUBMIT_RETRIES 16U
#define SUBSCRIBER_RECONCILE_SUBMIT_BACKOFF_MS 10U
#define SUBSCRIBER_RECONCILE_IDLE_SLEEP_MS 100U

typedef struct ZOO_SMB_SUBSCRIBER_STRUCT
{
    int32_t id[2];
    ZOO_SMB_NODE_HANDLE node;
    ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE session_manager;
    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity;
    ZOO_MUTEX_T mutex;
    ZOO_SMB_SERVICE_OBSERVER_HANDLE service_observer;
    atomic_bool running;
    atomic_bool reconcile_active;
    atomic_bool reconcile_requested;
} ZOO_SMB_SUBSCRIBER_STRUCT;

static void subscriber_request_reconcile(ZOO_SMB_SUBSCRIBER_STRUCT* subscriber);
static ZOO_ERROR_T subscriber_reconcile_task(void* user_data, void* argument);
static void subscriber_on_service_change_cb(const ZOO_SMB_SERVICE_HANDLE service, void* user_data);

/**
 * @brief Dispatches PUB payload to a session when message id matches.
 */
static void subscriber_notify_session(
    IN const ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session,
    IN const ZOO_SMB_MSG_STRUCT* message)
{
    if (!session || !message)
    {
        return;
    }

    if (session->handler != NULL && message->header.msg_id == session->msg_id)
    {
        session->handler(
            session->usr_data,
            message->header.msg_id,
            message->header.request_id,
            message->header.timestamp,
            message->header.sender,
            message->payload,
            message->header.payload_size);
    }
}

/**
 * @brief Handles incoming SUBACK and updates QoS/session state.
 */
static void subscriber_on_handle_SUBACK_msg_cb(IN void* context, IN const void* msg, IN size_t msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p, msg_len=%zu", msg, context, msg_len);
        return;
    }

    ZOO_SMB_SUBSCRIBER_STRUCT* subscriber = (ZOO_SMB_SUBSCRIBER_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;

    ZOO_LOG_INFO("Received SUBACK message for subscriber: %s, request_id=%llu",
                 subscriber->node->name,
                 message->header.request_id);

    if (subscriber->qos_entity)
    {
        ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(subscriber->qos_entity);
        ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT negotiation_result = {ZOO_TRUE, 0};
        if (message->payload && message->header.payload_size >= sizeof(ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT))
        {
            memcpy(&negotiation_result, message->payload, sizeof(ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT));
        }

        (void)zoo_smb_qos_ctx_set_match_result(
            qos_ctx,
            message->header.request_id,
            negotiation_result.matched,
            negotiation_result.incompatibility_mask);

        if (negotiation_result.matched)
        {
            zoo_smb_qos_ctx_set_msg_status(qos_ctx, message->header.request_id, ZOO_SMB_MSG_ST_ACKED);
        }
        else
        {
            zoo_smb_qos_ctx_set_msg_status(qos_ctx, message->header.request_id, ZOO_SMB_MSG_ST_FAILED);
        }

        ZOO_MUTEX_LOCK(&subscriber->mutex);
        zoo_smb_subscription_session_manager_on_suback(
            subscriber->session_manager,
            message->header.request_id,
            negotiation_result.matched);
        ZOO_MUTEX_UNLOCK(&subscriber->mutex);
    }
}

/**
 * @brief Sends PUBACK for reliability-enabled subscriber delivery.
 */
static void subscriber_send_ack(ZOO_SMB_SUBSCRIBER_STRUCT* subscriber, uint32_t msg_id, uint64_t request_id, const char* receiver)
{
    ZOO_SMB_MSG_STRUCT* message = zoo_smb_create_message(
        ZOO_SMB_MSG_TYPE_PUBACK,
        subscriber->node->target,
        subscriber->node->topic,
        NULL,
        0,
        msg_id,
        request_id);
    if (message == NULL)
    {
        ZOO_LOG_ERROR("Failed to create request acknowledgment message for request_id=%llu", request_id);
        return;
    }

    zoo_smb_send_message(subscriber->node, message, receiver, ZOO_TRUE);
}

/**
 * @brief Handles incoming PUB and forwards to matching sessions.
 */
static void subscriber_on_handle_PUB_msg_cb(IN void* context, IN const void* msg, IN size_t msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p, msg_len=%zu", msg, context, msg_len);
        return;
    }

    ZOO_SMB_SUBSCRIBER_STRUCT* subscriber = (ZOO_SMB_SUBSCRIBER_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;

    ZOO_LOG_INFO("Received PUB message: msg_id=%u, req_id=%llu, payload_size=%zu",
                 message->header.msg_id,
                 message->header.request_id,
                 message->header.payload_size);

    if (subscriber->qos_entity)
    {
        ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(subscriber->qos_entity);
        if (qos_policy->reliability_enabled)
        {
            subscriber_send_ack(subscriber, message->header.msg_id, message->header.request_id, message->header.sender);
        }
    }

    ZOO_MUTEX_LOCK(&subscriber->mutex);
    zoo_smb_subscription_session_manager_visit(subscriber->session_manager, subscriber_notify_session, message);
    ZOO_MUTEX_UNLOCK(&subscriber->mutex);
}

/**
 * @brief Creates or reactivates a subscription session and returns stable handle.
 */
static ZOO_ERROR_TYPE subscriber_register_session(
    IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER handler,
    IN void* user_data,
    OUT int32_t* handle)
{
    if (subscriber == NULL || handler == NULL || handle == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE existing =
        zoo_smb_subscription_session_manager_find_by_handler(subscriber->session_manager, handler);
    if (existing != NULL)
    {
        *handle = existing->id;
        existing->usr_data = user_data;
        existing->msg_id = msg_id;
        existing->desired_active = ZOO_TRUE;
        return ZOO_SMB_OK;
    }

    int32_t session_id = (int32_t)(zoo_generate_uuid64() & 0x7FFFFFFF);
    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session = zoo_smb_subscription_session_create(
        msg_id,
        handler,
        user_data,
        session_id,
        zoo_get_timestamp_milliseconds(),
        zoo_smb_service_is_available(subscriber->node->target));
    if (!session)
    {
        ZOO_LOG_ERROR("Failed to create session for subscriber: %s", subscriber->node->name);
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    ZOO_ERROR_TYPE add_result = zoo_smb_subscription_session_manager_add(subscriber->session_manager, session);
    if (add_result != ZOO_SMB_OK)
    {
        zoo_smb_subscription_session_destroy(session);
        return add_result;
    }

    *handle = session_id;
    ZOO_LOG_DEBUG("Registered subscriber session: %s, topic: %s, handle: %u",
                  subscriber->node->name,
                  subscriber->node->topic,
                  *handle);
    return ZOO_SMB_OK;
}

/**
 * @brief Checks whether a matching session already exists.
 */
static ZOO_BOOL subscriber_has_session(
    IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER handler)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
        zoo_smb_subscription_session_manager_find_by_handler(subscriber->session_manager, handler);
    return zoo_smb_subscription_session_matches_handler(session, msg_id, handler);
}

/**
 * @brief Applies service availability updates to subscriber sessions.
 */
static void subscriber_on_service_change_cb(const ZOO_SMB_SERVICE_HANDLE service, void* user_data)
{
    ZOO_SMB_SUBSCRIBER_STRUCT* subscriber = (ZOO_SMB_SUBSCRIBER_STRUCT*)user_data;
    if (!subscriber || !service)
    {
        return;
    }

    if (strcmp(service->name, subscriber->node->target) != 0)
    {
        return;
    }

    ZOO_MUTEX_LOCK(&subscriber->mutex);
    zoo_smb_subscription_session_manager_on_service_change(
        subscriber->session_manager,
        subscriber->qos_entity,
        service->is_online,
        zoo_get_timestamp_milliseconds(),
        subscriber->node);
    ZOO_MUTEX_UNLOCK(&subscriber->mutex);
    subscriber_request_reconcile(subscriber);
}

/**
 * @brief Schedules asynchronous reconcile work, coalescing duplicate requests.
 */
static void subscriber_request_reconcile(ZOO_SMB_SUBSCRIBER_STRUCT* subscriber)
{
    atomic_store(&subscriber->reconcile_requested, true);
    if (atomic_exchange(&subscriber->reconcile_active, true))
    {
        return;
    }

    for (uint32_t retry = 0; retry < SUBSCRIBER_RECONCILE_SUBMIT_RETRIES; ++retry)
    {
        if (zoo_thread_pool_submit_task("subscriber_reconcile", subscriber_reconcile_task, subscriber, NULL, ZOO_TRUE) == ZOO_SMB_OK)
        {
            return;
        }
        usleep(SUBSCRIBER_RECONCILE_SUBMIT_BACKOFF_MS * 1000U);
    }

    atomic_store(&subscriber->reconcile_active, false);
    ZOO_LOG_ERROR("Failed to submit subscriber reconcile task for subscriber: %s", subscriber->node->name);
}

/**
 * @brief Worker task driving subscription state reconciliation loop.
 */
static ZOO_ERROR_T subscriber_reconcile_task(void* user_data, void* argument)
{
    ZOO_SMB_SUBSCRIBER_STRUCT* subscriber = (ZOO_SMB_SUBSCRIBER_STRUCT*)user_data;
    ZOO_SMB_UNUSED(argument);
    if (!subscriber)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    while (atomic_load(&subscriber->running))
    {
        ZOO_BOOL has_pending_work = ZOO_FALSE;
        uint32_t sleep_ms = SUBSCRIBER_RECONCILE_IDLE_SLEEP_MS;
        atomic_store(&subscriber->reconcile_requested, false);

        ZOO_MUTEX_LOCK(&subscriber->mutex);
        zoo_smb_subscription_session_manager_reconcile(
            subscriber->session_manager,
            subscriber->node,
            subscriber->qos_entity,
            zoo_smb_service_is_available(subscriber->node->target),
            zoo_get_timestamp_milliseconds(),
            &has_pending_work,
            &sleep_ms);
        ZOO_MUTEX_UNLOCK(&subscriber->mutex);

        if (!has_pending_work && !atomic_load(&subscriber->reconcile_requested))
        {
            break;
        }

        if (sleep_ms > 0U)
        {
            usleep((useconds_t)sleep_ms * 1000U);
        }
    }

    atomic_store(&subscriber->reconcile_active, false);
    if (atomic_load(&subscriber->running) && atomic_load(&subscriber->reconcile_requested))
    {
        subscriber_request_reconcile(subscriber);
    }
    return ZOO_OK;
}

/**
 * @brief Creates a subscriber node and initializes session/QoS observers.
 */
ZOO_SMB_SUBSCRIBER_HANDLE zoo_smb_create_subscriber(
    IN const char* name,
    IN const char* target,
    IN const char* topic,
    IN const ZOO_SMB_QOS_POLICY_STRUCT* policy)
{
    if (topic == NULL)
    {
        ZOO_LOG_ERROR("Topic cannot be NULL");
        return NULL;
    }

    ZOO_SMB_SUBSCRIBER_STRUCT* subscriber =
        (ZOO_SMB_SUBSCRIBER_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_SUBSCRIBER_STRUCT));
    if (!subscriber)
    {
        return NULL;
    }

    subscriber->node = zoo_smb_create_node(name, target, topic, ZOO_SMB_NODE_TYPE_SUBSCRIBER, ZOO_SMB_TRANSPORT_TYPE_DEFAULT);
    if (NULL == subscriber->node)
    {
        ZOO_LOG_ERROR("Fail to create subscriber: %s", name);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    subscriber->session_manager = zoo_smb_subscription_session_manager_create();
    if (!subscriber->session_manager)
    {
        ZOO_LOG_ERROR("Failed to create session manager for subscriber: %s", name);
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_HANDLE pub_observer = zoo_smb_create_node_observer_and_insert(
        subscriber->node->observers,
        subscriber->node->name,
        0,
        ZOO_SMB_MSG_TYPE_PUB,
        subscriber_on_handle_PUB_msg_cb,
        subscriber,
        &subscriber->id[0]);
    if (pub_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create subscriber pub_observer");
        zoo_smb_subscription_session_manager_destroy(subscriber->session_manager, NULL);
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_HANDLE suback_observer = zoo_smb_create_node_observer_and_insert(
        subscriber->node->observers,
        subscriber->node->name,
        0,
        ZOO_SMB_MSG_TYPE_SUBACK,
        subscriber_on_handle_SUBACK_msg_cb,
        subscriber,
        &subscriber->id[1]);
    if (suback_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create subscriber suback_observer");
        zoo_smb_subscription_session_manager_destroy(subscriber->session_manager, NULL);
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    subscriber->qos_entity = policy ? zoo_smb_create_qos_entity(policy) : zoo_smb_create_qos_default_entity();
    if (subscriber->qos_entity == NULL)
    {
        ZOO_LOG_ERROR("Failed to create QoS entity for subscriber: %s", name);
        zoo_smb_subscription_session_manager_destroy(subscriber->session_manager, NULL);
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    if (ZOO_SMB_OK != zoo_smb_register_node(subscriber->node))
    {
        ZOO_LOG_ERROR("Failed to register node");
        zoo_smb_subscription_session_manager_destroy(subscriber->session_manager, subscriber->qos_entity);
        zoo_smb_destroy_qos_entity(subscriber->qos_entity);
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    ZOO_MUTEX_INIT(&subscriber->mutex);
    atomic_init(&subscriber->running, true);
    atomic_init(&subscriber->reconcile_active, false);
    atomic_init(&subscriber->reconcile_requested, false);

    subscriber->service_observer = zoo_smb_create_service_observer(subscriber_on_service_change_cb, subscriber);
    if (!subscriber->service_observer || zoo_smb_add_service_observer(subscriber->service_observer) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("Failed to register subscriber service observer for subscriber: %s", name);
        if (subscriber->service_observer)
        {
            zoo_smb_destroy_service_observer(subscriber->service_observer);
        }
        ZOO_MUTEX_DESTROY(&subscriber->mutex);
        zoo_smb_subscription_session_manager_destroy(subscriber->session_manager, subscriber->qos_entity);
        zoo_smb_destroy_qos_entity(subscriber->qos_entity);
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    ZOO_LOG_DEBUG("Created subscriber node: %s, target: %s, topic: %s", name, target, topic);
    return subscriber;
}

/**
 * @brief Stops reconcile flow and releases subscriber-owned resources.
 */
void zoo_smb_destroy_subscriber(IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber)
{
    ZOO_LOG_DEBUG("Destroying subscriber node: %p", subscriber);
    if (subscriber == NULL)
    {
        ZOO_LOG_ERROR("Invalid node handle: %p", subscriber);
        return;
    }

    atomic_store(&subscriber->running, false);
    atomic_store(&subscriber->reconcile_requested, false);
    zoo_smb_remove_service_observer(subscriber->service_observer);

    for (uint32_t i = 0; i < 200U && atomic_load(&subscriber->reconcile_active); ++i)
    {
        usleep(10U * 1000U);
    }

    if (subscriber->service_observer)
    {
        zoo_smb_destroy_service_observer(subscriber->service_observer);
    }

    ZOO_MUTEX_DESTROY(&subscriber->mutex);
    zoo_smb_subscription_session_manager_destroy(subscriber->session_manager, subscriber->qos_entity);
    zoo_smb_destroy_qos_entity(subscriber->qos_entity);
    zoo_smb_destroy_node(subscriber->node);
    zoo_free_to_pool(subscriber);
}

/**
 * @brief Registers or reactivates a subscription session for a message id.
 *
 * This API records subscriber intent immediately and then schedules background
 * reconciliation to negotiate SUB/SUBACK with the target service. The call does
 * not block for remote compatibility outcome.
 *
 * Behavior summary:
 * - If a session with the same handler and msg_id already exists, the existing
 *   handle is returned and desired_active is re-enabled.
 * - If the handler exists with a different msg_id, the same session is reused,
 *   its message filter is updated, and negotiation is retriggered.
 * - For a new handler, a new stable session handle is created and returned.
 *
 * @param subscriber Subscriber handle owning session state.
 * @param msg_id Message identifier to receive from PUB traffic.
 * @param message_handler User callback invoked for matching messages.
 * @param user_data Opaque context passed to message_handler.
 * @param handle Output stable session handle used by unsubscribe.
 * @return ZOO_SMB_OK when intent is accepted; otherwise an error code such as
 *         ZOO_SMB_ERROR_INVALID_PARAM, ZOO_SMB_ERROR_OUT_OF_MEMORY, or list/
 *         manager insertion failures.
 */
ZOO_ERROR_TYPE zoo_smb_subscribe_message(
    IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER message_handler,
    IN void* user_data,
    OUT int32_t* handle)
{
    if (subscriber == NULL || message_handler == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: node=%p, msg_id=%d, handler=%p", subscriber, msg_id, message_handler);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_ERROR_TYPE result = ZOO_SMB_OK;
    ZOO_MUTEX_LOCK(&subscriber->mutex);
    if (subscriber_has_session(subscriber, msg_id, message_handler))
    {
        ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE existing =
            zoo_smb_subscription_session_manager_find_by_handler(subscriber->session_manager, message_handler);
        if (existing && handle)
        {
            *handle = existing->id;
            existing->desired_active = ZOO_TRUE;
        }
        ZOO_MUTEX_UNLOCK(&subscriber->mutex);
        subscriber_request_reconcile(subscriber);
        return ZOO_SMB_OK;
    }

    result = subscriber_register_session(subscriber, msg_id, message_handler, user_data, handle);
    ZOO_MUTEX_UNLOCK(&subscriber->mutex);

    if (result != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("Failed to register session for subscriber: %s", subscriber->node->name);
        return result;
    }

    subscriber_request_reconcile(subscriber);
    ZOO_LOG_INFO("Subscription intent registered: topic=%s, msg_id=%u, handle=%d. Negotiation continues asynchronously.",
                 subscriber->node->topic,
                 msg_id,
                 *handle);
    return ZOO_SMB_OK;
}

/**
 * @brief Cancels and removes one subscription session by handle.
 */
void zoo_smb_unsubscribe_message(IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber, IN int32_t handle)
{
    if (!subscriber)
    {
        ZOO_LOG_ERROR("Invalid node handle: %p", subscriber);
        return;
    }

    ZOO_MUTEX_LOCK(&subscriber->mutex);
    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
        zoo_smb_subscription_session_manager_find_by_id(subscriber->session_manager, handle);
    if (session == NULL)
    {
        ZOO_MUTEX_UNLOCK(&subscriber->mutex);
        ZOO_LOG_ERROR("Session not found for handle: %d", handle);
        return;
    }

    zoo_smb_subscription_session_mark_cancelled(session);
    zoo_smb_subscription_session_manager_remove(subscriber->session_manager, subscriber->qos_entity, session);
    ZOO_MUTEX_UNLOCK(&subscriber->mutex);

    ZOO_LOG_INFO("Unsubscribed from topic: %s, handle: %d", subscriber->node->topic, handle);
}
