/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SERVICE_OBSERVER
 * File name: zoo_smb_service_observer.c
 * Description: Implementation of service observer management for ZOO SMB
 *              Provides creation, registration, notification, and destruction
 *              of SMB service observer structures.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-01     weiwang.sun       created
 ******************************************************************************/
#include "zoo_smb_service_observer.h"
#include "../../memory_pool/inc/zoo_memory_pool.h"
#include "../../log/inc/zoo_log.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Creates and initializes a new ZOO_SMB_SERVICE_OBSERVER_STRUCT instance.
 *
 * @param callback   The observer callback function.
 * @param user_data  User-defined data to be passed to the callback.
 * @return ZOO_SMB_SERVICE_OBSERVER_HANDLE Pointer to the newly created observer, or NULL on failure.
 */
ZOO_SMB_SERVICE_OBSERVER_HANDLE zoo_smb_create_service_observer(
    ZOO_SMB_ON_SERVICE_CHANGE_CB callback,
    void* user_data)
{
    if (!callback)
    {
        ZOO_LOG_ERROR("zoo_smb_create_service_observer: callback is NULL");
        return NULL;
    }
    ZOO_SMB_SERVICE_OBSERVER_HANDLE observer = (ZOO_SMB_SERVICE_OBSERVER_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_SERVICE_OBSERVER_STRUCT));
    if (!observer)
    {
        ZOO_LOG_ERROR("zoo_smb_create_service_observer: allocation failed");
        return NULL;
    }
    observer->handler_cb = callback;
    observer->user_data = user_data;
    ZOO_LOG_DEBUG("zoo_smb_create_service_observer: created observer %p (callback=%p, user_data=%p)", observer, callback, user_data);
    return observer;
}

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
    void* user_data)
{
    ZOO_SMB_SERVICE_OBSERVER_HANDLE observer = zoo_smb_create_service_observer(callback, user_data);
    if (!observer)
    {
        return NULL;
    }
    if (zoo_smb_register_service_observer(observer_list, observer) != ZOO_SMB_OK)
    {
        zoo_smb_destroy_service_observer(observer);
        return NULL;
    }
    return observer;
}

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
    ZOO_SMB_ON_SERVICE_CHANGE_CB callback)
{
    if (!observer_list || !callback)
    {
        ZOO_LOG_ERROR("zoo_smb_find_service_observer: invalid parameters");
        return NULL;
    }
    size_t size = zoo_list_size(observer_list);
    for (size_t i = 0; i < size; i++)
    {
        ZOO_SMB_SERVICE_OBSERVER_HANDLE observer = (ZOO_SMB_SERVICE_OBSERVER_HANDLE)zoo_list_at(observer_list, i);
        if (observer && observer->handler_cb == callback)
        {
            ZOO_LOG_DEBUG("zoo_smb_find_service_observer: found observer %p for callback %p", observer, callback);
            return observer;
        }
    }
    ZOO_LOG_DEBUG("zoo_smb_find_service_observer: no observer found for callback %p", callback);
    return NULL;
}

/**
 * @brief Registers a service observer in the observer list.
 *
 * @param observer_list The linked list handle for observer registry.
 * @param observer      The observer handle to register.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the registration.
 */
ZOO_ERROR_TYPE zoo_smb_register_service_observer(
    ZOO_LIST_HANDLE observer_list,
    ZOO_SMB_SERVICE_OBSERVER_HANDLE observer)
{
    if (!observer_list || !observer)
    {
        ZOO_LOG_ERROR("zoo_smb_register_service_observer: invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    if (zoo_list_push_back(observer_list, observer) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("zoo_smb_register_service_observer: failed to register observer");
        return ZOO_SMB_ERROR_OPERATION_FAILED;
    }
    ZOO_LOG_INFO("zoo_smb_register_service_observer: observer %p registered", observer);
    return ZOO_SMB_OK;
}

/**
 * @brief Unregisters a service observer from the observer list.
 *
 * @param observer_list The linked list handle for observer registry.
 * @param observer      The observer handle to unregister.
 */
void zoo_smb_unregister_service_observer(
    ZOO_LIST_HANDLE observer_list,
    ZOO_SMB_SERVICE_OBSERVER_HANDLE observer)
{
    if (!observer_list || !observer)
    {
        ZOO_LOG_ERROR("zoo_smb_unregister_service_observer: invalid parameters");
        return;
    }
    if (zoo_list_remove(observer_list, observer) == ZOO_SMB_OK)
    {
        ZOO_LOG_INFO("zoo_smb_unregister_service_observer: observer %p unregistered", observer);
    }
    else
    {
        ZOO_LOG_WARN("zoo_smb_unregister_service_observer: observer %p not found", observer);
    }
}

/**
 * @brief Destroys and frees resources associated with a service observer.
 *
 * @param observer Pointer to the service observer structure to be destroyed.
 */
void zoo_smb_destroy_service_observer(
    ZOO_SMB_SERVICE_OBSERVER_HANDLE observer)
{
    if (!observer)
    {
        ZOO_LOG_WARN("zoo_smb_destroy_service_observer: observer is NULL");
        return;
    }
    ZOO_LOG_DEBUG("zoo_smb_destroy_service_observer: destroying observer %p", observer);
    zoo_free_to_pool(observer);
}

/**
 * @brief Destroys and frees all resources associated with the given service observer linked list.
 *
 * This function iterates through the observer_list, releasing any memory and resources
 * held by each observer node, and finally frees the list itself.
 *
 * @param observer_list The handle to the head of the service observer linked list to be destroyed.
 */
void zoo_smb_destroy_service_observer_list(ZOO_LIST_HANDLE observer_list)
{
    ZOO_LOG_DEBUG("zoo_smb_destroy_service_observer_list: destroying observer list %p", observer_list);
    
    for (size_t i = 0; i < zoo_list_size(observer_list); i++)
    {
        ZOO_SMB_SERVICE_OBSERVER_HANDLE observer = (ZOO_SMB_SERVICE_OBSERVER_HANDLE)zoo_list_at(observer_list, i);
        if (observer)
        {
            zoo_smb_destroy_service_observer(observer);
        }
    }
    
    zoo_free_to_pool(observer_list);
    ZOO_LOG_DEBUG("zoo_smb_destroy_service_observer_list: destroyed observer list %p", observer_list);
}

/**
 * @brief Destroys the specified SMB service observer and removes it from the system.
 *
 * This function releases all resources associated with the given SMB service observer
 * handle and ensures it is properly removed from any internal tracking structures.
 *
 * @param observer The handle to the SMB service observer to be destroyed and removed.
 */
void zoo_smb_destroy_service_observer_and_remove(ZOO_SMB_SERVICE_OBSERVER_HANDLE observer)
{
    if (!observer)
    {
        ZOO_LOG_ERROR("zoo_smb_destroy_service_observer_and_remove: observer is NULL");
        return;
    }
    ZOO_LOG_DEBUG("zoo_smb_destroy_service_observer_and_remove: destroying and removing observer %p", observer);
    zoo_smb_destroy_service_observer(observer);
}
