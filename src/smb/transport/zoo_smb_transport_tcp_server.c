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

/**
 * @brief Initializes TCP server transport implementation storage.
 *
 * @param transport Transport instance with valid TCP server configuration.
 * @return ZOO_SMB_OK on success, or an error code when validation/allocation fails.
 */
ZOO_ERROR_TYPE tcp_server_init(const ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (!transport || !transport->config)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    TCP_SERVER_TRANSPORT_STRUCT* server =
        (TCP_SERVER_TRANSPORT_STRUCT*)zoo_allocate_from_pool(sizeof(TCP_SERVER_TRANSPORT_STRUCT));
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
    server->common.config = (ZOO_SMB_TRANSPORT_CONFIG_STRUCT*)transport->config;
    server->max_clients = TCP_MAX_CLIENTS;
    server->listen_backlog = TCP_DEFAULT_BACKLOG;
    server->accept_new_connections = ZOO_TRUE;
    server->clients = zoo_list_create(server->max_clients);

    if (!server->clients)
    {
        zoo_free_to_pool(server);
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    ((ZOO_SMB_TRANSPORT_STRUCT*)transport)->impl = server;
    return ZOO_SMB_OK;
}

ZOO_ERROR_TYPE tcp_server_destroy(void* impl)
{
    if (!impl)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    TCP_SERVER_TRANSPORT_STRUCT* server = (TCP_SERVER_TRANSPORT_STRUCT*)impl;
    if (server->clients)
    {
        zoo_list_destroy(server->clients);
    }
    zoo_free_to_pool(server);
    return ZOO_SMB_OK;
}

ZOO_ERROR_TYPE tcp_server_start(void* impl)
{
    if (!impl)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    TCP_SERVER_TRANSPORT_STRUCT* server = (TCP_SERVER_TRANSPORT_STRUCT*)impl;
    server->common.running = ZOO_TRUE;
    server->common.conn_state = TRANSPORT_CONN_STATE_CONNECTED;
    return ZOO_SMB_OK;
}

ZOO_ERROR_TYPE tcp_server_stop(void* impl)
{
    if (!impl)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    TCP_SERVER_TRANSPORT_STRUCT* server = (TCP_SERVER_TRANSPORT_STRUCT*)impl;
    server->common.running = ZOO_FALSE;
    server->common.conn_state = TRANSPORT_CONN_STATE_DISCONNECTED;
    return ZOO_SMB_OK;
}

ZOO_ERROR_TYPE tcp_server_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver)
{
    (void)impl;
    (void)msg;
    (void)receiver;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

ZOO_BOOL tcp_server_is_started(void* impl)
{
    if (!impl)
    {
        return ZOO_FALSE;
    }

    TCP_SERVER_TRANSPORT_STRUCT* server = (TCP_SERVER_TRANSPORT_STRUCT*)impl;
    return server->common.running;
}

ZOO_ERROR_TYPE tcp_server_create_listen_socket(TCP_SERVER_TRANSPORT_STRUCT* server)
{
    (void)server;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

ZOO_ERROR_TYPE tcp_server_bind_socket(TCP_SERVER_TRANSPORT_STRUCT* server)
{
    (void)server;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

ZOO_ERROR_TYPE tcp_server_start_listening(TCP_SERVER_TRANSPORT_STRUCT* server)
{
    (void)server;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

ZOO_ERROR_TYPE tcp_server_accept_client(TCP_SERVER_TRANSPORT_STRUCT* server)
{
    (void)server;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

ZOO_ERROR_TYPE tcp_server_add_client(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd, struct sockaddr_in* client_addr)
{
    (void)server;
    (void)client_fd;
    (void)client_addr;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

ZOO_ERROR_TYPE tcp_server_remove_client(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd)
{
    (void)server;
    (void)client_fd;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

TCP_CLIENT_INFO_STRUCT* tcp_server_find_client(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd)
{
    (void)server;
    (void)client_fd;
    return NULL;
}

ZOO_ERROR_TYPE tcp_server_broadcast_message(TCP_SERVER_TRANSPORT_STRUCT* server, const ZOO_SMB_MSG_STRUCT* msg)
{
    (void)server;
    (void)msg;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

ZOO_ERROR_TYPE tcp_server_send_to_client(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd, const void* msg, size_t size)
{
    (void)server;
    (void)client_fd;
    (void)msg;
    (void)size;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

void tcp_server_cleanup_disconnected_clients(TCP_SERVER_TRANSPORT_STRUCT* server)
{
    (void)server;
}

ZOO_ERROR_TYPE tcp_server_event_loop(TCP_SERVER_TRANSPORT_STRUCT* server)
{
    (void)server;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
}

void tcp_server_process_events(TCP_SERVER_TRANSPORT_STRUCT* server, struct epoll_event* events, int event_count)
{
    (void)server;
    (void)events;
    (void)event_count;
}

void tcp_server_handle_listen_events(TCP_SERVER_TRANSPORT_STRUCT* server, uint32_t events)
{
    (void)server;
    (void)events;
}

void tcp_server_handle_client_events(TCP_SERVER_TRANSPORT_STRUCT* server, int client_fd, uint32_t events)
{
    (void)server;
    (void)client_fd;
    (void)events;
}
