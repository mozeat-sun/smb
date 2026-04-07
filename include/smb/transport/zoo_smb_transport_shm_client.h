/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_SHM_CLIENT
 * File name: zoo_smb_transport_shm_client.h
 * Description: Shared Memory Transport Client Interface
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_SHM_CLIENT_H
#define ZOO_SMB_TRANSPORT_SHM_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_transport_shm_common.h"
#include <sys/shm.h>

    // ==============================================================================
    // CLIENT SPECIFIC STRUCTURES
    // ==============================================================================

    /**
     * @brief SHM client implementation structure
     */
    typedef struct
    {
        ZOO_SMB_TRANSPORT_STRUCT* transport; /**< Base transport pointer */
        ZOO_BOOL is_server;               /**< Server or client mode flag (always ZOO_FALSE for client) */
        ZOO_BOOL is_started;              /**< Started flag */
        volatile ZOO_BOOL should_stop;    /**< Stop signal flag */

        // Server connection
        key_t master_shm_key;                     /**< Master shared memory key */
        int master_shm_id;                        /**< Master shared memory ID */
        SHM_SERVER_HEADER_STRUCT* server_header; /**< Server shared memory header */
        int server_event_fd;                      /**< Server event file descriptor */
        // Client info
        int client_id;                        /**< Own client ID */
        int client_shm_id;                    /**< Client shared memory ID */
        SHM_CLIENT_INFO_STRUCT* client_info; /**< Own client info */

    } ZOO_SMB_SHM_CLIENT_IMPL;

    // ==============================================================================
    // CLIENT FUNCTION DECLARATIONS
    // ==============================================================================

    /**
     * @brief Initialize SHM client transport
     * @param transport Transport structure to initialize
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_client_init(ZOO_SMB_TRANSPORT_STRUCT* transport);

    /**
     * @brief Destroy SHM client transport
     * @param impl_ptr Implementation pointer
     */
    void shm_client_destroy(void* impl_ptr);

    /**
     * @brief Start SHM client transport (blocking)
     * @param impl_ptr Implementation pointer
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_client_start(void* impl_ptr);

    /**
     * @brief Stop SHM client transport
     * @param impl_ptr Implementation pointer
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_client_stop(void* impl_ptr);

    /**
     * @brief Send message through SHM client transport
     * @param impl_ptr Implementation pointer
     * @param msg Message data
     * @param size Message size
     * @param consumer Target consumer (ZOO_SMB_UNUSED in client mode)
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_client_send(void* impl_ptr, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver);

    /**
     * @brief Check if SHM client transport is started
     * @param impl_ptr Implementation pointer
     * @return ZOO_TRUE if started, ZOO_FALSE otherwise
     */
    ZOO_BOOL shm_client_is_started(void* impl_ptr);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_SHM_CLIENT_H */