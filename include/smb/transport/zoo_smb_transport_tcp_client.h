/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TCP_CLIENT
 * File name: zoo_smb_transport_tcp_client.h
 * Description: TCP client transport implementation
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_TCP_CLIENT_H
#define ZOO_SMB_TRANSPORT_TCP_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_transport_tcp_common.h"

    /**
     * @brief TCP Client Transport Structure
     */
    typedef struct
    {
        ZOO_SMB_TCP_TRANSPORT_COMMON common; /**< Common TCP transport base */

        // Client-specific fields
        int max_reconnect_attempts;  /**< Maximum reconnection attempts */
        int current_reconnect_count; /**< Current reconnection attempt count */
        ZOO_BOOL auto_reconnect;         /**< Auto-reconnection flag */
    } TCP_CLIENT_TRANSPORT_STRUCT;

    // ==============================================================================
    // CLIENT LIFECYCLE FUNCTIONS
    // ==============================================================================

    /**
     * @brief Initialize TCP client transport
     * @param config Transport configuration
     * @return Pointer to initialized client transport, NULL on failure
     */
    ZOO_ERROR_TYPE tcp_client_init(const ZOO_SMB_TRANSPORT_STRUCT* transport);

    /**
     * @brief Destroy TCP client transport and free resources
     * @param impl Client transport implementation pointer
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_client_destroy(void* impl);

    /**
     * @brief Start TCP client transport (connect to server)
     * @param impl Client transport implementation pointer
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_client_start(void* impl);

    /**
     * @brief Stop TCP client transport
     * @param impl Client transport implementation pointer
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_client_stop(void* impl);

    /**
     * @brief Send message through TCP client transport
     * @param impl Client transport implementation pointer
     * @param msg Message data to send
     * @param size Size of message data
     * @param consumer Target consumer (ignored for client)
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_client_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver);

    /**
     * @brief Check if TCP client transport is started
     * @param impl Client transport implementation pointer
     * @return ZOO_TRUE if started, ZOO_FALSE otherwise
     */
    ZOO_BOOL tcp_client_is_started(void* impl);

    // ==============================================================================
    // CLIENT CONNECTION FUNCTIONS
    // ==============================================================================

    /**
     * @brief Connect to TCP server
     * @param client TCP client transport structure
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_client_connect_to_server(TCP_CLIENT_TRANSPORT_STRUCT* client);

    /**
     * @brief Handle client connection events
     * @param client TCP client transport structure
     * @param events Epoll events
     */
    void tcp_client_handle_connection_events(TCP_CLIENT_TRANSPORT_STRUCT* client, uint32_t events);

    /**
     * @brief Perform automatic reconnection if enabled
     * @param client TCP client transport structure
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_client_auto_reconnect(TCP_CLIENT_TRANSPORT_STRUCT* client);

    /**
     * @brief Reset client connection state
     * @param client TCP client transport structure
     */
    void tcp_client_reset_connection(TCP_CLIENT_TRANSPORT_STRUCT* client);

    // ==============================================================================
    // CLIENT EVENT LOOP FUNCTIONS
    // ==============================================================================

    /**
     * @brief Main client event loop
     * @param client TCP client transport structure
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_client_event_loop(TCP_CLIENT_TRANSPORT_STRUCT* client);

    /**
     * @brief Process client events
     * @param client TCP client transport structure
     * @param events Array of epoll events
     * @param event_count Number of events
     */
    void tcp_client_process_events(TCP_CLIENT_TRANSPORT_STRUCT* client, struct epoll_event* events, int event_count);

#ifdef __cplusplus
}
#endif

#endif  // ZOO_SMB_TRANSPORT_TCP_CLIENT_H