/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_RULE_MANAGER
 * File name: zoo_smb_rule_manager.h
 * Description: Routing rule interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-30     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_RULE_MANAGER_H
#define ZOO_SMB_RULE_MANAGER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_types.h"
#include "zoo_smb_routing_rule.h"
#include "zoo_smb_node.h"

    typedef struct ZOO_SMB_RULE_MANAGER_STRUCT* ZOO_SMB_RULE_MANAGER_HANDLE;  // Forward declaration for rule handle

    /**
     * @brief Creates and initializes a new SMB rule manager instance.
     *
     * This function allocates and configures a new rule manager based on the provided
     * configuration structure. The returned handle can be used to manage SMB rules
     * throughout the application's lifecycle.
     *
     * @param config Pointer to a ZOO_SMB_CONFIG_STRUCT containing the configuration
     *               parameters for the rule manager. Must not be NULL.
     * @return A handle to the newly created rule manager (ZOO_SMB_RULE_MANAGER_HANDLE),
     *         or NULL if creation fails.
     */
    ZOO_SMB_RULE_MANAGER_HANDLE zoo_smb_create_rule_manager();

    /**
     * @brief Destroys the specified SMB rule manager and releases associated resources.
     *
     * This function cleans up and deallocates all resources held by the given
     * ZOO_SMB_RULE_MANAGER_HANDLE. After calling this function, the handle should
     * not be used in any further operations.
     *
     * @param[in] rule_manager Handle to the SMB rule manager to be destroyed.
     */
    void zoo_smb_destroy_rule_manager(IN ZOO_SMB_RULE_MANAGER_HANDLE rule_manager);

    /**
     * @brief Creates a new SMB rule and returns its handle.
     *
     * This function initializes and creates a new rule within the SMB rule manager.
     *
     * @return ZOO_SMB_RULE_HANDLE Handle to the newly created SMB rule.
     */
    ZOO_SMB_RULE_HANDLE zoo_smb_rule_manager_make_rule(IN ZOO_SMB_RULE_MANAGER_HANDLE rule_manager,
                                                   IN ZOO_SMB_NODE_HANDLE node,
                                                   IN ZOO_SMB_SERVICE_HANDLE service,
                                                   IN ZOO_SMB_TRANSPORT_HANDLE transport);

    /**
     * @brief Retrieves a rule handle by its name from the specified rule manager.
     *
     * @param rule_manager Handle to the rule manager instance.
     * @param rule_name    Name of the rule to retrieve.
     * @return ZOO_SMB_RULE_HANDLE Handle to the requested rule, or NULL if not found.
     */
    ZOO_SMB_RULE_HANDLE zoo_smb_rule_manager_get_rule(IN ZOO_SMB_RULE_MANAGER_HANDLE rule_manager,
                                                      IN const char* rule_name);

    /**
     * @brief Adds a new rule to the specified SMB rule manager.
     *
     * This function inserts the given rule into the rule manager instance.
     *
     * @param[in] rule_manager Handle to the SMB rule manager where the rule will be added.
     * @param[in] rule Pointer to a ZOO_SMB_RULE_STRUCT structure containing the rule to add.
     */
    ZOO_ERROR_TYPE zoo_smb_rule_manager_add_rule(IN ZOO_SMB_RULE_MANAGER_HANDLE rule_manager,
                                                     IN const ZOO_SMB_RULE_HANDLE rule);

    /**
     * @brief Removes a rule from the SMB rule manager.
     *
     * This function attempts to remove a specified rule from the SMB rule manager.
     *
     * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_rule_manager_remove_rule(
        IN ZOO_SMB_RULE_MANAGER_HANDLE manager,
        IN ZOO_SMB_RULE_HANDLE rule);
#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_ROUTING_RULE_MANAGER_H */