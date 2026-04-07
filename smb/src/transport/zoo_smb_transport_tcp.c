/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TYPE_TCP
 * File name: zoo_smb_transport_tcp.c
 * Description: TCP transport implementation for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-20     weiwang.sun       created
 * 2.0       2025-05-20     weiwang.sun       optimized
 * 3.0       2025-05-27     weiwang.sun       Reimplemented with epoll and auto-reconnect
 * 4.0       2025-06-05     weiwang.sun       Enhanced auto-reconnect and connection management
 ******************************************************************************/

#include "zoo_smb_transport_tcp.h"
#include "zoo_smb_transport_tcp_client.h"
#include "zoo_smb_transport_tcp_server.h"

/**
 * @brief TCP transport init
 */
static ZOO_ERROR_TYPE tcp_init( ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (transport->config->is_server)
        return tcp_server_init(transport);
    else
        return tcp_client_init(transport);
}

/**
 * @brief TCP transport destroy
 */
static void tcp_destroy(void* impl)
{
    if (!impl)
        return;
    ZOO_BOOL is_server = ((ZOO_SMB_TCP_TRANSPORT_COMMON*)impl)->is_server;
    if (is_server)
        tcp_server_destroy(impl);
    else
        tcp_client_destroy(impl);
    return;
}

/**
 * @brief TCP transport start
 */
static ZOO_ERROR_TYPE tcp_start(void* impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    ZOO_BOOL is_server = ((ZOO_SMB_TCP_TRANSPORT_COMMON*)impl)->is_server;
    return is_server ? tcp_server_start(impl) : tcp_client_start(impl);
}

/**
 * @brief TCP transport stop
 */
static ZOO_ERROR_TYPE tcp_stop(void* impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    ZOO_BOOL is_server = ((ZOO_SMB_TCP_TRANSPORT_COMMON*)impl)->is_server;
    return is_server ? tcp_server_stop(impl) : tcp_client_stop(impl);
}

/**
 * @brief TCP transport send
 */
static ZOO_ERROR_TYPE tcp_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    ZOO_BOOL is_server = ((ZOO_SMB_TCP_TRANSPORT_COMMON*)impl)->is_server;
    return is_server ? tcp_server_send(impl, msg, receiver) : tcp_client_send(impl, msg, receiver);
}

/**
 * @brief TCP transport is_started
 */
static ZOO_BOOL tcp_is_started(void* impl)
{
    if (!impl)
        return ZOO_FALSE;
    ZOO_BOOL is_server = ((ZOO_SMB_TCP_TRANSPORT_COMMON*)impl)->is_server;
    return is_server ? tcp_server_is_started(impl) : tcp_client_is_started(impl);
}

/**
 * @brief Register TCP transport with SMB framework
 */
void zoo_smb_transport_tcp_init(void)
{
    static ZOO_SMB_TRANSPORT_OPS_STRUCT tcp_ops = {
        .init = tcp_init,
        .destroy = tcp_destroy,
        .start = tcp_start,
        .stop = tcp_stop,
        .send = tcp_send,
        .is_started = tcp_is_started};
    zoo_smb_transport_register(ZOO_SMB_TRANSPORT_TYPE_TCP, &tcp_ops);
    ZOO_LOG_INFO("TCP transport module registered (dispatch to client/server)");
}