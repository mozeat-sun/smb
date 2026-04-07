/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TYPE_UDP
 * File name: zoo_smb_transport_udp_broadcast.c
 * Description: UDP broadcast transport implementation for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-20     weiwang.sun      created
 * 1.1       2025-05-23     weiwang.sun      Optimized with more checks and comments
 ******************************************************************************/

#include "zoo_util.h"
#include "zoo_smb_transport.h"
#include "zoo_smb_reactor.h"
#include "../../memory_pool/inc/zoo_memory_pool.h"
#include "zoo.h"
#include "../../../platform/inc/zoo_types.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#define MAX_BROADCAST_TRANSPORT_BUFFER_SIZE 512
/**
 * @brief UDP broadcast transport implementation structure.
 */
typedef struct
{
    int sockfd;                                    /**< UDP socket file descriptor */
    struct sockaddr_in addr;                       /**< Local address */
    struct sockaddr_in broadcast_addr;             /**< Broadcast address */
    ZOO_THREAD_T recv_thread;                      /**< Receive thread */
    ZOO_BOOL running;                                  /**< Running flag */
    ZOO_LIST_HANDLE data_observers;          /**< Receive callback list */
    ZOO_MUTEX_T* data_observers_lock;                /**< Observer list lock */
    void* user_data;                               /**< User data for callback */
    const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config; /**< Configuration */
} UDP_BROADCAST_TRANSPORT_STRUCT;

/**
 * @brief Create and configure an epoll instance for the UDP socket.
 * @param udp Pointer to the UDP broadcast transport structure.
 * @return Epoll file descriptor on success, ZOO_SMB_INVALID_FD on failure.
 */
static int create_and_configure_epoll(UDP_BROADCAST_TRANSPORT_STRUCT* udp)
{
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == INVALID_TRANSPORT_FD)
    {
        ZOO_LOG_TRACE("epoll_create1 failed\n");
        return INVALID_TRANSPORT_FD;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = udp->sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp->sockfd, &ev) == INVALID_TRANSPORT_FD)
    {
        ZOO_LOG_TRACE("epoll_ctl failed\n");
        close(epoll_fd);
        return INVALID_TRANSPORT_FD;
    }

    return epoll_fd;
}

/**
 * @brief Notifies all registered observers about transport events
 *
 * Iterates through the list of registered observer callbacks and invokes each one
 * to notify them about transport layer events/state changes.
 *
 * @note This is an internal helper function used by the UDP transport implementation
 */
static void udp_notify_data_observers(
    UDP_BROADCAST_TRANSPORT_STRUCT* udp,
    const ZOO_SMB_MSG_STRUCT* msg)
{
    size_t observer_count = 0;
    TRANSPORT_DATA_OBSERVER_STRUCT** snapshot = NULL;

    if (udp->data_observers_lock)
    {
        ZOO_MUTEX_LOCK(udp->data_observers_lock);
    }

    observer_count = zoo_list_size(udp->data_observers);
    if (observer_count > 0)
    {
        snapshot = (TRANSPORT_DATA_OBSERVER_STRUCT**)zoo_allocate_from_pool(sizeof(TRANSPORT_DATA_OBSERVER_STRUCT*) * observer_count);
    }

    for (size_t i = 0; i < observer_count; i++)
    {
        TRANSPORT_DATA_OBSERVER_STRUCT* observer = (TRANSPORT_DATA_OBSERVER_STRUCT*)zoo_list_at(udp->data_observers, i);
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
                observer->handler(observer->user_data, msg);
            }
        }
        zoo_free_to_pool(snapshot);
    }
}

/**
 * @brief Processes a received UDP message
 *
 * Handles processing of an incoming UDP message received by the transport layer.
 * This function is called internally after a UDP datagram is received.
 *
 * @param [in] msg The received message to process
 *
 * @note This function is static and intended for internal use only within the UDP transport implementation
 */
static void udp_process_received_message(
    UDP_BROADCAST_TRANSPORT_STRUCT* udp,
    const ZOO_SMB_MSG_STRUCT* msg)
{
    udp_notify_data_observers(udp, msg);
}

/**
 * @brief Handle received data and notify observers.
 * @param udp Pointer to the UDP broadcast transport structure.
 * @param buffer Pointer to the received data buffer.
 * @param bytes_received Number of bytes received.
 * @param sender_addr Sender address structure.
 */
static void handle_incoming_data(UDP_BROADCAST_TRANSPORT_STRUCT* udp, uint8_t* buffer, ssize_t bytes_received, struct sockaddr_in sender_addr)
{
    ZOO_SMB_MSG_STRUCT* msg_out = zoo_smb_default_message();
    if (ZOO_SMB_OK != zoo_smb_protocol_deserialize(buffer, bytes_received, msg_out, sizeof(ZOO_SMB_MSG_STRUCT)))
    {
        ZOO_LOG_ERROR("Failed to deserialize message from UDP transport\n");
        return;
    }

    ZOO_LOG_DEBUG("Received message from %s:%d, type: %d, size: %zu\n",
                     inet_ntoa(sender_addr.sin_addr),
                     ntohs(sender_addr.sin_port),
                     msg_out->header.msg_type,
                     bytes_received);

    udp_process_received_message(udp, msg_out);
    zoo_smb_destroy_message(msg_out);
}

/**
 * @brief Handle epoll events for the UDP socket.
 * @param udp Pointer to the UDP broadcast transport structure.
 * @param epoll_fd Epoll file descriptor.
 * @param buffer Pointer to the buffer for receiving data.
 */
static void handle_epoll_events(UDP_BROADCAST_TRANSPORT_STRUCT* udp, int epoll_fd, uint8_t* buffer)
{
    struct epoll_event events[10];
    int nfds = 0;
    ZOO_ERROR_TYPE wait_ret = zoo_smb_reactor_wait(epoll_fd, events, 10, 1000, &nfds);
    if (wait_ret != ZOO_SMB_OK)
    {
        ZOO_LOG_TRACE("reactor_wait failed\n");
        return;
    }
    if (nfds == 0)
    {
        // Timeout, continue the loop
        return;
    }

    for (int i = 0; i < nfds; i++)
    {
        if (events[i].data.fd == udp->sockfd)
        {
            // Clear the buffer
            memset(buffer, 0, MAX_BROADCAST_TRANSPORT_BUFFER_SIZE);
            struct sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);

            // Receive data from the socket
            ssize_t bytes_received = recvfrom(
                udp->sockfd, buffer, MAX_BROADCAST_TRANSPORT_BUFFER_SIZE, 0, (struct sockaddr*)&sender_addr, &sender_len);
            ZOO_LOG_TRACE("recvfrom: sockfd=%d, sender_ip=%s, sender_port=%d, bytes_received=%zd\n",
                              udp->sockfd, inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), bytes_received);
            if (bytes_received < 0)
            {
                ZOO_LOG_TRACE("recvfrom failed with error code: %zd\n", bytes_received);
                continue;
            }

            handle_incoming_data(udp, buffer, bytes_received, sender_addr);
        }
    }
}

/**
 * @brief UDP receive thread function.
 * @param arg Pointer to UDP_BROADCAST_TRANSPORT_STRUCT.
 */
static void udp_broadcast_receive_thread(void* arg)
{
    if (!arg)
    {
        ZOO_LOG_TRACE("udp_broadcast_receive_thread: arg is NULL\n");
        return;
    }

    UDP_BROADCAST_TRANSPORT_STRUCT* udp = (UDP_BROADCAST_TRANSPORT_STRUCT*)arg;

    int epoll_fd = create_and_configure_epoll(udp);
    if (epoll_fd == INVALID_TRANSPORT_FD)
    {
        return;
    }

    uint8_t* buffer = (uint8_t*)zoo_allocate_from_pool(MAX_BROADCAST_TRANSPORT_BUFFER_SIZE);
    if (buffer == NULL)
    {
        ZOO_LOG_ERROR("zoo_smb_allocate_from_pool failed\n");
        close(epoll_fd);
        return;
    }

    ZOO_LOG_TRACE("[UDP] Receive thread started");
    while (udp->running)
    {
        handle_epoll_events(udp, epoll_fd, buffer);
    }
    ZOO_LOG_TRACE("[UDP] Receive thread stopping, cleaning up");

    // Close the epoll instance
    close(epoll_fd);
    if (buffer)
        // Free the buffer
        zoo_free_to_pool((void*)buffer);
    ZOO_LOG_TRACE("[UDP] Receive thread stopped");
}

/**
 * @brief Create and configure the UDP socket for broadcast.
 * @param udp Pointer to the UDP broadcast transport structure.
 * @param config Pointer to the transport configuration.
 * @return 0 on success, ZOO_SMB_INVALID_FD on failure.
 */
static int create_and_configure_socket(UDP_BROADCAST_TRANSPORT_STRUCT* udp, const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config)
{
    // Create UDP socket
    udp->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp->sockfd < 0)
    {
        ZOO_LOG_ERROR("[transport]socket failed\n");
        return INVALID_TRANSPORT_FD;
    }

    // Set broadcast permission
    int reuse_addr = 1;
    if (setsockopt(udp->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0)
    {
        ZOO_LOG_ERROR("[transport]setsockopt SO_REUSEADDR failed\n");
        close(udp->sockfd);
        return INVALID_TRANSPORT_FD;
    }

    int broadcast_enable = 1;
    // Set broadcast option
    if (setsockopt(udp->sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0)
    {
        ZOO_LOG_ERROR("[transport]setsockopt SO_BROADCAST failed\n");
        close(udp->sockfd);
        return INVALID_TRANSPORT_FD;
    }

    // Set address
    memset(&udp->addr, 0, sizeof(udp->addr));
    udp->addr.sin_family = AF_INET;
    udp->addr.sin_port = htons(config->port);

    // Allow receiving broadcast messages from all addresses
    const char* ip = config->address;

    if (strcmp(ip, "0.0.0.0") == 0)
    {
        udp->addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if (inet_pton(AF_INET, ip, &udp->addr.sin_addr) <= 0)
        {
            ZOO_LOG_ERROR("[transport]inet_pton Invalid address/ Address not supported\n");
            close(udp->sockfd);
            return INVALID_TRANSPORT_FD;
        }
    }

    ZOO_LOG_DEBUG("[transport]udp addr:port %s:%d\n", inet_ntoa(udp->addr.sin_addr), config->port);

    // Bind socket
    if (bind(udp->sockfd, (struct sockaddr*)&udp->addr, sizeof(udp->addr)) < 0)
    {
        ZOO_LOG_ERROR("[transport]bind failed\n");
        close(udp->sockfd);
        return INVALID_TRANSPORT_FD;
    }

    return 0;
}

/**
 * @brief Initialize UDP broadcast transport.
 * @param transport Pointer to ZOO_SMB_TRANSPORT.
 * @return ZOO_SMB_OK on success, error code otherwise.
 */
static ZOO_ERROR_TYPE udp_broadcast_init(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (!transport || !transport->config)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    UDP_BROADCAST_TRANSPORT_STRUCT* udp = (UDP_BROADCAST_TRANSPORT_STRUCT*)zoo_allocate_from_pool(sizeof(UDP_BROADCAST_TRANSPORT_STRUCT));
    if (!udp)
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;

    memset(udp, 0, sizeof(UDP_BROADCAST_TRANSPORT_STRUCT));
    udp->data_observers = transport->data_observers;
    udp->data_observers_lock = &transport->data_observers_lock;
    udp->running = ZOO_FALSE;

    if (create_and_configure_socket(udp, transport->config) == INVALID_TRANSPORT_FD)
    {
        ZOO_LOG_DEBUG("[transport]Failed to create and configure socket\n");
        zoo_free_to_pool(udp);
        return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
    }

    transport->impl = udp;
    memset(&udp->broadcast_addr, 0, sizeof(udp->broadcast_addr));
    udp->broadcast_addr.sin_family = AF_INET;
    udp->broadcast_addr.sin_port = htons(transport->config->port);
    if (inet_pton(AF_INET, transport->config->address, &udp->broadcast_addr.sin_addr) <= 0)
    {
        ZOO_LOG_DEBUG("[transport]Invalid address/ Address not supported\n");
        close(udp->sockfd);
        zoo_free_to_pool(udp);
        return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
    }

    ZOO_LOG_INFO(
        "sockfd=%d, addr=%s:%d, broadcast_addr=%s:%d, running=%d, data_observers=%p, user_data=%p, config=%p",
        udp->sockfd,
        inet_ntoa(udp->addr.sin_addr),
        ntohs(udp->addr.sin_port),
        inet_ntoa(udp->broadcast_addr.sin_addr),
        ntohs(udp->broadcast_addr.sin_port),
        udp->running,
        udp->data_observers,
        udp->user_data,
        udp->config);

    return ZOO_SMB_OK;
}

/**
 * @brief Destroy UDP broadcast transport.
 * @param impl UDP transport instance.
 */
static void udp_broadcast_destroy(void* impl)
{
    if (!impl)
        return;

    UDP_BROADCAST_TRANSPORT_STRUCT* udp = (UDP_BROADCAST_TRANSPORT_STRUCT*)impl;
    udp->running = ZOO_FALSE;
    close(udp->sockfd);

    zoo_free_to_pool(udp);
}

/**
 * @brief Start UDP broadcast transport (start receive thread).
 * @param impl UDP transport instance.
 * @return ZOO_SMB_OK on success, error code otherwise.
 */
static ZOO_ERROR_TYPE udp_broadcast_start(void* impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    UDP_BROADCAST_TRANSPORT_STRUCT* udp = (UDP_BROADCAST_TRANSPORT_STRUCT*)impl;
    udp->running = ZOO_TRUE;
    ZOO_ERROR_TYPE res = ZOO_SMB_OK;
    udp_broadcast_receive_thread(impl);
    return res;
}

/**
 * @brief Stop UDP broadcast transport (stop receive thread).
 * @param impl UDP transport instance.
 * @return ZOO_SMB_OK on success, error code otherwise.
 */
static ZOO_ERROR_TYPE udp_broadcast_stop(void* impl)
{
    if (!impl)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    UDP_BROADCAST_TRANSPORT_STRUCT* udp = (UDP_BROADCAST_TRANSPORT_STRUCT*)impl;
    udp->running = ZOO_FALSE;

    return ZOO_SMB_OK;
}

/**
 * @brief Send UDP broadcast message.
 * @param impl UDP transport instance.
 * @param msg Message buffer.
 * @param size Message size.
 * @param destination Destination address (ZOO_SMB_UNUSED for broadcast)
 * @return ZOO_SMB_OK on success, error code otherwise.
 */
static ZOO_ERROR_TYPE udp_broadcast_send(void* impl,
                                             const ZOO_SMB_MSG_STRUCT* msg,
                                             const char* receiver)  // Added destination parameter
{
    ZOO_SMB_UNUSED(receiver);  // ZOO_SMB_UNUSED for broadcast
    if (!impl || !msg)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    size_t total_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + msg->header.payload_size;
    if (total_size > MAX_TRANSPORT_BUFFER_SIZE)
    {
        ZOO_LOG_ERROR("udp_send: message payload size exceeds maximum buffer size\n");
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    uint8_t* msg_buffer = zoo_allocate_from_pool(total_size);
    if (!msg_buffer)
    {
        ZOO_LOG_ERROR("udp_send: failed to allocate memory for message buffer\n");
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    size_t msg_size = zoo_smb_protocol_serialize(msg, msg_buffer, total_size);
    if (msg_size == 0)
    {
        ZOO_LOG_ERROR("udp_send: failed to serialize message\n");
        zoo_free_to_pool(msg_buffer);
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    UDP_BROADCAST_TRANSPORT_STRUCT* udp = (UDP_BROADCAST_TRANSPORT_STRUCT*)impl;

    // Send message
    ssize_t bytes_sent = sendto(
        udp->sockfd, msg_buffer, msg_size, 0, (struct sockaddr*)&udp->broadcast_addr, sizeof(udp->broadcast_addr));

    ZOO_LOG_TRACE("udp_broadcast_send to %s ,service_name:%s, send size:%zd\n", receiver == NULL ? "all" : receiver, msg->header.sender, bytes_sent);
    if (bytes_sent != (ssize_t)msg_size)
    {
        ZOO_LOG_ERROR("udp_send: sendto failed, sent %zd bytes, expected %zu bytes, error: %s\n",
                          bytes_sent,
                          msg_size,
                          strerror(errno));
        zoo_free_to_pool(msg_buffer);
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    zoo_free_to_pool(msg_buffer);
    return ZOO_SMB_OK;
}

/**
 * @brief Checks if the UDP broadcast transport has been started.
 *
 * @param impl Pointer to the UDP broadcast transport implementation.
 * @return ZOO_TRUE if the UDP broadcast is started, ZOO_FALSE otherwise.
 */
ZOO_BOOL udp_broadcast_is_started(void* impl)
{
    if (!impl)
        return ZOO_FALSE;

    UDP_BROADCAST_TRANSPORT_STRUCT* udp = (UDP_BROADCAST_TRANSPORT_STRUCT*)impl;
    return udp->running;
}

/**
 * @brief UDP broadcast transport operations table.
 */
static ZOO_SMB_TRANSPORT_OPS_STRUCT udp_broadcast_ops = {
    .init = udp_broadcast_init,
    .destroy = udp_broadcast_destroy,
    .start = udp_broadcast_start,
    .stop = udp_broadcast_stop,
    .send = udp_broadcast_send,
    .is_started = udp_broadcast_is_started};

/**
 * @brief Register UDP broadcast transport to the SMB transport layer.
 */
void zoo_smb_transport_udp_broadcast_init(void)
{
    ZOO_LOG_INFO("Registering UDP broadcast transport\n");
    zoo_smb_transport_register(ZOO_SMB_TRANSPORT_TYPE_UDP_BROADCAST, &udp_broadcast_ops);
}