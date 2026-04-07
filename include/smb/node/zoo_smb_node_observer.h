/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: zoo_smb
 * File name: zoo_smb_node_observer.h
 * Description: Observer interface for ZOO Soft Message Bus (SMB)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_SMB_NODE_OBSERVER_H
#define ZOO_SMB_NODE_OBSERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "zoo_smb_error.h"
#include "zoo_smb_types.h"
#include "zoo_smb_protocol.h"
#define MAX_OBSERVER_NAME_LENGTH 64

    typedef struct ZOO_SMB_NODE_OBSERVER_STRUCT* ZOO_SMB_NODE_OBSERVER_HANDLE;

    /**
     * @brief Callback function type for handling SMB events.
     *
     * This function pointer type defines the signature for event handler callbacks
     * that process SMB (Server Message Block) messages.
     *
     * @param context      Pointer to user-defined context data passed to the handler.
     * @param header       Pointer to the SMB message header structure.
     * @param payload      Pointer to the message payload data.
     * @param payload_len  Length of the payload data in bytes.
     */
    typedef void (*ZOO_NODE_MSG_HANDLER)(IN void* context, IN const void* msg, IN size_t msg_len);

    /**
     * @brief Defines a handler structure for SMB observer events.
     *
        * This union stores the callback type used by the observer runtime. The callback
        * selected by the creator must match the dispatch path that invokes it.
     */
    typedef union
    {
        ZOO_SMB_MSG_HANDLER smb_msg_cb;    // Callback function to handle messages
        ZOO_NODE_MSG_HANDLER node_msg_cb;  // Handler for processing messages
    } ZOO_SMB_NODE_OBSERVER_HANDLER_UNION;

    /**
     * @brief Structure representing an observer in the ZOO SMB module.
     *
     * This struct is used to define the properties and behaviors of an observer
     * within the ZOO SMB (Server Message Block) core system. Observers are typically
     * used to monitor or react to specific events or changes within the SMB subsystem.
     *
    * Fields store observer identity, message matching keys, callback target, and user context.
     */
    typedef struct ZOO_SMB_NODE_OBSERVER_STRUCT
    {
        char name[MAX_OBSERVER_NAME_LENGTH];  // Name of the observer
        uint32_t id;                          // Unique identifier for the observer
        uint32_t msg_id;                      // Message ID associated with the observer
        ZOO_SMB_MSG_TYPE_ENUM msg_type;            // Type of message the observer is interested in
        ZOO_SMB_NODE_OBSERVER_HANDLER_UNION handler;
        void* usr_data;  // User-defined context passed to the message handler
    } ZOO_SMB_NODE_OBSERVER_STRUCT;

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
        OUT int32_t* sub_id);

    /**
     * @brief Creates a new node observer and inserts it into the specified linked list.
     *
     * This function allocates and initializes a node observer with the provided parameters,
     * then inserts it into the given linked list of observers.
     *
     * @param list      The handle to the linked list where the new observer will be inserted.
     * @param name      The name of the observer node.
     * @param msg_id    The message identifier associated with the observer.
     * @param msg_type  The type of message the observer will handle.
     * @param handler   The callback function to handle messages for this observer.
     * @param usr_data  User-defined data to be associated with the observer.
     * @return          A handle to the newly created node observer, or NULL on failure.
     */
    ZOO_SMB_NODE_OBSERVER_HANDLE zoo_smb_create_node_observer_and_insert(IN ZOO_LIST_HANDLE list,
                                                                         IN const char* name,
                                                                         IN uint32_t msg_id,
                                                                         IN ZOO_SMB_MSG_TYPE_ENUM msg_type,
                                                                         IN ZOO_NODE_MSG_HANDLER handler,
                                                                         IN void* usr_data,
                                                                         OUT int32_t* sub_id);

    /**
     * @brief Destroys a SMB observer instance and releases associated resources.
     *
     * This function should be called to properly clean up and free any resources
     * allocated for the specified SMB observer handle.
     *
     * @param observer The handle to the SMB observer to be destroyed.
     */
    void zoo_smb_destroy_node_observer(IN ZOO_SMB_NODE_OBSERVER_HANDLE observer);

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
        IN ZOO_SMB_NODE_OBSERVER_HANDLE observer);

    /**
     * @brief Destroys a linked list of SMB nodes.
     *
     * This function releases all resources associated with the given linked list of SMB nodes.
     *
     * @param list The handle to the linked list of SMB nodes to be destroyed.
     */
    void zoo_smb_destroy_node_observer_list(IN ZOO_LIST_HANDLE list);

    /**
     * @brief Finds a node observer by its handler.
     *
     * This function searches for and returns a node observer handle that matches the specified handler.
     *
     * @param handler The handler used to identify the node observer.
     * @return ZOO_SMB_NODE_OBSERVER_HANDLE The handle to the found node observer, or NULL if not found.
     */
    ZOO_SMB_NODE_OBSERVER_HANDLE zoo_smb_find_node_observer_by_handler(
        IN ZOO_LIST_HANDLE list,
        IN ZOO_NODE_MSG_HANDLER handler);

    /**
     * @brief Finds a node observer by its unique identifier.
     *
     * This function searches for and returns a handle to a node observer
     * that matches the specified ID.
     *
     * @param id The unique identifier of the node observer to find.
     * @return ZOO_SMB_NODE_OBSERVER_HANDLE Handle to the found node observer, or NULL if not found.
     */
    ZOO_SMB_NODE_OBSERVER_HANDLE zoo_smb_find_node_observer_by_id(
        IN ZOO_LIST_HANDLE list,
        IN uint32_t id);

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
        IN void* usr_data);

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
        IN uint32_t msg_id);

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
        IN ZOO_SMB_MSG_TYPE_ENUM msg_type);

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
        IN const char* name);

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
        IN uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_NODE_OBSERVER_H */