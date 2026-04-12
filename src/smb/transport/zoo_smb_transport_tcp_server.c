/**
 * @brief Register a callback for received messages on the server.
 *
 * The callback will be invoked for each successfully parsed message received from any client.
 * Thread-safety: This function is not thread-safe. Register callbacks before starting the event loop.
 *
 * @param server TCP server transport structure
 * @param callback Function pointer to the callback (NULL to unregister)
 * @param user_data User data pointer to pass to callback
 */
void tcp_server_register_message_callback(TCP_SERVER_TRANSPORT_STRUCT *server, tcp_server_message_callback_t callback, void *user_data)
{
    if (!server)
        return;
    server->message_callback = callback;
    server->message_callback_user_data = user_data;
}
/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TCP_SERVER
 * File name: zoo_smb_transport_tcp_server.c
 * Description: TCP server transport implementation
 ******************************************************************************/

#include "zoo_smb_transport_tcp_server.h"
#include "zoo_smb_error.h"
#include "zoo_memory_pool.h"
#include <errno.h>

/**
 * @brief Initializes TCP server transport implementation storage.
 *
 * @param transport Transport instance with valid TCP server configuration.
 * @return ZOO_SMB_OK on success, or an error code when validation/allocation fails.
 */
/**
 * @brief Initialize TCP server transport implementation storage.
 *
 * Allocates and initializes the TCP server transport structure and its client list.
 *
 * @param transport Transport instance with valid TCP server configuration.
 * @return ZOO_SMB_OK on success, or an error code when validation/allocation fails.
 */
ZOO_ERROR_TYPE tcp_server_init(const ZOO_SMB_TRANSPORT_STRUCT *transport)
{
    if (!transport || !transport->config)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    TCP_SERVER_TRANSPORT_STRUCT *server =
        (TCP_SERVER_TRANSPORT_STRUCT *)zoo_allocate_from_pool(sizeof(TCP_SERVER_TRANSPORT_STRUCT));
    if (!server)
    {
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    memset(server, 0, sizeof(TCP_SERVER_TRANSPORT_STRUCT));
    server->common.is_server = ZOO_TRUE;
    server->common.running = ZOO_FALSE;
    server->common.conn_fd = INVALID_TRANSPORT_FD;
    server->common.epoll_fd = INVALID_TRANSPORT_FD;
    server->common.conn_state = TRANSPORT_CONN_STATE_DISCONNECTED;
    server->common.config = (ZOO_SMB_TRANSPORT_CONFIG_STRUCT *)transport->config;
    server->max_clients = TCP_MAX_CLIENTS;
    server->listen_backlog = TCP_DEFAULT_BACKLOG;
    server->accept_new_connections = ZOO_TRUE;
    server->clients = zoo_list_create(server->max_clients);

    if (!server->clients)
    {
        zoo_free_to_pool(server);
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    ((ZOO_SMB_TRANSPORT_STRUCT *)transport)->impl = server;
    return ZOO_SMB_OK;
}

/**
 * @brief Destroy TCP server transport and release resources.
 *
 * Frees the TCP server transport structure and destroys its client list.
 *
 * @param impl Server transport pointer.
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_server_destroy(void *impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    TCP_SERVER_TRANSPORT_STRUCT *server = (TCP_SERVER_TRANSPORT_STRUCT *)impl;
    tcp_server_stop(server);
    if (server->clients)
    {
        zoo_list_destroy(server->clients);
    }
    zoo_free_to_pool(server);
    return ZOO_SMB_OK;
}

/**
 * @brief Start TCP server transport.
 *
 * Sets the running and connection state flags to indicate the server is started.
 *
 * @param impl Server transport pointer.
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_server_start(void *impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    TCP_SERVER_TRANSPORT_STRUCT *server = (TCP_SERVER_TRANSPORT_STRUCT *)impl;
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT *config = server->common.config;
    if (!config)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    // Create socket
    ZOO_ERROR_TYPE err = zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &server->common.socket_info);
    if (err != ZOO_SMB_OK)
        return err;

    // Prepare address
    ZOO_SOCKADDR_UNION addr;
    memset(&addr, 0, sizeof(addr));
    addr.in.family = ZOO_AF_INET;
    addr.in.port = config->port;
    // TODO: Convert config->address (string) to uint32_t IP (host byte order)
    // For now, bind to any address
    addr.in.addr = ZOO_INADDR_ANY;

    err = zoo_socket_bind(&server->common.socket_info, &addr);
    if (err != ZOO_SMB_OK)
        return err;

    err = zoo_socket_listen(&server->common.socket_info, server->listen_backlog);
    if (err != ZOO_SMB_OK)
        return err;

    server->listen_fd = server->common.socket_info.fd;
    server->common.running = ZOO_TRUE;
    server->common.conn_state = TRANSPORT_CONN_STATE_CONNECTED;
    return ZOO_SMB_OK;
}

/**
 * @brief Stop TCP server transport.
 *
 * Sets the running and connection state flags to indicate the server is stopped.
 *
 * @param impl Server transport pointer.
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_server_stop(void *impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    TCP_SERVER_TRANSPORT_STRUCT *server = (TCP_SERVER_TRANSPORT_STRUCT *)impl;
    // Close listening socket
    zoo_socket_close(&server->common.socket_info);
    server->listen_fd = INVALID_TRANSPORT_FD;
    server->common.running = ZOO_FALSE;
    server->common.conn_state = TRANSPORT_CONN_STATE_DISCONNECTED;
    // Close all client sockets and clean up client list
    if (server->clients)
    {
        size_t count = zoo_list_size(server->clients);
        for (size_t i = 0; i < count; ++i)
        {
            TCP_CLIENT_INFO_STRUCT *client = (TCP_CLIENT_INFO_STRUCT *)zoo_list_at(server->clients, i);
            if (client)
            {
                zoo_socket_close(&client->socket_info);
                zoo_free_to_pool(client);
            }
        }
        zoo_list_clear(server->clients);
    }
    return ZOO_SMB_OK;
}

/**
 * @brief Send a message through TCP server transport (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param impl Server transport pointer.
 * @param msg  Message data pointer.
 * @param receiver Target receiver (unused).
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_send(void *impl, const ZOO_SMB_MSG_STRUCT *msg, const char *receiver)
{
    if (!impl || !msg)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    TCP_SERVER_TRANSPORT_STRUCT *server = (TCP_SERVER_TRANSPORT_STRUCT *)((ZOO_SMB_TRANSPORT_STRUCT *)impl)->impl ? (TCP_SERVER_TRANSPORT_STRUCT *)((ZOO_SMB_TRANSPORT_STRUCT *)impl)->impl : (TCP_SERVER_TRANSPORT_STRUCT *)impl;

    // If receiver is NULL, broadcast
    if (!receiver)
    {
        return tcp_server_broadcast_message(server, msg);
    }

    // Otherwise, send to the client whose name matches receiver
    size_t count = zoo_list_size(server->clients);
    for (size_t i = 0; i < count; ++i)
    {
        TCP_CLIENT_INFO_STRUCT *client = (TCP_CLIENT_INFO_STRUCT *)zoo_list_at(server->clients, i);
        if (client && client->active && strcmp(client->client_name, receiver) == 0)
        {
            size_t sent = 0;
            return zoo_socket_send(&client->socket_info, msg->payload, msg->header.payload_size, &sent);
        }
    }
    return ZOO_SMB_ERROR_INVALID_PARAM;
}

/**
 * @brief Check if TCP server transport is started.
 *
 * @param impl Server transport pointer.
 * @return ZOO_TRUE if started, ZOO_FALSE otherwise.
 */
ZOO_BOOL tcp_server_is_started(void *impl)
{
    if (!impl)
    {
        return ZOO_FALSE;
    }

    TCP_SERVER_TRANSPORT_STRUCT *server = (TCP_SERVER_TRANSPORT_STRUCT *)impl;
    return server->common.running;
}

/**
 * @brief Create a listen socket for the TCP server (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param server Server transport pointer.
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_create_listen_socket(TCP_SERVER_TRANSPORT_STRUCT *server)
{
    if (!server)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_ERROR_TYPE err = zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &server->common.socket_info);
    if (err != ZOO_SMB_OK)
        return err;
    server->listen_fd = server->common.socket_info.fd;
    return ZOO_SMB_OK;
}

/**
 * @brief Bind the TCP server socket (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param server Server transport pointer.
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_bind_socket(TCP_SERVER_TRANSPORT_STRUCT *server)
{
    if (!server || !server->common.config)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_SOCKADDR_UNION addr;
    memset(&addr, 0, sizeof(addr));
    addr.in.family = ZOO_AF_INET;
    addr.in.port = server->common.config->port;
    addr.in.addr = ZOO_INADDR_ANY; // TODO: parse config->address
    return zoo_socket_bind(&server->common.socket_info, &addr);
}

/**
 * @brief Start listening on the TCP server socket (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param server Server transport pointer.
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_start_listening(TCP_SERVER_TRANSPORT_STRUCT *server)
{
    if (!server)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    return zoo_socket_listen(&server->common.socket_info, server->listen_backlog);
}

/**
 * @brief Accept a client connection on the TCP server (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param server Server transport pointer.
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_accept_client(TCP_SERVER_TRANSPORT_STRUCT *server)
{
    if (!server)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    TCP_CLIENT_INFO_STRUCT *client = (TCP_CLIENT_INFO_STRUCT *)zoo_allocate_from_pool(sizeof(TCP_CLIENT_INFO_STRUCT));
    if (!client)
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;

    memset(client, 0, sizeof(TCP_CLIENT_INFO_STRUCT));
    ZOO_ERROR_TYPE err = zoo_socket_accept(&server->common.socket_info, &client->socket_info, &client->client_addr);
    if (err != ZOO_SMB_OK)
    {
        zoo_free_to_pool(client);
        return err;
    }
    client->active = ZOO_TRUE;
    client->connect_time = time(NULL);
    // Add to client list
    zoo_list_push_back(server->clients, client);
    return ZOO_SMB_OK;
}

/**
 * @brief Add a client to the TCP server (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param server      Server transport pointer.
 * @param client_fd   Client file descriptor.
 * @param client_addr Client address pointer.
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_add_client(TCP_SERVER_TRANSPORT_STRUCT *server, int client_fd, struct sockaddr_in *client_addr)
{
    if (!server || !client_addr)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    TCP_CLIENT_INFO_STRUCT *client = (TCP_CLIENT_INFO_STRUCT *)zoo_allocate_from_pool(sizeof(TCP_CLIENT_INFO_STRUCT));
    if (!client)
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    memset(client, 0, sizeof(TCP_CLIENT_INFO_STRUCT));
    client->socket_info.fd = client_fd;
    client->client_addr.in.family = ZOO_AF_INET;
    client->client_addr.in.port = ntohs(client_addr->sin_port);
    client->client_addr.in.addr = ntohl(client_addr->sin_addr.s_addr);
    client->active = ZOO_TRUE;
    client->connect_time = time(NULL);
    zoo_list_push_back(server->clients, client);
    return ZOO_SMB_OK;
}

/**
 * @brief Remove a client from the TCP server (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param server    Server transport pointer.
 * @param client_fd Client file descriptor.
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_remove_client(TCP_SERVER_TRANSPORT_STRUCT *server, int client_fd)
{
    if (!server)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    size_t count = zoo_list_size(server->clients);
    for (size_t i = 0; i < count; ++i)
    {
        TCP_CLIENT_INFO_STRUCT *client = (TCP_CLIENT_INFO_STRUCT *)zoo_list_at(server->clients, i);
        if (client && client->socket_info.fd == client_fd)
        {
            zoo_socket_close(&client->socket_info);
            zoo_list_remove_at(server->clients, i);
            zoo_free_to_pool(client);
            return ZOO_SMB_OK;
        }
    }
    return ZOO_SMB_ERROR_INVALID_PARAM;
}

/**
 * @brief Find a client by file descriptor (not supported).
 *
 * This function is a stub and always returns NULL.
 *
 * @param server    Server transport pointer.
 * @param client_fd Client file descriptor.
 * @return NULL always.
 */
TCP_CLIENT_INFO_STRUCT *tcp_server_find_client(TCP_SERVER_TRANSPORT_STRUCT *server, int client_fd)
{
    if (!server)
        return NULL;
    size_t count = zoo_list_size(server->clients);
    for (size_t i = 0; i < count; ++i)
    {
        TCP_CLIENT_INFO_STRUCT *client = (TCP_CLIENT_INFO_STRUCT *)zoo_list_at(server->clients, i);
        if (client && client->socket_info.fd == client_fd)
            return client;
    }
    return NULL;
}

/**
 * @brief Broadcast a message to all clients (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param server Server transport pointer.
 * @param msg    Message pointer.
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_broadcast_message(TCP_SERVER_TRANSPORT_STRUCT *server, const ZOO_SMB_MSG_STRUCT *msg)
{
    if (!server || !msg)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    size_t count = zoo_list_size(server->clients);
    ZOO_ERROR_TYPE last_err = ZOO_SMB_OK;
    for (size_t i = 0; i < count; ++i)
    {
        TCP_CLIENT_INFO_STRUCT *client = (TCP_CLIENT_INFO_STRUCT *)zoo_list_at(server->clients, i);
        if (client && client->active)
        {
            size_t sent = 0;
            ZOO_ERROR_TYPE err = zoo_socket_send(&client->socket_info, msg->payload, msg->header.payload_size, &sent);
            if (err != ZOO_SMB_OK)
                last_err = err;
        }
    }
    return last_err;
}

/**
 * @brief Send a message to a specific client (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param server    Server transport pointer.
 * @param client_fd Client file descriptor.
 * @param msg       Message pointer.
 * @param size      Message size.
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_send_to_client(TCP_SERVER_TRANSPORT_STRUCT *server, int client_fd, const void *msg, size_t size)
{
    if (!server || !msg || size == 0)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    TCP_CLIENT_INFO_STRUCT *client = tcp_server_find_client(server, client_fd);
    if (!client || !client->active)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    size_t sent = 0;
    ZOO_ERROR_TYPE err = zoo_socket_send(&client->socket_info, msg, size, &sent);
    return err;
}

/**
 * @brief Clean up disconnected clients (not supported).
 *
 * This function is a stub and does nothing.
 *
 * @param server Server transport pointer.
 */
void tcp_server_cleanup_disconnected_clients(TCP_SERVER_TRANSPORT_STRUCT *server)
{
    if (!server || !server->clients)
        return;
    size_t i = 0;
    while (i < zoo_list_size(server->clients))
    {
        TCP_CLIENT_INFO_STRUCT *client = (TCP_CLIENT_INFO_STRUCT *)zoo_list_at(server->clients, i);
        if (client && !client->active)
        {
            zoo_socket_close(&client->socket_info);
            zoo_list_remove_at(server->clients, i);
            zoo_free_to_pool(client);
        }
        else
        {
            ++i;
        }
    }
}

/**
 * @brief Main server event loop (not supported).
 *
 * This function is a stub and always returns not supported.
 *
 * @param server Server transport pointer.
 * @return ZOO_SMB_ERROR_NOT_SUPPORTED always.
 */
ZOO_ERROR_TYPE tcp_server_event_loop(TCP_SERVER_TRANSPORT_STRUCT *server)
{
    if (!server || !server->common.running)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
#ifndef ZOO_SMB_ERROR_INTERNAL
#define ZOO_SMB_ERROR_INTERNAL ZOO_SMB_MAKE_ERROR(ZOO_SMB_CATEGORY_GENERAL, ZOO_SMB_GENERAL_OPERATION, 99)
#endif
        return ZOO_SMB_ERROR_INTERNAL;
    server->common.epoll_fd = epoll_fd;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server->listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->listen_fd, &ev) < 0)
    {
        close(epoll_fd);
        return ZOO_SMB_ERROR_INTERNAL;
    }

    struct epoll_event events[TCP_EPOLL_MAX_EVENTS];
    while (server->common.running)
    {
        int n = epoll_wait(epoll_fd, events, TCP_EPOLL_MAX_EVENTS, TCP_EPOLL_TIMEOUT_MS);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        for (int i = 0; i < n; ++i)
        {
            if (events[i].data.fd == server->listen_fd)
            {
                // Accept new client
                tcp_server_accept_client(server);
                // Add new client fd to epoll
                TCP_CLIENT_INFO_STRUCT *client = (TCP_CLIENT_INFO_STRUCT *)zoo_list_at(server->clients, zoo_list_size(server->clients) - 1);
                if (client)
                {
                    struct epoll_event client_ev;
                    client_ev.events = EPOLLIN | EPOLLRDHUP;
                    client_ev.data.fd = client->socket_info.fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->socket_info.fd, &client_ev);
                }
            }
            else
            {
                // Find client and handle events
                tcp_server_handle_client_events(server, events[i].data.fd, events[i].events);
            }
        }
    }
    close(epoll_fd);
    server->common.epoll_fd = INVALID_TRANSPORT_FD;
    return ZOO_SMB_OK;
}

/**
 * @brief Process epoll events for the server (not supported).
 *
 * This function is a stub and does nothing.
 *
 * @param server      Server transport pointer.
 * @param events      Array of epoll events.
 * @param event_count Number of events.
 */
void tcp_server_process_events(TCP_SERVER_TRANSPORT_STRUCT *server, struct epoll_event *events, int event_count)
{
    // TODO: Iterate over epoll events and dispatch to listen/client handlers
    (void)server;
    (void)events;
    (void)event_count;
}

/**
 * @brief Handle listen socket events (not supported).
 *
 * This function is a stub and does nothing.
 *
 * @param server Server transport pointer.
 * @param events Epoll event flags.
 */
void tcp_server_handle_listen_events(TCP_SERVER_TRANSPORT_STRUCT *server, uint32_t events)
{
    // TODO: Handle new incoming connections on the listen socket
    (void)server;
    (void)events;
}

/**
 * @brief Handle events for a client socket (epoll-driven, non-blocking).
 *
 * This function processes epoll events for a client socket, performing non-blocking reads,
 * message framing, deserialization, and callback/observer notification. It follows the ZOO code
 * specifications for error handling, state management, and documentation.
 *
 * Blocking: Non-blocking. All socket operations are non-blocking; no thread is blocked.
 * State: Updates client->active and removes client on disconnect or protocol error.
 * Ownership: Message struct passed to callback/observers is stack-allocated; payload ownership is not transferred.
 * Error handling: All errors are logged and mapped to state transitions. Invalid messages cause disconnect.
 * Thread safety: This function is not thread-safe; must be called from the event loop thread only.
 *
 * @param server    TCP server transport pointer (must not be NULL)
 * @param client_fd Client socket file descriptor
 * @param events    Epoll event flags (EPOLLIN, EPOLLRDHUP, etc.)
 */
void tcp_server_handle_client_events(TCP_SERVER_TRANSPORT_STRUCT *server, int client_fd, uint32_t events)
{
    if (!server)
        return;
    TCP_CLIENT_INFO_STRUCT *client = tcp_server_find_client(server, client_fd);
    if (!client)
        return;

    // NOTE: For production, use a per-client receive buffer to handle partial messages across reads.
    // Here, we use a stack buffer for demonstration; partial messages are dropped with a warning.
    uint8_t recv_buf[TCP_RECEIVE_BUFFER_SIZE];
    size_t received = 0;

    // Handle incoming data (EPOLLIN)
    if (events & EPOLLIN)
    {
        // Non-blocking read; may return 0 (disconnect) or error
        ZOO_ERROR_TYPE err = zoo_socket_recv(&client->socket_info, recv_buf, sizeof(recv_buf), &received);
        if (err != ZOO_SMB_OK || received == 0)
        {
            // Log and mark client as inactive on error or disconnect
            ZOO_LOG_WARN("Client %d disconnected or error on recv (err=%d)", client_fd, err);
            client->active = ZOO_FALSE;
        }
        else
        {
            // Message framing: process as many complete messages as possible
            size_t offset = 0;
            while (received - offset >= sizeof(ZOO_SMB_MSG_HEADER_STRUCT))
            {
                // Copy header for validation
                ZOO_SMB_MSG_HEADER_STRUCT header;
                memcpy(&header, recv_buf + offset, sizeof(ZOO_SMB_MSG_HEADER_STRUCT));
                if (!zoo_smb_protocol_is_header_valid(&header))
                {
                    // Protocol violation: log and disconnect
                    ZOO_LOG_ERROR("Invalid message header from client %d", client_fd);
                    client->active = ZOO_FALSE;
                    break;
                }
                size_t total_msg_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + header.payload_size;
                if (received - offset < total_msg_size)
                {
                    // Incomplete message: drop and warn (production: buffer for next read)
                    ZOO_LOG_WARN("Partial message from client %d (need %zu bytes, have %zu)", client_fd, total_msg_size, received - offset);
                    break;
                }
                // Deserialize message (payload is not owned by caller)
                ZOO_SMB_MSG_STRUCT msg;
                memset(&msg, 0, sizeof(msg));
                msg.payload = NULL;
                ZOO_ERROR_TYPE derr = zoo_smb_protocol_deserialize(recv_buf + offset, total_msg_size, &msg, sizeof(msg));
                if (derr != ZOO_SMB_OK)
                {
                    // Deserialization error: log and skip this message
                    ZOO_LOG_ERROR("Failed to deserialize message from client %d (err=%d)", client_fd, derr);
                    offset += total_msg_size;
                    continue;
                }
                // Contract: callback is invoked for each valid message; ownership is not transferred.
                if (server->message_callback)
                {
                    server->message_callback(server, client, &msg, server->message_callback_user_data);
                }
                // Notify observers (if any)
                tcp_notify_data_observers(&server->common, &msg);
                // Clean up payload if allocated by deserializer (implementation-dependent)
                if (msg.payload)
                {
                    free(msg.payload);
                    msg.payload = NULL;
                }
                offset += total_msg_size;
            }
        }
    }

    // Handle disconnect or error events
    if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    {
        ZOO_LOG_INFO("Client %d hangup or error event", client_fd);
        client->active = ZOO_FALSE;
    }

    // Remove client if inactive (state transition)
    if (!client->active)
    {
        tcp_server_remove_client(server, client_fd);
    }
}
}
