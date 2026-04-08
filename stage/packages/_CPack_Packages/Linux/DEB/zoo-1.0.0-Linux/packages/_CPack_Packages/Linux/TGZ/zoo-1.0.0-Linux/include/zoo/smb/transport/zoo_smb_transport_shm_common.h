/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_SHM_COMMON
 * File name: zoo_smb_transport_shm_common.h
 * Description: Shared Memory Transport Common Definitions
 *              Defines common data structures, constants, and function
 *              declarations shared between SHM client and server components.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-23     weiwang.sun       Created
 * 2.0       2025-07-28     weiwang.sun       Enhanced registration system
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_SHM_COMMON_H
#define ZOO_SMB_TRANSPORT_SHM_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_transport.h"
#include "zoo_smb_ring_buffer.h"
#include "zoo_list.h"
#include <sys/types.h>
#include <time.h>
#include <signal.h>

    // ==============================================================================
    // COMMON CONSTANTS
    // ==============================================================================

#define ZOO_SMB_SHM_MAX_CLIENTS 1024               /**< Maximum number of clients */
#define ZOO_SMB_SHM_RING_BUFFER_SIZE (2 * 1024 * 1024) /**< Ring buffer size per client */
#define ZOO_SMB_SHM_HEARTBEAT_INTERVAL 5           /**< Heartbeat interval in seconds */
#define ZOO_SMB_SHM_CLIENT_TIMEOUT 15              /**< Client timeout in seconds */
#define ZOO_SMB_SHM_KEY_BASE 0x12345000            /**< Base key for shared memory */
#define ZOO_SMB_SHM_CLIENT_SIZE (ZOO_SMB_SHM_RING_BUFFER_SIZE * 2 + 4096)
#define ZOO_SMB_SHM_MAX_PENDING_REGISTRATIONS 16
#define ZOO_SMB_REGISTRATION_TIMEOUT 30  // seconds

    // ==============================================================================
    // SHARED MEMORY DATA STRUCTURES
    // ==============================================================================

    /**
     * @brief Client header structure stored in shared memory
     */
    typedef struct
    {
        uint32_t magic;                            /**< Magic number */
        uint32_t version;                          /**< Protocol version */
        char client_name[MAX_TRANSPORT_NAME_LENGTH]; /**< Client name */
        pid_t pid;                                 /**< Client process ID */
        TRANSPORT_CONNECTION_STATE_ENUM state;     /**< Connection state */
        time_t last_heartbeat;                     /**< Last heartbeat */
        time_t connect_time;                       /**< Connect time */
        uint32_t reserved[8];                      /**< Reserved for future use */
    } SHM_CLIENT_HEADER_STRUCT;

    /**
     * @brief Client information structure for runtime management
     */
    typedef struct
    {
        int client_id;                         /**< Unique client ID */
        char name[MAX_TRANSPORT_NAME_LENGTH];    /**< Client name */
        pid_t pid;                             /**< Client process ID */
        TRANSPORT_CONNECTION_STATE_ENUM state; /**< Client connection state */
        time_t last_heartbeat;                 /**< Last heartbeat timestamp */
        time_t connect_time;                   /**< Connection establishment time */

        // Shared memory resources
        key_t shm_key;   /**< Shared memory key */
        int shm_id;      /**< Shared memory ID */
        void* shm_addr;  /**< Shared memory address */
        size_t shm_size; /**< Shared memory size */

        // Ring buffers for bidirectional communication
        ZOO_SMB_RING_BUFFER_HANDLE tx_ring; /**< Transmit ring buffer (server->client) */
        ZOO_SMB_RING_BUFFER_HANDLE rx_ring; /**< Receive ring buffer (client->server) */

        // File descriptor for event notification
        int event_fd; /**< Event file descriptor */

    } SHM_CLIENT_INFO_STRUCT;

    /**
     * @brief Server shared memory header structure
     */
    typedef struct
    {
        uint32_t magic;                     /**< Magic number */
        uint32_t version;                   /**< Protocol version */
        uint32_t client_count;              /**< Current client count */
        uint32_t max_clients;               /**< Maximum clients */
        time_t server_start_time;           /**< Server start time */
        volatile ZOO_BOOL server_running;       /**< Server running flag */
        volatile ZOO_BOOL has_pending_messages; /**< Flag indicating pending messages */
        int server_event_fd;                /**< Server event file descriptor for notifications */

        // Client registration area (for new client requests)
        volatile int registration_lock; /**< Registration mutex */
        volatile int next_available_id; /**< Next available client ID */

        uint32_t reserved[16]; /**< Reserved for future use */

    } SHM_SERVER_HEADER_STRUCT;

    /**
     * @brief Client registration status enumeration
     */
    typedef enum
    {
        ZOO_SMB_REGISTRATION_EMPTY = 0, /**< Registration slot is empty */
        ZOO_SMB_REGISTRATION_PENDING,   /**< Registration request pending */
        ZOO_SMB_REGISTRATION_SUCCESS,   /**< Registration successful */
        ZOO_SMB_REGISTRATION_FAILED     /**< Registration failed */
    } ZOO_SMB_REGISTRATION_STATUS_ENUM;

    /**
     * @brief Client registration request structure
     */
    typedef struct
    {
        ZOO_SMB_REGISTRATION_STATUS_ENUM status;        /**< Registration status */
        char client_name[MAX_TRANSPORT_NAME_LENGTH]; /**< Client name */
        pid_t client_pid;                          /**< Client process ID */
        key_t client_shm_key;                      /**< Client shared memory key */
        size_t client_shm_size;                    /**< Client shared memory size */
        int client_event_fd;                       /**< Client event file descriptor */
        int assigned_client_id;                    /**< Assigned client ID (for success response) */
        ZOO_ERROR_TYPE error_code;             /**< Error code (for failed registration) */
        time_t timestamp;                          /**< Registration timestamp */
        uint8_t reserved[32];                      /**< Reserved for future use */
    } ZOO_SMB_SHM_CLIENT_REGISTRATION;

    /**
     * @brief Common implementation base structure
     */
    typedef struct
    {
        ZOO_SMB_TRANSPORT_STRUCT* transport; /**< Base transport pointer */
        ZOO_BOOL is_server;               /**< Server or client mode flag */
        ZOO_BOOL is_started;              /**< Started flag */
        volatile ZOO_BOOL should_stop;    /**< Stop signal flag */

        // Shared memory connection
        key_t master_shm_key;                     /**< Master shared memory key */
        int master_shm_id;                        /**< Master shared memory ID */
        SHM_SERVER_HEADER_STRUCT* server_header; /**< Server shared memory header */
    } ZOO_SMB_SHM_COMMON_IMPL;

    // ==============================================================================
    // COMMON FUNCTION DECLARATIONS
    // ==============================================================================

    /**
     * @brief Create client shared memory segment
     *
     * @param client Client information structure
     * @param reg_info Registration information
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_create_client_segment(SHM_CLIENT_INFO_STRUCT* client,
                                                 ZOO_SMB_SHM_CLIENT_REGISTRATION* reg_info);

    /**
     * @brief Initialize client resources (ring buffers, event fd)
     * @param client Client info structure
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_init_client_resources(SHM_CLIENT_INFO_STRUCT* client);

    /**
     * @brief Clean up client resources
     *
     * @param client Client information structure
     */
    void shm_cleanup_client_resources(SHM_CLIENT_INFO_STRUCT* client);

    /**
     * @brief Validate shared memory header basic fields
     * @param magic Magic number to validate
     * @param version Version number to validate
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_validate_header(uint32_t magic, uint32_t version);

    /**
     * @brief Validate complete server header structure
     * @param header Server header to validate
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_validate_server_header(const SHM_SERVER_HEADER_STRUCT* header);

    /**
     * @brief Validate client header structure
     * @param header Client header to validate
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_validate_client_header(const SHM_CLIENT_HEADER_STRUCT* header);

    /**
     * @brief Generate shared memory key based on name
     * @param name Name to generate key from
     * @param segment_id Key segment_id
     * @return Generated key
     */
    key_t shm_generate_key(const char* name, int segment_id);

    /**
     * @brief Check if process is still alive
     * @param pid Process ID to check
     * @return ZOO_TRUE if alive, ZOO_FALSE otherwise
     */
    ZOO_BOOL shm_is_process_alive(pid_t pid);

    /**
     * @brief Notify peer through event fd
     * @param event_fd Event file descriptor
     */
    void shm_notify_peer(int event_fd);

    /**
     * @brief Notify client through event fd
     * @param client Client info structure
     */
    void shm_notify_client(SHM_CLIENT_INFO_STRUCT* client);

    /**
     * @brief Request client registration with server
     * @param server_header Server header
     * @param client_name Client name
     * @param client_pid Client PID
     * @param client_shm_key Client shared memory key
     * @param client_event_fd Client event fd
     * @return Assigned client ID, or -1 on failure
     */
    int shm_request_client_registration(SHM_SERVER_HEADER_STRUCT* server_header,
                                        const char* client_name,
                                        pid_t client_pid,
                                        key_t client_shm_key,
                                        int client_event_fd);

    /**
     * @brief Create or attach to shared memory segment
     * @param key Shared memory key
     * @param size Segment size
     * @param flags Creation flags
     * @param shm_id Output shared memory ID
     * @param shm_addr Output shared memory address
     * @return Error code
     */
    ZOO_ERROR_TYPE shm_create_or_attach_segment(key_t key, size_t size, int flags, int* shm_id, void** shm_addr);

    /**
     * @brief Validates a client registration request
     *
     * @param server_header Pointer to the server header structure
     * @param client_name Client name to validate
     * @param client_pid Client process ID
     * @param client_shm_key Client shared memory key
     * @return ZOO_ERROR_TYPE Success or specific validation error
     */
    ZOO_ERROR_TYPE shm_validate_client_registration(const SHM_SERVER_HEADER_STRUCT* server_header,
                                                        const char* client_name,
                                                        pid_t client_pid,
                                                        key_t client_shm_key);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_SHM_COMMON_H */