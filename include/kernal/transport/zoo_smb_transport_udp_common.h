/**
 * @file zoo_smb_transport_udp_common.h
 * @brief Common UDP transport definitions and utilities for ZOO SMB messaging system
 * @details This module provides shared UDP transport functionality including:
 *          - Common UDP transport structure definition
 *          - Shared utility functions for client and server
 *          - Message handling and observer notification
 *          - Client registration and management
 *          - Multicast support for publisher/subscriber
 * @author ZOO SMB Development Team
 * @date 2025
 * @version 1.0
 * @copyright Copyright (c) 2025 ZOO SMB Project
 */

#ifndef ZOO_SMB_TRANSPORT_UDP_COMMON_H
#define ZOO_SMB_TRANSPORT_UDP_COMMON_H

#include "zoo_smb_transport.h"
#include "zoo_smb_message.h"
#include "zoo_list.h"
#include "zoo_socket.h"
#include <netinet/in.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Common UDP transport structure
     * @details Contains fields shared between UDP client and server implementations
     *          including socket management, addressing, and observer handling
     */
    typedef struct
    {
        const char* name;                        /**< Transport name */
        int sockfd;                              /**< Native UDP socket descriptor */
        ZOO_SOCKET_INFO_STRUCT socket_info;      /**< ZOO socket information */
        struct sockaddr_in server_addr;          /**< Target server address (for clients) */
        struct sockaddr_in multicast_addr;       /**< Multicast address (for clients) */
        struct sockaddr_in broadcast_addr;       /**< Broadcast address (for clients) */
        ZOO_BOOL running;                            /**< Transport running state flag */
        ZOO_BOOL is_server;                          /**< Server/Client mode indicator */
        ZOO_LIST_HANDLE data_observers;    /**< List of data observers */
        ZOO_MUTEX_T* data_observers_lock; /**< Pointer to transport observer lock */
        ZOO_LIST_HANDLE clients;           /**< List of connected consumers/clients */
        ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config; /**< User-defined data pointer */
    } ZOO_SMB_UDP_TRANSPORT_COMMON;

    /**
     * @brief Notify all registered data observers about received message
     * @details Iterates through all registered observers and calls their notification
     *          callbacks with the received message. Provides thread-safe iteration.
     * @param[in] udp Pointer to UDP transport common structure
     * @param[in] msg Received message to notify observers about
     * @note Thread-safe operation with proper error handling
     * @note Skips NULL observers without error
     */
    void udp_notify_data_observers(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, const ZOO_SMB_MSG_STRUCT* msg);

    /**
     * @brief Register new UDP client/consumer based on message header
     * @details Registers a new client or updates existing client information based on
     *          the sender address and message header. Maintains client list for routing.
     * @param[in] udp Pointer to UDP transport common structure
     * @param[in] sender_addr ZOO socket address of the sender
     * @param[in] header Message header containing sender information
     * @note Creates new consumer entry if sender not found
     * @note Updates existing entry timestamp if sender already registered
     * @note Thread-safe operation with proper memory management
     */
    void udp_register_client(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, struct sockaddr_in* sender_addr, const ZOO_SMB_MSG_HEADER_STRUCT* header);

    /**
     * @brief Process received UDP message and notify observers
     * @details Central message processing function that handles message validation,
     *          logging, and observer notification for received UDP messages.
     * @param[in] udp Pointer to UDP transport common structure
     * @param[in] msg Received message to process
     * @note Validates message integrity before processing
     * @note Provides comprehensive debug logging
     * @note Handles observer notification errors gracefully
     */
    void udp_process_received_message(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, const ZOO_SMB_MSG_STRUCT* msg);

    /**
     * @brief Handle incoming raw UDP data from socket
     * @details Processes raw UDP data with improved multicast support including:
     *          - Message deserialization using protocol handler
     *          - Client registration and management
     *          - Message processing through observers
     *          - Comprehensive error handling and resource cleanup
     * @param[in] udp Pointer to UDP transport common structure
     * @param[in] buffer Raw data buffer received from socket
     * @param[in] bytes_received Number of bytes received
     * @param[in] sender_addr ZOO socket address of the sender
     * @note Automatically handles message deserialization failures
     * @note Registers unknown senders as new clients
     * @note Cleans up allocated resources on completion
     */
    void handle_incoming_data(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, uint8_t* buffer, size_t bytes_received, struct sockaddr_in* sender_addr);

    /**
     * @brief Send message via UDP socket with common serialization
     * @details Common send function for all UDP transport types including:
     *          - Message serialization using protocol handler
     *          - UDP socket transmission to specified address
     *          - Error handling and resource cleanup
     * @param[in] udp Pointer to UDP transport common structure
     * @param[in] msg Message structure to send
     * @param[in] target_addr Target socket address
     * @return ZOO_SMB_OK on success, error code otherwise
     * @retval ZOO_SMB_OK Message sent successfully
     * @retval ZOO_SMB_ERROR_INVALID_PARAM Invalid parameters
     * @retval ZOO_SMB_ERROR_ALLOCATION_FAILED Memory allocation failed
     * @retval ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED Network send failed
     * @note Handles message serialization automatically
     * @note Manages memory allocation and cleanup
     * @note Provides detailed logging for debugging
     */
    ZOO_ERROR_TYPE udp_common_send(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, const ZOO_SMB_MSG_STRUCT* msg, const struct sockaddr_in* target_addr);

    /**
     * @brief Setup multicast socket options
     * @details Configures socket for multicast communication including:
     *          - Socket reuse options
     *          - Multicast TTL and loop settings (for senders)
     *          - Appropriate socket options for publishers/subscribers
     * @param[in] udp Pointer to UDP transport common structure
     * @param[in] is_sender Flag indicating if this is sender (publisher) or receiver (subscriber)
     * @return ZOO_SMB_OK on success, error code otherwise
     * @retval ZOO_SMB_OK Socket options configured successfully
     * @retval ZOO_SMB_ERROR_INVALID_PARAM Invalid parameters
     * @retval ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED Socket option configuration failed
     * @note Sets SO_REUSEADDR for all multicast sockets
     * @note Configures IP_MULTICAST_TTL and IP_MULTICAST_LOOP for senders
     * @note Provides appropriate defaults for multicast communication
     */
    ZOO_ERROR_TYPE udp_setup_multicast(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, ZOO_BOOL is_sender);

    /**
     * @brief Join multicast group for subscriber
     * @details Joins the specified multicast group for receiving multicast messages including:
     *          - IP_ADD_MEMBERSHIP socket option configuration
     *          - Multicast group address validation
     *          - Network interface selection
     * @param[in] udp Pointer to UDP transport common structure
     * @param[in] multicast_ip Multicast group IP address string
     * @return ZOO_SMB_OK on success, error code otherwise
     * @retval ZOO_SMB_OK Successfully joined multicast group
     * @retval ZOO_SMB_ERROR_INVALID_PARAM Invalid parameters or IP address
     * @retval ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED Failed to join multicast group
     * @note Validates multicast IP address format
     * @note Uses INADDR_ANY for network interface selection
     * @note Provides detailed error logging for troubleshooting
     */
    ZOO_ERROR_TYPE udp_join_multicast_group(ZOO_SMB_UDP_TRANSPORT_COMMON* udp, const char* multicast_ip);

    /**
     * @brief Common initialization for UDP transport (client and server)
     * @details Performs shared initialization tasks including:
     *          - Socket creation and configuration
     *          - Address configuration and validation
     *          - Client list initialization
     *          - Observer setup
     * @param[in] transport Pointer to transport configuration structure
     * @param[out] udp Pointer to UDP transport common structure to initialize
     * @param[in] is_server Flag indicating if this is server (ZOO_TRUE) or client (ZOO_FALSE) mode
     * @return ZOO_SMB_OK on success, error code otherwise
     * @retval ZOO_SMB_OK Initialization successful
     * @retval ZOO_SMB_ERROR_INVALID_PARAM Invalid parameters
     * @retval ZOO_SMB_ERROR_OUT_OF_MEMORY Memory allocation failed
     * @retval ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED Socket or network configuration failed
     * @note Sets up socket file descriptor and basic addressing
     * @note Initializes client list with proper size limits
     * @note Copies observer references from transport configuration
     */
    ZOO_ERROR_TYPE udp_common_init(ZOO_SMB_TRANSPORT_STRUCT* transport,
                                       ZOO_SMB_UDP_TRANSPORT_COMMON* udp);

    /**
     * @brief Send message via UDP multicast
     * @param udp UDP transport common structure
     * @param msg Message to send
     * @param multicast_ip Multicast group IP address
     * @param port Target port
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE udp_send_multicast(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                          const ZOO_SMB_MSG_STRUCT* msg,
                                          const struct sockaddr_in* multicast_addr);

    /**
     * @brief Send message via UDP broadcast
     * @param udp UDP transport common structure
     * @param msg Message to send
     * @param port Target port
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE udp_send_broadcast(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                          const ZOO_SMB_MSG_STRUCT* msg,
                                          const struct sockaddr_in* broadcast_addr);

    /**
     * @brief Send message via UDP unicast
     * @param udp UDP transport common structure
     * @param msg Message to send
     * @param target_addr Target client address
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE udp_send_unicast(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                        const ZOO_SMB_MSG_STRUCT* msg,
                                        const struct sockaddr_in* target_addr);

    /**
     * @brief Send message to client by name using unicast
     * @param udp UDP transport common structure
     * @param msg Message to send
     * @param client_name Target client name
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE udp_send_unicast_by_name(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                                const ZOO_SMB_MSG_STRUCT* msg,
                                                const char* client_name);

    /**
     * @brief Send broadcast message on specific network interface
     * @param udp UDP transport common structure
     * @param msg Message to send
     * @param port Target port
     * @param interface_ip Interface IP address (NULL for default)
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE udp_send_broadcast_on_interface(ZOO_SMB_UDP_TRANSPORT_COMMON* udp,
                                                       const ZOO_SMB_MSG_STRUCT* msg,
                                                       uint16_t port,
                                                       const char* interface_ip);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_UDP_COMMON_H */