/**
 * @file zoo_smb_transport_udp_server.c
 * @brief UDP server transport implementation for ZOO SMB messaging system (unicast only)
 * @details This module implements UDP server transport functionality including:
 *          - UDP server socket creation and management
 *          - Message serialization and transmission (unicast only)
 *          - Asynchronous message receiving with timeout
 *          - Thread-safe server operations using common UDP transport layer
 * @author ZOO SMB Development Team
 * @date 2025
 * @version 1.0
 * @copyright Copyright (c) 2025 ZOO SMB Project
 */

#include "zoo_smb_transport_udp_server.h"
#include "zoo_smb_transport_udp_common.h"
#include "zoo_smb_protocol.h"
#include "../../memory_pool/inc/zoo_memory_pool.h"
#include "zoo_smb_consumer.h"
#include "../../log/inc/zoo_log.h"
#include "zoo_smb_util.h"
#include "zoo.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>

/**
 * @brief UDP server receive thread function
 * @param arg Pointer to ZOO_SMB_UDP_SERVER_TRANSPORT
 * @return NULL
 */
static ZOO_ERROR_TYPE udp_server_receive_loop(void* impl)
{
    ZOO_SMB_UDP_SERVER_TRANSPORT* server = (ZOO_SMB_UDP_SERVER_TRANSPORT*)impl;
    ZOO_SMB_UDP_TRANSPORT_COMMON* udp = &server->common;

    uint8_t* buffer = zoo_allocate_from_pool(MAX_TRANSPORT_BUFFER_SIZE);
    if (!buffer)
    {
        ZOO_LOG_ERROR("Failed to allocate receive buffer");
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    ZOO_LOG_DEBUG("UDP server receive thread started");

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(udp->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (udp->running)
    {
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        ssize_t bytes_received = recvfrom(udp->sockfd, buffer, MAX_TRANSPORT_BUFFER_SIZE, 0, (struct sockaddr*)&sender_addr, &sender_len);
        if (bytes_received > 0)
        {
            handle_incoming_data(udp, buffer, bytes_received, &sender_addr);
        }
    }

    zoo_free_to_pool(buffer);
    ZOO_LOG_INFO("UDP server receive loop stopped");
    return ZOO_SMB_OK;
}

/**
 * @brief Initialize UDP server transport (unicast only)
 * @details Initializes UDP server for unicast communication using common UDP layer.
 * @param transport Pointer to transport structure to initialize
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_server_init(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (!transport || !transport->config)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_SMB_UDP_SERVER_TRANSPORT* server = (ZOO_SMB_UDP_SERVER_TRANSPORT*)
        zoo_allocate_from_pool(sizeof(ZOO_SMB_UDP_SERVER_TRANSPORT));
    if (!server)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for UDP server transport");
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    // Use common initialization
    ZOO_ERROR_TYPE result = udp_common_init(transport, &server->common);
    if (result != ZOO_SMB_OK)
    {
        if (server->common.sockfd >= 0)
            close(server->common.sockfd);
        if (server->common.clients)
            zoo_list_destroy(server->common.clients);
        zoo_free_to_pool(server);
        return result;
    }
    transport->impl = server;

    ZOO_LOG_INFO("UDP server transport initialized for unicast: target %s:%d",
                     transport->config->address,
                     transport->config->port);

    return ZOO_SMB_OK;
}

/**
 * @brief Destroy UDP server transport and release all resources
 * @param impl Pointer to UDP server transport implementation
 */
void udp_server_destroy(void* impl)
{
    if (!impl)
        return;

    ZOO_SMB_UDP_SERVER_TRANSPORT* server = (ZOO_SMB_UDP_SERVER_TRANSPORT*)impl;

    udp_server_stop(impl);

    if (server->common.sockfd >= 0)
        close(server->common.sockfd);

    if (server->common.clients)
        zoo_list_destroy(server->common.clients);

    zoo_free_to_pool(server);
    ZOO_LOG_INFO("UDP server transport destroyed");
}

/**
 * @brief Start UDP server transport operations
 * @param impl Pointer to UDP server transport implementation
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_server_start(void* impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_SMB_UDP_SERVER_TRANSPORT* server = (ZOO_SMB_UDP_SERVER_TRANSPORT*)impl;
    if (server->common.running)
    {
        ZOO_LOG_WARN("UDP server transport already started");
        return ZOO_SMB_OK;
    }

    server->common.running = ZOO_TRUE;
    ZOO_LOG_INFO("UDP server transport started");
    return udp_server_receive_loop(server);
}

/**
 * @brief Stop UDP server transport operations
 * @param impl Pointer to UDP server transport implementation
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_server_stop(void* impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_SMB_UDP_SERVER_TRANSPORT* server = (ZOO_SMB_UDP_SERVER_TRANSPORT*)impl;
    if (!server->common.running)
        return ZOO_SMB_OK;

    server->common.running = ZOO_FALSE;
    ZOO_LOG_INFO("UDP server transport stopped");
    return ZOO_SMB_OK;
}

/**
 * @brief Send message via UDP server to client (unicast reply)
 * @param impl Pointer to UDP server transport implementation
 * @param msg Message structure to send
 * @param receiver Receiver identifier (client name)
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_server_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver)
{
    if (!impl || !msg || !receiver)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_SMB_UDP_SERVER_TRANSPORT* server = (ZOO_SMB_UDP_SERVER_TRANSPORT*)impl;
    ZOO_LOG_DEBUG("Sending UDP message to %s", receiver);
    if (zoo_smb_is_same_string(receiver, SMB_MULTICAST))
    {
        return udp_send_multicast(&server->common, msg, &server->common.multicast_addr);
    }

    else if (zoo_smb_is_same_string(receiver, SMB_BROADCAST))
    {
        return udp_send_broadcast(&server->common, msg, &server->common.broadcast_addr);
    }

    // Use common send function with client's address
    return udp_send_unicast_by_name(&server->common, msg, receiver);
}

/**
 * @brief Check if UDP server transport is started and running
 * @param impl Pointer to UDP server transport implementation
 * @return ZOO_TRUE if server is running, ZOO_FALSE otherwise
 */
ZOO_BOOL udp_server_is_started(void* impl)
{
    if (!impl)
        return ZOO_FALSE;

    ZOO_SMB_UDP_SERVER_TRANSPORT* server = (ZOO_SMB_UDP_SERVER_TRANSPORT*)impl;
    return server->common.running;
}