
/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_MANAGER
 * File name: zoo_smb_transport_manager.h
 * Description: Transport manager interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-30     weiwang.sun         created
 ******************************************************************************/
#ifndef ZOO_SMB_TRANSPORT_MANAGER_H
#define ZOO_SMB_TRANSPORT_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include "zoo_smb_transport.h"
#include "zoo_smb_transport_observer.h"
#include "zoo_smb_service_manager.h"
#include "zoo_smb_service.h"
#include "zoo_smb_config.h"
#include "zoo_smb_error.h"

    typedef struct
    {
        uint64_t total_send_success;
        uint64_t total_send_failures;
        uint64_t backpressure_drops;
        uint64_t circuit_open_rejections;
        uint32_t in_flight_sends;
        uint32_t send_high_watermark;
        uint32_t send_low_watermark;
        ZOO_BOOL backpressure_active;
    } ZOO_SMB_TRANSPORT_MANAGER_METRICS_STRUCT;

    typedef struct
    {
        char transport_name[MAX_TRANSPORT_NAME_LENGTH];
        char address[MAX_TRANSPORT_ADDRESS_LENGTH];
        uint16_t port;
        int transport_type;
        uint64_t total_send_success;
        uint64_t total_send_failures;
        uint64_t backpressure_drops;
        uint64_t circuit_open_rejections;
        uint32_t in_flight_sends;
        uint32_t send_high_watermark;
        uint32_t send_low_watermark;
        ZOO_BOOL backpressure_active;
        ZOO_BOOL circuit_open;
    } ZOO_SMB_TRANSPORT_CHANNEL_METRICS_STRUCT;

    typedef struct ZOO_SMB_TRANSPORT_MANAGER_STRUCT* ZOO_SMB_TRANSPORT_MANAGER_HANDLE;

    /**
     * @brief Creates a new transport manager instance.
     *
     * This function initializes and returns a handle to a transport manager
     * that can be used to manage various transport protocols within the ZOO SMB framework.
     *
     * @param global_config Pointer to the configuration structure for the transport manager.
     * @return A handle to the created transport manager, or NULL on failure.
     */
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE zoo_smb_create_transport_manager(ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager,
                                                                      const ZOO_SMB_CONFIG_STRUCT* global_config);

    /**
     * @brief Destroys the specified SMB transport manager and releases all associated resources.
     *
     * This function should be called when the transport manager is no longer needed.
     * After calling this function, the manager handle becomes invalid and must not be used.
     *
     * @param manager The handle to the SMB transport manager to destroy.
     */
    void zoo_smb_destroy_transport_manager(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager);

    /**
     * @brief Retrieves a transport handle for the specified service.
     *
     * This function obtains a transport handle from the given transport manager
     * based on the provided service information.
     *
     * @param manager        Handle to the transport manager instance.
     * @param service_info   Pointer to a structure containing information about the service
     *                      for which the transport handle is requested.
     * @return ZOO_SMB_TRANSPORT_HANDLE
     *         A handle to the transport associated with the specified service.
     *         Returns NULL if the transport could not be found or created.
     */
    ZOO_SMB_TRANSPORT_HANDLE zoo_smb_transport_manager_get_broadcast_transport(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager);
        
    /**
     * @brief Creates and initializes a new SMB transport instance.
     *
     * This function is responsible for setting up a new transport mechanism
     * for SMB (Server Message Block) communication. It typically allocates
     * necessary resources and configures the transport according to the
     * specified parameters.
     *
    * @param manager Transport manager handle.
    * @param service Service descriptor used to build or fetch transport.
    * @return ZOO_SMB_TRANSPORT_HANDLE Transport handle, or NULL on failure.
     */
    ZOO_SMB_TRANSPORT_HANDLE zoo_smb_transport_manager_make_transport(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
        ZOO_SMB_SERVICE_HANDLE service);

    /**
     * @brief Starts the SMB transport using the transport manager.
     *
     * This function initializes and starts the SMB transport layer, allowing
     * communication over the SMB protocol. It should be called before attempting
     * any SMB operations that require network transport.
     *
     * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
     *         Possible values include success or specific error types defined in ZOO_ERROR_TYPE.
     */
    ZOO_ERROR_TYPE zoo_smb_transport_manager_start_transport(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
        ZOO_SMB_TRANSPORT_HANDLE transport);

    /**
     * @brief Registers an observer for incoming SMB transport data.
     *
     * This function allows a client to register a callback or observer that will be notified
     * whenever new data is received by the SMB transport manager.
     *
    * @param manager Transport manager handle.
    * @param transport Transport handle to subscribe on.
    * @param observer Incoming data observer handle.
     * @return ZOO_ERROR_TYPE indicating the result of the registration operation.
     */
    ZOO_ERROR_TYPE zoo_smb_transport_manager_register_incoming_data_observer(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
        ZOO_SMB_TRANSPORT_HANDLE transport,
        TRANSPORT_DATA_OBSERVER_HANDLE observer);

    /**
     * @brief Unregisters an observer for incoming data events in the SMB transport manager.
     *
     * This function removes a previously registered observer that was receiving
     * notifications or callbacks when new data arrives via the SMB transport.
     *
     * @param observer The observer instance to unregister.
     *
     * @note After calling this function, the specified observer will no longer
     *       receive incoming data notifications.
     */
    void zoo_smb_transport_manager_unregister_incoming_data_observer(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
        ZOO_SMB_TRANSPORT_HANDLE transport,
        TRANSPORT_DATA_OBSERVER_HANDLE observer);

    /**
     * @brief Sends a message using the SMB transport manager.
     *
     * This function is responsible for transmitting a message through the SMB transport layer.
     *
        * @param manager Transport manager handle.
        * @param transport Transport handle used for sending.
        * @param message Message to send.
        * @param receiver Optional receiver identifier for point-to-point sends.
     * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_transport_manager_send_message(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
        ZOO_SMB_TRANSPORT_HANDLE transport,
        const ZOO_SMB_MSG_STRUCT* message,
        const char* receiver);

    /**
     * @brief Broadcasts a message using the SMB transport manager.
     *
     * This function sends a broadcast message to all connected SMB clients.
     *
     * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the broadcast operation.
     */
    ZOO_ERROR_TYPE zoo_smb_transport_manager_broadcast_message(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
        const ZOO_SMB_SERVICE_HANDLE service,
        const ZOO_SMB_MSG_STRUCT* message);

    ZOO_ERROR_TYPE zoo_smb_transport_manager_get_metrics(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
        ZOO_SMB_TRANSPORT_MANAGER_METRICS_STRUCT* out_metrics);

    ZOO_ERROR_TYPE zoo_smb_transport_manager_reset_metrics(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager);

    ZOO_ERROR_TYPE zoo_smb_transport_manager_get_transport_metrics(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
        ZOO_SMB_TRANSPORT_HANDLE transport,
        ZOO_SMB_TRANSPORT_CHANNEL_METRICS_STRUCT* out_metrics);

    ZOO_ERROR_TYPE zoo_smb_transport_manager_set_transport_watermarks(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
        ZOO_SMB_TRANSPORT_HANDLE transport,
        uint32_t high_watermark,
        uint32_t low_watermark);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_MANAGER_H */
