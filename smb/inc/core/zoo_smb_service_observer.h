/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SERVICE
 * File name: zoo_smb_service_observer.h
 * Description: Observer interface for ZOO Soft Message Bus (SMB) service discovery
 *              This header defines the structures and types used for service discovery
 *              and observation in the ZOO SMB framework. It includes definitions for
 *              service discovery callbacks, service information structures, and observer
 *              structures that allow users to register for service discovery events.
 *              The observer can be used to monitor the availability and properties of services
 *              in the SMB network, enabling dynamic service management and interaction.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-18     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_SERVICE_OBSERVER_H
#define ZOO_SMB_SERVICE_OBSERVER_H


#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo_smb_service.h"
#include "zoo_smb_types.h"
#include "zoo_smb_error.h"
#include "../../buffer/inc/zoo_list.h"
#include <stdint.h>

#define MAX_SERVICE_OBSERVER_SIZE 64

    typedef struct ZOO_SMB_SERVICE_OBSERVER_STRUCT* ZOO_SMB_SERVICE_OBSERVER_HANDLE;

    /**
     * @brief Service discovery callback function type
     * @param service_info Pointer to discovered service information
     * @param user_data User data pointer
     * @return void
     */
    typedef void (*ZOO_SMB_ON_SERVICE_CHANGE_CB)(
        const ZOO_SMB_SERVICE_HANDLE service,
        void* user_data);

    /**
     * @brief Structure representing an SMB service observer.
     *
     * This structure holds a callback function and associated user data
     * for observing SMB service discovery events.
     */
    typedef struct ZOO_SMB_SERVICE_OBSERVER_STRUCT
    {
        ZOO_SMB_ON_SERVICE_CHANGE_CB handler_cb; /**< Discovery callback */
        void* user_data;                         /**< User data for callback */
    } ZOO_SMB_SERVICE_OBSERVER_STRUCT;

    /**
     * @brief Creates a new SMB service observer.
     *
     * This function allocates and initializes a new SMB service observer structure,
     * which can be used to monitor SMB service discovery events.
     *
     * @param name The name of the observer.
     * @param callback Callback function to be invoked on service discovery events.
     * @param user_data User-defined data to be passed to the callback.
     * @return Pointer to the newly created ZOO_SMB_SERVICE_OBSERVER_STRUCT, or NULL on failure.
     */
    ZOO_SMB_SERVICE_OBSERVER_HANDLE zoo_smb_create_service_observer(
        ZOO_SMB_ON_SERVICE_CHANGE_CB callback,
        void* user_data);

    /**
     * @brief Creates a new service observer and inserts it into the observer list.
     *
     * This function allocates and initializes a new ZOO_SMB_SERVICE_OBSERVER_STRUCT,
     * associates it with the specified observer list, and registers the provided
     * callback and user data for service discovery events.
     *
     * @param observer_list The handle to the observer list where the new observer will be inserted.
     * @param name The name to assign to the new service observer.
     * @param callback The callback function to be invoked on service discovery events.
     * @param user_data User-defined data to be passed to the callback function.
     * @return Pointer to the newly created ZOO_SMB_SERVICE_OBSERVER_STRUCT, or NULL on failure.
     */
    ZOO_SMB_SERVICE_OBSERVER_HANDLE zoo_smb_create_service_observer_and_insert(
        ZOO_LIST_HANDLE observer_list,
        ZOO_SMB_ON_SERVICE_CHANGE_CB callback,
        void* user_data);

    /**
     * @brief Finds a service observer in the given observer list that matches the specified callback.
     *
     * This function searches through the provided observer list and returns the handle
     * to the service observer whose callback matches the given callback function pointer.
     *
     * @param observer_list The handle to the list of registered service observers.
     * @param callback The callback function pointer to search for in the observer list.
     * @return ZOO_SMB_SERVICE_OBSERVER_HANDLE Handle to the found service observer, or NULL if not found.
     */
    ZOO_SMB_SERVICE_OBSERVER_HANDLE zoo_smb_find_service_observer(
        ZOO_LIST_HANDLE observer_list,
        ZOO_SMB_ON_SERVICE_CHANGE_CB callback);
    /**
     * @brief Registers a service observer in the observer list.
     *
     * @param observer_list The linked list handle for observer registry.
     * @param observer      The observer handle to register.
     * @return ZOO_ERROR_TYPE Error code indicating the result of the registration.
     */
    ZOO_ERROR_TYPE zoo_smb_register_service_observer(
        ZOO_LIST_HANDLE observer_list,
        ZOO_SMB_SERVICE_OBSERVER_HANDLE observer);

    /**
     * @brief Unregisters a service observer from the observer list.
     *
     * @param observer_list The linked list handle for observer registry.
     * @param observer      The observer handle to unregister.
     */
    void zoo_smb_unregister_service_observer(
        ZOO_LIST_HANDLE observer_list,
        ZOO_SMB_SERVICE_OBSERVER_HANDLE observer);

    /**
     * @brief Destroys and cleans up the specified SMB service observer.
     *
     * This function releases all resources associated with the given
     * ZOO_SMB_SERVICE_OBSERVER_HANDLE instance. After calling this function,
     * the observer pointer should not be used unless it is reinitialized.
     *
     * @param observer Pointer to the SMB service observer structure to destroy.
     */
    void zoo_smb_destroy_service_observer(ZOO_SMB_SERVICE_OBSERVER_HANDLE observer);

    /**
     * @brief Destroys and frees all resources associated with a list of SMB service observers.
     *
     * This function iterates through the provided observer list and releases any memory or resources
     * held by each observer, as well as the list container itself.
     *
     * @param observer_list The handle to the linked list of SMB service observers to be destroyed.
     */
    void zoo_smb_destroy_service_observer_list(ZOO_LIST_HANDLE observer_list);

    /**
     * @brief Destroys the specified SMB service observer and removes it from the observer list.
     *
     * This function releases all resources associated with the given observer and ensures
     * it is properly removed from any internal tracking structures.
     *
     * @param observer Pointer to the ZOO_SMB_SERVICE_OBSERVER_STRUCT to be destroyed and removed.
     */
    void zoo_smb_destroy_service_observer_and_remove(
        ZOO_SMB_SERVICE_OBSERVER_HANDLE observer);
#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_SERVICE_H */