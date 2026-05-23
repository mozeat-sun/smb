/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TCP_COMMON
 * File name: zoo_smb_transport_tcp_common.c
 * Description: Common utilities for TCP transport
 ******************************************************************************/

#include "zoo_smb_transport_tcp_common.h"
#include "zoo_smb_reactor.h"
#include "zoo_log.h"
#include "zoo_memory_pool.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <poll.h>

// ==============================================================================
// SOCKET UTILITY FUNCTIONS
// ==============================================================================

/**
 * @brief Set socket to non-blocking mode
 *
 * This function configures a socket to operate in non-blocking mode,
 * which allows I/O operations to return immediately if they cannot be completed
 * without waiting.
 *
 * @param socket_info ZOO socket information structure
 * @return ZOO_TRUE if successfully set to non-blocking mode, ZOO_FALSE on failure
 */
ZOO_BOOL tcp_set_nonblocking(ZOO_SOCKET_INFO_STRUCT* socket_info)
{
    if (!socket_info)
    {
        ZOO_LOG_ERROR("Invalid socket_info parameter");
        return ZOO_FALSE;
    }

    ZOO_ERROR_TYPE result = zoo_socket_set_blocking(socket_info, false);
    if (result != ZOO_OK)
    {
        ZOO_LOG_ERROR("Failed to set non-blocking mode: error=%s", zoo_socket_error_string(result));
        return ZOO_FALSE;
    }

    ZOO_LOG_DEBUG("Socket set to non-blocking mode successfully");
    return ZOO_TRUE;
}

/**
 * @brief Configure socket keep-alive options
 *
 * This function enables TCP keep-alive mechanism on a socket and configures
 * the keep-alive parameters for detecting broken connections.
 *
 * @param socket_info ZOO socket information structure
 * @return ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE tcp_set_keepalive(ZOO_SOCKET_INFO_STRUCT* socket_info)
{
    if (!socket_info)
    {
        ZOO_LOG_ERROR("Invalid socket_info parameter");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Enable keep-alive
    int keepalive = 1;
    ZOO_ERROR_TYPE result = zoo_socket_setsockopt(socket_info, ZOO_SOL_SOCKET, ZOO_SO_KEEPALIVE, 
                                                  &keepalive, sizeof(keepalive));
    if (result != ZOO_OK)
    {
        ZOO_LOG_ERROR("Failed to set SO_KEEPALIVE: error=%s", zoo_socket_error_string(result));
        return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
    }

    ZOO_LOG_DEBUG("Socket keep-alive configured successfully");
    return ZOO_SMB_OK;
}

/**
 * @brief Waits for a file descriptor to become ready for I/O.
 *
 * @param fd File descriptor
 * @param events Poll events to wait for
 * @param timeout_ms Timeout in milliseconds
 * @return ZOO_TRUE if ready, ZOO_FALSE on timeout/error
 */
static ZOO_BOOL tcp_wait_fd_ready(int fd, short events, int timeout_ms)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;

    for (;;)
    {
        int ready = poll(&pfd, 1, timeout_ms);
        if (ready > 0)
        {
            return ZOO_TRUE;
        }
        if (ready == 0)
        {
            return ZOO_FALSE;
        }
        if (errno == EINTR)
        {
            continue;
        }
        return ZOO_FALSE;
    }
}

/**
 * @brief Setup epoll monitoring for a socket
 *
 * This function adds or modifies a socket in an epoll instance for event monitoring.
 * It first tries to modify existing monitoring, and if the socket is not present,
 * it adds it to the epoll instance.
 *
 * @param epoll_fd Epoll instance file descriptor
 * @param sock_fd Socket file descriptor to monitor
 * @param events Epoll events to monitor (EPOLLIN, EPOLLOUT, etc.)
 * @return ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE tcp_setup_epoll_monitoring(int epoll_fd, int sock_fd, uint32_t events)
{
    ZOO_ERROR_TYPE ret = zoo_smb_reactor_watch_fd(epoll_fd, sock_fd, events);
    if (ret != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("Failed to watch socket in reactor: fd=%d, events=0x%x", sock_fd, events);
        return ret;
    }

    ZOO_LOG_TRACE("Socket reactor watch set: fd=%d, events=0x%x", sock_fd, events);
    return ZOO_SMB_OK;
}

/**
 * @brief Check if epoll events indicate connection loss
 *
 * This function analyzes epoll event flags to determine if they indicate
 * a lost or broken connection. It checks for error conditions and hangup events.
 *
 * @param events Epoll events flags to analyze
 * @return ZOO_TRUE if connection is lost, ZOO_FALSE otherwise
 */
ZOO_BOOL tcp_is_connection_lost_event(uint32_t events)
{
    // Check for connection loss indicators:
    // EPOLLRDHUP - Stream socket peer closed connection or shut down writing half
    // EPOLLHUP   - Hang up happened on associated file descriptor
    // EPOLLERR   - Error condition happened on associated file descriptor
    return (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) != 0;
}

// ==============================================================================
// CONNECTION MANAGEMENT FUNCTIONS
// ==============================================================================

/**
 * @brief Update TCP transport connection state
 *
 * This function updates the connection state of a TCP transport instance and
 * triggers appropriate callbacks when the state changes, particularly for
 * connection/disconnection events.
 *
 * @param tcp TCP transport common structure pointer
 * @param new_state New connection state to set
 */
void tcp_update_connection_state(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, TRANSPORT_CONNECTION_STATE_ENUM new_state)
{
    // Skip if state is already current
    if (tcp->conn_state == new_state)
    {
        return;
    }

    // State names for logging
    const char* state_names[] = {
        "DISCONNECTED", "CONNECTING", "CONNECTED", "RECONNECTING"};

    ZOO_LOG_INFO("Connection state: %s -> %s",
                     state_names[tcp->conn_state],
                     state_names[new_state]);

    tcp->conn_state = new_state;
}

/**
 * @brief Read data from socket with retry mechanism
 *
 * This function attempts to read data from a socket with built-in retry logic
 * for handling temporary unavailability (EAGAIN/EWOULDBLOCK). It's designed
 * for non-blocking sockets.
 *
 * @param fd Socket file descriptor to read from
 * @param buffer Buffer to store read data
 * @param size Maximum number of bytes to read
 * @return Number of bytes read on success, -1 on error, 0 on connection close
 */
ssize_t tcp_read_with_retry(int fd, uint8_t* buffer, size_t size)
{
    size_t total_read = 0;
    int retry_count = 0;

    // Continue reading until we get all requested data or hit retry limit
    while (total_read < size && retry_count < TCP_MAX_RETRY_COUNT)
    {
        ssize_t n = recv(fd, buffer + total_read, size - total_read, MSG_DONTWAIT);

        if (n > 0)
        {
            // Successful read - accumulate data and reset retry counter
            total_read += n;
            retry_count = 0;
        }
        else if (n == 0)
        {
            // Connection closed by peer
            ZOO_LOG_DEBUG("Connection closed by peer: fd=%d", fd);
            return 0;
        }
        else
        {
            // Handle read errors
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Temporary unavailability - wait for readability then retry
                retry_count++;
                if (retry_count < TCP_MAX_RETRY_COUNT)
                {
                    (void)tcp_wait_fd_ready(fd, POLLIN, (int)(TCP_RETRY_DELAY_US / 1000));
                    continue;
                }
                ZOO_LOG_TRACE("No data available after retries: fd=%d", fd);
                return -1;
            }
            else
            {
                // Permanent error
                ZOO_LOG_ERROR("Read error: fd=%d, error=%s", fd, strerror(errno));
                return -1;
            }
        }
    }

    return (ssize_t)total_read;
}

/**
 * @brief Write data to socket with retry mechanism
 *
 * This function attempts to write all requested bytes with built-in retry
 * logic for handling temporary unavailability (EAGAIN/EWOULDBLOCK).
 *
 * @param fd Socket file descriptor to write to
 * @param buffer Buffer containing data to write
 * @param size Number of bytes to write
 * @return Number of bytes written on success, -1 on error
 */
ssize_t tcp_write_with_retry(int fd, const uint8_t* buffer, size_t size)
{
    size_t total_written = 0;
    int retry_count = 0;

    while (total_written < size && retry_count < TCP_MAX_RETRY_COUNT)
    {
        ssize_t n = send(fd, buffer + total_written, size - total_written, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (n > 0)
        {
            total_written += (size_t)n;
            retry_count = 0;
            continue;
        }

        if (n == 0)
        {
            return -1;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            retry_count++;
            if (retry_count < TCP_MAX_RETRY_COUNT)
            {
                (void)tcp_wait_fd_ready(fd, POLLOUT, (int)(TCP_RETRY_DELAY_US / 1000));
                continue;
            }
            return -1;
        }

        if (errno == EINTR)
        {
            continue;
        }

        return -1;
    }

    return (ssize_t)total_written;
}

/**
 * @brief Clean up connection resources
 *
 * This function performs comprehensive cleanup of a connection, including
 * removing it from epoll monitoring, closing the socket, and updating
 * the transport state appropriately for client or server mode.
 *
 * @param tcp TCP transport common structure pointer
 * @param fd Socket file descriptor to clean up
 */
void tcp_cleanup_connection(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, int fd)
{
    ZOO_LOG_DEBUG("Cleaning up connection: fd=%d", fd);

    // Remove socket from epoll monitoring
    if (tcp->epoll_fd > 0)
    {
        epoll_ctl(tcp->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    }

    // Close the socket
    close(fd);

    // Update state based on server/client mode
    if (!tcp->is_server)
    {
        // Client mode: update state for potential reconnection
        if (fd == tcp->conn_fd)
        {
            tcp->conn_fd = INVALID_TRANSPORT_FD;
            tcp_update_connection_state(tcp, TRANSPORT_CONN_STATE_DISCONNECTED);
        }
    }

    ZOO_LOG_DEBUG("Connection cleanup completed: fd=%d", fd);
}

// ==============================================================================
// MESSAGE PROCESSING FUNCTIONS
// ==============================================================================

/**
 * @brief Notify all registered observers about received message
 *
 * This function iterates through all registered message observers and
 * invokes them with the received message data. Observers are typically
 * application handlers that process incoming messages.
 *
 * @param tcp TCP transport common structure pointer
 * @param fd Socket file descriptor where message was received
 * @param header Pointer to message header containing metadata
 * @param msg Pointer to message payload data
 * @param msg_len Message payload length in bytes
 */
void tcp_notify_data_observers(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, const ZOO_SMB_MSG_STRUCT* msg)
{
    ZOO_LOG_TRACE("Notifying observers for sender: %s, fd=%d, msg_type=%d, payload_size=%zu",
                      msg->header.sender,
                      tcp->conn_fd,
                      msg->header.msg_type,
                      msg->header.payload_size);

    size_t observer_count = 0;
    TRANSPORT_DATA_OBSERVER_STRUCT** snapshot = NULL;

    if (tcp->data_observers_lock)
    {
        ZOO_MUTEX_LOCK(tcp->data_observers_lock);
    }

    observer_count = zoo_list_size(tcp->data_observers);
    if (observer_count > 0)
    {
        snapshot = (TRANSPORT_DATA_OBSERVER_STRUCT**)zoo_allocate_from_pool(sizeof(TRANSPORT_DATA_OBSERVER_STRUCT*) * observer_count);
    }

    for (size_t i = 0; i < observer_count; i++)
    {
        TRANSPORT_DATA_OBSERVER_STRUCT* observer = (TRANSPORT_DATA_OBSERVER_STRUCT*)zoo_list_at(tcp->data_observers, i);
        if (snapshot)
        {
            snapshot[i] = observer;
        }
        else if (observer && observer->handler)
        {
            observer->handler(observer->user_data, msg);
        }
    }

    if (tcp->data_observers_lock)
    {
        ZOO_MUTEX_UNLOCK(tcp->data_observers_lock);
    }

    if (snapshot)
    {
        for (size_t i = 0; i < observer_count; i++)
        {
            TRANSPORT_DATA_OBSERVER_STRUCT* observer = snapshot[i];
            if (observer && observer->handler)
            {
                observer->handler(observer->user_data, msg);
            }
        }
        zoo_free_to_pool(snapshot);
    }
}

/**
 * @brief Process received message and handle different message types
 *
 * This function is the main entry point for processing complete messages
 * received from the network. It currently forwards all messages to observers,
 * but can be extended to handle different message types differently.
 *
 * @param tcp TCP transport common structure pointer
 * @param fd Socket file descriptor where message was received
 * @param header Pointer to message header
 * @param msg Pointer to message payload data
 * @param msg_len Message payload length in bytes
 */
void tcp_process_received_message(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, int fd, const ZOO_SMB_MSG_STRUCT* msg)
{
    ZOO_LOG_TRACE("Processing message: fd=%d, payload_size=%zu", fd, msg->header.payload_size);

    // Forward message to all registered observers
    tcp_notify_data_observers(tcp, msg);
}

/**
 * @brief Handle incoming data from socket
 *
 * This function reads a complete message from the socket, including the header
 * and payload, and processes it. It handles errors gracefully and cleans up
 * the connection if necessary.
 *
 * @param tcp TCP transport common structure pointer
 * @param fd Socket file descriptor with incoming data
 */
void tcp_handle_incoming_data(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, int fd, TCP_CLIENT_INFO_STRUCT* client_info)
{
    ZOO_LOG_TRACE("Handling incoming data: fd=%d", fd);
    // printf removed
    // Step 1: Allocate memory for header only
    uint8_t* header_buffer = (uint8_t*)zoo_allocate_from_pool(sizeof(ZOO_SMB_MSG_HEADER_STRUCT));
    if (!header_buffer)
    {
        ZOO_LOG_ERROR("Failed to allocate header buffer: fd=%d", fd);
        return;
    }

    // Step 2: Read header first
    size_t header_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT);
    ssize_t n = tcp_read_with_retry(fd, header_buffer, header_size);
    if (n != (ssize_t)header_size)  // 修复符号比较警告
    {
        ZOO_LOG_ERROR("Failed to read message header: fd=%d, read=%zd", fd, n);
        zoo_free_to_pool(header_buffer);
        return;
    }

    // Step 3: Parse header to get payload size
    ZOO_SMB_MSG_HEADER_STRUCT* header = (ZOO_SMB_MSG_HEADER_STRUCT*)header_buffer;
    size_t payload_size = header->payload_size;

    // Step 4: Validate payload size (修复符号比较警告)
    if (payload_size > (size_t)(MAX_TRANSPORT_BUFFER_SIZE - header_size))
    {
        ZOO_LOG_ERROR("Payload size too large: %zu, max allowed: %zu", 
                          payload_size, (size_t)(MAX_TRANSPORT_BUFFER_SIZE - header_size));
        zoo_free_to_pool(header_buffer);
        return;
    }

    // Step 5: Allocate memory for complete message (header + payload)
    size_t total_size = header_size + payload_size;
    uint8_t* complete_buffer = (uint8_t*)zoo_allocate_from_pool(total_size);
    if (!complete_buffer)
    {
        ZOO_LOG_ERROR("Failed to allocate complete message buffer: fd=%d, size=%zu", fd, total_size);
        zoo_free_to_pool(header_buffer);
        return;
    }

    // Step 6: Copy header to complete buffer
    memcpy(complete_buffer, header_buffer, header_size);
    zoo_free_to_pool(header_buffer);  // Free header buffer early

    // Step 7: Read payload if exists
    if (payload_size > 0)
    {
        n = tcp_read_with_retry(fd, complete_buffer + header_size, payload_size);
        if (n != (ssize_t)payload_size)  // 修复符号比较警告
        {
            ZOO_LOG_ERROR("Failed to read message payload: fd=%d, read=%zd, expected=%zu", 
                              fd, n, payload_size);
            zoo_free_to_pool(complete_buffer);
            return;
        }
    }

    // Step 8: Deserialize and process message
    ZOO_SMB_MSG_STRUCT* msg = zoo_smb_default_message();
    if (ZOO_SMB_OK == zoo_smb_protocol_deserialize(complete_buffer, total_size, msg, sizeof(ZOO_SMB_MSG_STRUCT)))
    {
        // Update client info if provided
        if (client_info && msg->header.sender[0])
        {
            snprintf(client_info->client_name, sizeof(client_info->client_name), "%s", msg->header.sender);
        }
        tcp_process_received_message(tcp, fd, msg);
    }

    // Step 9: Update heartbeat for client connections
    if (!tcp->is_server && fd == tcp->conn_fd)
    {
        tcp->last_heartbeat = time(NULL);
    }

    // Step 10: Clean up
    zoo_free_to_pool(complete_buffer);
    zoo_smb_destroy_message(msg);
}

// ==============================================================================
// HEARTBEAT FUNCTIONS
// ==============================================================================

/**
 * @brief Check for heartbeat timeout and handle disconnection
 *
 * This function monitors heartbeat timing for client connections and
 * automatically closes connections that have been inactive for too long.
 * Only applies to client-side connections as servers handle multiple clients.
 *
 * @param tcp TCP transport common structure pointer
 * @param current_time Current timestamp to compare against last heartbeat
 */
void tcp_check_heartbeat_timeout(ZOO_SMB_TCP_TRANSPORT_COMMON* tcp, time_t current_time)
{
    // Only check timeout for connected clients
    if (tcp->is_server || tcp->conn_state != TRANSPORT_CONN_STATE_CONNECTED)
    {
        return;
    }

    // Calculate timeout threshold (heartbeat interval * multiplier)
    time_t timeout_threshold = tcp->heartbeat_interval * TCP_HEARTBEAT_TIMEOUT_MULTIPLIER;

    // Check if connection has been inactive too long
    if (current_time - tcp->last_heartbeat > timeout_threshold)
    {
        ZOO_LOG_WARN("Heartbeat timeout detected, closing connection: fd=%d", tcp->conn_fd);
        tcp_cleanup_connection(tcp, tcp->conn_fd);
    }
}

/**
 * @brief Remove socket from epoll monitoring
 *
 * This function removes a socket file descriptor from epoll monitoring.
 * It's used when cleaning up connections or when sockets are no longer
 * needed for event monitoring.
 *
 * @param epoll_fd Epoll instance file descriptor
 * @param sock_fd Socket file descriptor to remove
 * @return ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE tcp_remove_epoll_monitoring(int epoll_fd, int sock_fd)
{
    ZOO_ERROR_TYPE ret = zoo_smb_reactor_unwatch_fd(epoll_fd, sock_fd);
    if (ret != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("Failed to remove socket from reactor: fd=%d", sock_fd);
        return ret;
    }

    ZOO_LOG_TRACE("Socket removed from reactor monitoring: fd=%d", sock_fd);
    return ZOO_SMB_OK;
}

/**
 * @brief Send complete message (header + payload) to socket
 *
 * This function sends a complete message including both header and payload
 * to the specified socket. It handles the protocol framing by sending
 * the header first followed by the payload data.
 *
 * @param fd Socket file descriptor to send to
 * @param header Pointer to message header containing metadata
 * @param payload Pointer to message payload (can be NULL if payload_size is 0)
 * @param payload_size Size of message payload in bytes
 * @return ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE tcp_send_complete_message(int fd, const ZOO_SMB_MSG_STRUCT* msg)
{
    if (!msg)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    size_t total_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + msg->header.payload_size;
    if (total_size > MAX_TRANSPORT_BUFFER_SIZE)
    {
        ZOO_LOG_ERROR("tcp_send: message payload size exceeds maximum buffer size\n");
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    uint8_t* msg_buffer = zoo_allocate_from_pool(total_size);
    if (!msg_buffer)
    {
        ZOO_LOG_ERROR("tcp_send: failed to allocate memory for message buffer\n");
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    size_t msg_size = zoo_smb_protocol_serialize(msg, msg_buffer, total_size);
    if (msg_size == 0)
    {
        ZOO_LOG_ERROR("tcp_send: failed to serialize message\n");
        zoo_free_to_pool(msg_buffer);
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    ssize_t sent_size = tcp_write_with_retry(fd, msg_buffer, msg_size);
    if (sent_size != (ssize_t)msg_size)
    {
        ZOO_LOG_ERROR("tcp_send: failed to send payload (sent_size=%zd, expected=%zu, errno=%d)",
                          sent_size,
                          msg_size,
                          errno);
        zoo_free_to_pool(msg_buffer);
        return ZOO_SMB_ERROR_NETWORK_SEND_FAILED;
    }

    zoo_free_to_pool(msg_buffer);
    ZOO_LOG_DEBUG("tcp_send: sent message (size=%zu, fd=%d)", msg_size, fd);
    return ZOO_SMB_OK;
}
