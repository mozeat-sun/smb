/**
 * @file zoo_smb_transport_udp_common.c
 * @brief Common UDP transport implementation for ZOO SMB messaging system
 * @details This module implements shared UDP transport functionality including:
 *          - Message deserialization and observer notification
 *          - Client registration and management
 *          - Socket creation and address configuration
 *          - Common initialization routines for client, server, publisher and subscriber
 * @author ZOO SMB Development Team
 * @date 2025
 * @version 1.0
 * @copyright Copyright (c) 2025 ZOO SMB Project
 */

#include "zoo_smb_transport_udp_common.h"
#include "zoo_smb_protocol.h"
#include "zoo_memory_pool.h"
#include "zoo_smb_consumer.h"
#include "zoo_log.h"
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

/**
 * @brief Notify all registered data observers about received message
 * @details Iterates through all registered observers and calls their handler
 *          functions with the received message. Used to distribute incoming
 *          messages to all interested components.
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] msg Received message to notify observers about
 */
void udp_notify_data_observers(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, const ZOO_SMB_MSG_STRUCT* msg)
{
    if (!udp || !msg)
    {
        return;
    }

    size_t observer_count = zoo_list_size(udp->data_observers);
    ZOO_LOG_DEBUG("[udp-debug] Notifying %zu observers for msg_type=%u, payload_size=%u", observer_count, msg->header.msg_type, msg->header.payload_size);
    TRANSPORT_DATA_OBSERVER_STRUCT** snapshot = NULL;
    if (observer_count > 0)
    {
        snapshot = (TRANSPORT_DATA_OBSERVER_STRUCT**)zoo_allocate_from_pool(sizeof(TRANSPORT_DATA_OBSERVER_STRUCT*) * observer_count);
    }

    if (udp->data_observers_lock)
    {
        ZOO_MUTEX_LOCK(udp->data_observers_lock);
    }

    for (size_t i = 0; i < observer_count; i++)
    {
        TRANSPORT_DATA_OBSERVER_STRUCT* observer = (TRANSPORT_DATA_OBSERVER_STRUCT*)zoo_list_at(udp->data_observers, i);
        if (observer && observer->handler) 
        {
            ZOO_LOG_DEBUG("[udp-debug] Calling observer %zu handler=%p user_data=%p for msg_type=%u, payload_size=%u", i, (void*)observer->handler, observer->user_data, msg->header.msg_type, msg->header.payload_size);
        }
        if (snapshot)
        {
            snapshot[i] = observer;
        }
        else if (observer && observer->handler)
        {
            observer->handler(observer->user_data, msg);
        }
    }

    if (udp->data_observers_lock)
    {
        ZOO_MUTEX_UNLOCK(udp->data_observers_lock);
    }

    if (snapshot)
    {
        for (size_t i = 0; i < observer_count; i++)
        {
            TRANSPORT_DATA_OBSERVER_STRUCT* observer = snapshot[i];
            if (observer && observer->handler)
            {
                ZOO_LOG_DEBUG("[udp-debug] (snapshot) Calling observer %zu handler=%p user_data=%p for msg_type=%u, payload_size=%u", i, (void*)observer->handler, observer->user_data, msg->header.msg_type, msg->header.payload_size);
                observer->handler(observer->user_data, msg);
            }
        }
        zoo_free_to_pool(snapshot);
    }
}

/**
 * @brief Register new UDP client/consumer based on message header
 * @details Creates a new consumer entry if the sender is not already registered.
 *          This allows the server to maintain a list of active clients for
 *          targeted message delivery.
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] sender_addr Socket address of the sender
 * @param[in] header Message header containing sender information
 */
void udp_register_client(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, struct sockaddr_in* sender_addr, const ZOO_SMB_MSG_HEADER_STRUCT* header)
{
    if (!udp || !sender_addr || !header)
    {
        return;
    }

    if (!zoo_smb_consumer_find_by_name(udp->clients, header->sender))
    {
        if (!zoo_smb_create_consumer_and_insert(udp->clients, header->sender, udp->sockfd, sender_addr))
        {
            ZOO_LOG_ERROR("Failed to create consumer for %s", header->sender);
        }
    }
}

/**
 * @brief Process received UDP message and notify observers
 * @details Simple wrapper that forwards the message to all registered observers.
 *          Provides a clean interface for message processing.
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] msg Received message to process
 */
void udp_process_received_message(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, const ZOO_SMB_MSG_STRUCT* msg)
{
    if (!udp || !msg)
    {
        return;
    }
    udp_notify_data_observers(udp, msg);
}

/**
 * @brief Handle incoming raw UDP data from socket
 * @details Processes raw UDP data with improved multicast support:
 *          - Deserializes the message using protocol handler
 *          - Registers the sender as a client
 *          - Processes the message through observers
 *          - Cleans up allocated resources
 * @param udp Pointer to UDP transport common structure
 * @param buffer Raw data buffer received from socket
 * @param bytes_received Number of bytes received
 * @param sender_addr Socket address of the sender
 */
void handle_incoming_data(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, uint8_t* buffer, size_t bytes_received, struct sockaddr_in* sender_addr)
{
    if (!udp || !buffer || !sender_addr)
    {
        return;
    }


    ZOO_LOG_DEBUG("[udp-debug] handle_incoming_data: bytes_received=%zu, first4=0x%02x%02x%02x%02x", bytes_received, buffer[0], buffer[1], buffer[2], buffer[3]);

    ZOO_SMB_MSG_STRUCT* msg_out = zoo_smb_default_message();
    if (!msg_out)
    {
        ZOO_LOG_ERROR("Failed to allocate message structure");
        return;
    }

    ZOO_ERROR_TYPE deser_result = zoo_smb_protocol_deserialize(buffer, bytes_received, msg_out, sizeof(ZOO_SMB_MSG_STRUCT));

    ZOO_LOG_DEBUG("[udp-debug] protocol_deserialize result=%d, msg_type=%u, payload_size=%u", deser_result, msg_out->header.msg_type, msg_out->header.payload_size);
    if (deser_result != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("Failed to deserialize UDP message from %s:%d",
                          inet_ntoa(sender_addr->sin_addr),
                          ntohs(sender_addr->sin_port));
        zoo_smb_destroy_message(msg_out);
        return;
    }

    ZOO_LOG_DEBUG("Received UDP message from %s:%d, sender:%s, size: %zu bytes,udp->name:%s, payload_size=%u",
                      inet_ntoa(sender_addr->sin_addr),
                      ntohs(sender_addr->sin_port),
                      msg_out->header.sender,
                      bytes_received,
                      udp->name,
                      msg_out->header.payload_size);

    udp_register_client(udp, sender_addr, &msg_out->header);
    udp_process_received_message(udp, msg_out);

    zoo_smb_destroy_message(msg_out);
}

/**
 * @brief Send message via UDP socket with common serialization
 * @details Common send function for all UDP transport types:
 *          - Serializes message using protocol handler
 *          - Sends via UDP socket to specified address
 *          - Handles error cases and cleanup
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] msg Message structure to send
 * @param[in] target_addr Target socket address
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_common_send(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, const ZOO_SMB_MSG_STRUCT* msg, const struct sockaddr_in* target_addr)
{
    if (!udp || !msg || !target_addr)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    size_t total_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + msg->header.payload_size;
    uint8_t* msg_buffer = zoo_allocate_from_pool(total_size);
    if (!msg_buffer)
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;

    size_t msg_size = zoo_smb_protocol_serialize(msg, msg_buffer, total_size);
    if (msg_size == 0)
    {
        zoo_free_to_pool(msg_buffer);
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    ssize_t bytes_sent = -1;
    int max_send_attempts = (udp->config && udp->config->max_send_attempts > 0)
                                ? (int)udp->config->max_send_attempts
                                : 3;
    for (int attempt = 0; attempt < max_send_attempts; ++attempt)
    {
        bytes_sent = sendto(udp->sockfd, msg_buffer, msg_size, 0, (const struct sockaddr*)target_addr, sizeof(struct sockaddr_in));
        if (bytes_sent == (ssize_t)msg_size)
        {
            break;
        }

        if (bytes_sent < 0 && errno == EINTR)
        {
            continue;
        }

        if (attempt + 1 < max_send_attempts && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            usleep(1000);
            continue;
        }

        break;
    }

    ZOO_LOG_DEBUG("UDP sent to %s:%d, sender:%s, size:%zd",
                      inet_ntoa(target_addr->sin_addr),
                      ntohs(target_addr->sin_port),
                      msg->header.sender,
                      bytes_sent);

    zoo_free_to_pool(msg_buffer);

    if (bytes_sent != (ssize_t)msg_size)
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;

    return ZOO_SMB_OK;
}

/**
 * @brief Send message via UDP multicast
 * @details Sends a message to a multicast group using the specified multicast address.
        // printf removed
 *          members of the multicast group.
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] msg Message structure to send
 * @param[in] multicast_ip Multicast group IP address (e.g., "224.0.0.1")
 * @param[in] port Target port for multicast transmission
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_send_multicast(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                      const ZOO_SMB_MSG_STRUCT* msg,
                                      const struct sockaddr_in* multicast_addr)
{
    if (!udp || !msg || !multicast_addr)
    {
        ZOO_LOG_ERROR("Invalid parameters for multicast send");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Validate multicast IP range (224.0.0.0 to 239.255.255.255)
                // printf removed
    uint32_t ip_addr = ntohl(multicast_addr->sin_addr.s_addr);
    if ((ip_addr < 0xE0000000) || (ip_addr > 0xEFFFFFFF))
    {
        ZOO_LOG_ERROR("IP address %s is not in multicast range (224.0.0.0-239.255.255.255)",
                          inet_ntoa(multicast_addr->sin_addr));
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Use common send function
    ZOO_ERROR_TYPE result = udp_common_send(udp, msg, multicast_addr);

    if (ZOO_SMB_IS_SUCCESS(result))
    {
        ZOO_LOG_DEBUG("Multicast message sent successfully to %s:%d, sender:%s",
                          inet_ntoa(multicast_addr->sin_addr),
                          ntohs(multicast_addr->sin_port),
                          msg->header.sender ? msg->header.sender : "unknown");
    }
    else
    {
        ZOO_LOG_ERROR("Failed to send multicast message to %s:%d: %s",
                          inet_ntoa(multicast_addr->sin_addr),
                          ntohs(multicast_addr->sin_port),
                          zoo_smb_get_error_string(result));
    }

                    // printf removed
    return result;
}

/**
 * @brief Send message via UDP broadcast
 * @details Sends a message to all hosts on the local network segment using
 *          broadcast addressing. Automatically enables broadcast option on socket.
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] msg Message structure to send
 * @param[in] port Target port for broadcast transmission
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_send_broadcast(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                      const ZOO_SMB_MSG_STRUCT* msg,
                                      const struct sockaddr_in* broadcast_addr)
{
    if (!udp || !msg || !broadcast_addr)
    {
        ZOO_LOG_ERROR("Invalid parameters for broadcast send");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Enable broadcast option on socket
    int broadcast_enable = 1;
    if (setsockopt(udp->sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0)
    {
        ZOO_LOG_ERROR("Failed to set broadcast option: %s", strerror(errno));
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    // Use common send function
    ZOO_ERROR_TYPE result = udp_common_send(udp, msg, broadcast_addr);

    if (ZOO_SMB_IS_SUCCESS(result))
    {
        ZOO_LOG_DEBUG("Broadcast message sent successfully to port %d, sender:%s",
                          ntohs(broadcast_addr->sin_port),
                          msg->header.sender ? msg->header.sender : "unknown");
    }
    else
    {
        ZOO_LOG_ERROR("Failed to send broadcast message to port %d: %s",
                          ntohs(broadcast_addr->sin_port),
                          zoo_smb_get_error_string(result));
    }

    return result;
}

/**
 * @brief Send message via UDP unicast to specific client
 * @details Sends a message directly to a specific client using unicast addressing.
 *          The target client address should be obtained from a previous registration
 *          or discovery process.
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] msg Message structure to send
 * @param[in] client_addr Target client socket address
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_send_unicast(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                    const ZOO_SMB_MSG_STRUCT* msg,
                                    const struct sockaddr_in* target_addr)
{
    if (!udp || !msg || !target_addr)
    {
        ZOO_LOG_ERROR("Invalid parameters for unicast send");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Validate target address
    if (target_addr->sin_family != AF_INET || target_addr->sin_port == 0)
    {
        ZOO_LOG_ERROR("Invalid target address for unicast send");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Use common send function
    ZOO_ERROR_TYPE result = udp_common_send(udp, msg, target_addr);

    if (ZOO_SMB_IS_SUCCESS(result))
    {
        ZOO_LOG_DEBUG("Unicast message sent successfully to %s:%d, sender:%s",
                          inet_ntoa(target_addr->sin_addr),
                          ntohs(target_addr->sin_port),
                          msg->header.sender ? msg->header.sender : "unknown");
    }
    else
    {
        ZOO_LOG_ERROR("Failed to send unicast message to %s:%d: %s",
                          inet_ntoa(target_addr->sin_addr),
                          ntohs(target_addr->sin_port),
                          zoo_smb_get_error_string(result));
    }

    return result;
}

/**
 * @brief Send message to client by name using unicast
 * @details Convenience function that looks up a client by name in the client list
 *          and sends a unicast message to that client's registered address.
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] msg Message structure to send
 * @param[in] client_name Target client name to look up
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_send_unicast_by_name(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                            const ZOO_SMB_MSG_STRUCT* msg,
                                            const char* client_name)
{
    if (!udp || !msg || !client_name)
    {
        ZOO_LOG_ERROR("Invalid parameters for unicast send by name");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (!udp->clients)
    {
        ZOO_LOG_ERROR("Client list not initialized");
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    // Find the target client in the client list
    ZOO_SMB_CONSUMER_STRUCT* consumer = zoo_smb_consumer_find_by_name(udp->clients, client_name);
    if (!consumer)
    {
        ZOO_LOG_WARN("Target client not found: %s", client_name);

        // Log available clients for debugging
        ZOO_LOG_DEBUG("Available clients:");
        for (size_t i = 0; i < zoo_list_size(udp->clients); i++)
        {
            ZOO_SMB_CONSUMER_STRUCT* client = (ZOO_SMB_CONSUMER_STRUCT*)zoo_list_at(udp->clients, i);
            if (client && client->name[0] != '\0')
            {
                ZOO_LOG_DEBUG("  - %s", client->name);
            }
        }

        return ZOO_SMB_ERROR_SERVICE_NOT_FOUND;
    }

    // Get the client's socket address
    struct sockaddr_in* client_addr = (struct sockaddr_in*)&consumer->addr;
    if (!client_addr)
    {
        ZOO_LOG_ERROR("Client %s has no registered address", client_name);
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    // Send the unicast message
    return udp_send_unicast(udp, msg, client_addr);
}

/**
 * @brief Enhanced broadcast function with network interface selection
 * @details Advanced broadcast function that allows specifying a network interface
 *          for broadcast transmission. Useful in multi-homed systems.
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] msg Message structure to send
 * @param[in] port Target port for broadcast transmission
 * @param[in] interface_ip IP address of the interface to broadcast from (NULL for any)
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_send_broadcast_on_interface(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                                   const ZOO_SMB_MSG_STRUCT* msg,
                                                   uint16_t port,
                                                   const char* interface_ip)
{
    if (!udp || !msg)
    {
        ZOO_LOG_ERROR("Invalid parameters for interface broadcast send");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (udp->sockfd < 0)
    {
        ZOO_LOG_ERROR("Invalid socket for interface broadcast send");
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    // Enable broadcast option
    int broadcast_enable = 1;
    if (setsockopt(udp->sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0)
    {
        ZOO_LOG_ERROR("Failed to set broadcast option: %s", strerror(errno));
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    // Set interface for broadcast if specified
    if (interface_ip)
    {
        struct in_addr interface_addr;
        if (inet_pton(AF_INET, interface_ip, &interface_addr) <= 0)
        {
            ZOO_LOG_ERROR("Invalid interface IP address: %s", interface_ip);
            return ZOO_SMB_ERROR_INVALID_PARAM;
        }

        if (setsockopt(udp->sockfd, IPPROTO_IP, IP_MULTICAST_IF, &interface_addr, sizeof(interface_addr)) < 0)
        {
            ZOO_LOG_WARN("Failed to set broadcast interface to %s: %s",
                             interface_ip,
                             strerror(errno));
            // Continue anyway, will use default interface
        }
        else
        {
            ZOO_LOG_DEBUG("Set broadcast interface to %s", interface_ip);
        }
    }

    // Setup broadcast address
    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(port);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

    // Serialize and send the message
    ZOO_ERROR_TYPE result = udp_common_send(udp, msg, &broadcast_addr);

    if (ZOO_SMB_IS_SUCCESS(result))
    {
        ZOO_LOG_DEBUG("Interface broadcast message sent successfully to port %d via %s, sender:%s",
                          port,
                          interface_ip ? interface_ip : "default",
                          msg->header.sender ? msg->header.sender : "unknown");
    }
    else
    {
        ZOO_LOG_ERROR("Failed to send interface broadcast message to port %d: %s",
                          port,
                          zoo_smb_get_error_string(result));
    }

    return result;
}

/**
 * @brief Setup multicast socket options
 * @details Configures socket for multicast communication
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] is_sender Flag indicating if this is sender (publisher) or receiver (subscriber)
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_setup_multicast(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, ZOO_BOOL is_sender)
{
    if (!udp)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    // Set socket reuse
    int reuse = 1;
    if (setsockopt(udp->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        ZOO_LOG_WARN("Failed to set SO_REUSEADDR: %s", strerror(errno));
    }

    if (is_sender)
    {
        // Publisher settings
        int ttl = 1;
        if (setsockopt(udp->sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
        {
            ZOO_LOG_WARN("Failed to set multicast TTL: %s", strerror(errno));
        }

        int loop = 1;
        if (setsockopt(udp->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0)
        {
            ZOO_LOG_WARN("Failed to set multicast loop: %s", strerror(errno));
        }
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Join multicast group for subscriber
 * @details Joins the specified multicast group
 * @param[in] udp Pointer to UDP transport common structure
 * @param[in] multicast_ip Multicast group IP address
 * @return ZOO_SMB_OK on success, error code otherwise
 */
ZOO_ERROR_TYPE udp_join_multicast_group(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, const char* multicast_ip)
{
    if (!udp || !multicast_ip)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    ZOO_LOG_DEBUG("Attempting to join multicast group: %s on socket %d", multicast_ip, udp->sockfd);

    // Set up multicast membership request
    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    // Attempt to join the multicast group
    if (setsockopt(udp->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        if (errno == ENODEV)
        {
            ZOO_LOG_ERROR("No suitable network interface for multicast group %s", multicast_ip);
            return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
        }
        else if (errno == EACCES)
        {
            ZOO_LOG_ERROR("Permission denied when joining multicast group %s (may need root privileges)", multicast_ip);
            return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
        }
        else
        {
            ZOO_LOG_ERROR("Failed to join multicast group %s: %s", multicast_ip, strerror(errno));
            return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
        }
    }

    ZOO_LOG_INFO("Successfully joined multicast group: %s", multicast_ip);
    return ZOO_SMB_OK;
}

/**
 * @brief Validate transport configuration parameters
 * @details Performs comprehensive validation of transport configuration
 *          to ensure all required fields are properly set
 * @param[in] transport Pointer to transport structure to validate
 * @return ZOO_TRUE if configuration is valid, ZOO_FALSE otherwise
 */
static ZOO_BOOL validate_transport_config(const ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (!transport)
    {
        ZOO_LOG_ERROR("Transport structure is NULL");
        return ZOO_FALSE;
    }

    if (!transport->config)
    {
        ZOO_LOG_ERROR("Transport configuration is NULL");
        return ZOO_FALSE;
    }

    // Validate port range
    if (transport->config->port == 0)
    {
        ZOO_LOG_ERROR("Transport port cannot be zero");
        return ZOO_FALSE;
    }

    return ZOO_TRUE;
}

/**
 * @brief Initialize UDP transport structure fields
 * @details Sets up the basic structure fields and creates the client list
 * @param[in] transport Source transport configuration
 * @param[out] udp UDP transport structure to initialize
 * @param[in] is_server Server mode flag
 * @return ZOO_SMB_OK on success, error code otherwise
 */
static ZOO_ERROR_TYPE init_udp_structure(const ZOO_SMB_TRANSPORT_STRUCT* transport,
                                             ZOO_SMB_UDP_TRANSPORT_COMMON* udp)
{
    // Clear structure and set basic fields
    memset(udp, 0, sizeof(ZOO_SMB_UDP_TRANSPORT_COMMON));
    udp->sockfd = -1;  // Initialize to invalid socket descriptor
    udp->data_observers = transport->data_observers;
    udp->data_observers_lock = (ZOO_MUTEX_T*)&transport->data_observers_lock;
    udp->running = ZOO_FALSE;
    udp->is_server = transport->config->is_server;
    udp->config = (void*)transport->config;
    udp->name = transport->config->name;
    
    // Create client list with proper size validation
    const size_t max_clients = MAX_TRANSPORT_CONSUMER_SIZE;

    udp->clients = zoo_list_create(max_clients);
    if (!udp->clients)
    {
        ZOO_LOG_ERROR("Failed to create client list with capacity %zu", max_clients);
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    ZOO_LOG_DEBUG("Initialized UDP %s structure with client capacity: %zu",
                      udp->is_server ? "server" : "client",
                      max_clients);

    return ZOO_SMB_OK;
}

/**
 * @brief Create and configure UDP socket
 * @details Creates UDP socket with appropriate options for optimal performance
 * @param[out] udp UDP transport structure containing socket field to set
 * @return ZOO_SMB_OK on success, error code otherwise
 */
static ZOO_ERROR_TYPE create_udp_socket(ZOO_SMB_UDP_TRANSPORT_COMMON* udp)
{
    // Create UDP socket
    udp->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp->sockfd < 0)
    {
        ZOO_LOG_ERROR("Failed to create UDP socket: %s (errno: %d)", strerror(errno), errno);
        return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
    }

    int optval = 1;
    if (setsockopt(udp->sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        ZOO_LOG_ERROR("Failed to set SO_REUSEADDR on UDP socket: %s", strerror(errno));
        return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
    }

    ZOO_LOG_DEBUG("Created UDP socket with file descriptor: %d", udp->sockfd);
    return ZOO_SMB_OK;
}

/**
 * @brief Bind socket based on transport mode (server vs client)
 * @details Fixed binding logic that handles different transport modes correctly:
 *          - Servers: bind to configured address and port for listening
 *          - Clients: bind to any address with system-assigned port
 * @param[in,out] udp UDP transport structure
 * @return ZOO_SMB_OK on success, error code otherwise
 */
static ZOO_ERROR_TYPE bind_udp_socket(ZOO_SMB_UDP_TRANSPORT_COMMON* udp)
{
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    
    // Set multicast-specific socket options ONLY for multicast clients
    if (!udp->is_server && udp->multicast_addr.sin_port > 0)
    {
        int reuse_port = 1;
        if (setsockopt(udp->sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port)) < 0)
        {
            ZOO_LOG_WARN("Failed to set SO_REUSEPORT: %s (continuing anyway)", strerror(errno));
            // Continue without SO_REUSEPORT as it may not be available
        }
        
        ZOO_LOG_DEBUG("Set socket options for multicast port sharing");
    }
    
    // Configure bind address based on mode
    if (udp->is_server)
    {
        bind_addr.sin_addr = udp->server_addr.sin_addr;
        bind_addr.sin_port = udp->server_addr.sin_port;
        
        ZOO_LOG_DEBUG("Binding UDP server to %s:%d",
                          inet_ntoa(bind_addr.sin_addr),
                          ntohs(bind_addr.sin_port));
    }
    else if (udp->multicast_addr.sin_port > 0)
    {
        // Client with multicast: bind to INADDR_ANY with multicast port
        bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind_addr.sin_port = udp->multicast_addr.sin_port;
        
        ZOO_LOG_INFO("Binding UDP client to multicast port %d for group %s",
                         ntohs(bind_addr.sin_port),
                         inet_ntoa(udp->multicast_addr.sin_addr));
    }
    else
    {
        // Regular client: bind to system-assigned port
        bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind_addr.sin_port = htons(0);
        
        ZOO_LOG_DEBUG("Binding UDP client to auto-assigned port");
    }

    // Perform the bind operation
    if (bind(udp->sockfd, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0)
    {
        ZOO_LOG_ERROR("Failed to bind UDP %s socket to %s:%d: %s",
                          udp->is_server ? "server" : "client",
                          inet_ntoa(bind_addr.sin_addr),
                          ntohs(bind_addr.sin_port),
                          strerror(errno));
        return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Configure socket addresses for different modes
 * @details Sets up socket address structures appropriately:
 *          - server_addr: For servers (bind address), for clients (target server address)
 *          - target_addr: Target address for client sending operations
 * @param[in] transport Source transport configuration
 * @param[out] udp UDP transport structure containing address fields to set
 * @return ZOO_SMB_OK on success, error code otherwise
 */
static ZOO_ERROR_TYPE configure_socket_address(const ZOO_SMB_TRANSPORT_STRUCT* transport,
                                                   ZOO_SMB_UDP_TRANSPORT_COMMON* udp)
{
    // Configure server address
    memset(&udp->server_addr, 0, sizeof(udp->server_addr));
    udp->server_addr.sin_family = AF_INET;
    udp->server_addr.sin_port = htons(transport->config->port);
    if (udp->is_server)
    {
        // Server: bind to specified address or INADDR_ANY
        udp->server_addr.sin_addr.s_addr = transport->config->address[0] != '\0' ?
            inet_addr(transport->config->address) : htonl(INADDR_ANY);
        ZOO_LOG_DEBUG("Configured server bind address: %s:%d", 
                          transport->config->address[0] != '\0' ? transport->config->address : "0.0.0.0",
                          transport->config->port);
    }
    else
    {
        // Client: server_addr is the target server address for sending
        udp->server_addr.sin_addr.s_addr = transport->config->address[0] != '\0' ?
            inet_addr(transport->config->address) : inet_addr("127.0.0.1");
        ZOO_LOG_DEBUG("Configured client target server address: %s:%d", 
                          transport->config->address[0] != '\0' ? transport->config->address : "127.0.0.1",
                          transport->config->port);
    }

    // Configure multicast address if provided
    if (transport->config->multicast_address[0] != '\0' && transport->config->multicast_port > 0)
    {
        memset(&udp->multicast_addr, 0, sizeof(udp->multicast_addr));
        udp->multicast_addr.sin_family = AF_INET;
        udp->multicast_addr.sin_port = htons(transport->config->multicast_port);
        udp->multicast_addr.sin_addr.s_addr = inet_addr(transport->config->multicast_address);
        ZOO_LOG_DEBUG("Configured multicast address: %s:%d",
                          transport->config->multicast_address,
                          transport->config->multicast_port);
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Cleanup resources on initialization failure
 * @details Centralized cleanup function to prevent resource leaks on errors
 * @param[in,out] udp UDP transport structure to cleanup
 */
static void cleanup_udp_init_failure(ZOO_SMB_UDP_TRANSPORT_COMMON* udp)
{
    if (!udp)
        return;

    // Close socket if created
    if (udp->sockfd >= 0)
    {
        close(udp->sockfd);
        udp->sockfd = -1;
    }

    // Destroy client list if created
    if (udp->clients)
    {
        zoo_list_destroy(udp->clients);
        udp->clients = NULL;
    }

    // Clear structure
    memset(udp, 0, sizeof(ZOO_SMB_UDP_TRANSPORT_COMMON));
    udp->sockfd = -1;
}

/**
 * @brief Common initialization for UDP transport (client and server)
 * @details Enhanced initialization with proper client/server distinction and removal of redundant code
 */
ZOO_ERROR_TYPE udp_common_init(ZOO_SMB_TRANSPORT_STRUCT* transport,
                                   ZOO_SMB_UDP_TRANSPORT_COMMON* udp)
{
    // Step 1: Validate transport configuration
    if (!validate_transport_config(transport))
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_LOG_DEBUG("Starting UDP %s initialization: %s:%d",
                      transport->config->is_server ? "server" : "client",
                      transport->config->address ? transport->config->address : "default",
                      transport->config->port);

    // Step 2: Initialize UDP structure fields
    ZOO_ERROR_TYPE result = init_udp_structure(transport, udp);
    if (result != ZOO_SMB_OK)
    {
        return result;
    }

    // Step 3: Create UDP socket
    result = create_udp_socket(udp);
    if (result != ZOO_SMB_OK)
    {
        cleanup_udp_init_failure(udp);
        return result;
    }

    // Step 4: Configure socket addresses
    result = configure_socket_address(transport, udp);
    if (result != ZOO_SMB_OK)
    {
        cleanup_udp_init_failure(udp);
        return result;
    }

    // Step 5: Bind socket (this will set multicast-specific options internally)
    result = bind_udp_socket(udp);
    if (result != ZOO_SMB_OK)
    {
        cleanup_udp_init_failure(udp);
        return result;
    }

    // Step 6: Join multicast group for clients (after successful binding)
    ZOO_BOOL has_multicast = (transport->config->multicast_port > 0 &&
                         strlen(transport->config->multicast_address) > 0);
    
    if (!udp->is_server && has_multicast)
    {
        const int max_join_attempts = 3;
        for (int attempt = 0; attempt < max_join_attempts; ++attempt)
        {
            result = udp_join_multicast_group(udp, transport->config->multicast_address);
            if (result == ZOO_SMB_OK)
            {
                break;
            }

            if (attempt + 1 < max_join_attempts)
            {
                usleep(10000); // 10ms
            }
        }

        if (result != ZOO_SMB_OK)
        {
            cleanup_udp_init_failure(udp);
            return result;
        }
    }

    ZOO_LOG_INFO("UDP %s initialization completed successfully: socket=%d, multicast=%s",
                     udp->is_server ? "server" : "client",
                     udp->sockfd,
                     has_multicast ? "enabled" : "disabled");

    return ZOO_SMB_OK;
}