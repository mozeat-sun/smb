/**
 * @file zoo_smb_transport_udp_client.c
 * @brief UDP client transport implementation for ZOO SMB messaging system (unicast only)
 * @details This module implements UDP client transport functionality including:
 *          - UDP client socket creation and management
 *          - Message serialization and transmission (unicast only)
 *          - Asynchronous message receiving with timeout
 *          - Thread-safe client operations using common UDP transport layer
 * @author ZOO SMB Development Team
 * @date 2025
 * @version 1.0
 * @copyright Copyright (c) 2025 ZOO SMB Project
 */

#include "zoo_smb_transport_udp_client.h"
#include "zoo_smb_transport_udp_common.h"
#include "zoo_smb_protocol.h"
#include "zoo_memory_pool.h"
#include "zoo_smb_consumer.h"
#include "zoo_log.h"
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
 * @brief UDP client receive thread function
 * @param arg Pointer to ZOO_SMB_UDP_CLIENT_TRANSPORT
 * @return NULL
 */
static ZOO_ERROR_TYPE udp_client_receive_loop(void* impl)
{
    ZOO_SMB_UDP_CLIENT_TRANSPORT* client = (ZOO_SMB_UDP_CLIENT_TRANSPORT*)impl;
    ZOO_SMB_UDP_TRANSPORT_COMMON* udp = &client->common;

    uint8_t* buffer = zoo_allocate_from_pool(MAX_TRANSPORT_BUFFER_SIZE);
    if (!buffer)
    {
        ZOO_LOG_ERROR("Failed to allocate receive buffer");
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    ZOO_LOG_DEBUG("UDP client receive thread started");

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
    ZOO_LOG_INFO("UDP client receive loop stopped");
    return ZOO_SMB_OK;
}

/**
 * @brief Initialize UDP client transport (unicast only)
 * @details Initializes UDP client for unicast communication using common UDP layer.
 * @param transport Pointer to transport structure to initialize
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_client_init(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (!transport || !transport->config)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_SMB_UDP_CLIENT_TRANSPORT* client = (ZOO_SMB_UDP_CLIENT_TRANSPORT*)
        zoo_allocate_from_pool(sizeof(ZOO_SMB_UDP_CLIENT_TRANSPORT));
    if (!client)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for UDP client transport");
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    // Use common initialization
    ZOO_ERROR_TYPE result = udp_common_init(transport, &client->common);
    if (result != ZOO_SMB_OK)
    {
        if (client->common.sockfd >= 0)
            close(client->common.sockfd);
        if (client->common.clients)
            zoo_list_destroy(client->common.clients);
        zoo_free_to_pool(client);
        return result;
    }

    transport->impl = client;

    ZOO_LOG_INFO("UDP client transport initialized for unicast: target %s:%d",
                     transport->config->address,
                     transport->config->port);

    return ZOO_SMB_OK;
}

/**
 * @brief Destroy UDP client transport and release all resources
 * @param impl Pointer to UDP client transport implementation
 */
void udp_client_destroy(void* impl)
{
    if (!impl)
        return;

    ZOO_SMB_UDP_CLIENT_TRANSPORT* client = (ZOO_SMB_UDP_CLIENT_TRANSPORT*)impl;

    udp_client_stop(impl);

    if (client->common.sockfd >= 0)
        close(client->common.sockfd);

    if (client->common.clients)
        zoo_list_destroy(client->common.clients);

    zoo_free_to_pool(client);
    ZOO_LOG_INFO("UDP client transport destroyed");
}

/**
 * @brief Start UDP client transport operations
 * @param impl Pointer to UDP client transport implementation
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_client_start(void* impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_SMB_UDP_CLIENT_TRANSPORT* client = (ZOO_SMB_UDP_CLIENT_TRANSPORT*)impl;
    if (client->common.running)
    {
        ZOO_LOG_DEBUG("UDP client transport already started");
        return ZOO_SMB_OK;
    }

    client->common.running = ZOO_TRUE;
    ZOO_LOG_INFO("UDP client transport started");
    return udp_client_receive_loop(client);
}

/**
 * @brief Stop UDP client transport operations
 * @param impl Pointer to UDP client transport implementation
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_client_stop(void* impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_SMB_UDP_CLIENT_TRANSPORT* client = (ZOO_SMB_UDP_CLIENT_TRANSPORT*)impl;
    if (!client->common.running)
        return ZOO_SMB_OK;

    client->common.running = ZOO_FALSE;

    ZOO_LOG_INFO("UDP client transport stopped");
    return ZOO_SMB_OK;
}

/**
 * @brief Send message via UDP client to configured server (unicast)
 * @param impl Pointer to UDP client transport implementation
 * @param msg Message structure to send
 * @param receiver Receiver identifier (ignored for client)
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_client_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver)
{
    ZOO_SMB_UNUSED(receiver);  // Receiver is ignored for client
    if (!impl || !msg)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_SMB_UDP_CLIENT_TRANSPORT* client = (ZOO_SMB_UDP_CLIENT_TRANSPORT*)impl;

    // Use common send function with target address
    return udp_send_unicast(&client->common, msg, &client->common.server_addr);
}

/**
 * @brief Check if UDP client transport is started and running
 * @param impl Pointer to UDP client transport implementation
 * @return ZOO_TRUE if client is running, ZOO_FALSE otherwise
 */
ZOO_BOOL udp_client_is_started(void* impl)
{
    if (!impl)
        return ZOO_FALSE;

    ZOO_SMB_UDP_CLIENT_TRANSPORT* client = (ZOO_SMB_UDP_CLIENT_TRANSPORT*)impl;
    return client->common.running;
}
