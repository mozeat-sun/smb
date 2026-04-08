/**
 * @file zoo_smb_transport_udp_client.h
 * @brief UDP client transport implementation for ZOO SMB messaging system
 * @details This module provides UDP client transport functionality including:
 *          - UDP client socket management
 *          - Message sending to configured target
 *          - Asynchronous message receiving
 *          - Thread-safe operations
 * @author ZOO SMB Development Team
 * @date 2025
 * @version 1.0
 * @copyright Copyright (c) 2025 ZOO SMB Project
 */

#ifndef ZOO_SMB_TRANSPORT_UDP_CLIENT_H
#define ZOO_SMB_TRANSPORT_UDP_CLIENT_H

#include "zoo_smb_transport_udp_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief UDP client transport structure
     * @details Contains UDP client-specific data including common UDP transport
     *          fields and client receive thread management
     * @note In external thread management mode, receive_thread_id may not be used
     */
    typedef struct
    {
        ZOO_SMB_UDP_TRANSPORT_COMMON common; /**< Common UDP transport fields */
    } ZOO_SMB_UDP_CLIENT_TRANSPORT;

    /**
     * @brief Initialize UDP client transport with multicast/broadcast support
     * @details Initializes UDP client transport including socket creation, multicast group joining,
     *          and address binding. Does not create any internal threads.
     * @param[in] transport Pointer to transport structure to initialize
     * @return ZOO_SMB_OK on success, error code otherwise
     * @retval ZOO_SMB_OK Initialization successful
     * @retval ZOO_SMB_ERROR_INVALID_PARAM Invalid transport parameter
     * @retval ZOO_SMB_ERROR_OUT_OF_MEMORY Memory allocation failed
     * @retval ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED Socket or network configuration failed
     */
    ZOO_ERROR_TYPE udp_client_init(ZOO_SMB_TRANSPORT_STRUCT* transport);

    /**
     * @brief Destroy UDP client transport and free all allocated resources
     * @details Performs comprehensive cleanup including multicast group leaving,
     *          socket closure, and memory deallocation
     * @param[in] impl Pointer to UDP client transport implementation
     * @note Safe to call with NULL pointer
     */
    void udp_client_destroy(void* impl);

    /**
     * @brief Start UDP client transport operations without creating internal threads
     * @details Enables the client transport for message processing by setting the running flag.
     *          External caller is responsible for thread management.
     * @param[in] impl Pointer to UDP client transport implementation
     * @return ZOO_SMB_OK on success, error code otherwise
     * @retval ZOO_SMB_OK Started successfully
     * @retval ZOO_SMB_ERROR_INVALID_PARAM Invalid implementation parameter
     * @note This function does NOT create any internal threads
     * @note Call udp_client_receive_loop() or udp_client_receive_once() from external thread
     */
    ZOO_ERROR_TYPE udp_client_start(void* impl);

    /**
     * @brief Stop UDP client transport operations
     * @details Gracefully shuts down client operations by setting the running flag to ZOO_FALSE.
     *          External threads should detect this change and terminate gracefully.
     * @param[in] impl Pointer to UDP client transport implementation
     * @return ZOO_SMB_OK on success, error code otherwise
     * @retval ZOO_SMB_OK Stopped successfully
     * @retval ZOO_SMB_ERROR_INVALID_PARAM Invalid implementation parameter
     * @note This function does NOT wait for threads as they are managed externally
     */
    ZOO_ERROR_TYPE udp_client_stop(void* impl);

    /**
     * @brief Send message via UDP client to configured target server
     * @details Serializes and sends message to the configured target address.
     *          Supports both unicast to specific server and response to multicast messages.
     * @param[in] impl Pointer to UDP client transport implementation
     * @param[in] msg Message structure containing header and payload to send
     * @param[in] receiver Receiver identifier (may be ignored for client, sends to configured target)
     * @return ZOO_SMB_OK on success, error code otherwise
     * @retval ZOO_SMB_OK Message sent successfully
     * @retval ZOO_SMB_ERROR_INVALID_PARAM Invalid parameters
     * @retval ZOO_SMB_ERROR_OUT_OF_MEMORY Memory allocation failed
     * @retval ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED Network send operation failed
     */
    ZOO_ERROR_TYPE udp_client_send(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver);

    /**
     * @brief Check if UDP client transport is currently active and running
     * @details Returns the operational state of the UDP client transport,
     *          indicating whether it can process incoming and outgoing messages.
     * @param[in] impl Pointer to UDP client transport implementation
     * @return ZOO_TRUE if client is active and running, ZOO_FALSE otherwise
     * @note Returns ZOO_FALSE if impl is NULL
     * @note Thread-safe read operation
     */
    ZOO_BOOL udp_client_is_started(void* impl);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_UDP_CLIENT_H */