/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_QOS
 * File name: zoo_smb_qos.c
 * Description: Quality of Service (QoS) implementation for ZOO SMB
 *              Provides creation, configuration, and destruction of QoS entities.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-01     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_qos.h"
#include "zoo_types.h"
#include "zoo_smb_error.h"
#include "zoo_list.h"
#include "zoo_log.h"
#include "zoo_smb.h"
#include <string.h>
#include <stdlib.h>

typedef struct ZOO_SMB_QOS_ENTITY_STRUCT
{
    ZOO_SMB_QOS_POLICY_HANDLE policy; /**< QoS policy associated with this entity */
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx;   /**< QoS context for this entity */
} ZOO_SMB_QOS_ENTITY_STRUCT;

/**
 * @brief Retrieves the effective wait timeout for a QoS entity.
 *
 * This function calculates the effective wait timeout based on the QoS policy
 * associated with the given QoS entity. If no specific timeout is set, a default
 * value of 5000 milliseconds is returned.
 *
 * @param[in] qos_entity Handle to the QoS entity.
 *
 * @return uint32_t Effective wait timeout in milliseconds.
 */
static uint32_t qos_entity_get_effective_wait_timeout_ms(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity)
{
    if (!qos_entity || !qos_entity->policy)
    {
        return 5000;
    }

    if (qos_entity->policy->reliability_enabled &&
        qos_entity->policy->reliability.max_blocking_time_ms > 0)
    {
        return (uint32_t)qos_entity->policy->reliability.max_blocking_time_ms;
    }

    if (qos_entity->policy->deadline_enabled &&
        qos_entity->policy->deadline.deadline_duration_ms > 0)
    {
        return (uint32_t)qos_entity->policy->deadline.deadline_duration_ms;
    }

    return 5000;
}
/**
 * @brief Creates a new QoS (Quality of Service) entity associated with the specified policy.
 *
 * This function initializes and returns a handle to a new QoS entity, which is managed under the
 * provided QoS policy handle.
 *
 * @param[in] policy Handle to the QoS policy under which the new entity will be created.
 *
 * @return ZOO_SMB_QOS_ENTITY_HANDLE Handle to the newly created QoS entity.
 */
ZOO_SMB_QOS_ENTITY_HANDLE zoo_smb_create_qos_entity(IN const ZOO_SMB_QOS_POLICY_STRUCT* policy)
{
    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity = (ZOO_SMB_QOS_ENTITY_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_QOS_ENTITY_STRUCT));
    if (!qos_entity)
    {
        ZOO_LOG_ERROR("zoo_smb_create_qos_entity: allocation failed");
        return NULL;
    }
    memset(qos_entity, 0, sizeof(ZOO_SMB_QOS_ENTITY_STRUCT));

    if (!policy)
    {
        ZOO_LOG_ERROR("zoo_smb_create_qos_entity: QoS policy is NULL");
        zoo_free_to_pool(qos_entity);
        return NULL;
    }

    if (!zoo_smb_qos_policy_is_valid((const ZOO_SMB_QOS_POLICY_HANDLE)policy))
    {
        ZOO_LOG_ERROR("zoo_smb_create_qos_entity: Invalid QoS policy");
        zoo_free_to_pool(qos_entity);
        return NULL;
    }

    qos_entity->policy = zoo_smb_create_qos_policy(policy);
    if (!qos_entity->policy)
    {
        zoo_free_to_pool(qos_entity);
        ZOO_LOG_ERROR("zoo_smb_create_qos_entity: QoS policy allocation failed");
        return NULL;
    }

    qos_entity->qos_ctx = zoo_smb_create_qos_ctx(qos_entity->policy);
    if (!qos_entity->qos_ctx)
    {
        zoo_smb_destroy_qos_policy(qos_entity->policy);
        zoo_free_to_pool(qos_entity);
        ZOO_LOG_ERROR("zoo_smb_create_qos_entity: QoS context creation failed");
        return NULL;
    }

    return qos_entity;
}

/**
 * @brief Creates a default QoS (Quality of Service) entity for SMB.
 *
 * This function initializes and returns a handle to a default QoS entity,
 * which can be used to manage and enforce QoS policies in the SMB subsystem.
 *
 * @return ZOO_SMB_QOS_ENTITY_HANDLE Handle to the newly created default QoS entity.
 */
ZOO_SMB_QOS_ENTITY_HANDLE zoo_smb_create_qos_default_entity()
{
    ZOO_SMB_QOS_POLICY_STRUCT default_policy_struct = ZOO_SMB_DEFAULT_QOS_POLICY();
    ZOO_SMB_QOS_POLICY_HANDLE default_policy = zoo_smb_init_qos_policy(&default_policy_struct);
    return zoo_smb_create_qos_entity(default_policy);
}

/**
 * @brief Destroys a QoS (Quality of Service) entity in the SMB subsystem.
 *
 * This function releases all resources associated with the specified QoS entity handle.
 * After calling this function, the provided handle becomes invalid and should not be used.
 *
 * @param[in] qos_entity Handle to the QoS entity to be destroyed.
 */
void zoo_smb_destroy_qos_entity(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity)
{
    if (qos_entity)
    {
        zoo_smb_destroy_qos_policy(qos_entity->policy);
        zoo_smb_destroy_qos_ctx(qos_entity->qos_ctx);
        zoo_free_to_pool(qos_entity);
    }
}

/**
 * @brief Builds a QoS negotiate request message for SMB.
 *
 * This function constructs and returns a pointer to a ZOO_SMB_MSG_STRUCT
 * representing a QoS negotiate request message, which is used during the
 * negotiation phase of the SMB protocol to establish quality of service parameters.
 *
 * @return ZOO_SMB_MSG_STRUCT* Pointer to the constructed QoS negotiate request message.
 */
ZOO_SMB_MSG_STRUCT* zoo_smb_qos_build_msg(
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN ZOO_STRING name,
    IN ZOO_STRING topic,
    IN ZOO_SMB_MSG_TYPE_ENUM msg_type,
    IN ZOO_U32 msg_id,
    IN ZOO_U64 request_id)
{
    return zoo_smb_create_message(
        msg_type, name, topic, qos_entity->policy ? (void*)qos_entity->policy : NULL, qos_entity->policy ? sizeof(ZOO_SMB_QOS_POLICY_STRUCT) : 0, msg_id, request_id);
}

/**
 * @brief Retrieves the QoS policy handle associated with the given QoS context.
 *
 * This function returns the policy handle for the specified QoS context.
 *
 * @param[in] qos_ctx The handle to the QoS context.
 * @return ZOO_SMB_QOS_POLICY_HANDLE The handle to the QoS policy.
 */
ZOO_SMB_QOS_POLICY_HANDLE zoo_smb_qos_get_policy(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity)
{
    return qos_entity ? qos_entity->policy : NULL;
}

/**
 * @brief Retrieves the QoS context handle associated with a given QoS entity.
 *
 * This function returns the QoS context handle for the specified QoS entity handle.
 *
 * @param[in] qos_entity The handle to the QoS entity for which the context is requested.
 * @return ZOO_SMB_QOS_CTX_HANDLE The handle to the QoS context associated with the given entity.
 */
ZOO_SMB_QOS_CTX_HANDLE zoo_smb_qos_get_ctx(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity)
{
    return qos_entity ? qos_entity->qos_ctx : NULL;
}

/**
 * @brief Waits for the QoS negotiation to complete.
 *
 * This function blocks until the QoS negotiation for the specified request ID
 * reaches the given status. It will return an error if the negotiation times out.
 *
 * @param qos_entity The handle to the QoS entity.
 * @param request_id The unique identifier for the QoS negotiation request.
 * @return ZOO_ERROR_TYPE Returns ZOO_SMB_OK on success, or an error code on failure.
 */
ZOO_ERROR_TYPE zoo_smb_qos_wait_for_negotiation_completed(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
                                                              IN uint64_t request_id)
{
    uint32_t timeout_ms = qos_entity_get_effective_wait_timeout_ms(qos_entity);
    ZOO_LOG_INFO(
        "Waiting for QoS negotiation to complete for request_id: %llu, timeout_ms: %u", request_id, timeout_ms);
    if (!zoo_smb_qos_ctx_wait_for_status(qos_entity->qos_ctx, request_id, ZOO_SMB_MSG_ST_ACKED, timeout_ms))
    {
        ZOO_SMB_MSG_ST_ENUM current_status = zoo_smb_qos_ctx_get_msg_status(qos_entity->qos_ctx, request_id);
        if (current_status == ZOO_SMB_MSG_ST_FAILED)
        {
            if (zoo_smb_qos_ctx_get_incompatibility_mask(qos_entity->qos_ctx, request_id) != 0)
            {
                ZOO_LOG_ERROR("QoS negotiation request_id:%llu rejected due to QoS incompatibility", request_id);
                return ZOO_SMB_ERROR_PUBLICATION_QOS_VIOLATION;
            }
            return ZOO_SMB_ERROR_OPERATION_FAILED;
        }

        ZOO_LOG_ERROR("QoS negotiation request_id:%llu timed out after %u ms", request_id, timeout_ms);
        return ZOO_SMB_ERROR_TIMEOUT;
    }

    ZOO_LOG_INFO(
        "QoS negotiation completed for request_id: %llu", request_id);
    return ZOO_SMB_OK;
}

/**
 * @brief Waits for a specific message status in the QoS context.
 *
 * This function blocks until the message with the specified request ID reaches the given status.
 * It will return an error if the status is not reached within the timeout period defined in the QoS policy.
 *
 * @param qos_entity The handle to the QoS entity.
 * @param request_id The unique identifier for the QoS message.
 * @param status The expected status of the QoS message.
 * @return ZOO_ERROR_TYPE Returns ZOO_SMB_OK on success, or an error code on failure.
 */
ZOO_ERROR_TYPE zoo_smb_qos_wait_for_msg_status(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity, IN uint64_t request_id, IN ZOO_SMB_MSG_ST_ENUM status)
{
    ZOO_LOG_INFO(
        "Waiting for QoS for request_id: %llu, status: %d", request_id, status);
    uint32_t timeout_ms = qos_entity_get_effective_wait_timeout_ms(qos_entity);
    ZOO_LOG_INFO(
        "Waiting for QoS negotiation to complete for request_id: %llu, timeout_ms: %u", request_id, timeout_ms);
    if (!zoo_smb_qos_ctx_wait_for_status(qos_entity->qos_ctx, request_id, status, timeout_ms))
    {
        ZOO_LOG_ERROR("QoS negotiation request_id:%llu timed out after %u ms", request_id, timeout_ms);
        return ZOO_SMB_ERROR_TIMEOUT;
    }
    return ZOO_SMB_OK;
}
