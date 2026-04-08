/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_ROUTING_ENGINE
 * File name: zoo_smb_routing_rule.h
 * Description: Routing rule interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-30     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_ROUTING_RULE_H
#define ZOO_SMB_ROUTING_RULE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_types.h"
#include "zoo_util.h"
#include "zoo_smb_transport.h"

#define MAX_RULE_NAME_LENGTH 64  // Maximum length for rule names

    typedef struct ZOO_SMB_RULE_STRUCT* ZOO_SMB_RULE_HANDLE;  // Forward declaration for rule handle

    // define rule structure
    typedef struct ZOO_SMB_RULE_STRUCT
    {
        char name[MAX_RULE_NAME_LENGTH];     // Service name
        ZOO_SMB_SERVICE_HANDLE service;      // Service information associated with the rule
        ZOO_SMB_TRANSPORT_HANDLE transport;  // Transport handle for the rule
        ZOO_LIST_HANDLE incoming_data_observers;
        void* engine;
    } ZOO_SMB_RULE_STRUCT;  // Forward declaration for service info structure

    /**
     * @brief Creates a new SMB routing rule.
     *
     * This function initializes and creates a routing rule for SMB (Server Message Block) services,
     * associating it with the specified service information, transport configuration, transport handle,
     * QoS policy, and node type.
     *
     * @param name           The name of the routing rule.
     * @param service   Pointer to the structure containing SMB service information.
     * @param config         Pointer to the transport configuration structure.
     * @param transporte     Handle to the transport instance to be used.
     * @param qos_policy     Pointer to the QoS (Quality of Service) policy structure.
     * @param node_type      The type of node for which the rule is being created.
     * @return ZOO_SMB_RULE_HANDLE
     *         Handle to the newly created routing rule, or NULL on failure.
     */
    ZOO_SMB_RULE_HANDLE zoo_smb_create_routing_rule(
        IN const char* name,
        IN ZOO_SMB_SERVICE_HANDLE service,
        IN ZOO_SMB_TRANSPORT_HANDLE transport,
        IN ZOO_LIST_HANDLE observers);

    /**
     * @brief Retrieves the name of the specified SMB routing rule.
     *
     * This function returns a pointer to a constant string containing the name
     * of the routing rule associated with the given rule handle.
     *
     * @param[in] rule The handle to the SMB routing rule.
     * @return A pointer to a constant character string representing the rule name.
     *         The returned pointer is valid as long as the rule handle is valid.
     */
    const char* zoo_smb_get_routing_rule_name(IN ZOO_SMB_RULE_HANDLE rule);

    /**
     * @brief Destroys a SMB routing rule and releases associated resources.
     *
     * This function deallocates any memory and resources associated with the specified
     * SMB routing rule handle. After calling this function, the rule handle becomes invalid
     * and should not be used in subsequent operations.
     *
     * @param[in] rule The handle to the SMB routing rule to be destroyed.
     */
    void zoo_smb_destroy_routing_rule(IN ZOO_SMB_RULE_HANDLE rule);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_ROUTING_ENGINE_H */