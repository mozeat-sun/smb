/**
 * @file zoo_smb_transport_udp_server.h
 * @brief UDP server transport implementation for ZOO SMB messaging system
 * @details This module provides UDP server transport functionality including:
 *          - UDP server socket management and binding
 *          - Multiple client connection handling
 *          - Multicast/broadcast message distribution
 *          - Asynchronous message receiving with epoll
 * @author ZOO SMB Development Team
 * @date 2025
 * @version 1.0
 * @copyright Copyright (c) 2025 ZOO SMB Project
 */

#ifndef ZOO_SMB_TRANSPORT_UDP_SERVER_H
#define ZOO_SMB_TRANSPORT_UDP_SERVER_H

#include "zoo_smb_transport_udp_common.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief UDP server transport structure
     */
    typedef struct
    {
        ZOO_SMB_UDP_TRANSPORT_COMMON common; /**< Common UDP transport fields */
    } ZOO_SMB_UDP_SERVER_TRANSPORT;

    /**
     * @brief Initialize UDP server transport (unicast only)
     * @param transport Pointer to transport structure to initialize
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE udp_server_init(ZOO_SMB_TRANSPORT_STRUCT* transport);

    /**
     * @brief Destroy UDP server transport and release all resources
     * @param impl Pointer to UDP server transport implementation
     */
    void udp_server_destroy(void* impl);

    /**
     * @brief Start UDP server transport operations
     * @param impl Pointer to UDP server transport implementation
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE udp_server_start(void* impl);

    /**
     * @brief Stop UDP server transport operations
     * @param impl Pointer to UDP server transport implementation
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE udp_server_stop(void* impl);

    /**
     * @brief Send message via UDP server to client (unicast reply)
     * @param impl Pointer to UDP server transport implementation
     * @param msg Message structure to send
     * @param receiver Receiver identifier (client name)
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE udp_server_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver);

    /**
     * @brief Check if UDP server transport is started and running
     * @param impl Pointer to UDP server transport implementation
     * @return ZOO_TRUE if server is running, ZOO_FALSE otherwise
     */
    ZOO_BOOL udp_server_is_started(void* impl);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_UDP_SERVER_H */