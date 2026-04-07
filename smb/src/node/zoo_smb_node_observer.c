/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_NODE_OBSERVER
 * File name: zoo_smb_node_observer.c
 * Description: Implementation of node observer management for ZOO SMB
 *              Provides creation, configuration, and destruction of node observer entities.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-01     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_node_observer.h"
#include "../../memory_pool/inc/zoo_memory_pool.h"
#include "../../buffer/inc/zoo_list.h"
#include "../../log/inc/zoo_log.h"
#include "zoo_smb_node_observer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int32_t unique_id_counter = 1;  // Global counter for unique IDs

/**
 * @brief Creates a new SMB observer instance.
 *
 * This function initializes and returns a handle to a new SMB observer,
 * which can be used to monitor SMB-related events or activities.
 *
 * @return ZOO_SMB_NODE_OBSERVER_HANDLE Handle to the newly created SMB observer.
 *         Returns NULL if the observer could not be created.
 */
ZOO_SMB_NODE_OBSERVER_HANDLE zoo_smb_create_node_observer(
    IN const char* name,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_TYPE_ENUM msg_type,
    IN ZOO_NODE_MSG_HANDLER handler,
    IN void* usr_data,
    OUT int32_t* sub_id)
{
    if (!name || !handler || !sub_id)
    {
        ZOO_LOG_ERROR("name: %p, handler: %p, sub_id: %p", name, handler, sub_id);
        return NULL;
    }
    ZOO_SMB_NODE_OBSERVER_HANDLE observer = (ZOO_SMB_NODE_OBSERVER_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_NODE_OBSERVER_STRUCT));
    if (!observer)
    {
        ZOO_LOG_ERROR("allocation failed");
        return NULL;
    }
    memset(observer, 0, sizeof(ZOO_SMB_NODE_OBSERVER_STRUCT));
    snprintf(observer->name, sizeof(observer->name), "%s", name);
    observer->name[MAX_OBSERVER_NAME_LENGTH - 1] = '\0';  //
    observer->id = unique_id_counter++;                   // Assuming a function to generate unique IDs
    observer->msg_id = msg_id;
    observer->msg_type = msg_type;
    observer->handler.node_msg_cb = handler;
    observer->usr_data = usr_data;
    *sub_id = observer->id;  // Return the unique ID as the subscription ID
    ZOO_LOG_DEBUG("created observer '%s' id=%u msg_id=%u", observer->name, observer->id, msg_id);
    return observer;
}

/**
 * @brief Creates a new node observer and inserts it into the specified linked list.
 *
 * This function allocates and initializes a node observer with the provided parameters,
 * then inserts it into the given linked list of observers.
 *
 * @param list      The handle to the linked list where the new observer will be inserted.
 * @param name      The name of the observer node.
 * @param id        The unique identifier for the observer node.
 * @param msg_id    The message identifier associated with the observer.
 * @param msg_type  The type of message the observer will handle.
 * @param handler   The callback function to handle messages for this observer.
 * @param usr_data  User-defined data to be associated with the observer.
 * @return          A handle to the newly created node observer, or NULL on failure.
 */
ZOO_SMB_NODE_OBSERVER_HANDLE zoo_smb_create_node_observer_and_insert(
    IN ZOO_LIST_HANDLE list,
    IN const char* name,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_TYPE_ENUM msg_type,
    IN ZOO_NODE_MSG_HANDLER handler,
    IN void* usr_data,
    OUT int32_t* sub_id)
{
    if (!list)
    {
        ZOO_LOG_ERROR("list is NULL");
        return NULL;
    }
    ZOO_SMB_NODE_OBSERVER_HANDLE observer = zoo_smb_create_node_observer(name, msg_id, msg_type, handler, usr_data, sub_id);
    if (!observer)
    {
        ZOO_LOG_ERROR("observer creation failed");
        return NULL;
    }
    if (zoo_list_push_back(list, observer) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("failed to insert observer into list");
        zoo_free_to_pool(observer);
        return NULL;
    }
    ZOO_LOG_INFO("observer '%s' inserted", name);
    return observer;
}

/**
 * @brief Destroys a SMB observer instance and releases associated resources.
 *
 * This function should be called to properly clean up and free any resources
 * allocated for the specified SMB observer handle.
 *
 * @param observer The handle to the SMB observer to be destroyed.
 */
void zoo_smb_destroy_node_observer(IN ZOO_SMB_NODE_OBSERVER_HANDLE observer)
{
    if (!observer)
    {
        ZOO_LOG_WARN("observer is NULL");
        return;
    }
    ZOO_LOG_DEBUG("destroying observer '%s' id=%u", observer->name, observer->id);
    zoo_free_to_pool(observer);
}

/**
 * @brief Destroys the specified node observer and removes it from the observer list.
 *
 * This function is responsible for properly cleaning up and deallocating the resources
 * associated with a node observer, and ensuring it is removed from any internal data
 * structures or lists where it is registered.
 *
 * @param observer Pointer to the node observer to be destroyed and removed.
 */
void zoo_smb_destroy_node_observer_and_remove(
    IN ZOO_LIST_HANDLE list,
    IN ZOO_SMB_NODE_OBSERVER_HANDLE observer)
{
    if (!list || !observer)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return;
    }
    if (zoo_list_remove(list, observer) == ZOO_SMB_OK)
    {
        ZOO_LOG_INFO("observer '%s' removed from list", observer->name);
    }
    else
    {
        ZOO_LOG_WARN("observer '%s' not found in list", observer->name);
    }
    zoo_smb_destroy_node_observer(observer);
}

/**
 * @brief Destroys a linked list of node observers.
 *
 * This function releases all resources associated with the given
 * node observer linked list. After calling this function, the list
 * handle should not be used.
 *
 * @param[in] list The handle to the linked list of node observers to destroy.
 */
void zoo_smb_destroy_node_observer_list(IN ZOO_LIST_HANDLE list)
{
    if (!list)
    {
        ZOO_LOG_ERROR("list is NULL");
        return;
    }

    for (size_t i = 0; i < zoo_list_size(list); i++)
    {
        ZOO_SMB_NODE_OBSERVER_HANDLE observer = (ZOO_SMB_NODE_OBSERVER_HANDLE)zoo_list_at(list, i);
        if (observer)
        {
            zoo_smb_destroy_node_observer(observer);
        }
    }
    zoo_list_destroy(list);
}

/**
 * @brief Finds a node observer in the linked list by its message handler.
 *
 * This function searches through the provided linked list of node observers
 * and returns the observer handle that matches the specified message handler.
 *
 * @param list     The linked list of node observers to search.
 * @param handler  The message handler to match against the observers.
 * @return         The handle to the matching node observer, or NULL if not found.
 */
ZOO_SMB_NODE_OBSERVER_HANDLE zoo_smb_find_node_observer_by_handler(IN ZOO_LIST_HANDLE list, IN ZOO_NODE_MSG_HANDLER handler)
{
    if (!list || !handler)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return NULL;
    }

    for (size_t i = 0; i < zoo_list_size(list); i++)
    {
        ZOO_SMB_NODE_OBSERVER_HANDLE observer = (ZOO_SMB_NODE_OBSERVER_HANDLE)zoo_list_at(list, i);
        if (observer && observer->handler.node_msg_cb == handler)
        {
            ZOO_LOG_INFO("found observer '%s'", observer->name);
            return observer;
        }
    }

    ZOO_LOG_WARN("observer not found");
    return NULL;
}

/**
 * @brief Finds a node observer in the linked list by its unique identifier.
 *
 * This function searches through the provided linked list of node observers
 * and returns the handle to the observer that matches the specified ID.
 *
 * @param list The head of the linked list containing node observer handles.
 * @param id The unique identifier of the node observer to find.
 * @return ZOO_SMB_NODE_OBSERVER_HANDLE Handle to the found node observer, or NULL if not found.
 */
ZOO_SMB_NODE_OBSERVER_HANDLE zoo_smb_find_node_observer_by_id(IN ZOO_LIST_HANDLE list, IN uint32_t id)
{
    if (!list)
    {
        ZOO_LOG_ERROR("list is NULL");
        return NULL;
    }

    for (size_t i = 0; i < zoo_list_size(list); i++)
    {
        ZOO_SMB_NODE_OBSERVER_HANDLE observer = (ZOO_SMB_NODE_OBSERVER_HANDLE)zoo_list_at(list, i);
        if (observer && observer->id == id)
        {
            ZOO_LOG_INFO("found observer '%s' with id=%u", observer->name, id);
            return observer;
        }
    }

    ZOO_LOG_WARN("observer with id=%u not found", id);
    return NULL;
}

/**
 * @brief Sets the handler function for the SMB observer.
 *
 * This function allows you to specify a handler that will be called
 * when certain SMB observer events occur.
 *
 * @param handler The function pointer to the handler to be set.
 * @return ZOO_ERROR_TYPE indicating the result of the operation.
 */
ZOO_ERROR_TYPE zoo_smb_node_observer_set_handler(
    IN ZOO_SMB_NODE_OBSERVER_HANDLE observer,
    IN ZOO_NODE_MSG_HANDLER handler,
    IN void* usr_data)
{
    if (!observer || !handler)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    observer->handler.node_msg_cb = handler;
    observer->usr_data = usr_data;
    ZOO_LOG_DEBUG("handler set for observer '%s'", observer->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Sets the message ID for the SMB observer.
 *
 * This function assigns a specific message ID to the SMB observer, which can be used
 * for tracking or correlating SMB operations.
 *
 * @param msg_id The message ID to set for the observer.
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE zoo_smb_node_observer_set_msg_id(
    IN ZOO_SMB_NODE_OBSERVER_HANDLE observer,
    IN uint32_t msg_id)
{
    if (!observer)
    {
        ZOO_LOG_ERROR("observer is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    observer->msg_id = msg_id;
    ZOO_LOG_DEBUG("set msg_id=%u for observer '%s'", msg_id, observer->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Sets the message type for the SMB observer.
 *
 * This function configures the observer to handle a specific type of SMB message.
 *
 * @param msg_type The type of message to set for the observer.
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE zoo_smb_node_observer_set_msg_type(
    IN ZOO_SMB_NODE_OBSERVER_HANDLE observer,
    IN ZOO_SMB_MSG_TYPE_ENUM msg_type)
{
    if (!observer)
    {
        ZOO_LOG_ERROR("observer is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    observer->msg_type = msg_type;
    ZOO_LOG_DEBUG("set msg_type=%d for observer '%s'", msg_type, observer->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Sets the name for the SMB observer.
 *
 * This function assigns a new name to the specified SMB observer instance.
 *
 * @param observer Pointer to the SMB observer object.
 * @param name     The new name to set for the observer.
 * @return ZOO_ERROR_TYPE indicating success or the type of error encountered.
 */
ZOO_ERROR_TYPE zoo_smb_node_observer_set_name(
    IN ZOO_SMB_NODE_OBSERVER_HANDLE observer,
    IN const char* name)
{
    if (!observer || !name)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    snprintf(observer->name, sizeof(observer->name), "%s", name);  // Ensure null-termination
    observer->name[sizeof(observer->name) - 1] = '\0';             // Ensure null-termination
    ZOO_LOG_DEBUG("set name='%s'", observer->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Sets the observer ID for the SMB observer.
 *
 * This function assigns a unique identifier to the SMB observer instance.
 *
 * @param id The identifier to set for the observer.
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE zoo_smb_node_observer_set_id(
    IN ZOO_SMB_NODE_OBSERVER_HANDLE observer,
    IN uint32_t id)
{
    if (!observer)
    {
        ZOO_LOG_ERROR(" observer is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    observer->id = id;
    ZOO_LOG_DEBUG(" set id=%u for observer '%s'", id, observer->name);
    return ZOO_SMB_OK;
}
