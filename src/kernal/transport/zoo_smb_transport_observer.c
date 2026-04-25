/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_OBSERVER
 * File name: zoo_smb_transport_observer.c
 * Description: Observer interface for ZOO Soft Message Bus (SMB)
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 ******************************************************************************/

#include "zoo_smb_transport_observer.h"
#include "zoo_memory_pool.h"
#include "zoo_log.h"
#include <stdlib.h>

/**
 * @brief Creates a new transport data observer context.
 *
 * This function allocates and initializes a context structure for observing
 * transport data events. The observer callback will be invoked with the provided
 * user data whenever relevant transport data events occur.
 *
 * @param observer   Callback function to be called on transport data events.
 * @param user_data  Pointer to user-defined data to be passed to the callback.
 * @return Pointer to the newly created TRANSPORT_DATA_OBSERVER_STRUCT, or
 *         NULL on failure.
 */
TRANSPORT_DATA_OBSERVER_HANDLE create_transport_data_observer(
    IN TRANSPORT_DATA_OBSERVER_CB handler,
    IN void* user_data)
{
    if (!handler)
    {
        ZOO_LOG_ERROR("handler is NULL");
        return NULL;
    }

    TRANSPORT_DATA_OBSERVER_STRUCT* observer =
        (TRANSPORT_DATA_OBSERVER_STRUCT*)zoo_allocate_from_pool(sizeof(TRANSPORT_DATA_OBSERVER_STRUCT));
    if (!observer)
    {
        ZOO_LOG_ERROR("allocation failed");
        return NULL;
    }

    observer->handler = handler;
    observer->user_data = user_data;
    ZOO_LOG_DEBUG("created observer %p (handler=%p, user_data=%p)", observer, handler, user_data);
    return observer;
}

/**
 * @brief Compares a data observer by its callback function.
 *
 * This function checks if the callback function associated with the given data observer
 * matches the target callback function.
 *
 * @param data Pointer to the data observer structure.
 * @param target Pointer to the target callback function to compare against.
 * @return ZOO_TRUE if the callback functions match, ZOO_FALSE otherwise.
 */
TRANSPORT_DATA_OBSERVER_HANDLE find_transport_data_observer(IN ZOO_LIST_HANDLE list, IN TRANSPORT_DATA_OBSERVER_CB handler)
{
    for (size_t i = 0; i < zoo_list_size(list); i++)
    {
        TRANSPORT_DATA_OBSERVER_HANDLE observer = (TRANSPORT_DATA_OBSERVER_HANDLE)zoo_list_at(list, i);
        if (observer->handler == handler)
            return observer;
    }
    return NULL;
}

/**
 * @brief Creates a new transport data observer context and inserts it into the given linked list.
 *
 * This function allocates and initializes a TRANSPORT_DATA_OBSERVER_STRUCT, associates it with the provided
 * observer callback and user data, and inserts it into the specified linked list of observer contexts.
 *
 * @param list        The linked list handle where the new observer context will be inserted.
 * @param handler    The callback function to be invoked when transport data events occur.
 * @param user_data   A pointer to user-defined data to be associated with the observer context.
 *
 * @return A pointer to the newly created TRANSPORT_DATA_OBSERVER_STRUCT on success, or NULL on failure.
 */
TRANSPORT_DATA_OBSERVER_HANDLE create_transport_data_observer_and_insert(IN ZOO_LIST_HANDLE list, IN TRANSPORT_DATA_OBSERVER_CB handler, IN void* user_data)
{
    if (!list)
    {
        ZOO_LOG_ERROR("create_transport_data_observer_observer_and_insert: list is NULL");
        return NULL;
    }

    TRANSPORT_DATA_OBSERVER_HANDLE observer = find_transport_data_observer(list, handler);
    if (observer)
    {
        observer->user_data = user_data;
        ZOO_LOG_WARN("create_transport_data_observer_observer_and_insert: Observer context already exists");
        return observer;
    }

    observer = create_transport_data_observer(handler, user_data);
    if (!observer)
    {
        ZOO_LOG_ERROR("create_transport_data_observer_observer_and_insert: Failed to create observer context");
        return NULL;
    }

    if (zoo_list_push_back(list, observer) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("create_transport_data_observer_observer_and_insert: Failed to insert observer context into list");
        zoo_free_to_pool(observer);
        return NULL;
    }
    return observer;
}

/**
 * @brief Destroys and cleans up the resources associated with a TRANSPORT_DATA_OBSERVER_STRUCT context.
 *
 * This function releases any memory and resources held by the specified transport data observer context.
 * After calling this function, the context pointer should not be used unless it is reinitialized.
 *
 * @param observer Pointer to the TRANSPORT_DATA_OBSERVER_STRUCT to be destroyed.
 */
void destroy_transport_data_observer(IN TRANSPORT_DATA_OBSERVER_HANDLE observer)
{
    if (!observer)
    {
        ZOO_LOG_WARN("destroy_transport_data_observer_observer: observer is NULL");
        return;
    }
    ZOO_LOG_DEBUG("destroy_transport_data_observer_observer: destroying observer %p", observer);
    zoo_free_to_pool(observer);
}

/**
 * @brief Destroys a TRANSPORT_DATA_OBSERVER_STRUCT context and removes it from the linked handle list.
 *
 * This function deallocates the resources associated with the specified transport data observer context
 * and removes it from the provided linked handle list.
 *
 * @param list The linked handle list from which the context should be removed.
 * @param observer Pointer to the TRANSPORT_DATA_OBSERVER_STRUCT to be destroyed and removed.
 */
void destroy_transport_data_observer_and_remove(IN ZOO_LIST_HANDLE list, IN TRANSPORT_DATA_OBSERVER_HANDLE observer)
{
    if (!list || !observer)
    {
        ZOO_LOG_ERROR("list or observer is NULL");
        return;
    }
    ZOO_LOG_DEBUG("destroying observer %p and removing from list %p", observer, list);

    // Remove the context from the linked list
    if (zoo_list_remove(list, observer) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("Failed to remove observer from list");
        return;
    }

    // Free the context memory
    zoo_free_to_pool(observer);
    ZOO_LOG_DEBUG("observer %p destroyed and removed from list %p", observer, list);
}

/**
 * @brief Removes a previously registered transport data observer.
 *
 * This function unregisters an observer that was monitoring transport data events.
 * After calling this function, the specified observer will no longer receive notifications
 * about transport data changes.
 *
 * @param observer Pointer to the observer instance to be removed.
 */
void remove_transport_data_observer(
    IN ZOO_LIST_HANDLE list,
    IN TRANSPORT_DATA_OBSERVER_HANDLE observer)
{
    if (!list || !observer)
    {
        ZOO_LOG_ERROR("remove_transport_data_observer: Invalid parameters");
        return;
    }

    ZOO_LOG_DEBUG("remove_transport_data_observer: Removing observer %p from list %p", observer, list);
    zoo_list_remove(list, observer);
    ZOO_LOG_INFO("remove_transport_data_observer: Observer %p removed successfully", observer);
}

/**
 * @brief Registers a new observer for transport data events.
 *
 * This function allows clients to add an observer that will be notified
 * when transport data events occur. The observer should implement the
 * appropriate callback interface to handle these events.
 *
 * @param observer Pointer to the observer instance to be added.
 */
void add_transport_data_observer(
    IN ZOO_LIST_HANDLE list,
    IN TRANSPORT_DATA_OBSERVER_HANDLE observer)
{
    if (!list || !observer)
    {
        ZOO_LOG_ERROR("add_transport_data_observer: Invalid parameters");
        return;
    }

    ZOO_LOG_DEBUG("add_transport_data_observer: Adding observer %p to list %p", observer, list);
    zoo_list_push_back(list, observer);
    ZOO_LOG_INFO("add_transport_data_observer: Observer %p added successfully", observer);
}
