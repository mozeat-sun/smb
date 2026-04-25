/* ******************************************************************************
 * Copyright (C) 2026, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SUBSCRIPTION_SESSION
 * File name: zoo_smb_subscription_session.c
 * Description: Subscription session object implementation used by subscriber
 *              session reconciliation and message dispatch flows.
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-11     github.copilot    add file/function comments
 ****************************************************************************** */

#include "zoo_smb_subscription_session.h"
#include "zoo_memory_pool.h"
#include "zoo.h"

/* Session objects are intentionally lightweight; policy and retry decisions live in manager. */
/**
 * @brief Allocates and initializes one subscription session.
 *
 * Initial state is chosen from current service availability so reconcile can
 * either attempt immediate negotiation or wait for service recovery.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE zoo_smb_subscription_session_create(
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER handler,
    IN void* user_data,
    IN int32_t id,
    IN uint64_t now_ms,
    IN ZOO_BOOL service_available)
{
    if (!handler)
    {
        return NULL;
    }

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session =
        (ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_SUBSCRIPTION_SESSION_STRUCT));
    if (!session)
    {
        return NULL;
    }

    session->usr_data = user_data;
    session->msg_id = msg_id;
    session->id = id;
    session->handler = handler;
    session->request_id = 0;
    session->next_retry_at_ms = now_ms;
    session->retry_attempts = 0;
    session->desired_active = ZOO_TRUE;
    /* Initial state follows service readiness to avoid immediate failed negotiation attempts. */
    session->state = service_available
                         ? ZOO_SMB_SUBSCRIPTION_SESSION_STATE_INIT
                         : ZOO_SMB_SUBSCRIPTION_SESSION_STATE_PENDING_SERVICE;

    return session;
}

/**
 * @brief Releases a session created by zoo_smb_subscription_session_create().
 */
void zoo_smb_subscription_session_destroy(IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session)
{
    if (!session)
    {
        return;
    }

    zoo_free_to_pool(session);
}

/**
 * @brief Marks a session cancelled and clears active request linkage.
 */
void zoo_smb_subscription_session_mark_cancelled(IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session)
{
    if (!session)
    {
        return;
    }

    session->desired_active = ZOO_FALSE;
    session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_CANCELLED;
    session->request_id = 0;
}

/**
 * @brief Checks whether session matches both handler and message id.
 */
ZOO_BOOL zoo_smb_subscription_session_matches_handler(
    IN const ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER handler)
{
    if (!session || !handler)
    {
        return ZOO_FALSE;
    }

    return session->handler == handler && session->msg_id == msg_id;
}
