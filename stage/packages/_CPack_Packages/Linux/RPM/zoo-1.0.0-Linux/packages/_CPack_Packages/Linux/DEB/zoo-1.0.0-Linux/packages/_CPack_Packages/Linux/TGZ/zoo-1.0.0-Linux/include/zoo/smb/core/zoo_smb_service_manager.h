/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SERVICE
 * File name: zoo_smb_service_manager.h
 * Description: Service information structure for ZOO SMB
 *              This structure holds basic service information such as name, topic,
 *              address, port, transport type, last seen timestamp, online status,
 *              and broadcast interval.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-18     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_SERVICE_MANAGER_H
#define ZOO_SMB_SERVICE_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_service_observer.h"
#include "zoo_smb_node.h"
#include "zoo_smb_config.h"
#include "zoo_list.h"
    typedef struct ZOO_SMB_SERVICE_MANAGER_STRUCT* ZOO_SMB_SERVICE_MANAGER_HANDLE;

    /**
     * @brief Creates and initializes a new SMB service manager instance.
     *
     * This function allocates and configures a service manager for SMB operations
     * using the provided configuration structure.
     *
     * @param config Pointer to a ZOO_SMB_CONFIG_STRUCT containing configuration
     *               parameters for the service manager. Must not be NULL.
     * @return A handle to the newly created ZOO_SMB_SERVICE_MANAGER_HANDLE on success,
     *         or NULL on failure.
     */
    ZOO_SMB_SERVICE_MANAGER_HANDLE zoo_smb_create_service_manager(const ZOO_SMB_CONFIG_STRUCT* config);

    /**
     * @brief Destroys the specified SMB service manager instance and releases associated resources.
     *
     * This function cleans up and deallocates any resources held by the given
     * ZOO_SMB_SERVICE_MANAGER_HANDLE. After calling this function, the handle
     * should not be used in any further operations.
     *
     * @param manager The handle to the SMB service manager to be destroyed.
     */
    void zoo_smb_destroy_service_manager(ZOO_SMB_SERVICE_MANAGER_HANDLE manager);

    /**
     * @brief Creates or initializes a new SMB service within the Zoo SMB Service Manager.
     *
     * This function is responsible for setting up a new service instance, configuring
     * necessary parameters, and registering it with the service manager. The exact
     * behavior and required parameters depend on the implementation details.
     *
    * @param manager Service manager handle.
    * @param node Node handle used to derive or create service metadata.
    * @return ZOO_SMB_SERVICE_HANDLE Created or matched service handle, NULL on failure.
     */
    ZOO_SMB_SERVICE_HANDLE zoo_smb_service_manager_make_service(
        ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
        ZOO_SMB_NODE_HANDLE node);

    /**
     * @brief Adds a service to the specified SMB service manager.
     *
     * This function registers a new service with the given service manager,
     * associating it with the specified service type.
     *
     * @param manager       Handle to the SMB service manager instance.
     * @param service_type  The type of the service to be added.
     * @param service       Handle to the service to be added.
     */
    void zoo_smb_service_manager_register_service(
        ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
        ZOO_SMB_SERVICE_TYPE_ENUM service_type,
        ZOO_SMB_SERVICE_HANDLE service);

    /**
     * @brief Unregisters a service from the SMB service manager.
     *
     * This function removes a previously registered service from the specified
     * SMB service manager instance. After unregistration, the service will no longer
     * be managed or accessible through the manager.
     *
     * @param manager       Handle to the SMB service manager instance.
     * @param service_type  Type of the service to unregister.
     * @param service       Handle to the service to be unregistered.
     */
    void zoo_smb_service_manager_unregister_service(
        ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
        ZOO_SMB_SERVICE_TYPE_ENUM service_type,
        ZOO_SMB_SERVICE_HANDLE service);

    /**
     * @brief Retrieves information about a specific SMB service.
     *
     * This function queries the service manager for information about the SMB service
     * identified by the given service name.
     *
     * @param manager Pointer to the service manager handle.
     * @param service_name Name of the SMB service to retrieve information for.
     * @return Pointer to a ZOO_SMB_SERVICE_STRUCT containing the service information,
     *         or nullptr if the service is not found or an error occurs.
     */
    ZOO_SMB_SERVICE_HANDLE zoo_smb_service_manager_get_service(
        ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
        const char* service_name,
        ZOO_SMB_SERVICE_TYPE_ENUM service_type);

    /**
     * @brief Retrieves a handle to a specified SMB service, waiting up to a given timeout if necessary.
     *
     * This function attempts to obtain a handle to the SMB service identified by `service_name`
     * from the specified service manager. If the service is not immediately available, the function
     * will wait for up to `timeout_ms` milliseconds for the service to become available.
     *
     * @param manager        Handle to the SMB service manager.
     * @param service_name   Name of the SMB service to retrieve.
     * @param timeout_ms     Maximum time to wait for the service, in milliseconds.
     * @return ZOO_SMB_SERVICE_HANDLE
     *         Handle to the requested SMB service, or NULL if the service could not be obtained
     *         within the specified timeout.
     */
    ZOO_SMB_SERVICE_HANDLE zoo_smb_service_manager_get_service_wait(
        ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
        const char* service_name,
        uint32_t timeout_ms);

    /**
     * @brief Retrieves the list of services managed by the specified service manager.
     *
     * @param manager Pointer to a ZOO_SMB_SERVICE_MANAGER_HANDLE representing the service manager instance.
     * @return ZOO_LIST_HANDLE Handle to a linked list containing the available services.
     *
     * @note The caller is responsible for managing the memory of the returned linked list.
     */
    ZOO_LIST_HANDLE zoo_smb_service_manager_get_service_list(
        ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
        ZOO_SMB_SERVICE_TYPE_ENUM service_type);

    /**
     * @brief Registers an observer with the SMB service manager.
     *
     * This function adds the specified observer to the given SMB service manager,
     * allowing the observer to receive notifications or updates about service events.
     *
     * @param manager  Handle to the SMB service manager instance.
     * @param observer Handle to the observer to be registered.
     */
    void zoo_smb_service_manager_register_observer(
        ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
        ZOO_SMB_SERVICE_OBSERVER_HANDLE observer);

    /**
     * @brief Unregisters an observer from the SMB service manager.
     *
     * This function removes a previously registered observer from the specified
     * SMB service manager instance. After unregistration, the observer will no
     * longer receive notifications or callbacks from the manager.
     *
     * @param manager  Handle to the SMB service manager from which the observer
     *                 should be unregistered.
     * @param observer Handle to the observer to be unregistered.
     */
    void zoo_smb_service_manager_unregister_observer(
        ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
        ZOO_SMB_SERVICE_OBSERVER_HANDLE observer);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_SERVICE_MANAGER_H */