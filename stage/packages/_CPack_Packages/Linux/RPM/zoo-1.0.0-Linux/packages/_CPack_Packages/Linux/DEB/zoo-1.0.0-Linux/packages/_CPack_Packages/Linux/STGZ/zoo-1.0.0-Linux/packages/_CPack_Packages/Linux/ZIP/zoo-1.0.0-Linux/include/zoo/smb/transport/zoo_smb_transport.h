/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT
 * File name: zoo_smb_transport.h
 * Description: Transport layer interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-16     congcong.qiu         created
 * 2.2       2025-05-20     congcong.qiu         modularized transport layers
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_H
#define ZOO_SMB_TRANSPORT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_util.h"
#include "zoo_smb_transport_observer.h"
#include "zoo_list.h"
#include <pthread.h>

#define INVALID_TRANSPORT_FD (-1)
#define MAX_TRANSPORT_BUFFER_SIZE (40 * 1024)
#define MAX_TRANSPORT_DATA_OBSERVER_SIZE 128
#define MAX_TRANSPORT_CONSUMER_OBSERVER_SIZE 128
#define MAX_TRANSPORT_CONSUMER_SIZE 1024
#define MAX_TRANSPORT_NAME_LENGTH 32
#define MAX_TRANSPORT_TOPIC_LENGTH 64
#define MAX_TRANSPORT_ADDRESS_LENGTH 64

    /**
     * @typedef ZOO_SMB_TRANSPORT_HANDLE
     * @brief Opaque handle to a ZOO SMB transport instance.
     *
     * This typedef defines a handle for managing and interacting with
     * ZOO SMB transport objects. The actual structure is hidden from
     * the user to provide encapsulation and abstraction.
     */
    typedef struct ZOO_SMB_TRANSPORT_STRUCT* ZOO_SMB_TRANSPORT_HANDLE;

    /**
     * @brief Configuration structure for SMB transport layer.
     *
     * This structure defines the configuration parameters required to initialize
     * and manage a transport connection for SMB (Server Message Block) communication.
     *
     * Members:
     * - type: Specifies the transport type (e.g., TCP, UDP, ZeroMQ).
     * - address: The IP address or hostname of the transport endpoint (null-terminated string, max 31 chars).
     * - port: The port number used for the transport connection.
     * - timeout_ms: Timeout value in milliseconds for send and receive operations.
     * - is_server: Indicates whether the configuration is for a server (ZOO_TRUE) or client (ZOO_FALSE).
     */
    typedef struct
    {
        ZOO_SMB_TRANSPORT_TYPE_ENUM type;           /**< Transport type (TCP, UDP, ZeroMQ, etc.) */
        char name[MAX_TRANSPORT_NAME_LENGTH];       /**< Name of the transport instance */
        char address[MAX_TRANSPORT_ADDRESS_LENGTH]; /**< Address of the transport instance */
        uint16_t port;                              /**< Port of the transport instance */
        char multicast_address[MAX_TRANSPORT_ADDRESS_LENGTH]; /**< Multicast address for UDP transports */
        uint16_t multicast_port;                    /**< Multicast port for UDP transports */
        char broadcast_address[MAX_TRANSPORT_ADDRESS_LENGTH]; /**< Broadcast address for UDP transports */
        uint16_t broadcast_port;                   /**< Broadcast port for UDP transports */
        uint32_t timeout_ms;                        /**< Timeout in milliseconds for send/recv operations */
        ZOO_BOOL is_server;                             /**< Indicates if the transport instance is a server */
        uint8_t retry_interval_s;                   /**< Retry interval in seconds for reconnection attempts */
        uint32_t max_send_attempts;                 /**< Maximum number of send attempts */
    } ZOO_SMB_TRANSPORT_CONFIG_STRUCT;

    // Connection states
    typedef enum
    {
        TRANSPORT_CONN_STATE_DISCONNECTED, /**< Connection is disconnected */
        TRANSPORT_CONN_STATE_CONNECTING,   /**< Connection attempt in progress */
        TRANSPORT_CONN_STATE_CONNECTED,    /**< Connection is established */
        TRANSPORT_CONN_STATE_RECONNECTING, /**< Reconnection attempt in progress */
        TRANSPORT_CONN_STATE_MAX           /**<  Invalid state */
    } TRANSPORT_CONNECTION_STATE_ENUM;

    /**
     * @brief Internal transport structure.
     */
    typedef struct ZOO_SMB_TRANSPORT_STRUCT
    {
        char name[MAX_TRANSPORT_NAME_LENGTH];
        TRANSPORT_CONNECTION_STATE_ENUM connection_state; /**< Current connection state */
        ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config;
        ZOO_LIST_HANDLE data_observers;  // Changed from node to linked list
        ZOO_MUTEX_T data_observers_lock; /**< Synchronization for data_observers list */
        void* impl;
    } ZOO_SMB_TRANSPORT_STRUCT;

    /**
     * @brief Create a transport layer instance.
     *
     * @param config Pointer to the transport configuration.
     * @param memory_pool Memory pool handle for allocation.
     * @return Handle to the created transport instance.
     */
    ZOO_SMB_TRANSPORT_HANDLE zoo_smb_create_transport(const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config);

    /**
     * @brief Destroy a transport layer instance.
     *
     * @param handle Handle to the transport instance to destroy.
     */
    void zoo_smb_destroy_transport(ZOO_SMB_TRANSPORT_HANDLE handle);

    /**
     * @brief Register a message receive callback.
     *
     * @param handle Transport handle.
     * @param observer Callback function to register.
     * @param user_data User data to pass to the callback.
     * @return Error code.
     */
    ZOO_ERROR_TYPE zoo_smb_transport_add_data_observer(
        ZOO_SMB_TRANSPORT_HANDLE handle,
        TRANSPORT_DATA_OBSERVER_HANDLE observer);

    /**
     * @Brief Removes a receive callback observer from the SMB transport handle.
     *
     * This function unregisters a previously registered receive callback observer
     * from the specified SMB transport handle. After removal, the observer will
     * no longer receive notifications when data is received on the transport.
     *
     * @param handle The SMB transport handle from which to remove the observer.
     *               Must be a valid handle obtained from zoo_smb_transport_create().
     * @param observer The receive callback function to remove. Must match a
     *                 previously registered observer callback.
     *
     * @return None
     *
     * @note If the observer was not previously registered, this function has no effect.
     * @note It is safe to call this function multiple times with the same observer.
     * @warning The handle parameter must be valid, otherwise behavior is undefined.
     */
    void zoo_smb_transport_remove_data_observer(ZOO_SMB_TRANSPORT_HANDLE handle, TRANSPORT_DATA_OBSERVER_HANDLE observer);

    /**
     * @brief Checks if the SMB transport has been started.
     *
     * This function determines whether the specified SMB transport handle
     * is currently in a started state.
     *
     * @param handle The handle to the SMB transport instance.
     * @return ZOO_TRUE if the transport is started, ZOO_FALSE otherwise.
     */
    ZOO_BOOL zoo_smb_transport_is_started(ZOO_SMB_TRANSPORT_HANDLE handle);

    /**
     * @brief Starts the SMB transport.
     *
     * Initializes and starts the SMB transport associated with the given handle.
     * If the background parameter is ZOO_TRUE, the transport will run in the background.
     *
     * @param handle The handle to the SMB transport instance.
     * @param background If ZOO_TRUE, starts the transport in the background; otherwise, runs in the foreground.
     * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_transport_start(ZOO_SMB_TRANSPORT_HANDLE handle, ZOO_BOOL background);

    /**
     * @brief Stop the transport layer.
     *
     * @param handle Transport handle.
     * @return Error code.
     */
    ZOO_ERROR_TYPE zoo_smb_transport_stop(ZOO_SMB_TRANSPORT_HANDLE handle);

    /**
     * @brief Send a message through the transport layer.
     *
     * @param handle Transport handle.
     * @param msg Pointer to the message data.
     * @param size Size of the message in bytes.
     * @param context Pointer to the context address.
     * @return Error code.
     */
    ZOO_ERROR_TYPE zoo_smb_transport_send(
        ZOO_SMB_TRANSPORT_HANDLE handle,
        const ZOO_SMB_MSG_STRUCT* msg,
        const char* receiver);

    /**
     * @brief Get the transport configuration.
     *
     * @param handle Transport handle.
     * @return Pointer to the transport configuration.
     */
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT* zoo_smb_transport_get_config(ZOO_SMB_TRANSPORT_HANDLE handle);

    /**
     * @brief Retrieves the name of the SMB transport.
     *
     * @return A constant pointer to a null-terminated string representing the name of the SMB transport.
     */
    const char* zoo_smb_transport_get_name(ZOO_SMB_TRANSPORT_HANDLE handle);

    /**
     * @struct ZOO_SMB_TRANSPORT_OPS_STRUCT
     * @brief Structure defining the operations for SMB transport.
     *
     * This structure contains function pointers and data members that
     * represent the operations and state required for SMB transport
     * functionality within the system.
     *
     * Detailed descriptions of each member should be provided above their
     * respective declarations within the structure definition.
     */
    typedef struct
    {
        ZOO_ERROR_TYPE (*init)(ZOO_SMB_TRANSPORT_STRUCT* transport);
        void (*destroy)(void* impl);
        ZOO_ERROR_TYPE (*start)(void* impl);
        ZOO_ERROR_TYPE (*stop)(void* impl);
        ZOO_ERROR_TYPE (*send)(void* impl, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver);
        ZOO_BOOL (*is_started)(void* impl);
    } ZOO_SMB_TRANSPORT_OPS_STRUCT;

    /**
     * @brief Register a transport implementation.
     *
     * @param type Transport type.
     * @param ops Pointer to the transport operations structure.
     */
    void zoo_smb_transport_register(ZOO_SMB_TRANSPORT_TYPE_ENUM type, ZOO_SMB_TRANSPORT_OPS_STRUCT* ops);

#ifdef __cplusplus
}
#endif
#endif /* ZOO_SMB_TRANSPORT_H */