/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TCP_SERVER
 * File name: zoo_smb_transport_tcp_server.h
 * Description: TCP server transport implementation
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_TCP_SERVER_H
#define ZOO_SMB_TRANSPORT_TCP_SERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_transport_tcp_common.h"
#include "zoo_list.h"



    /**
     * @brief TCP Server Transport Structure
     */



    /* Forward declarations for type compatibility */
    typedef struct TCP_SERVER_TRANSPORT_STRUCT TCP_SERVER_TRANSPORT_STRUCT;
    /* Forward declaration only; do not typedef, already defined in common header */
    struct TCP_CLIENT_INFO_STRUCT;

    /**
     * @brief Callback type for received messages on the server.
     *
     * @param server Pointer to the server transport struct.
     * @param client Pointer to the client info struct.
     * @param msg Pointer to the received message struct (ownership not transferred).
     * @param user_data User-provided context pointer.
     */
    typedef void (*tcp_server_message_callback_t)(TCP_SERVER_TRANSPORT_STRUCT* server, TCP_CLIENT_INFO_STRUCT* client, const ZOO_SMB_MSG_STRUCT* msg, void* user_data);


    struct TCP_SERVER_TRANSPORT_STRUCT {
        ZOO_SMB_TCP_TRANSPORT_COMMON common; /**< Common TCP transport base */

        // Server-specific fields
        int listen_fd;                     /**< Listening socket file descriptor */
        int max_clients;                   /**< Maximum allowed client connections */
        ZOO_LIST_HANDLE clients; /**< List of connected clients */

        // Server configuration
        int listen_backlog;          /**< Listen socket backlog */
        ZOO_BOOL accept_new_connections; /**< Flag to control new connection acceptance */

        // Message receive callback
        tcp_server_message_callback_t message_callback; /**< Callback for received messages */
        void* message_callback_user_data; /**< User data for callback */
    };

    /**
     * @brief Register a callback for received messages on the server.
     *
     * The callback will be invoked for each successfully parsed message received from any client.
     *
     * @param server TCP server transport structure
     * @param callback Function pointer to the callback (NULL to unregister)
     * @param user_data User data pointer to pass to callback
     */
    void tcp_server_register_message_callback(TCP_SERVER_TRANSPORT_STRUCT* server, tcp_server_message_callback_t callback, void* user_data);

    // ==============================================================================
    // SERVER LIFECYCLE FUNCTIONS
    // ==============================================================================

    /**
     * @brief Initialize TCP server transport
     * @param config Transport configuration
     * @return Pointer to initialized server transport, NULL on failure
     */
    ZOO_ERROR_TYPE tcp_server_init(const ZOO_SMB_TRANSPORT_STRUCT* transport);

    /**
     * @brief Destroy TCP server transport and free resources
     * @param impl Server transport implementation pointer
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_destroy(void* impl);

    /**
     * @brief Start TCP server transport (bind and listen)
     * @param impl Server transport implementation pointer
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_start(void* impl);

    /**
     * @brief Stop TCP server transport
     * @param impl Server transport implementation pointer
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_stop(void* impl);

    /**
     * @brief Send message through TCP server transport
     * @param impl Server transport implementation pointer
     * @param msg Message data to send
     * @param size Size of message data
     * @param consumer Target consumer (NULL for broadcast)
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver);

    /**
     * @brief Check if TCP server transport is started
     * @param impl Server transport implementation pointer
     * @return ZOO_TRUE if started, ZOO_FALSE otherwise
     */
    ZOO_BOOL tcp_server_is_started(void* impl);

    // ==============================================================================
    // SERVER SOCKET FUNCTIONS
    // ==============================================================================

    /**
     * @brief Create and configure server listening socket
     * @param server TCP server transport structure
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_create_listen_socket(TCP_SERVER_TRANSPORT_STRUCT* server);

    /**
     * @brief Bind server socket to specified address and port
     * @param server TCP server transport structure
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_bind_socket(TCP_SERVER_TRANSPORT_STRUCT* server);

    /**
     * @brief Start listening for incoming connections
     * @param server TCP server transport structure
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_start_listening(TCP_SERVER_TRANSPORT_STRUCT* server);

    // ==============================================================================
    // CLIENT MANAGEMENT FUNCTIONS
    // ==============================================================================

    /**
     * @brief Accept new client connection
     * @param server TCP server transport structure
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_accept_client(TCP_SERVER_TRANSPORT_STRUCT* server);

    /**
     * @brief Add client to server's client list
     * @param server TCP server transport structure
     * @param client_fd Client socket file descriptor
     * @param client_addr Client address information
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_add_client(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd, struct sockaddr_in* client_addr);

    /**
     * @brief Remove client from server's client list
     * @param server TCP server transport structure
     * @param client_fd Client socket file descriptor
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_remove_client(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd);

    /**
     * @brief Find client information by file descriptor
     * @param server TCP server transport structure
     * @param client_fd Client socket file descriptor
     * @return Pointer to client info, NULL if not found
     */
    TCP_CLIENT_INFO_STRUCT* tcp_server_find_client(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd);

    /**
     * @brief Broadcast message to all connected clients
     * @param server TCP server transport structure
     * @param msg Message data to broadcast
     * @param size Size of message data
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_broadcast_message(TCP_SERVER_TRANSPORT_STRUCT* server, const ZOO_SMB_MSG_STRUCT* msg);

    /**
     * @brief Send message to specific client
     * @param server TCP server transport structure
     * @param client_fd Target client file descriptor
     * @param msg Message data to send
     * @param size Size of message data
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_send_to_client(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd, const void* msg, size_t size);

    /**
     * @brief Clean up disconnected clients
     * @param server TCP server transport structure
     */
    void tcp_server_cleanup_disconnected_clients(TCP_SERVER_TRANSPORT_STRUCT* server);

    // ==============================================================================
    // SERVER EVENT LOOP FUNCTIONS
    // ==============================================================================

    /**
     * @brief Main server event loop
     * @param server TCP server transport structure
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_server_event_loop(TCP_SERVER_TRANSPORT_STRUCT* server);

    /**
     * @brief Process server events
     * @param server TCP server transport structure
     * @param events Array of epoll events
     * @param event_count Number of events
     */
    void tcp_server_process_events(TCP_SERVER_TRANSPORT_STRUCT* server, struct epoll_event* events, int event_count);

    /**
     * @brief Handle events for listening socket
     * @param server TCP server transport structure
     * @param events Epoll events
     */
    void tcp_server_handle_listen_events(TCP_SERVER_TRANSPORT_STRUCT* server, uint32_t events);

    /**
     * @brief Handle events for client socket
     * @param server TCP server transport structure
     * @param client_fd Client socket file descriptor
     * @param events Epoll events
     */
    void tcp_server_handle_client_events(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd, uint32_t events);

#ifdef __cplusplus
}
#endif

#endif  // ZOO_SMB_TRANSPORT_TCP_SERVER_H