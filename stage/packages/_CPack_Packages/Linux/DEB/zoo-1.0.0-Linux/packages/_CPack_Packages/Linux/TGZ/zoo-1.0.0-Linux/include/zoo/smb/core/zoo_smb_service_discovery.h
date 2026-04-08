/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SERVICE_DISCOVERY
 * File name: zoo_smb_service_discovery.h
 * Description: Service discovery implementation for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun       created
 * 2.0       2025-05-25     weiwang.sun       optimized version
 ******************************************************************************/

#ifndef ZOO_SMB_SERVICE_DISCOVERY_H
#define ZOO_SMB_SERVICE_DISCOVERY_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo_smb_service_observer.h"
#include "zoo_smb_service_manager.h"
#include "zoo_smb_transport_manager.h"
#include "zoo_util.h"
#include "zoo_log.h"
#include "zoo_smb_transport.h"
#include "zoo_thread_pool.h"
#include "zoo_smb_protocol.h"
    /**
     * @defgroup ZOO_SMB_SERVICE_DISCOVERY Service Discovery Module
     * @brief Service discovery module for automatic service discovery and management in distributed systems
     * @{
     */

    /* Forward declaration of service discovery handle */
    typedef struct ZOO_SMB_SERVICE_DISCOVERY_STRUCT* ZOO_SMB_SERVICE_DISCOVERY_HANDLE;

    /**
     * @brief Create service discovery instance
     * @param service_name Service name to discover/advertise
     * @param address Multicast address, NULL for default address
     * @param port Service discovery port, 0 for default port
     * @param user_data User context data pointer
     * @param thread_pool Thread pool handle for async operations
     * @param memory_pool Memory pool handle for allocations
     * @return ZOO_SMB_SERVICE_DISCOVERY_HANDLE Service discovery handle, NULL on failure
     */
    ZOO_SMB_SERVICE_DISCOVERY_HANDLE zoo_smb_create_service_discovery(
        const ZOO_SMB_CONFIG_STRUCT* config,
        ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager,
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager);

    /**
     * @brief Destroy service discovery instance
     * @param discovery_handle Service discovery handle to destroy
     * @return void
     */
    void zoo_smb_destroy_service_discovery(ZOO_SMB_SERVICE_DISCOVERY_HANDLE discovery_handle);

    /**
     * @brief Start service discovery service
     * @param discovery_handle Service discovery handle
     * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_start_service_discovery(ZOO_SMB_SERVICE_DISCOVERY_HANDLE discovery_handle);

    /**
     * @brief Stop service discovery process
     * @param discovery_handle Service discovery handle
     * @return void
     */
    void zoo_smb_stop_service_discovery(ZOO_SMB_SERVICE_DISCOVERY_HANDLE discovery_handle);
#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_SERVICE_DISCOVERY_H */