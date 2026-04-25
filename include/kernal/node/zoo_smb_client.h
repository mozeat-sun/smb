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
     * Client routing is discovery-driven; the effective transport is resolved from
     * discovered server/service information when the client associates and sends.
     *
     * @param name   The name to assign to the SMB client node.
     * @param target The target address or identifier for the SMB client connection.
     * @param topic  The topic string to associate with the SMB client node.
     * @param policy Optional QoS policy. Implementation uses default policy when NULL.
     * @return ZOO_SMB_CLIENT_HANDLE The created SMB client handle, or NULL on failure.
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
     * @brief Sends one request message from client context.
     *
     * The function validates service availability, allocates/requests an id when
     * needed, tracks QoS state, and dispatches a REQ message.
     *
     * @param client Client handle.
     * @param msg_id Application message identifier.
     * @param payload Request payload pointer.
     * @param payload_size Request payload size in bytes.
     * @param request_id Input/output request id. If *request_id is 0, a new id is generated.
     * @return ZOO_SMB_OK on send success; otherwise error (for example invalid
     *         parameter, service unavailable, allocation failure, or send failure).
     */
    ZOO_ERROR_TYPE zoo_smb_client_send_request(ZOO_SMB_CLIENT_HANDLE client,
                                                  IN uint32_t msg_id,
                                                  IN const void* payload,
                                                  IN size_t payload_size,
                                                  INOUT int64_t* request_id);

    /**
     * @brief Waits for a reply that matches message id and request id.
     *
     * This call may block up to timeout_ms while waiting for buffered REPL data.
     * On success, ownership of payload buffer is transferred to caller.
     *
     * @param client Client handle.
     * @param msg_id Application message identifier expected in reply.
     * @param request_id Request identifier to match.
     * @param payload Output pointer receiving allocated reply payload.
     * @param payload_size Output payload size in bytes.
     * @param timeout_ms Maximum wait duration in milliseconds.
     * @return ZOO_SMB_OK on success; timeout/not-found/invalid-param or other
     *         module error code on failure.
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
