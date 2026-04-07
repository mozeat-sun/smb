

/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_QOS
 * File name: zoo_smb_qos_ctx.c
 * Description: Quality of Service (QoS) definitions for ZOO SMB
 *              Similar to DDS QoS profiles, these settings control message
 *              delivery characteristics, reliability, and resource usage.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-18     weiwang.sun       created
 ******************************************************************************/
#include "zoo_smb_qos_ctx.h"
#include "../../buffer/inc/zoo_list.h"
#include "zoo_util.h"
#define MAX_QOS_MSG_LIST_SIZE (10240)
#define QOS_STATE_INDEX_BUCKET_COUNT (257)
#define QOS_PRUNE_INTERVAL_MS (1000)

/**
 * @brief Computes the QoS context storage capacity from policy settings.
 *
 * @param qos_policy QoS policy used to derive capacity.
 * @return ZOO_SIZE_T Maximum number of tracked QoS states for the context.
 */
static ZOO_SIZE_T get_qos_ctx_capacity(const ZOO_SMB_QOS_POLICY_HANDLE qos_policy)
{
    if (!qos_policy)
    {
        return MAX_QOS_MSG_LIST_SIZE;
    }

    if (qos_policy->resource_limits_enabled && qos_policy->resource_limits.max_samples > 0)
    {
        return qos_policy->resource_limits.max_samples;
    }

    if (qos_policy->history_enabled &&
        qos_policy->history.kind == ZOO_SMB_QOS_HISTORY_KEEP_LAST &&
        qos_policy->history.depth > 0)
    {
        return qos_policy->history.depth;
    }

    return MAX_QOS_MSG_LIST_SIZE;
}

/**
 * @brief Structure representing the context for a QoS (Quality of Service) request in the SMB service.
 *
 * This structure holds the status and unique identifier associated with a QoS request.
 */
typedef struct ZOO_SMB_QOS_CTX_STRUCT
{
    ZOO_LIST_HANDLE messages;       /**< Unique identifier for the QoS request */
    ZOO_LIST_HANDLE state_index_buckets[QOS_STATE_INDEX_BUCKET_COUNT];
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy; /**< Handle to the QoS policy */
    uint64_t next_prune_deadline_ms;
    uint64_t lock_hold_total_ms;
    uint64_t lock_acquire_count;
    uint64_t cond_wait_wakeup_count;
    ZOO_MUTEX_T mutex;
    ZOO_COND_T cond;
} ZOO_SMB_QOS_CTX_STRUCT;

static ZOO_BOOL compare_qos_msg(IN const void* a, IN const void* b);

/**
 * @brief Maps a request identifier to an index bucket.
 *
 * @param request_id Request identifier.
 * @return ZOO_USIZE Bucket index for the request state lookup table.
 */
static ZOO_USIZE qos_state_bucket_of(uint64_t request_id)
{
    return (ZOO_USIZE)(request_id % QOS_STATE_INDEX_BUCKET_COUNT);
}

/**
 * @brief Finds a QoS state by request identifier while the context lock is held.
 *
 * @param qos_ctx QoS context.
 * @param request_id Request identifier to search for.
 * @return ZOO_SMB_QOS_STATE_STRUCT* Matching state, or NULL if not found.
 */
static ZOO_SMB_QOS_STATE_STRUCT* qos_ctx_find_state_locked(
    IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
    IN uint64_t request_id)
{
    if (!qos_ctx)
    {
        return NULL;
    }

    ZOO_LIST_HANDLE bucket = qos_ctx->state_index_buckets[qos_state_bucket_of(request_id)];
    if (!bucket)
    {
        return NULL;
    }

    return (ZOO_SMB_QOS_STATE_STRUCT*)zoo_list_find_if(bucket, compare_qos_msg, &request_id);
}

/**
 * @brief Inserts a QoS state into the request-id index while the context lock is held.
 *
 * @param qos_ctx QoS context.
 * @param state State entry to index.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code otherwise.
 */
static ZOO_ERROR_TYPE qos_ctx_index_insert_locked(
    IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
    IN ZOO_SMB_QOS_STATE_STRUCT* state)
{
    if (!qos_ctx || !state)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_LIST_HANDLE bucket = qos_ctx->state_index_buckets[qos_state_bucket_of(state->request_id)];
    if (!bucket)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    return zoo_list_push_back(bucket, state);
}

/**
 * @brief Removes a QoS state from the request-id index while the context lock is held.
 *
 * @param qos_ctx QoS context.
 * @param state State entry to remove from the index.
 */
static void qos_ctx_index_remove_locked(
    IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
    IN ZOO_SMB_QOS_STATE_STRUCT* state)
{
    if (!qos_ctx || !state)
    {
        return;
    }

    ZOO_LIST_HANDLE bucket = qos_ctx->state_index_buckets[qos_state_bucket_of(state->request_id)];
    if (bucket)
    {
        (void)zoo_list_remove(bucket, state);
    }
}

/**
 * @brief Compares two QoS (Quality of Service) message objects.
 *
 * This function is intended to be used as a comparator for QoS message structures,
 * typically in sorting or searching operations. It compares the two provided objects
 * and returns a boolean value indicating whether they are considered equal.
 *
 * @param a Pointer to the first QoS message object to compare.
 * @param b Pointer to the second QoS message object to compare.
 * @return ZOO_TRUE if the two QoS messages are considered equal, ZOO_FALSE otherwise.
 */
static ZOO_BOOL compare_qos_msg(IN const void* a, IN const void* b)
{
/**
 * @brief Checks whether a QoS state has expired according to policy lifespan.
 *
 * @param qos_ctx QoS context providing lifespan policy.
 * @param state QoS state to evaluate.
 * @param now_ms Current timestamp in milliseconds.
 * @return ZOO_TRUE if the state has expired, ZOO_FALSE otherwise.
 */
    const ZOO_SMB_QOS_STATE_STRUCT* msg = (const ZOO_SMB_QOS_STATE_STRUCT*)a;
    return msg->request_id == *(uint64_t*)b;
}

static ZOO_BOOL qos_state_is_expired(
    IN const ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
    IN const ZOO_SMB_QOS_STATE_STRUCT* state,
    IN uint64_t now_ms)
{
    if (!qos_ctx || !state || !qos_ctx->qos_policy)
    {
        return ZOO_FALSE;
    }

    if (!qos_ctx->qos_policy->lifespan_enabled || qos_ctx->qos_policy->lifespan.duration_ms == 0)
    {
        return ZOO_FALSE;
    }

    return (now_ms - state->timestamp_ms) >= qos_ctx->qos_policy->lifespan.duration_ms;
}

/**
 * @brief Prunes expired QoS states while the context lock is held.
 *
 * @param qos_ctx QoS context to prune.
 */
static void qos_ctx_prune_expired_locked(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx)
{
    if (!qos_ctx)
    {
        return;
    }

    uint64_t now_ms = zoo_get_timestamp_milliseconds();
    if (now_ms < qos_ctx->next_prune_deadline_ms)
    {
        return;
    }

    qos_ctx->next_prune_deadline_ms = now_ms + QOS_PRUNE_INTERVAL_MS;
    ZOO_SIZE_T initial_size = zoo_list_size(qos_ctx->messages);
    for (ZOO_SIZE_T i = 0; i < initial_size; ++i)
    {
        ZOO_SMB_QOS_STATE_STRUCT* state = (ZOO_SMB_QOS_STATE_STRUCT*)zoo_list_pop_front(qos_ctx->messages);
        if (!state)
        {
            continue;
        }

        if (qos_state_is_expired(qos_ctx, state, now_ms))
        {
            qos_ctx_index_remove_locked(qos_ctx, state);
            zoo_free_to_pool(state);
            continue;
        }

        (void)zoo_list_push_back(qos_ctx->messages, state);
    }
}

/**
 * @brief Cleans up and releases resources associated with the given QoS context handle.
 *
 * This function is responsible for performing any necessary cleanup operations
 * for the specified ZOO_SMB_QOS_CTX_HANDLE. It should be called when the QoS context
 * is no longer needed to prevent resource leaks.
 *
 * @param[in] qos_ctx The handle to the QoS context to be cleaned up.
 */
static void clean_up_resource(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx)
{
    if (qos_ctx == NULL)
    {
        return;
    }

    for (size_t i = 0; i < zoo_list_size(qos_ctx->messages); i++)
    {
        ZOO_SMB_QOS_STATE_STRUCT* state = (ZOO_SMB_QOS_STATE_STRUCT*)zoo_list_at(qos_ctx->messages, i);
        if (state)
        {
            zoo_free_to_pool(state);
        }
    }

    zoo_list_destroy(qos_ctx->messages);
    for (ZOO_USIZE i = 0; i < QOS_STATE_INDEX_BUCKET_COUNT; i++)
    {
        if (qos_ctx->state_index_buckets[i])
        {
            zoo_list_destroy(qos_ctx->state_index_buckets[i]);
            qos_ctx->state_index_buckets[i] = NULL;
        }
    }
}

/**
 * @brief Creates and initializes a new QoS (Quality of Service) context handle for SMB operations.
 *
 * This function allocates and sets up a new ZOO_SMB_QOS_CTX_HANDLE, which can be used to manage
 * and track QoS parameters for SMB (Server Message Block) services.
 *
 * @return ZOO_SMB_QOS_CTX_HANDLE
 *         A handle to the newly created QoS context, or NULL if creation fails.
 */
ZOO_SMB_QOS_CTX_HANDLE zoo_smb_create_qos_ctx(IN const ZOO_SMB_QOS_POLICY_HANDLE qos_policy)
{
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = (ZOO_SMB_QOS_CTX_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_QOS_CTX_STRUCT));
    if (qos_ctx == NULL)
    {
        ZOO_LOG_ERROR("zoo_smb_create_qos_ctx: Failed to allocate QoS context");
        return NULL;
    }

    qos_ctx->messages = zoo_list_create(get_qos_ctx_capacity(qos_policy));  // Initialize the QoS message context
    if (qos_ctx->messages == NULL)
    {
        ZOO_LOG_ERROR("zoo_smb_create_qos_ctx: Failed to create QoS message list");
        zoo_free_to_pool(qos_ctx);
        return NULL;
    }

    memset(qos_ctx->state_index_buckets, 0, sizeof(qos_ctx->state_index_buckets));
    for (ZOO_USIZE i = 0; i < QOS_STATE_INDEX_BUCKET_COUNT; i++)
    {
        qos_ctx->state_index_buckets[i] = zoo_list_create(16);
        if (!qos_ctx->state_index_buckets[i])
        {
            ZOO_LOG_ERROR("zoo_smb_create_qos_ctx: Failed to create QoS state index bucket");
            for (ZOO_USIZE j = 0; j < i; j++)
            {
                zoo_list_destroy(qos_ctx->state_index_buckets[j]);
                qos_ctx->state_index_buckets[j] = NULL;
            }
            zoo_list_destroy(qos_ctx->messages);
            zoo_free_to_pool(qos_ctx);
            return NULL;
        }
    }

    if (!ZOO_MUTEX_INIT(&qos_ctx->mutex) || !ZOO_COND_INIT(&qos_ctx->cond))
    {
        ZOO_LOG_ERROR("zoo_smb_create_qos_ctx: Failed to initialize synchronization primitives");
        zoo_list_destroy(qos_ctx->messages);
        zoo_free_to_pool(qos_ctx);
        return NULL;
    }

    qos_ctx->qos_policy = qos_policy;  // Set the QoS handle in the context
    qos_ctx->next_prune_deadline_ms = 0;
    qos_ctx->lock_hold_total_ms = 0;
    qos_ctx->lock_acquire_count = 0;
    qos_ctx->cond_wait_wakeup_count = 0;

    ZOO_LOG_DEBUG("zoo_smb_create_qos_ctx: QoS context created");
    return qos_ctx;
}

/**
 * @brief Destroys a QoS (Quality of Service) context for SMB operations.
 *
 * This function releases all resources associated with the specified
 * QoS context handle. After calling this function, the handle should
 * not be used in any further operations.
 *
 * @param qos_ctx The handle to the QoS context to be destroyed.
 */
void zoo_smb_destroy_qos_ctx(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx)
{
    if (qos_ctx == NULL)
    {
        ZOO_LOG_WARN("zoo_smb_destroy_qos_ctx: qos_ctx is NULL");
        return;
    }
    clean_up_resource(qos_ctx);
    if (qos_ctx->lock_acquire_count > 0)
    {
        ZOO_LOG_DEBUG("qos_ctx metrics: lock_acquires=%llu lock_hold_total_ms=%llu cond_wakeups=%llu",
            (unsigned long long)qos_ctx->lock_acquire_count,
            (unsigned long long)qos_ctx->lock_hold_total_ms,
            (unsigned long long)qos_ctx->cond_wait_wakeup_count);
    }
    ZOO_COND_DESTROY(&qos_ctx->cond);
    ZOO_MUTEX_DESTROY(&qos_ctx->mutex);
    zoo_free_to_pool(qos_ctx);
    ZOO_LOG_DEBUG("zoo_smb_destroy_qos_ctx: QoS context destroyed");
}

/**
 * @brief Retrieves the QoS entity handle associated with the given QoS context.
 *
 * @param[in] qos_ctx Handle to the QoS context.
 * @return ZOO_SMB_QOS_ENTITY_HANDLE Handle to the corresponding QoS entity.
 */
ZOO_SMB_QOS_POLICY_HANDLE zoo_smb_qos_ctx_get_qos_policy(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx)
{
    return qos_ctx->qos_policy;
}

/**
 * @brief Creates a new QoS state structure for a given request.
 *
 * @param request_id The unique identifier for the request.
 * @param status The status of the SMB message, represented by ZOO_SMB_MSG_ST_ENUM.
 * @param msg Pointer to the message data associated with the request.
 * @return Pointer to the newly created ZOO_SMB_QOS_STATE_STRUCT, or NULL on failure.
 */
ZOO_SMB_QOS_STATE_STRUCT* qos_ctx_new_state(IN uint64_t request_id, IN ZOO_SMB_MSG_ST_ENUM status, IN void* msg)
{
    ZOO_SMB_QOS_STATE_STRUCT* state = (ZOO_SMB_QOS_STATE_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_QOS_STATE_STRUCT));
    if (state)
    {
        state->status = status;
        state->request_id = request_id;
        state->msg = msg;                                  // Store the message associated with this QoS state
        state->timestamp_ms = zoo_get_timestamp_milliseconds();  // Store the current timestamp
        state->matched = ZOO_FALSE;
        state->incompatibility_mask = 0;
    }
    return state;
}

/**
 * @brief Sets the status of the specified SMB QoS handle.
 *
 * This function updates the status of the given Quality of Service (QoS) handle
 * for SMB (Server Message Block) operations to the provided status value.
 *
 * @param qos    The handle to the SMB QoS instance whose status is to be set.
 * @param status The new status to assign to the QoS handle. This should be a value
 *               from the ZOO_SMB_MSG_ST_ENUM enumeration.
 */
ZOO_ERROR_TYPE zoo_smb_qos_ctx_set_state(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id, IN ZOO_SMB_MSG_ST_ENUM status, IN void* msg)
{
    if (!qos_ctx)
    {
        ZOO_LOG_ERROR("qos_ctx is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    uint64_t lock_start_ms = zoo_get_timestamp_milliseconds();
    ZOO_MUTEX_LOCK(&qos_ctx->mutex);
    qos_ctx->lock_acquire_count++;
    qos_ctx_prune_expired_locked(qos_ctx);
    ZOO_SMB_QOS_STATE_STRUCT* state = qos_ctx_find_state_locked(qos_ctx, request_id);
    if (!state)
    {
        state = qos_ctx_new_state(request_id, status, msg);
        ZOO_LOG_DEBUG("New qos state:  req_id:%llu, status:%d", request_id, status);
        ZOO_ERROR_TYPE ret = zoo_list_push_back(qos_ctx->messages, state);
        if (ret == ZOO_SMB_OK)
        {
            ret = qos_ctx_index_insert_locked(qos_ctx, state);
            if (ret != ZOO_SMB_OK)
            {
                (void)zoo_list_remove(qos_ctx->messages, state);
                zoo_free_to_pool(state);
            }
        }
        ZOO_COND_BROADCAST(&qos_ctx->cond);
        ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
        qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
        return ret;
    }
    else
    {
        state->status = status;
        state->timestamp_ms = zoo_get_timestamp_milliseconds();  // Update the timestamp
        ZOO_LOG_DEBUG("Update qos state: req_id:%llu, status:%d", request_id, status);
    }
    ZOO_COND_BROADCAST(&qos_ctx->cond);
    ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
    qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
    return ZOO_SMB_OK;
}

/**
 * @brief Sets the message status for a specific request in the QoS context.
 *
 * This function updates the status of a message identified by the given request ID
 * within the specified Quality of Service (QoS) context.
 *
 * @param[in] qos_ctx     Handle to the QoS context.
 * @param[in] request_id  Unique identifier for the request whose status is to be set.
 * @param[in] status      The status to assign to the message (of type ZOO_SMB_MSG_ST_ENUM).
 */
void zoo_smb_qos_ctx_set_msg_status(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id, IN ZOO_SMB_MSG_ST_ENUM status)
{
    if (!qos_ctx)
    {
        ZOO_LOG_ERROR("qos_ctx is NULL");
        return;
    }

    uint64_t lock_start_ms = zoo_get_timestamp_milliseconds();
    ZOO_MUTEX_LOCK(&qos_ctx->mutex);
    qos_ctx->lock_acquire_count++;
    qos_ctx_prune_expired_locked(qos_ctx);
    ZOO_SMB_QOS_STATE_STRUCT* state = qos_ctx_find_state_locked(qos_ctx, request_id);
    if (state)
    {
        state->status = status;
        state->timestamp_ms = zoo_get_timestamp_milliseconds();
    }
    ZOO_COND_BROADCAST(&qos_ctx->cond);
    ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
    qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
}

/**
 * @brief Retrieves the status of a specific QoS request from the given QoS context.
 *
 * @param qos_ctx      Handle to the QoS context.
 * @param request_id   Identifier of the QoS request whose status is to be retrieved.
 *
 * @return ZOO_SMB_MSG_ST_ENUM
 *         The status of the specified QoS request.
 */
ZOO_SMB_MSG_ST_ENUM zoo_smb_qos_ctx_get_msg_status(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id)
{
    if (!qos_ctx)
    {
        return ZOO_SMB_MSG_ST_UNKNOWN;
    }

    uint64_t lock_start_ms = zoo_get_timestamp_milliseconds();
    ZOO_MUTEX_LOCK(&qos_ctx->mutex);
    qos_ctx->lock_acquire_count++;
    qos_ctx_prune_expired_locked(qos_ctx);
    ZOO_SMB_QOS_STATE_STRUCT* state = qos_ctx_find_state_locked(qos_ctx, request_id);
    ZOO_SMB_MSG_ST_ENUM result = (state) ? state->status : ZOO_SMB_MSG_ST_UNKNOWN;
    ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
    qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
    return result;
}

/**
 * @brief Retrieves the QoS message structure associated with a specific request ID.
 *
 * This function searches the given QoS context for a message structure that matches
 * the provided request ID. If found, a pointer to the corresponding ZOO_SMB_QOS_STATE_STRUCT
 * is returned; otherwise, NULL may be returned.
 *
 * @param[in] qos_ctx     Handle to the QoS context from which to retrieve the message.
 * @param[in] request_id  The unique identifier of the request whose message is to be retrieved.
 *
 * @return Pointer to the ZOO_SMB_QOS_STATE_STRUCT associated with the request ID, or NULL if not found.
 */
ZOO_SMB_QOS_STATE_STRUCT* zoo_smb_qos_ctx_get_state(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id)
{
    if (!qos_ctx)
    {
        return NULL;
    }

    uint64_t lock_start_ms = zoo_get_timestamp_milliseconds();
    ZOO_MUTEX_LOCK(&qos_ctx->mutex);
    qos_ctx->lock_acquire_count++;
    qos_ctx_prune_expired_locked(qos_ctx);
    ZOO_SMB_QOS_STATE_STRUCT* state = qos_ctx_find_state_locked(qos_ctx, request_id);
    ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
    qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
    return state;
}

/**
 * @brief Removes the state associated with a specific request ID from the QoS context.
 *
 * This function deletes or cleans up any state information related to the given
 * request ID within the specified Quality of Service (QoS) context handle.
 *
 * @param[in] qos_ctx     Handle to the QoS context from which the state should be removed.
 * @param[in] request_id  Unique identifier of the request whose state is to be removed.
 */
void zoo_smb_qos_ctx_remove_state(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id)
{
    if (!qos_ctx)
    {
        ZOO_LOG_ERROR("qos_ctx is NULL");
        return;
    }

    uint64_t lock_start_ms = zoo_get_timestamp_milliseconds();
    ZOO_MUTEX_LOCK(&qos_ctx->mutex);
    qos_ctx->lock_acquire_count++;
    qos_ctx_prune_expired_locked(qos_ctx);
    ZOO_SMB_QOS_STATE_STRUCT* state = qos_ctx_find_state_locked(qos_ctx, request_id);
    if (state)
    {
        zoo_list_remove(qos_ctx->messages, state);
        qos_ctx_index_remove_locked(qos_ctx, state);
        zoo_free_to_pool(state);
    }
    ZOO_COND_BROADCAST(&qos_ctx->cond);
    ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
    qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
    ZOO_LOG_DEBUG("Removed QoS state for request_id: %llu", request_id);
}

/**
 * @brief Checks if the message status for a given request ID matches the specified status in the QoS context.
 *
 * @param qos_ctx      Handle to the QoS context.
 * @param request_id   The unique identifier of the request to check.
 * @param status       The status to compare against the message status of the request.
 *
 * @return ZOO_TRUE if the message status matches the specified status, ZOO_FALSE otherwise.
 */
ZOO_BOOL zoo_smb_qos_ctx_is_msg_status(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id, IN ZOO_SMB_MSG_ST_ENUM status)
{
    if (!qos_ctx)
    {
        return ZOO_FALSE;
    }

    ZOO_SMB_MSG_ST_ENUM current_status = zoo_smb_qos_ctx_get_msg_status(qos_ctx, request_id);
    return (current_status == status);
}

/**
 * @brief Stores QoS negotiation match results for a specific request.
 *
 * @param qos_ctx QoS context.
 * @param request_id Request identifier.
 * @param matched Match result flag.
 * @param incompatibility_mask Detailed incompatibility bitmask.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code otherwise.
 */
ZOO_ERROR_TYPE zoo_smb_qos_ctx_set_match_result(
    IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
    IN uint64_t request_id,
    IN ZOO_BOOL matched,
    IN uint32_t incompatibility_mask)
{
    if (!qos_ctx)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    uint64_t lock_start_ms = zoo_get_timestamp_milliseconds();
    ZOO_MUTEX_LOCK(&qos_ctx->mutex);
    qos_ctx->lock_acquire_count++;
    qos_ctx_prune_expired_locked(qos_ctx);
    ZOO_SMB_QOS_STATE_STRUCT* state = qos_ctx_find_state_locked(qos_ctx, request_id);
    if (!state)
    {
        state = qos_ctx_new_state(request_id, matched ? ZOO_SMB_MSG_ST_ACKED : ZOO_SMB_MSG_ST_FAILED, NULL);
        if (!state)
        {
            ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
            qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
            return ZOO_SMB_ERROR_OUT_OF_MEMORY;
        }
        state->matched = matched;
        state->incompatibility_mask = incompatibility_mask;
        ZOO_ERROR_TYPE ret = zoo_list_push_back(qos_ctx->messages, state);
        if (ret == ZOO_SMB_OK)
        {
            ret = qos_ctx_index_insert_locked(qos_ctx, state);
            if (ret != ZOO_SMB_OK)
            {
                (void)zoo_list_remove(qos_ctx->messages, state);
                zoo_free_to_pool(state);
            }
        }
        ZOO_COND_BROADCAST(&qos_ctx->cond);
        ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
        qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
        return ret;
    }

    state->matched = matched;
    state->incompatibility_mask = incompatibility_mask;
    state->timestamp_ms = zoo_get_timestamp_milliseconds();
    ZOO_COND_BROADCAST(&qos_ctx->cond);
    ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
    qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
    return ZOO_SMB_OK;
}

/**
 * @brief Retrieves the incompatibility mask stored for a specific request.
 *
 * @param qos_ctx QoS context.
 * @param request_id Request identifier.
 * @return uint32_t Incompatibility bitmask, or 0 if not found.
 */
uint32_t zoo_smb_qos_ctx_get_incompatibility_mask(
    IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
    IN uint64_t request_id)
{
    if (!qos_ctx)
    {
        return 0;
    }

    uint64_t lock_start_ms = zoo_get_timestamp_milliseconds();
    ZOO_MUTEX_LOCK(&qos_ctx->mutex);
    qos_ctx->lock_acquire_count++;
    qos_ctx_prune_expired_locked(qos_ctx);
    ZOO_SMB_QOS_STATE_STRUCT* state = qos_ctx_find_state_locked(qos_ctx, request_id);
    uint32_t mask = state ? state->incompatibility_mask : 0;
    ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
    qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - lock_start_ms);
    return mask;
}

/**
 * @brief Waits for a request state to reach the specified status or timeout.
 *
 * @param qos_ctx QoS context.
 * @param request_id Request identifier.
 * @param status Desired terminal or intermediate status.
 * @param timeout_ms Maximum wait time in milliseconds.
 * @return ZOO_TRUE if the desired status is observed, ZOO_FALSE otherwise.
 */
ZOO_BOOL zoo_smb_qos_ctx_wait_for_status(
    IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
    IN uint64_t request_id,
    IN ZOO_SMB_MSG_ST_ENUM status,
    IN uint32_t timeout_ms)
{
    if (!qos_ctx)
    {
        return ZOO_FALSE;
    }

    uint64_t start_time = zoo_get_timestamp_milliseconds();
    uint32_t wakeups = 0;
    ZOO_MUTEX_LOCK(&qos_ctx->mutex);
    qos_ctx->lock_acquire_count++;

    while (ZOO_TRUE)
    {
        qos_ctx_prune_expired_locked(qos_ctx);
        ZOO_SMB_QOS_STATE_STRUCT* state = qos_ctx_find_state_locked(qos_ctx, request_id);
        if (state && state->status == status)
        {
            ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
            return ZOO_TRUE;
        }

        if (state && (state->status == ZOO_SMB_MSG_ST_FAILED || state->status == ZOO_SMB_MSG_ST_TIMEOUT))
        {
            ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
            return ZOO_FALSE;
        }

        uint64_t elapsed = zoo_get_timestamp_milliseconds() - start_time;
        if (elapsed >= timeout_ms)
        {
            ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
            return ZOO_FALSE;
        }

        uint32_t wait_ms = (uint32_t)(timeout_ms - elapsed);
        if (qos_ctx->qos_policy &&
            qos_ctx->qos_policy->reliability_enabled &&
            qos_ctx->qos_policy->reliability.retry_interval_ms > 0 &&
            qos_ctx->qos_policy->reliability.retry_interval_ms < wait_ms)
        {
            wait_ms = (uint32_t)qos_ctx->qos_policy->reliability.retry_interval_ms;
        }

        if (!ZOO_COND_WAIT_TIMEOUT(&qos_ctx->cond, &qos_ctx->mutex, wait_ms))
        {
            ZOO_MUTEX_UNLOCK(&qos_ctx->mutex);
            qos_ctx->lock_hold_total_ms += (zoo_get_timestamp_milliseconds() - start_time);
            qos_ctx->cond_wait_wakeup_count += wakeups;
            return ZOO_FALSE;
        }
        wakeups++;
    }
}
