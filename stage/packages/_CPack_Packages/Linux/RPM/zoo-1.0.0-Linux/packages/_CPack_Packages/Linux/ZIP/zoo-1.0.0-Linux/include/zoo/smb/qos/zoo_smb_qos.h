/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_QOS
 * File name: zoo_smb_qos.h
 * Description: Quality of Service (QoS) definitions for ZOO SMB
 *              Similar to DDS QoS profiles, these settings control message
 *              delivery characteristics, reliability, and resource usage.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-18     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_QOS_H
#define ZOO_SMB_QOS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_qos_policy.h"
#include "zoo_smb_qos_ctx.h"
#include "zoo_smb_message.h"
#include <stdint.h>

    typedef struct
    {
        ZOO_BOOL matched;                 /**< Whether publisher/subscriber QoS matched */
        uint32_t incompatibility_mask;    /**< DDS-like incompatibility reason mask */
    } ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT;

    typedef struct ZOO_SMB_QOS_ENTITY_STRUCT* ZOO_SMB_QOS_ENTITY_HANDLE;

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
    ZOO_SMB_QOS_ENTITY_HANDLE zoo_smb_create_qos_entity(IN const ZOO_SMB_QOS_POLICY_STRUCT* policy);

    /**
     * @brief Creates a default QoS (Quality of Service) entity handle for SMB.
     *
     * This function initializes and returns a handle to a default QoS entity,
     * which can be used to manage or apply QoS policies in the SMB context.
     *
     * @return ZOO_SMB_QOS_ENTITY_HANDLE Handle to the newly created default QoS entity.
     */
    ZOO_SMB_QOS_ENTITY_HANDLE zoo_smb_create_qos_default_entity();

    /**
     * @brief Destroys a QoS (Quality of Service) entity.
     *
     * This function releases all resources associated with the specified QoS entity handle.
     * After calling this function, the handle becomes invalid and should not be used.
     *
     * @param[in] qos_entity Handle to the QoS entity to be destroyed.
     */
    void zoo_smb_destroy_qos_entity(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity);

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
        IN const char * name,
        IN const char * topic,
        IN ZOO_SMB_MSG_TYPE_ENUM msg_type,
        IN uint32_t msg_id,
        IN uint64_t request_id);

    /**
     * @brief Retrieves the QoS policy handle associated with the given QoS context.
     *
     * This function returns the policy handle for the specified QoS context.
     *
     * @param[in] qos_ctx The handle to the QoS context.
     * @return ZOO_SMB_QOS_POLICY_HANDLE The handle to the QoS policy.
     */
    ZOO_SMB_QOS_POLICY_HANDLE zoo_smb_qos_get_policy(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity);

    /**
     * @brief Retrieves the QoS context handle associated with a given QoS entity.
     *
     * This function returns the QoS context handle for the specified QoS entity handle.
     *
     * @param[in] qos_entity The handle to the QoS entity for which the context is requested.
     * @return ZOO_SMB_QOS_CTX_HANDLE The handle to the QoS context associated with the given entity.
     */
    ZOO_SMB_QOS_CTX_HANDLE zoo_smb_qos_get_ctx(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity);

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
                                                                 IN uint64_t request_id);

    /**
     * @brief Waits for the QoS negotiation to complete.
     *
     * This function blocks until the QoS negotiation for the specified request ID
     * reaches the given status. It will return an error if the negotiation times out.
     *
     * @param qos_entity The handle to the QoS entity.
     * @param request_id The unique identifier for the QoS negotiation request.
     * @param status The expected status of the QoS negotiation.
     * @return ZOO_ERROR_TYPE Returns ZOO_SMB_OK on success, or an error code on failure.
     */
    ZOO_ERROR_TYPE zoo_smb_qos_wait_for_msg_status(IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
                                                       IN uint64_t request_id,
                                                       IN ZOO_SMB_MSG_ST_ENUM status);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_QOS_H */