/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_CONFIG
 * File name: zoo_smb_config.h
 * Description: Configuration interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_CONFIG_H
#define ZOO_SMB_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_types.h"

#define ZOO_SMB_MAX_QUEUE_SIZE 1024
#define ZOO_SMB_DEFAULT_MAX_TRANSPORT_BUFFER_SIZE (40U * 1024U)
#define ZOO_SMB_MEMORY_HIGH_WATERMARK_PCT_DEFAULT 85U
#define ZOO_SMB_MEMORY_LOW_WATERMARK_PCT_DEFAULT 70U
    typedef struct
    {
        char address[64];  /**< Local machine address for routing */
        uint16_t port_min; /**< Local machine port for routing */
        uint16_t port_max; /**< Local machine port for routing */
    } LOCAL_MACHINE_CONFIG_STRUCT;

    typedef struct
    {
        char address[64];              /**< Broadcast address */
        uint16_t port;                 /**< Broadcast port */
        uint32_t interval_ms;          /**< Broadcast interval in milliseconds */
        uint32_t max_check_timeout_ms; /**< Max check timeout in milliseconds */
    } BROADCAST_CONFIG_STRUCT;

    typedef struct
    {
        char address[64];   /**< Multicast address */
        uint16_t port;      /**< Multicast port */
        uint8_t ttl;        /**< Time To Live for multicast packets (1 = local segment) */
        char interface[16]; /**< Network interface IP for multicast (0.0.0.0 = default) */
        ZOO_BOOL loopback;      /**< Enable multicast loopback for local delivery */
    } MULTICAST_CONFIG_STRUCT;

    typedef struct
    {
        ZOO_BOOL enable_bridge;      /**< Enable bridge mode */
        char from_service[64];   /**< Service name to bridge from */
        char to_service[64];     /**< Service name to bridge to */
        char from_topic[64];     /**< Topic name to bridge from */
        char to_topic[64];       /**< Topic name to bridge to */
        int from_transport_type; /**< Transport type to bridge from */
        int to_transport_type;   /**< Transport type to bridge to */
    } BRIDGE_CONFIG_STRUCT;

    typedef struct
    {
        char file_path[64];      /**< Log file path */
        int level;               /**< Log level (see ZOO_LOG_LEVEL_ENUM) */
        int target;              /**< Log target (see ZOO_LOG_TARGET_ENUM) */
        size_t max_file_size;    /**< Max log file size (bytes) */
        size_t max_backup_files; /**< Max backup log files */
    } LOG_CONFIG_STRUCT;

    typedef struct
    {
        uint16_t default_thread_numbers;    /**< Number of threads in the thread pool */
        uint32_t max_thread_queue_size;     /**< Max size of the thread queue */
        uint32_t send_high_watermark;       /**< Max concurrent sends before backpressure rejects */
        uint32_t send_low_watermark;        /**< Backpressure release threshold for concurrent sends */
        uint32_t ingress_high_watermark;    /**< Max in-flight ingress messages before drops */
        uint32_t ingress_low_watermark;     /**< Ingress backpressure release threshold */
        ZOO_BOOL enable_deterministic_memory_profile; /**< Enable bounded-memory profile validation */
        uint32_t memory_high_watermark_pct; /**< Pool usage percentage considered high pressure */
        uint32_t memory_low_watermark_pct;  /**< Pool usage percentage considered recovered */
        ZOO_BOOL enable_transport_telemetry;/**< Enable transport manager telemetry counters */
        ZOO_BOOL enable_routing_telemetry;  /**< Enable routing ingress telemetry counters */
        ZOO_BOOL enforce_encrypted_messages;/**< Reject non-encrypted messages when enabled */
        ZOO_BOOL require_sender_identity;   /**< Reject messages with empty sender when enabled */
        uint32_t mem_pool_size;             /**< Reserved for future use */
        uint32_t max_queue_size;            /**< Reserved for future use */
        uint32_t max_node_size;             /**< Reserved for future use */
        uint32_t max_transport_buffer_size; /**< Maximum transport/message buffer size in bytes */
        int32_t max_timeout;                /**< Max timeout for operations in milliseconds */
        int msg_flag;                       /**< Message flags: compress/encrypt */
        ZOO_BOOL reuse_transport;               /**< Reuse transport connections */
        int default_transport_type;         /**< Default transport type (see ZOO_SMB_TRANSPORT_TYPE_ENUM) */
        int transport_auto_select;          /**< Transport auto-select flag */
    } SYS_CONFIG_STRUCT;

    /**
     * @brief Configuration structure for ZOO SMB
     */
    typedef struct
    {
        LOCAL_MACHINE_CONFIG_STRUCT local_machine; /**< Local machine configuration */
        BROADCAST_CONFIG_STRUCT broadcast;         /**< Broadcast configuration */
        MULTICAST_CONFIG_STRUCT multicast;         /**< Multicast configuration */
        LOG_CONFIG_STRUCT log;                     /**< Log configuration */
        BRIDGE_CONFIG_STRUCT bridge;               /**< Bridge configuration */
        SYS_CONFIG_STRUCT sys;                     /**< System configuration */
    } ZOO_SMB_CONFIG_STRUCT;

    /**
     * @brief Initialize a configuration structure with default values
     * @param config Pointer to the configuration structure to initialize
     */
    const ZOO_SMB_CONFIG_STRUCT* zoo_smb_config_init();

    /**
     * @brief Retrieves the current SMB configuration.
     *
     * This function returns a pointer to a constant ZOO_SMB_CONFIG_STRUCT
     * containing the current configuration settings for SMB.
     *
     * @return const ZOO_SMB_CONFIG_STRUCT* Pointer to the current SMB configuration structure.
     */
    const ZOO_SMB_CONFIG_STRUCT* zoo_smb_get_config();

    /**
     * @brief Returns the active SMB configuration without forcing initialization.
     * @return Initialized configuration pointer, or NULL if config init has not run yet.
     */
    const ZOO_SMB_CONFIG_STRUCT* zoo_smb_config_peek(void);

    /**
     * @brief Saves the SMB configuration to a file.
     *
     * @param config Pointer to the SMB configuration structure to be saved
     * @param file_path Path to the file where the configuration will be saved
     * @return ZOO_TRUE if the configuration was successfully saved
     * @return ZOO_FALSE if there was an error saving the configuration
     */
    ZOO_BOOL zoo_smb_config_save_to_file(const ZOO_SMB_CONFIG_STRUCT* config, const char* file_path);

    /**
     * @brief Check if logging system is initialized
     * @details Returns the initialization status of the logging system
     * @return ZOO_TRUE if logging system is initialized and ready to use, ZOO_FALSE otherwise
     * @note Thread-safe function
     * @note Can be called safely before log initialization
     */
    ZOO_BOOL zoo_log_is_initialized(void);
#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_CONFIG_H */