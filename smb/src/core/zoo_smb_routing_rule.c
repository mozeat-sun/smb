/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_ROUTING_ENGINE
 * File name: zoo_smb_routing_rule.c
 * Description: Routing rule interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-30     weiwang.sun       created
 ******************************************************************************/
#include "zoo_smb_routing_rule.h"
#include "../../memory_pool/inc/zoo_memory_pool.h"
#include <stdio.h>
/**
 * @brief Creates a new SMB routing rule.
 *
 * This function initializes and creates a routing rule for SMB (Server Message Block) services,
 * associating it with the specified service information, transport configuration, transport handle,
 * QoS policy, and node type.
 *
 * @param name           The name of the routing rule.
 * @param service   Pointer to the structure containing SMB service information.
 * @param transport      Handle to the transport instance to be used.
 * @param observers      Handle to the linked list of observers for incoming data.
 * @return ZOO_SMB_RULE_HANDLE
 *         Handle to the newly created routing rule, or NULL on failure.
 */
ZOO_SMB_RULE_HANDLE zoo_smb_create_routing_rule(
    IN const char* name,
    IN ZOO_SMB_SERVICE_HANDLE service,
    IN ZOO_SMB_TRANSPORT_HANDLE transport,
    IN ZOO_LIST_HANDLE observers)
{
    if (!name || !service || !transport || !observers)
    {
        ZOO_LOG_ERROR("Invalid parameters for creating routing rule.");
        return NULL;
    }

    ZOO_SMB_RULE_STRUCT* rule = (ZOO_SMB_RULE_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_RULE_STRUCT));
    if (!rule)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for routing rule.");
        return NULL;
    }

    snprintf(rule->name, sizeof(rule->name), "%s", name);
    rule->name[MAX_RULE_NAME_LENGTH - 1] = '\0';  // Ensure null-termination
    rule->service = service;
    rule->transport = transport;
    rule->incoming_data_observers = observers;
    return (ZOO_SMB_RULE_HANDLE)rule;
}

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
const char* zoo_smb_get_routing_rule_name(IN ZOO_SMB_RULE_HANDLE rule)
{
    if (!rule)
    {
        ZOO_LOG_ERROR("Invalid rule handle provided.");
        return NULL;
    }

    ZOO_SMB_RULE_STRUCT* rule_struct = (ZOO_SMB_RULE_STRUCT*)rule;
    return rule_struct->name;
}

/**
 * @brief Destroys a SMB routing rule and releases associated resources.
 *
 * This function deallocates any memory and resources associated with the specified
 * SMB routing rule handle. After calling this function, the rule handle becomes invalid
 * and should not be used in subsequent operations.
 *
 * @param[in] rule The handle to the SMB routing rule to be destroyed.
 */
void zoo_smb_destroy_routing_rule(IN ZOO_SMB_RULE_HANDLE rule)
{
    if (rule)
    {
        zoo_free_to_pool(rule);
    }
}
