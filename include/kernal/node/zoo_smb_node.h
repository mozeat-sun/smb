/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_NODE
 * File name: zoo_smb_node.h
 * Description: Node interface for ZOO Soft Message Bus (SMB)
 *              Defines APIs for node creation, destruction, lifecycle management,
 *              and user data access.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     your.name         created
 ******************************************************************************/
#ifndef ZOO_SMB_NODE_H
#define ZOO_SMB_NODE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_util.h"
#include "zoo_smb_node_observer.h"
#include "zoo_memory_pool.h"
#define MAX_NODE_NAME_LENGTH 64
#define MAX_NODE_TOPIC_LENGTH 64
#define MAX_NODE_TARGET_LENGTH 64
#define MAX_NODE_OBSERVER_SIZE 1024
    /**
     * @typedef ZOO_SMB_NODE_HANDLE
     * @brief Opaque handle to a ZOO_SMB_NODE_STRUCT structure.
     *
     * This typedef defines a handle to a ZOO_SMB_NODE_STRUCT, allowing users to
     * reference SMB node structures without exposing their internal implementation.
     * It is typically used to provide type safety and encapsulation in the API.
     */
    typedef struct ZOO_SMB_NODE_STRUCT* ZOO_SMB_NODE_HANDLE;

    /**
     * @brief Structure representing a base node in the ZOO SMB system.
     *
     * This structure is used as the foundational element for nodes within the ZOO SMB core module.
     * It typically contains common fields and metadata required by all derived node types.
     *
     * Detailed field descriptions should be provided within the structure definition.
     */
    typedef struct ZOO_SMB_NODE_STRUCT
    {
        ZOO_BOOL active;                                 // Indicates if the node is currently active
        char name[MAX_NODE_NAME_LENGTH];             // Name of the client node
        char target[MAX_NODE_TARGET_LENGTH];         // Target address or identifier for the client
        char topic[MAX_NODE_TOPIC_LENGTH];           // Topic name to client
        ZOO_SMB_NODE_TYPE_ENUM node_type;            // Type of the node (e.g., client, server, publisher, subscriber)
        ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type;  // Transport type used by the node
        ZOO_LIST_HANDLE observers;             // List of observers associated with the node
        char * rule_name;
    } ZOO_SMB_NODE_STRUCT;

    /**
     * @brief Creates a node object with role and transport metadata.
     *
     * The created node is inactive until registration/association succeeds.
     *
     * @param name Node logical name.
     * @param target Target service or receiver identity.
     * @param topic Topic namespace used by the node.
     * @param node_type Node role (client/server/publisher/subscriber).
     * @param transport_type Transport backend selection.
     * @return Node handle on success; NULL on allocation or initialization failure.
     */
    ZOO_SMB_NODE_HANDLE zoo_smb_create_node(
        IN const char* name,
        IN const char* target,
        IN const char* topic,
        IN ZOO_SMB_NODE_TYPE_ENUM node_type,
        IN ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type);

    /**
     * @brief Destroys a SMB node and releases associated resources.
     *
     * This function deallocates any memory and resources associated with the specified
     * SMB node handle. After calling this function, the node handle should not be used.
     *
     * @param[in] node_handle The handle to the SMB node to be destroyed.
     */
    void zoo_smb_destroy_node(IN ZOO_SMB_NODE_HANDLE node_handle);

    /**
     * @brief Finds first node in list matching type and optional identity filters.
     *
     * Each of name, target, and topic may be NULL to disable that filter.
     *
     * @param list Node list to search.
     * @param node_type Required node type, or ZOO_SMB_NODE_TYPE_MAX for any type.
     * @param name Optional node name filter.
     * @param target Optional target filter.
     * @param topic Optional topic filter.
     * @return Matching node handle, or NULL when no node satisfies filters.
     */
    ZOO_SMB_NODE_HANDLE zoo_smb_find_node_by_type(IN ZOO_LIST_HANDLE list, IN ZOO_SMB_NODE_TYPE_ENUM node_type, IN const char* name, IN const char* target, IN const char* topic);

    /**
     * @brief Searches for a specific node within a linked list of SMB nodes.
     *
     * This function traverses the given linked list and attempts to find the node
     * that matches the specified node handle.
     *
     * @param list The handle to the head of the linked list of SMB nodes.
     * @param node The handle of the node to search for within the list.
     * @return The handle to the found node if it exists in the list; otherwise, returns NULL.
     */
    ZOO_SMB_NODE_HANDLE zoo_smb_find_node(IN ZOO_LIST_HANDLE list, ZOO_SMB_NODE_HANDLE node);

    /**
     * @brief Retrieves the name of the specified SMB node.
     *
     * This function returns a pointer to a constant character string representing
     * the name of the SMB node associated with the provided node handle.
     *
     * @param node_handle The handle to the SMB node whose name is to be retrieved.
     * @return A constant character pointer to the name of the SMB node.
     *         The returned pointer is valid as long as the node handle remains valid.
     */
    const char * zoo_smb_node_get_name(IN ZOO_SMB_NODE_HANDLE node_handle);
#ifdef __cplusplus
}
#endif
#endif /* ZOO_SMB_NODE_H */