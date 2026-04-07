/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SERVER
 * File name: zoo_smb_server.h
 * Description: Server node interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_SERVER_H
#define ZOO_SMB_SERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_error.h"
#include "zoo_smb_types.h"
#include "zoo_smb_qos.h"

    typedef struct ZOO_SMB_SERVER_STRUCT* ZOO_SMB_SERVER_HANDLE;

    /**
     * @brief Create a new server node.
     *
     * This function creates a new server node for the Soft Message Bus.
     *
     * @param name   Name of the server node.
     * @param target Target address or identifier for the server.
     * @param topic  Name of the topic to serve.
     * @return ZOO_SMB_SERVER_HANDLE Handle to the newly created server node, or NULL on error.
     */
    ZOO_SMB_SERVER_HANDLE zoo_smb_create_server(
        IN const char* name,
        IN const char* target,
        IN const char* topic,
        IN ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type,
        IN const ZOO_SMB_QOS_POLICY_STRUCT* policy);

    /**
     * @brief Destroys and cleans up resources associated with a SMB server instance
     *
     * This function cleans up and releases all resources that were allocated for
     * the SMB server instance. After calling this function, the server instance
     * should no longer be used.
     *
     * @param server Pointer to the SMB server instance to destroy
     */
    void zoo_smb_destroy_server(IN ZOO_SMB_SERVER_HANDLE server);

    /**
     * @brief Send a reply message from the server.
     *
     * This function sends a reply message to a client from the server node.
     *
     * @param node         Handle to the server node.
     * @param topic        Name of the reply topic.
     * @param msg_id       Message ID to reply to.
     * @param payload      Pointer to the reply payload data.
     * @param payload_size Size of the reply payload in bytes.
     * @param request_id   Pointer to the request ID (may be updated).
     * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_server_send_reply(
        IN ZOO_SMB_SERVER_HANDLE server,
        IN const char* receiver,
        IN uint32_t msg_id,
        IN const void* payload,
        IN size_t payload_size,
        IN int64_t request_id);

    /**
     * @brief Set the event handler for the server node.
     *
     * This function sets the event handler callback for the server node to handle incoming requests.
     *
     * @param server          Handle to the server node.
     * @param event_handler Pointer to the event handler function.
     * @param user_data     Pointer to user-defined data to be passed to the event handler.
     */
    ZOO_ERROR_TYPE zoo_smb_server_set_message_handler(
        IN ZOO_SMB_SERVER_HANDLE server,
        IN ZOO_SMB_MSG_HANDLER message_handler,
        IN void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_SERVER_H */