/* ******************************************************************************
 * Copyright (C) 2026, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER
 * File name: zoo_smb_subscription_session_manager.c
 * Description: Subscription session manager implementation for subscriber-side
 *              negotiation, retries, and lifecycle reconciliation.
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-11     github.copilot    add file/function comments
 ****************************************************************************** */

#include "zoo_smb_subscription_session_manager.h"
#include "zoo_memory_pool.h"
#include "zoo.h"

typedef struct ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_STRUCT
{
    ZOO_LIST_HANDLE sessions;
} ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_STRUCT;

/**
 * @brief Returns retry interval configured by QoS, or a safe default.
 */
/**
 * @brief Get the retry interval (ms) for subscription negotiation from QoS policy.
 *
 * Returns the retry interval as configured in the QoS policy for the given entity.
 * If not set, returns a safe default (1000 ms).
 *
 * @param qos_entity Handle to the QoS entity.
 * @return Retry interval in milliseconds.
 */
static uint32_t manager_get_retry_interval_ms(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity)
{
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(qos_entity);
    if (qos_policy && qos_policy->reliability.retry_interval_ms > 0)
    {
        return (uint32_t)qos_policy->reliability.retry_interval_ms;
    }
    return 1000U;
}

/**
 * @brief Returns SUBACK wait timeout configured by QoS, or a safe default.
 */
/**
 * @brief Get the SUBACK wait timeout (ms) from QoS policy.
 *
 * Returns the maximum blocking time for SUBACK as configured in the QoS policy for the given entity.
 * If not set, returns a safe default (5000 ms).
 *
 * @param qos_entity Handle to the QoS entity.
 * @return Wait timeout in milliseconds.
 */
static uint32_t manager_get_wait_timeout_ms(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity)
{
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(qos_entity);
    if (qos_policy && qos_policy->reliability.max_blocking_time_ms > 0)
    {
        return (uint32_t)qos_policy->reliability.max_blocking_time_ms;
    }
    return 5000U;
}

/**
 * @brief Computes capped exponential retry delay from QoS policy.
 *
 * Delay growth is capped to avoid unbounded backoff and preserve responsiveness.
 */
/**
 * @brief Compute capped exponential backoff delay for retries.
 *
 * Calculates the retry delay using exponential backoff, capped by the SUBACK wait timeout.
 *
 * @param qos_entity    Handle to the QoS entity.
 * @param retry_attempts Number of retry attempts so far.
 * @return Delay in milliseconds before next retry.
 */
static uint32_t manager_get_backoff_delay_ms(
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN uint32_t retry_attempts)
{
    /* Exponential backoff with capped growth to keep retries bounded and predictable. */
    uint32_t base_delay = manager_get_retry_interval_ms(qos_entity);
    uint32_t multiplier = 1U;
    if (retry_attempts > 0)
    {
        uint32_t shift = retry_attempts > 3U ? 3U : retry_attempts;
        multiplier = (1U << shift);
    }

    uint64_t delay = (uint64_t)base_delay * multiplier;
    uint32_t max_delay = manager_get_wait_timeout_ms(qos_entity);
    if (delay > max_delay)
    {
        delay = max_delay;
    }
    return (uint32_t)delay;
}

/**
 * @brief Clears QoS request tracking and resets session in-flight request id.
 */
/**
 * @brief Reset the in-flight request state for a session.
 *
 * Removes any request state from the QoS context and clears the session's request_id.
 *
 * @param qos_entity Handle to the QoS entity.
 * @param session    Handle to the subscription session.
 */
static void manager_reset_session_request(
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session)
{
    /* Request state is removed from QoS context before reusing session->request_id. */
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(qos_entity);
    if (session->request_id != 0 && qos_ctx)
    {
        zoo_smb_qos_ctx_remove_state(qos_ctx, session->request_id);
    }

    session->request_id = 0;
}

/**
 * @brief Sends a SUB negotiation message for one session.
 *
 * On success the session moves to WAITING_SUBACK. On failure the session is
 * degraded or pending-service, and next retry time is scheduled.
 */
/**
 * @brief Send a SUB negotiation message for a session and update its state.
 *
 * Allocates a new request_id, builds and sends a SUB message, and updates the session state.
 * Handles error and retry logic, including degraded and pending-service states.
 *
 * @param node        Handle to the node.
 * @param qos_entity  Handle to the QoS entity.
 * @param session     Handle to the subscription session.
 * @param now_ms      Current time in milliseconds.
 * @return ZOO_TRUE on success, ZOO_FALSE on failure.
 */
static ZOO_BOOL manager_send_subscription(
    IN ZOO_SMB_NODE_HANDLE node,
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session,
    IN uint64_t now_ms)
{
    /* Sending SUB is the only state transition that allocates a new request_id. */
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(qos_entity);
    if (!qos_ctx)
    {
        session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED;
        return ZOO_FALSE;
    }

    manager_reset_session_request(qos_entity, session);

    uint64_t request_id = zoo_generate_uuid64();
    ZOO_SMB_MSG_STRUCT* msg = zoo_smb_qos_build_msg(
        qos_entity,
        node->name,
        node->topic,
        ZOO_SMB_MSG_TYPE_SUB,
        session->msg_id,
        request_id);
    if (!msg)
    {
        session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED;
        return ZOO_FALSE;
    }

    if (zoo_smb_qos_ctx_set_state(qos_ctx, request_id, ZOO_SMB_MSG_ST_SENDING, NULL) != ZOO_SMB_OK)
    {
        zoo_smb_destroy_message(msg);
        session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED;
        return ZOO_FALSE;
    }
    zoo_smb_qos_ctx_set_msg_status(qos_ctx, request_id, ZOO_SMB_MSG_ST_WAITING_ACK);

    session->request_id = request_id;
    session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    session->retry_attempts += 1U;
    session->next_retry_at_ms = now_ms + manager_get_wait_timeout_ms(qos_entity);

    ZOO_ERROR_TYPE send_result = zoo_smb_send_message(node, msg, NULL, ZOO_TRUE);
    if (send_result == ZOO_SMB_OK)
    {
        ZOO_LOG_INFO("Subscription negotiation started: topic=%s, msg_id=%u, request_id=%llu, handle=%d",
                     node->topic,
                     session->msg_id,
                     request_id,
                     session->id);
        return ZOO_TRUE;
    }

    manager_reset_session_request(qos_entity, session);
    /* Transport/service failure degrades session and schedules bounded retry. */
    node->active = ZOO_FALSE;
    session->state = (send_result == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SERVICE_NOT_FOUND)
                         ? ZOO_SMB_SUBSCRIPTION_SESSION_STATE_PENDING_SERVICE
                         : ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED;
    session->next_retry_at_ms = now_ms + manager_get_backoff_delay_ms(qos_entity, session->retry_attempts);
    return ZOO_FALSE;
}

/**
 * @brief Creates a manager instance and backing session list.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE zoo_smb_subscription_session_manager_create(void)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager =
        (ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_STRUCT));
    if (!manager)
    {
        return NULL;
    }

    manager->sessions = zoo_list_create(MAX_NODE_OBSERVER_SIZE);
    if (!manager->sessions)
    {
        zoo_free_to_pool(manager);
        return NULL;
    }

    return manager;
}

/**
 * @brief Destroys manager and all owned sessions.
 *
 * Any in-flight request state is removed from QoS context before session free.
 */
void zoo_smb_subscription_session_manager_destroy(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity)
{
    if (!manager)
    {
        return;
    }

    while (!zoo_list_empty(manager->sessions))
    {
        ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
            (ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE)zoo_list_pop_front(manager->sessions);
        if (session)
        {
            manager_reset_session_request(qos_entity, session);
            zoo_smb_subscription_session_destroy(session);
        }
    }
    zoo_list_destroy(manager->sessions);
    zoo_free_to_pool(manager);
}

/**
 * @brief Finds the first session bound to the specified message handler.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE zoo_smb_subscription_session_manager_find_by_handler(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_MSG_HANDLER handler)
{
    if (!manager || !handler)
    {
        return NULL;
    }

    for (size_t i = 0; i < zoo_list_size(manager->sessions); ++i)
    {
        ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
            (ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE)zoo_list_at(manager->sessions, i);
        if (session && session->handler == handler)
        {
            return session;
        }
    }
    return NULL;
}

/**
 * @brief Finds a session by stable API handle id.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE zoo_smb_subscription_session_manager_find_by_id(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN int32_t id)
{
    if (!manager)
    {
        return NULL;
    }

    for (size_t i = 0; i < zoo_list_size(manager->sessions); ++i)
    {
        ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
            (ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE)zoo_list_at(manager->sessions, i);
        if (session && session->id == id)
        {
            return session;
        }
    }
    return NULL;
}

/**
 * @brief Finds a session by current in-flight SUB request id.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE zoo_smb_subscription_session_manager_find_by_request_id(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN uint64_t request_id)
{
    if (!manager || request_id == 0)
    {
        return NULL;
    }

    for (size_t i = 0; i < zoo_list_size(manager->sessions); ++i)
    {
        ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
            (ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE)zoo_list_at(manager->sessions, i);
        if (session && session->request_id == request_id)
        {
            return session;
        }
    }
    return NULL;
}

/**
 * @brief Adds a session to manager ownership.
 */
ZOO_ERROR_TYPE zoo_smb_subscription_session_manager_add(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session)
{
    if (!manager || !session)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    return zoo_list_push_back(manager->sessions, session);
}

/**
 * @brief Removes and destroys one owned session.
 */
void zoo_smb_subscription_session_manager_remove(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session)
{
    if (!manager || !session)
    {
        return;
    }

    manager_reset_session_request(qos_entity, session);
    (void)zoo_list_remove(manager->sessions, session);
    zoo_smb_subscription_session_destroy(session);
}

/**
 * @brief Visits all sessions and invokes caller-provided callback.
 */
void zoo_smb_subscription_session_manager_visit(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_SUBSCRIPTION_SESSION_VISITOR visitor,
    IN const ZOO_SMB_MSG_STRUCT* message)
{
    if (!manager || !visitor)
    {
        return;
    }

    for (size_t i = 0; i < zoo_list_size(manager->sessions); ++i)
    {
        ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
            (ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE)zoo_list_at(manager->sessions, i);
        if (session)
        {
            visitor(session, message);
        }
    }
}

/**
 * @brief Applies service availability changes to desired-active sessions.
 *
 * Service loss clears in-flight request ownership and marks sessions degraded.
 * Service recovery schedules immediate retry.
 */
void zoo_smb_subscription_session_manager_on_service_change(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN ZOO_BOOL service_available,
    IN uint64_t now_ms,
    IN ZOO_SMB_NODE_HANDLE node)
{
    if (!manager || !node)
    {
        return;
    }

    for (size_t i = 0; i < zoo_list_size(manager->sessions); ++i)
    {
        ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
            (ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE)zoo_list_at(manager->sessions, i);
        if (!session || !session->desired_active)
        {
            continue;
        }

        if (!service_available)
        {
            /* Service loss invalidates in-flight request ownership and forces degraded state. */
            manager_reset_session_request(qos_entity, session);
            node->active = ZOO_FALSE;
            if (session->state != ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED)
            {
                session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED;
                session->next_retry_at_ms = now_ms + manager_get_retry_interval_ms(qos_entity);
            }
            continue;
        }

        if (session->state == ZOO_SMB_SUBSCRIPTION_SESSION_STATE_PENDING_SERVICE ||
            session->state == ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED)
        {
            /* Service recovery triggers immediate reconcile attempt. */
            session->next_retry_at_ms = now_ms;
        }
    }
}

/**
 * @brief Handles SUBACK result for the request-matching session.
 */
void zoo_smb_subscription_session_manager_on_suback(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN uint64_t request_id,
    IN ZOO_BOOL matched)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
        zoo_smb_subscription_session_manager_find_by_request_id(manager, request_id);
    if (!session)
    {
        return;
    }

    if (matched)
    {
        session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ACTIVE;
        session->retry_attempts = 0;
        session->next_retry_at_ms = 0;
    }
    else
    {
        session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED;
    }
}

/**
 * @brief Reconciles desired session intent with observed runtime state.
 *
 * This function drives state transitions, retry scheduling, and wait timing
 * for asynchronous subscription negotiation.
 */
void zoo_smb_subscription_session_manager_reconcile(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_NODE_HANDLE node,
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN ZOO_BOOL service_available,
    IN uint64_t now_ms,
    OUT ZOO_BOOL* has_pending_work,
    OUT uint32_t* sleep_ms)
{
    if (!manager || !node || !has_pending_work || !sleep_ms)
    {
        return;
    }

    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(qos_entity);
    uint32_t local_sleep_ms = manager_get_retry_interval_ms(qos_entity);
    ZOO_BOOL pending = ZOO_FALSE;

    /* Reconcile always starts by aligning session state with latest service availability. */
    zoo_smb_subscription_session_manager_on_service_change(
        manager, qos_entity, service_available, now_ms, node);

    size_t session_count = zoo_list_size(manager->sessions);
    for (size_t i = 0; i < session_count; ++i)
    {
        ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
            (ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE)zoo_list_at(manager->sessions, i);
        if (!session || !session->desired_active)
        {
            continue;
        }

        switch (session->state)
        {
            case ZOO_SMB_SUBSCRIPTION_SESSION_STATE_INIT:
            case ZOO_SMB_SUBSCRIPTION_SESSION_STATE_PENDING_SERVICE:
            case ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED:
                if (!service_available)
                {
                    pending = ZOO_TRUE;
                    break;
                }

                if (session->next_retry_at_ms <= now_ms)
                {
                    (void)manager_send_subscription(
                        node, qos_entity, session, now_ms);
                }
                pending = (
                    session->state != ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ACTIVE &&
                    session->state != ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED);
                break;

            case ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK:
                if (!qos_ctx)
                {
                    session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED;
                    break;
                }

                if (session->request_id == 0)
                {
                    session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED;
                    session->next_retry_at_ms =
                        now_ms + manager_get_retry_interval_ms(qos_entity);
                    pending = ZOO_TRUE;
                    break;
                }

                switch (zoo_smb_qos_ctx_get_msg_status(qos_ctx, session->request_id))
                {
                    case ZOO_SMB_MSG_ST_ACKED:
                        session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ACTIVE;
                        session->retry_attempts = 0;
                        manager_reset_session_request(qos_entity, session);
                        break;

                    case ZOO_SMB_MSG_ST_FAILED:
                    {
                        uint32_t incompatibility_mask =
                            zoo_smb_qos_ctx_get_incompatibility_mask(
                                qos_ctx, session->request_id);
                        manager_reset_session_request(qos_entity, session);
                        if (incompatibility_mask != 0U)
                        {
                            session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED;
                        }
                        else
                        {
                            session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED;
                            session->next_retry_at_ms =
                                now_ms + manager_get_backoff_delay_ms(
                                    qos_entity, session->retry_attempts);
                            pending = ZOO_TRUE;
                        }
                        break;
                    }

                    default:
                        /* Unknown/pending status beyond deadline is treated as retryable timeout. */
                        if (session->next_retry_at_ms <= now_ms)
                        {
                            manager_reset_session_request(qos_entity, session);
                            session->state = service_available
                                ? ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED
                                : ZOO_SMB_SUBSCRIPTION_SESSION_STATE_PENDING_SERVICE;
                            session->next_retry_at_ms =
                                now_ms + manager_get_backoff_delay_ms(
                                    qos_entity, session->retry_attempts);
                        }
                        pending = ZOO_TRUE;
                        break;
                }
                break;

            case ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ACTIVE:
                break;

            case ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED:
            case ZOO_SMB_SUBSCRIPTION_SESSION_STATE_CANCELLED:
            default:
                break;
        }

        if (session->desired_active &&
            session->state != ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ACTIVE &&
            session->state != ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED &&
            session->state != ZOO_SMB_SUBSCRIPTION_SESSION_STATE_CANCELLED)
        {
            pending = ZOO_TRUE;
            if (session->next_retry_at_ms > now_ms)
            {
                uint64_t remaining_ms = session->next_retry_at_ms - now_ms;
                if (remaining_ms < local_sleep_ms)
                {
                    local_sleep_ms = (uint32_t)remaining_ms;
                }
            }
            else
            {
                local_sleep_ms = 0U;
            }
        }
    }

    *has_pending_work = pending;
    *sleep_ms = local_sleep_ms;
}
