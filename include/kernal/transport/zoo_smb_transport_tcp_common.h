/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TCP_COMMON
 * File name: zoo_smb_transport_tcp_common.h
 * Description: Common definitions and utilities for TCP transport
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_TCP_COMMON_H
#define ZOO_SMB_TRANSPORT_TCP_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_util.h"
#include "zoo_smb_transport.h"
#include "zoo_smb_protocol.h"
#include "zoo_smb_error.h"
#include "zoo_list.h"
#include "zoo_memory_pool.h"
#include "zoo_socket.h"
#include <sys/epoll.h>
#include <netinet/in.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

// ==============================================================================
// CONSTANTS AND MACROS
// ==============================================================================

// TCP specific constants
#define TCP_EPOLL_MAX_EVENTS 10            /**< Maximum epoll events per wait */
#define TCP_EPOLL_TIMEOUT_MS 5000          /**< Epoll timeout in milliseconds */
#define TCP_CONNECTION_TIMEOUT_SEC 5       /**< Connection timeout in seconds */
#define TCP_MAX_RETRY_COUNT 3              /**< Maximum retry attempts */
#define TCP_RETRY_DELAY_US 1000            /**< Retry delay in microseconds */
#define TCP_DEFAULT_RECONNECT_INTERVAL 5   /**< Default reconnect interval in seconds */
#define TCP_DEFAULT_HEARTBEAT_INTERVAL 5   /**< Default heartbeat interval in seconds */
#define TCP_HEARTBEAT_TIMEOUT_MULTIPLIER 3 /**< Heartbeat timeout multiplier */
#define TCP_DEFAULT_BACKLOG 256            /**< Default listen backlog size */
#define TCP_MAX_CLIENTS 1024               /**< Maximum number of clients for server */

// Buffer sizes
#define TCP_RECEIVE_BUFFER_SIZE 8192 /**< TCP receive buffer size */
#define TCP_SEND_BUFFER_SIZE 8192    /**< TCP send buffer size */

    // ==============================================================================
    // STRUCTURE DEFINITIONS
    // ==============================================================================

    /**
     * @brief Client connection information structure
     */
    typedef struct
    {
        ZOO_SOCKET_INFO_STRUCT socket_info; /**< ZOO socket information */
        ZOO_SOCKADDR_UNION client_addr;     /**< Client address information */
        char client_name[64];               /**< Client name identifier */
        time_t connect_time;                /**< Connection establishment time */
        time_t disconnect_time;             /**< Disconnection timestamp */
        time_t last_activity;               /**< Last activity timestamp */
        ZOO_BOOL active;                    /**< Connection active status */

        // Client statistics
        uint64_t bytes_sent;        /**< Total bytes sent to client */
        uint64_t bytes_received;    /**< Total bytes received from client */
        uint32_t messages_sent;     /**< Total messages sent to client */
        uint32_t messages_received; /**< Total messages received from client */
    } TCP_CLIENT_INFO_STRUCT;

    /**
     * @brief TCP Transport Common Context Structure (Base)
     * This structure contains common fields shared between client and server implementations
     */
    typedef struct
    {
        // Core socket descriptors
        int epoll_fd;                        /**< Epoll file descriptor for event monitoring */
        int conn_fd;                         /**< Active connection file descriptor */
        ZOO_SOCKET_INFO_STRUCT socket_info;  /**< Current connection socket information */

        // Network configuration  
        struct sockaddr_in addr;             /**< Legacy IPv4 endpoint address */
        ZOO_SOCKADDR_UNION server_addr;      /**< Network address information */
        ZOO_BOOL is_server;                  /**< Server/client mode flag */
        ZOO_BOOL running;                    /**< Running status flag */

        // Callback handlers
        ZOO_LIST_HANDLE data_observers; /**< Data observers list */
        ZOO_MUTEX_T* data_observers_lock; /**< Pointer to transport observer lock */

        // Connection management
        TRANSPORT_CONNECTION_STATE_ENUM conn_state; /**< Current connection state */
        int reconnect_interval;                     /**< Reconnect interval in seconds */
        time_t last_connect_attempt;                /**< Last connection attempt timestamp */

        // Heartbeat management
        int heartbeat_interval; /**< Heartbeat interval in seconds */
        time_t last_heartbeat;  /**< Last heartbeat timestamp */

        // Configuration reference
        ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config; /**< Transport configuration pointer */
    } ZOO_SMB_TCP_TRANSPORT_COMMON;

    // ==============================================================================
    // SOCKET UTILITY FUNCTIONS
    // ==============================================================================

    /**
     * @brief Set socket to non-blocking mode
     * @param socket_info ZOO socket information structure
     * @return ZOO_TRUE on success, ZOO_FALSE on failure
     */
    ZOO_BOOL tcp_set_nonblocking(ZOO_SOCKET_INFO_STRUCT* socket_info);

    /**
     * @brief Configure socket keep-alive options  
     * @param socket_info ZOO socket information structure
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_set_keepalive(ZOO_SOCKET_INFO_STRUCT* socket_info);

    /**
     * @brief Setup epoll monitoring for a socket
     * @param epoll_fd Epoll instance file descriptor
     * @param socket_info ZOO socket information structure
     * @param events Epoll events to monitor (EPOLLIN, EPOLLOUT, etc.)
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_setup_epoll_monitoring(int epoll_fd, int sock_fd, uint32_t events);

    /**
     * @brief Remove socket from epoll monitoring
     * @param epoll_fd Epoll instance file descriptor
     * @param socket_info ZOO socket information structure  
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_remove_epoll_monitoring(int epoll_fd, int sock_fd);

    /**
     * @brief Check if epoll events indicate connection loss
     * @param events Epoll events flags
     * @return ZOO_TRUE if connection is lost, ZOO_FALSE otherwise
     */
    ZOO_BOOL tcp_is_connection_lost_event(uint32_t events);

    /**
     * @brief Get socket error status
     * @param socket_info ZOO socket information structure
     * @return Socket error code (0 if no error)
     */
    int tcp_get_socket_error(ZOO_SOCKET_INFO_STRUCT* socket_info);

    // ==============================================================================
    // CONNECTION MANAGEMENT FUNCTIONS
    // ==============================================================================

    /**
     * @brief Update TCP transport connection state
     * @param tcp TCP transport common structure pointer
     * @param new_state New connection state to set
     */
    void tcp_update_connection_state(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, TRANSPORT_CONNECTION_STATE_ENUM new_state);

    /**
     * @brief Read data from socket with retry mechanism
     * @param fd Socket file descriptor
     * @param buffer Buffer to store read data
     * @param size Maximum number of bytes to read
     * @return Number of bytes read on success, -1 on error, 0 on connection close
     */
    ssize_t tcp_read_with_retry(int fd, uint8_t* buffer, size_t size);

    /**
     * @brief Write data to socket with retry mechanism
     * @param fd Socket file descriptor
     * @param buffer Buffer containing data to write
     * @param size Number of bytes to write
     * @return Number of bytes written on success, -1 on error
     */
    ssize_t tcp_write_with_retry(int fd, const uint8_t* buffer, size_t size);

    /**
     * @brief Trigger connection status change callbacks
     * @param tcp TCP transport common structure pointer
     * @param fd Socket file descriptor
     * @param connected Connection status (ZOO_TRUE for connected, ZOO_FALSE for disconnected)
     */
    void tcp_notify_consumer_observers(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, int fd, ZOO_BOOL connected);

    /**
     * @brief Clean up connection resources
     * @param tcp TCP transport common structure pointer
     * @param fd Socket file descriptor to clean up
     */
    void tcp_cleanup_connection(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, int fd);

    // ==============================================================================
    // MESSAGE PROCESSING FUNCTIONS
    // ==============================================================================

    /**
     * @brief Notify all registered observers about received message
     * @param tcp TCP transport common structure pointer
     * @param fd Socket file descriptor where message was received
     * @param header Pointer to message header
     * @param msg Pointer to message payload data
     * @param msg_len Message payload length
     */
    void tcp_notify_data_observers(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, const ZOO_SMB_MSG_STRUCT* msg);

    /**
     * @brief Process received message and handle different message types
     * @param tcp TCP transport common structure pointer
     * @param fd Socket file descriptor where message was received
     * @param header Pointer to message header
     * @param msg Pointer to message payload data
     * @param msg_len Message payload length
     */
    void tcp_process_received_message(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, int fd, const ZOO_SMB_MSG_STRUCT* msg);

    /**
     * @brief Handle incoming data from socket
     * @param tcp TCP transport common structure pointer
     * @param fd Socket file descriptor with incoming data
     */
    void tcp_handle_incoming_data(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, int fd,  TCP_CLIENT_INFO_STRUCT* client_info);

    /**
     * @brief Send complete message (header + payload) to socket
     * @param fd Socket file descriptor
     * @param header Pointer to message header
     * @param payload Pointer to message payload (can be NULL if payload_size is 0)
     * @param payload_size Size of message payload
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE tcp_send_complete_message(int fd, const ZOO_SMB_MSG_STRUCT* msg);

    // ==============================================================================
    // HEARTBEAT FUNCTIONS
    // ==============================================================================

    /**
     * @brief Check for heartbeat timeout and handle disconnection
     * @param tcp TCP transport common structure pointer
     * @param current_time Current timestamp
     */
    void tcp_check_heartbeat_timeout(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, time_t current_time);

#ifdef __cplusplus
}
#endif

#endif  // ZOO_SMB_TRANSPORT_TCP_COMMON_H