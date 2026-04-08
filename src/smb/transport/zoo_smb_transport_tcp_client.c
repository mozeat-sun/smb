/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TCP_CLIENT
 * File name: zoo_smb_transport_tcp_client.c
 * Description: TCP client transport implementation
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-23     weiwang.sun       Created
 * 2.0       2025-06-23     weiwang.sun       Modularized with sync interface
 ******************************************************************************/

#include "zoo_smb_transport_tcp_client.h"
#include "zoo_smb_transport_tcp_common.h"
#include "zoo_smb_reactor.h"
#include "zoo_log.h"
#include "zoo_smb_error.h"
#include "zoo_memory_pool.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

static void copy_sockaddr_to_union(
    ZOO_SOCKADDR_UNION* dst,
    const struct sockaddr_in* src)
{
    if (!dst || !src)
    {
        return;
    }

    memset(dst, 0, sizeof(*dst));
    dst->in.family = ZOO_AF_INET;
    dst->in.port = ntohs(src->sin_port);
    dst->in.addr = ntohl(src->sin_addr.s_addr);
}

/**
 * @brief Allocate and initialize a TCP client transport structure.
 * @param transport Pointer to ZOO_SMB_TRANSPORT (must contain config).
 * @return ZOO_SMB_OK on success, error code on failure.
 *         On success, transport->impl will be set to the created client instance.
 */
ZOO_ERROR_TYPE tcp_client_init(const ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (!transport || !transport->config)
    {
        ZOO_LOG_ERROR("Invalid parameter in tcp_client_init");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config = transport->config;
    ZOO_LOG_INFO("tcp_client_init: config info - name=%s, address=%s, port=%d, retry_interval_s=%d",
                     config->name[0] != '\0' ? config->name : "(empty)",
                     config->address[0] != '\0' ? config->address : "(empty)",
                     config->port,
                     config->retry_interval_s);

    TCP_CLIENT_TRANSPORT_STRUCT* client =
        (TCP_CLIENT_TRANSPORT_STRUCT*)zoo_allocate_from_pool(sizeof(TCP_CLIENT_TRANSPORT_STRUCT));
    if (!client)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for TCP client transport");
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    memset(client, 0, sizeof(TCP_CLIENT_TRANSPORT_STRUCT));
    client->common.is_server = ZOO_FALSE;
    client->common.running = ZOO_FALSE;
    client->common.conn_fd = INVALID_TRANSPORT_FD;
    client->common.epoll_fd = INVALID_TRANSPORT_FD;
    client->common.conn_state = TRANSPORT_CONN_STATE_DISCONNECTED;
    client->common.config = (ZOO_SMB_TRANSPORT_CONFIG_STRUCT*)config;
    client->common.reconnect_interval = config->retry_interval_s > 0 ? config->retry_interval_s : TCP_DEFAULT_RECONNECT_INTERVAL;
    client->common.last_connect_attempt = 0;

    client->auto_reconnect = ZOO_TRUE;
    client->max_reconnect_attempts = 5;
    client->common.addr.sin_family = AF_INET;
    client->common.addr.sin_port = htons(config->port);

    if (config->address[0] == '\0')
    {
        ZOO_LOG_ERROR("address is not set in transport config");
        zoo_free_to_pool(client);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (inet_pton(AF_INET, config->address, &client->common.addr.sin_addr) <= 0)
    {
        ZOO_LOG_ERROR("Invalid server address: %s", config->address);
        zoo_free_to_pool(client);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    copy_sockaddr_to_union(&client->common.server_addr, &client->common.addr);

    client->common.data_observers = transport->data_observers;
    client->common.data_observers_lock = (ZOO_MUTEX_T*)&transport->data_observers_lock;
    ((ZOO_SMB_TRANSPORT_STRUCT*)transport)->impl = client;

    ZOO_LOG_INFO("tcp_client_init: common info - is_server=%d, running=%d, conn_fd=%d, epoll_fd=%d, conn_state=%d, reconnect_interval=%d",
                     client->common.is_server,
                     client->common.running,
                     client->common.conn_fd,
                     client->common.epoll_fd,
                     client->common.conn_state,
                     client->common.reconnect_interval);

    ZOO_LOG_INFO("TCP client transport initialized successfully: %s", config->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Destroy TCP client transport and release resources.
 * @param impl Client transport pointer.
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_client_destroy(void* impl)
{
    if (!impl)
    {
        ZOO_LOG_ERROR("tcp_client_destroy: impl is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    TCP_CLIENT_TRANSPORT_STRUCT* client = (TCP_CLIENT_TRANSPORT_STRUCT*)impl;
    if (client->common.running)
    {
        ZOO_LOG_INFO("tcp_client_destroy: stopping running client");
        tcp_client_stop(client);
    }

    tcp_cleanup_connection(&client->common, client->common.conn_fd);

    if (client->common.epoll_fd != INVALID_TRANSPORT_FD)
    {
        ZOO_LOG_DEBUG("tcp_client_destroy: closing epoll fd %d", client->common.epoll_fd);
        close(client->common.epoll_fd);
    }

    ZOO_LOG_INFO("tcp_client_destroy: freeing client structure");
    zoo_free_to_pool(client);
    return ZOO_SMB_OK;
}

/**
 * @brief Start TCP client transport and connect to server.
 * @param impl Client transport pointer.
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_client_start(void* impl)
{
    if (!impl)
    {
        ZOO_LOG_ERROR("tcp_client_start: impl is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    TCP_CLIENT_TRANSPORT_STRUCT* client = (TCP_CLIENT_TRANSPORT_STRUCT*)impl;
    if (client->common.running)
    {
        ZOO_LOG_WARN("tcp_client_start: client already running");
        return ZOO_SMB_OK;
    }

    client->common.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (client->common.epoll_fd == -1)
    {
        ZOO_LOG_ERROR("tcp_client_start: failed to create epoll fd: %s", strerror(errno));
        return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
    }

    ZOO_ERROR_TYPE result = tcp_client_connect_to_server(client);
    if (result != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("tcp_client_start: failed to connect to server, error=%d", result);
        close(client->common.epoll_fd);
        client->common.epoll_fd = INVALID_TRANSPORT_FD;
        return result;
    }

    client->common.running = ZOO_TRUE;
    ZOO_LOG_INFO("tcp_client_start: client started and connected");
    tcp_update_connection_state(&client->common, TRANSPORT_CONN_STATE_CONNECTED);
    return tcp_client_event_loop(client);
}

/**
 * @brief Stop TCP client transport.
 * @param impl Client transport pointer.
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_client_stop(void* impl)
{
    if (!impl)
    {
        ZOO_LOG_ERROR("tcp_client_stop: impl is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    TCP_CLIENT_TRANSPORT_STRUCT* client = (TCP_CLIENT_TRANSPORT_STRUCT*)impl;
    client->common.running = ZOO_FALSE;
    ZOO_LOG_INFO("tcp_client_stop: client stopped");
    tcp_update_connection_state(&client->common, TRANSPORT_CONN_STATE_DISCONNECTED);

    return ZOO_SMB_OK;
}

/**
 * @brief Send a message through TCP client transport.
 * @param impl Client transport pointer.
 * @param msg Message data pointer.
 * @param size Message data size.
 * @param consumer Target consumer (ignored for client).
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_client_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver)
{
    (void)receiver;
    if (!impl || !msg)
    {
        ZOO_LOG_ERROR("tcp_client_send: invalid parameter (impl=%p, msg=%p, size=%zu)", impl, msg, msg ? msg->header.payload_size : 0);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    TCP_CLIENT_TRANSPORT_STRUCT* client = (TCP_CLIENT_TRANSPORT_STRUCT*)impl;
    if (!client->common.running || client->common.conn_fd == INVALID_TRANSPORT_FD)
    {
        ZOO_LOG_ERROR("tcp_client_send: client not connected (running=%d, conn_fd=%d)",
                          client->common.running,
                          client->common.conn_fd);
        return ZOO_SMB_ERROR_TRANSPORT_CONNECT_FAILED;
    }

    return tcp_send_complete_message(client->common.conn_fd, msg);
}

/**
 * @brief Check if TCP client transport is started.
 * @param impl Client transport pointer.
 * @return ZOO_TRUE if started, ZOO_FALSE otherwise.
 */
ZOO_BOOL tcp_client_is_started(void* impl)
{
    if (!impl)
        return ZOO_FALSE;
    TCP_CLIENT_TRANSPORT_STRUCT* client = (TCP_CLIENT_TRANSPORT_STRUCT*)impl;
    return client->common.running && (client->common.conn_state == TRANSPORT_CONN_STATE_CONNECTED);
}

/**
 * @brief Connect to the TCP server.
 * @param client TCP client transport pointer.
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_client_connect_to_server(TCP_CLIENT_TRANSPORT_STRUCT* client)
{
    if (!client)
    {
        ZOO_LOG_ERROR("tcp_client_connect_to_server: client is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_ERROR_TYPE result = zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &client->common.socket_info);
    if (result != ZOO_OK)
    {
        ZOO_LOG_ERROR("tcp_client_connect_to_server: socket creation failed, error=%s", zoo_socket_error_string(result));
        return ZOO_SMB_ERROR_SOCKET_CREATE_FAILED;
    }
    client->common.conn_fd = client->common.socket_info.fd;

    if (tcp_set_keepalive(&client->common.socket_info) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("tcp_client_connect_to_server: set keepalive failed");
        zoo_socket_close(&client->common.socket_info);
        return ZOO_SMB_ERROR_SOCKET_CONNECT_FAILED;
    }

    if (!tcp_set_nonblocking(&client->common.socket_info))
    {
        ZOO_LOG_ERROR("tcp_client_connect_to_server: set nonblocking failed");
        zoo_socket_close(&client->common.socket_info);
        return ZOO_SMB_ERROR_SOCKET_CONNECT_FAILED;
    }

    result = zoo_socket_connect(&client->common.socket_info, &client->common.server_addr);
    if (result != ZOO_OK && result != ZOO_ERROR_IN_PROGRESS)
    {
        ZOO_LOG_ERROR("tcp_client_connect_to_server: connect failed, error=%s", zoo_socket_error_string(result));
        zoo_socket_close(&client->common.socket_info);
        client->common.conn_fd = INVALID_TRANSPORT_FD;
        return ZOO_SMB_ERROR_SOCKET_CONNECT_FAILED;
    }

    if (tcp_setup_epoll_monitoring(client->common.epoll_fd, client->common.conn_fd, EPOLLIN | EPOLLOUT) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("tcp_client_connect_to_server: epoll monitoring setup failed");
        close(client->common.conn_fd);
        client->common.conn_fd = INVALID_TRANSPORT_FD;
        return ZOO_SMB_ERROR_SOCKET_CONNECT_FAILED;
    }

    client->common.last_connect_attempt = time(NULL);
    client->current_reconnect_count = 0;
    ZOO_LOG_INFO("tcp_client_connect_to_server: connection initiated (fd=%d)", client->common.conn_fd);
    return ZOO_SMB_OK;
}

/**
 * @brief Main client event loop.
 * @param client TCP client transport pointer.
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_client_event_loop(TCP_CLIENT_TRANSPORT_STRUCT* client)
{
    struct epoll_event events[TCP_EPOLL_MAX_EVENTS];
    while (client->common.running)
    {
        int n = 0;
        ZOO_ERROR_TYPE wait_ret = zoo_smb_reactor_wait(
            client->common.epoll_fd,
            events,
            TCP_EPOLL_MAX_EVENTS,
            TCP_EPOLL_TIMEOUT_MS,
            &n);
        if (wait_ret != ZOO_SMB_OK)
        {
            ZOO_LOG_ERROR("tcp_client_event_loop: reactor wait failed, err=%d", wait_ret);
            tcp_cleanup_connection(&client->common, client->common.conn_fd);
            continue;
        }

        if (n > 0)
        {
            tcp_client_process_events(client, events, n);
        }

        if (client->common.conn_state == TRANSPORT_CONN_STATE_DISCONNECTED && client->auto_reconnect)
        {
            tcp_client_auto_reconnect(client);
        }
    }
    return ZOO_SMB_OK;
}

/**
 * @brief Process epoll events for the client.
 * @param client TCP client transport pointer.
 * @param events Array of epoll events.
 * @param event_count Number of events.
 */
void tcp_client_process_events(TCP_CLIENT_TRANSPORT_STRUCT* client, struct epoll_event* events, int event_count)
{
    for (int i = 0; i < event_count; i++)
    {
        int fd = events[i].data.fd;
        uint32_t event_flags = events[i].events;
        if (fd == client->common.conn_fd)
            tcp_client_handle_connection_events(client, event_flags);
    }
}

/**
 * @brief Handle connection events for the client.
 * @param client TCP client transport pointer.
 * @param events Epoll event flags.
 */
void tcp_client_handle_connection_events(TCP_CLIENT_TRANSPORT_STRUCT* client, uint32_t events)
{
    if (tcp_is_connection_lost_event(events))
    {
        tcp_cleanup_connection(&client->common, client->common.conn_fd);
        tcp_update_connection_state(&client->common, TRANSPORT_CONN_STATE_DISCONNECTED);
    }
    else if (events & EPOLLIN)
    {
        tcp_handle_incoming_data(&client->common, client->common.conn_fd, NULL);
    }
}

/**
 * @brief Attempt automatic reconnection if enabled.
 * @param client TCP client transport pointer.
 * @return ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE tcp_client_auto_reconnect(TCP_CLIENT_TRANSPORT_STRUCT* client)
{
    if (client->current_reconnect_count >= client->max_reconnect_attempts)
    {
        ZOO_LOG_ERROR("tcp_client_auto_reconnect: reached max reconnect attempts (%d)", client->max_reconnect_attempts);
        return ZOO_SMB_ERROR_TRANSPORT_CONNECT_FAILED;
    }

    time_t now = time(NULL);
    if (now - client->common.last_connect_attempt < client->common.reconnect_interval)
    {
        ZOO_LOG_DEBUG("tcp_client_auto_reconnect: waiting for reconnect interval (%ld/%d)",
                          now - client->common.last_connect_attempt,
                          client->common.reconnect_interval);
        return ZOO_SMB_OK;
    }

    client->current_reconnect_count++;
    ZOO_LOG_INFO("tcp_client_auto_reconnect: attempt %d/%d",
                     client->current_reconnect_count,
                     client->max_reconnect_attempts);

    ZOO_ERROR_TYPE result = tcp_client_connect_to_server(client);
    if (result == ZOO_SMB_OK)
    {
        tcp_update_connection_state(&client->common, TRANSPORT_CONN_STATE_CONNECTED);
        client->current_reconnect_count = 0;
        ZOO_LOG_INFO("tcp_client_auto_reconnect: reconnected successfully");
    }
    else
    {
        ZOO_LOG_WARN("tcp_client_auto_reconnect: reconnect attempt failed (err=%d)", result);
    }
    return result;
}

/**
 * @brief Reset client connection state.
 * @param client TCP client transport pointer.
 */
void tcp_client_reset_connection(TCP_CLIENT_TRANSPORT_STRUCT* client)
{
    ZOO_LOG_INFO("tcp_client_reset_connection: resetting client state");
    client->current_reconnect_count = 0;
    client->common.last_connect_attempt = 0;
    client->common.last_heartbeat = 0;
    tcp_update_connection_state(&client->common, TRANSPORT_CONN_STATE_DISCONNECTED);
}