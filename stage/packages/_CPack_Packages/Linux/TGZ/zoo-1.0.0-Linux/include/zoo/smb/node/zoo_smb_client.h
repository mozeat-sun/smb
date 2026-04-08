/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_CLIENT
 * File name: zoo_smb_client.h
 * Description: RPC interface for ZOO Soft Message Bus (SMB)
 *              Defines APIs for request/reply messaging, payload access,
 *              reply sending, and event handler registration.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     your.name         created
 ******************************************************************************/
#ifndef ZOO_SMB_CLIENT_H
#define ZOO_SMB_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo_smb_error.h"
#include "zoo_smb_types.h"
#include "zoo_smb_qos.h"

    typedef struct ZOO_SMB_CLIENT_STRUCT* ZOO_SMB_CLIENT_HANDLE;

    /**
     * @brief Creates a new SMB client node.
     *
     * This function initializes and returns a new SMB client node with the specified name and target.
     *
     * @param name   The name to assign to the SMB client node.
     * @param target The target address or identifier for the SMB client connection.
     * @param topic  The topic string to associate with the SMB client node.
     * @return ZOO_SMB_NODE  The created SMB client node.
     */
    ZOO_SMB_CLIENT_HANDLE zoo_smb_create_client(
        IN const char* name,
        IN const char* target,
        IN const char* topic,
        IN const ZOO_SMB_QOS_POLICY_STRUCT* policy);

    /**
     * @brief Checks if the SMB server is ready and accessible
     *
     * This function verifies whether the specified SMB server is ready to accept
     * connections and handle requests.
     *
     * @param target The target SMB server address or hostname to check
     *
     * @return ZOO_TRUE if the server is ready and accessible, ZOO_FALSE otherwise
     */
    ZOO_BOOL zoo_smb_is_server_ready(const char* target);

    /**
     * @brief Destroys a SMB client and releases all associated resources
     *
     * This function cleans up and frees all resources allocated for the SMB client.
     * The client handle becomes invalid after this call and should not be used anymore.
     *
     * @param client Pointer to the SMB client handle to be destroyed
     */
    void zoo_smb_destroy_client(IN ZOO_SMB_CLIENT_HANDLE client);

    /**
     * @brief Sends a request to a specified SMB node with the given topic and payload.
     *
     * This function sends a request message to the specified SMB node, using the provided topic and payload data.
     * The function assigns or updates the request ID and waits for a response up to the specified timeout.
     *
     * @param node         The target SMB node to which the request will be sent.
     * @param topic        The topic string identifying the type or category of the request.
     * @param payload      Pointer to the payload data to be sent with the request.
     * @param payload_size Size of the payload data in bytes.
     * @param request_id   Pointer to an int64_t variable. On input, may specify a request ID; on output, receives the assigned request ID.
     * @return             ZOO_ERROR_TYPE indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_client_send_request(ZOO_SMB_CLIENT_HANDLE client,
                                                  IN uint32_t msg_id,
                                                  IN const void* payload,
                                                  IN size_t payload_size,
                                                  INOUT int64_t* request_id);

    /**
     * @brief Retrieves the reply message for a previously sent SMB request.
     *
     * This function waits for a reply to a request identified by `request_id` on the specified `node`.
     * The reply message and its size are returned via output parameters. The function will wait up to
     * `timeout_ms` milliseconds for the reply before timing out.
     *
     * @param node           The SMB node handle from which to retrieve the reply.
     * @param request_id     The unique identifier of the request whose reply is to be fetched.
     * @param payload        [out] Pointer to a buffer that will receive the reply message.
     * @param payload_size   [out] Pointer to a variable that will receive the size of the reply message.
     * @param timeout_ms     The maximum time to wait for the reply, in milliseconds.
     * @return ZOO_ERROR_TYPE  Error code indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_client_recv_reply(ZOO_SMB_CLIENT_HANDLE client,
                                                 IN uint32_t msg_id,
                                                 IN int64_t request_id,
                                                 OUT void** payload,
                                                 OUT size_t* payload_size,
                                                 IN uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
#endif /* ZOO_SMB_CLIENT_H */
