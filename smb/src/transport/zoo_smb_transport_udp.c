/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TYPE_UDP
 * File name: zoo_smb_transport_udp.c
 * Description: UDP transport implementation for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-20    weiwang.sun      created
 ******************************************************************************/

#include "zoo_smb_transport_udp.h"
#include "zoo_smb_transport_udp_client.h"
#include "zoo_smb_transport_udp_server.h"
/**
 * @brief Initialize UDP transport by dispatching to appropriate client or server implementation
 * @details Examines transport configuration to determine whether to initialize
 *          as UDP client or server, then delegates to the appropriate initialization
 *          function. This provides a unified entry point for UDP transport creation.
 * @param[in] transport Pointer to transport configuration structure containing
 *                      server/client mode flag and connection parameters
 * @return ZOO_SMB_OK on successful initialization,
 *         ZOO_SMB_ERROR_INVALID_PARAM if transport or config is NULL,
 *         error code from udp_client_init or udp_server_init otherwise
 */
static ZOO_ERROR_TYPE udp_init(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (!transport || !transport->config)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_BOOL is_server = transport->config->is_server;

    if (is_server)
    {
        return udp_server_init(transport);
    }
    else
    {
        return udp_client_init(transport);
    }
}

/**
 * @brief Destroy UDP transport by dispatching to appropriate client or server cleanup
 * @details Determines whether the transport implementation is client or server based
 *          on the is_server flag, then delegates to the appropriate destruction function.
 *          Ensures proper resource cleanup regardless of transport type.
 * @param[in] impl Pointer to UDP transport implementation (client or server)
 *                 Can be NULL, in which case function returns immediately
 */
static void udp_destroy(void* impl)
{
    if (!impl)
    {
        return;
    }

    ZOO_SMB_UDP_TRANSPORT_COMMON* common = (ZOO_SMB_UDP_TRANSPORT_COMMON*)impl;
    ZOO_BOOL is_server = common->is_server;

    if (is_server)
    {
        udp_server_destroy(impl);
    }
    else
    {
        udp_client_destroy(impl);
    }
}

/**
 * @brief Start UDP transport operations by dispatching to client or server start
 * @details Identifies transport type from common structure and delegates to the
 *          appropriate start function. This begins message processing threads and
 *          enables transport functionality.
 * @param[in] impl Pointer to UDP transport implementation (client or server)
 * @return ZOO_SMB_OK on successful start,
 *         ZOO_SMB_ERROR_INVALID_PARAM if impl is NULL,
 *         error code from udp_client_start or udp_server_start otherwise
 */
static ZOO_ERROR_TYPE udp_start(void* impl)
{
    if (!impl)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_UDP_TRANSPORT_COMMON* common = (ZOO_SMB_UDP_TRANSPORT_COMMON*)impl;
    ZOO_BOOL is_server = common->is_server;

    return is_server ? udp_server_start(impl) : udp_client_start(impl);
}

/**
 * @brief Stop UDP transport operations by dispatching to client or server stop
 * @details Determines transport type and delegates to the appropriate stop function.
 *          This gracefully terminates message processing threads and disables
 *          transport functionality while preserving the transport structure.
 * @param[in] impl Pointer to UDP transport implementation (client or server)
 * @return ZOO_SMB_OK on successful stop,
 *         ZOO_SMB_ERROR_INVALID_PARAM if impl is NULL,
 *         error code from udp_client_stop or udp_server_stop otherwise
 */
static ZOO_ERROR_TYPE udp_stop(void* impl)
{
    if (!impl)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_UDP_TRANSPORT_COMMON* common = (ZOO_SMB_UDP_TRANSPORT_COMMON*)impl;
    ZOO_BOOL is_server = common->is_server;

    return is_server ? udp_server_stop(impl) : udp_client_stop(impl);
}

/**
 * @brief Send message via UDP transport by dispatching to client or server send
 * @details Routes message transmission request to appropriate implementation based
 *          on transport type. For clients, sends to configured target; for servers,
 *          supports both unicast to specific clients and broadcast to all clients.
 * @param[in] impl Pointer to UDP transport implementation (client or server)
 * @param[in] msg Message structure containing header and payload to transmit
 * @param[in] receiver Target receiver identifier:
 *                     - For clients: ignored, sends to configured target
 *                     - For servers: client name for unicast, "broadcast"/"*" for broadcast
 * @return ZOO_SMB_OK on successful transmission,
 *         ZOO_SMB_ERROR_INVALID_PARAM if impl is NULL,
 *         error code from udp_client_send or udp_server_send otherwise
 */
static ZOO_ERROR_TYPE udp_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver)
{
    if (!impl)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_UDP_TRANSPORT_COMMON* common = (ZOO_SMB_UDP_TRANSPORT_COMMON*)impl;
    ZOO_BOOL is_server = common->is_server;

    return is_server ? udp_server_send(impl, msg, receiver) : udp_client_send(impl, msg, receiver);
}

/**
 * @brief Check if UDP transport is started by dispatching to client or server status
 * @details Determines transport type and delegates to the appropriate status check
 *          function. Returns the current operational state of the transport.
 * @param[in] impl Pointer to UDP transport implementation (client or server)
 * @return ZOO_TRUE if transport is started and running,
 *         ZOO_FALSE if transport is stopped or impl is NULL
 */
static ZOO_BOOL udp_is_started(void* impl)
{
    if (!impl)
    {
        return ZOO_FALSE;
    }

    ZOO_SMB_UDP_TRANSPORT_COMMON* common = (ZOO_SMB_UDP_TRANSPORT_COMMON*)impl;
    ZOO_BOOL is_server = common->is_server;

    return is_server ? udp_server_is_started(impl) : udp_client_is_started(impl);
}

/**
 * @brief UDP transport operations table
 * @details Static structure containing function pointers for all UDP transport
 *          operations. Used by the transport layer for polymorphic operation
 *          dispatch. All functions in this table act as dispatchers to the
 *          appropriate client or server implementation.
 */
static ZOO_SMB_TRANSPORT_OPS_STRUCT udp_ops = {
    .init = udp_init,
    .destroy = udp_destroy,
    .start = udp_start,
    .stop = udp_stop,
    .send = udp_send,
    .is_started = udp_is_started};

/**
 * @brief Register UDP transport module with the SMB transport layer
 * @details Registers the UDP transport operations table with the transport
 *          manager, making UDP transport available for use throughout the
 *          system. This function should be called during system initialization
 *          to enable UDP transport functionality.
 * @note This function is typically called once during application startup
 * @see zoo_smb_transport_register for transport registration details
 */
void zoo_smb_transport_udp_init(void)
{
    zoo_smb_transport_register(ZOO_SMB_TRANSPORT_TYPE_UDP, &udp_ops);
    ZOO_LOG_INFO("UDP transport module registered (dispatch to client/server)");
}
