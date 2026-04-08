/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_QOS
 * File name: zoo_smb_qos_ctx.h
 * Description: Quality of Service (QoS) definitions for ZOO SMB
 *              Similar to DDS QoS profiles, these settings control message
 *              delivery characteristics, reliability, and resource usage.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-18     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_QOS_CTX_H
#define ZOO_SMB_QOS_CTX_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_qos_policy.h"
#include "zoo_smb_node.h"
#include <stdint.h>

    typedef struct ZOO_SMB_QOS_CTX_STRUCT* ZOO_SMB_QOS_CTX_HANDLE;

    /**
     * @brief Structure representing the context for a QoS (Quality of Service) request in the SMB service.
     *
     * This structure holds the status and unique identifier associated with a QoS request.
     */
    typedef struct ZOO_SMB_QOS_STATE_STRUCT
    {
        ZOO_SMB_MSG_ST_ENUM status; /**< Indicates if the QoS entity is valid */
        uint64_t request_id;        /**< Unique identifier for the QoS request */
        void* msg;
        uint64_t timestamp_ms; /**< Timestamp when the QoS request was created */
        ZOO_BOOL matched;     /**< Whether QoS negotiation matched successfully */
        uint32_t incompatibility_mask; /**< DDS-like incompatibility mask */
    } ZOO_SMB_QOS_STATE_STRUCT;

    /**
     * @brief Creates and initializes a new SMB QoS (Quality of Service) context.
     *
     * This function allocates and returns a handle to a new QoS context,
     * which can be used to manage and enforce SMB-specific quality of service
     * parameters.
     *
     * @return ZOO_SMB_QOS_CTX_HANDLE Handle to the newly created QoS context.
     *         Returns NULL on failure.
     */
    ZOO_SMB_QOS_CTX_HANDLE zoo_smb_create_qos_ctx(IN const ZOO_SMB_QOS_POLICY_HANDLE qos_policy);

    /**
     * @brief Destroys a QoS (Quality of Service) context.
     *
     * This function releases all resources associated with the specified
     * QoS context handle. After calling this function, the handle should
     * not be used in any further operations.
     *
     * @param qos_ctx The handle to the QoS context to be destroyed.
     */
    void zoo_smb_destroy_qos_ctx(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx);

    /**
     * @brief Retrieves the QoS entity handle associated with the given QoS context.
     *
     * @param qos_ctx The handle to the QoS context.
     * @return The handle to the associated QoS entity.
     */
    ZOO_SMB_QOS_POLICY_HANDLE zoo_smb_qos_ctx_get_qos_policy(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx);

    /**
     * @brief Sets the status for the SMB QoS (Quality of Service) module.
     *
     * This function updates the current status of the SMB QoS service.
     *
     * @param status The new status to set for the SMB QoS module.
     */
    ZOO_ERROR_TYPE zoo_smb_qos_ctx_set_state(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id, IN ZOO_SMB_MSG_ST_ENUM status, IN void* msg);

    /**
     * @brief Sets the status of a specific SMB message in the QoS context.
     *
     * This function updates the status of a message identified by the given request ID
     * within the specified Quality of Service (QoS) context.
     *
     * @param[in] qos_ctx      Handle to the QoS context.
     * @param[in] request_id   Unique identifier for the SMB request/message.
     * @param[in] status       Status to set for the specified message (see ZOO_SMB_MSG_ST_ENUM).
     */
    void zoo_smb_qos_ctx_set_msg_status(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id, IN ZOO_SMB_MSG_ST_ENUM status);

    /**
     * @brief Retrieves the current status of the specified SMB QoS context.
     *
     * @param[in] qos_ctx Handle to the SMB QoS context.
     * @return ZOO_SMB_MSG_ST_ENUM The current status of the QoS context.
     */
    ZOO_SMB_MSG_ST_ENUM zoo_smb_qos_ctx_get_msg_status(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
                                                       IN uint64_t request_id);

    /**
     * @brief Retrieves the QoS message structure associated with the given QoS context handle.
     *
     * @param qos_ctx The handle to the QoS context.
     * @return Pointer to the ZOO_SMB_QOS_STATE_STRUCT associated with the specified context,
     *         or NULL if the context is invalid or no message is available.
     */
    ZOO_SMB_QOS_STATE_STRUCT* zoo_smb_qos_ctx_get_state(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
                                                        IN uint64_t request_id);

    /**
     * @brief Removes a message from the QoS context based on the given request ID.
     *
     * This function removes the message associated with the specified request ID from the
     * provided Quality of Service (QoS) context handle.
     *
     * @param[in] qos_ctx      Handle to the QoS context from which the message will be removed.
     * @param[in] request_id   Unique identifier of the request/message to be removed.
     */
    void zoo_smb_qos_ctx_remove_state(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id);

    /**
     * @brief Checks if the message with the specified request ID in the given QoS context has the specified status.
     *
     * @param qos_ctx      The handle to the QoS context.
     * @param request_id   The unique identifier of the request/message.
     * @param status       The status to check against (of type ZOO_SMB_MSG_ST_ENUM).
     * @return ZOO_TRUE if the message has the specified status, ZOO_FALSE otherwise.
     */
    ZOO_BOOL zoo_smb_qos_ctx_is_msg_status(IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx, IN uint64_t request_id, IN ZOO_SMB_MSG_ST_ENUM status);

    /**
     * @brief Wait for a specific message status with timeout.
     *
     * @param[in] qos_ctx      Handle to QoS context.
     * @param[in] request_id   Request identifier.
     * @param[in] status       Expected status.
     * @param[in] timeout_ms   Timeout in milliseconds.
     * @return ZOO_TRUE if the status is reached before timeout, ZOO_FALSE otherwise.
     */
    ZOO_BOOL zoo_smb_qos_ctx_wait_for_status(
        IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
        IN uint64_t request_id,
        IN ZOO_SMB_MSG_ST_ENUM status,
        IN uint32_t timeout_ms);

    ZOO_ERROR_TYPE zoo_smb_qos_ctx_set_match_result(
        IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
        IN uint64_t request_id,
        IN ZOO_BOOL matched,
        IN uint32_t incompatibility_mask);

    uint32_t zoo_smb_qos_ctx_get_incompatibility_mask(
        IN ZOO_SMB_QOS_CTX_HANDLE qos_ctx,
        IN uint64_t request_id);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_QOS_CTX_H */