/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: zoo_smb
 * File name: zoo_smb.h
 * Description: Public interface for the Soft Message Bus
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 * 1.1       2025-05-15     weiwang.sun         updated to match implementation
 ******************************************************************************/

#ifndef ZOO_SMB_H
#define ZOO_SMB_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo_smb_error.h"
#include "zoo_smb_node.h"
#include <stdint.h>
#include <stdbool.h>
#define ZOO_SMB_MAX_NODE_SIZE 1024
    /**
     * @typedef ZOO_SMB_HANDLE
     * @brief Opaque handle to a ZOO SMB structure.
     *
     * This typedef defines a handle for referencing an internal ZOO SMB structure.
     * The actual structure is hidden from the user to provide encapsulation and
     * abstraction. Use this handle with the provided API functions to interact
     * with the ZOO SMB subsystem.
     */
    typedef struct ZOO_SMB_STRUCT* ZOO_SMB_HANDLE;

    /**
     * @brief Retrieves the singleton instance handle for the SMB module.
     *
     * This function returns a handle to the SMB (Server Message Block) instance,
     * allowing access to SMB-related operations. The returned handle should be used
     * with other SMB API functions.
     *
     * @return ZOO_SMB_HANDLE Handle to the SMB instance.
     */
    ZOO_SMB_HANDLE zoo_smb_get_instance(void);

    /**
     * @brief Checks if the specified SMB handle is ready for operations.
     *
     * This function determines whether the given SMB (Server Message Block) handle
     * is properly initialized and ready to be used for further SMB operations.
     *
     * @param[in] smb The handle to the SMB context to check.
     * @return ZOO_TRUE if the SMB handle is ready; ZOO_FALSE otherwise.
     */
    ZOO_BOOL zoo_smb_is_ready();

    /**
     * @brief Registers a SMB node with the system.
     *
     * This function registers the specified SMB node, making it available for further operations.
     *
     * @param node The handle to the SMB node to be registered.
     * @return ZOO_ERROR_TYPE Error code indicating the result of the registration operation.
     */
    ZOO_ERROR_TYPE zoo_smb_register_node(IN ZOO_SMB_NODE_HANDLE node);

    /**
     * @brief Unregisters a node from the Zoo SMB system.
     *
     * This function removes the specified node from the Zoo SMB registry,
     * effectively unregistering it and releasing any associated resources.
     *
     * @param[in] node The handle to the node to be unregistered.
     */
    void zoo_smb_unregister_node(IN ZOO_SMB_NODE_HANDLE node);

    /**
     * @brief Checks if a specified SMB service is currently online/available
     *
     * @param service_name The name of the SMB service to check
     * @return ZOO_TRUE if the service is online and accessible
     * @return ZOO_FALSE if the service is offline or inaccessible
     */
    ZOO_BOOL zoo_smb_service_is_online(const char* service_name);

    /**
     * @brief Routes a message through the specified SMB node.
     *
     * @param node The handle to the SMB node through which the message will be routed.
     * @param msg The message structure containing the data to be sent.
     * @param consumer The handle to the consumer that will receive the message.
     * @param delete_after_send If ZOO_TRUE, the message will be deleted after sending.
     * @return ZOO_ERROR_TYPE Error code indicating the result of the routing operation.
     */
    ZOO_ERROR_TYPE zoo_smb_send_message(IN const ZOO_SMB_NODE_HANDLE node,
                                            IN const ZOO_SMB_MSG_STRUCT* msg,
                                            IN const char * receiver_name,
                                            IN ZOO_BOOL delete_after_send);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_H */