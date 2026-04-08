/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_SHM_SERVER
 * File name: zoo_smb_transport_shm_server.h
 * Description: Shared Memory Transport Server Interface
 *              Defines server-side APIs for managing client connections,
 *              message routing, sender mappings, and event handling
 *              in shared memory transport layer.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-23     weiwang.sun       Created
 * 2.0       2025-07-28     weiwang.sun       Added sender mapping structures
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_SHM_SERVER_H
#define ZOO_SMB_TRANSPORT_SHM_SERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_transport_shm_common.h"
#include <sys/select.h>

    // ==============================================================================
    // FORWARD DECLARATIONS
    // ==============================================================================

    typedef struct SHM_SENDER_MAPPING_STRUCT SHM_SENDER_MAPPING_STRUCT;
    typedef struct SHM_SERVER_IMPL_STRUCT SHM_SERVER_IMPL_STRUCT;

    // ==============================================================================
    // SERVER SPECIFIC STRUCTURES
    // ==============================================================================

    /**
     * @brief Sender to client mapping structure
     */
    struct SHM_SENDER_MAPPING_STRUCT
    {
        char sender_name[MAX_TRANSPORT_NAME_LENGTH]; /**< Logical sender name */
        SHM_CLIENT_INFO_STRUCT* client_info;         /**< Physical client info */
        time_t registration_time;                    /**< When this sender was registered */
        time_t last_message_time;                    /**< Last message from this sender */
        uint32_t message_count;                      /**< Total messages from this sender */
    };

    /**
     * @brief Shared memory server implementation structure
     */
    struct SHM_SERVER_IMPL_STRUCT
    {
        ZOO_SMB_SHM_COMMON_IMPL common;            /**< Common transport fields */
        ZOO_LIST_HANDLE client_list;         /**< List of connected clients */
        ZOO_LIST_HANDLE sender_mapping_list; /**< List of sender mappings */
        uint32_t next_client_id;                   /**< Next available client ID */
        int server_event_fd;                       /**< Server event file descriptor */

        // Event handling fields
        int max_fd;      /**< Maximum file descriptor for select */
        fd_set read_fds; /**< File descriptor set for reading */
    };

    // ==============================================================================
    // PUBLIC INTERFACE FUNCTIONS
    // ==============================================================================

    /**
     * @brief Initialize SHM server transport
     * @param transport Transport structure to initialize
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_server_init(ZOO_SMB_TRANSPORT_STRUCT* transport);

    /**
     * @brief Destroy SHM server transport
     * @param impl_ptr Implementation pointer
     */
    void shm_server_destroy(void* impl_ptr);

    /**
     * @brief Start SHM server transport (blocking)
     * @param impl_ptr Implementation pointer
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_server_start(void* impl_ptr);

    /**
     * @brief Stop SHM server transport
     * @param impl_ptr Implementation pointer
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_server_stop(void* impl_ptr);

    /**
     * @brief Send message through SHM server transport
     * @param impl_ptr Implementation pointer
     * @param msg Message data
     * @param receiver Target receiver (can be NULL for broadcast)
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_server_send(void* impl_ptr, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver);

    /**
     * @brief Check if SHM server transport is started
     * @param impl_ptr Implementation pointer
     * @return ZOO_TRUE if started, ZOO_FALSE otherwise
     */
    ZOO_BOOL shm_server_is_started(void* impl_ptr);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_SHM_SERVER_H */