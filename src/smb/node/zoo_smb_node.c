/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_NODE
 * File name: zoo_smb_node.c
 * Description: Implementation of node interface for ZOO Soft Message Bus (SMB)
 *              Provides APIs for node creation, destruction, lifecycle management,
 *              and user data access.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_node.h"
#include "zoo_list.h"
#include "zoo_log.h"
#include "zoo_smb_protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <pthread.h>

/**
 * @brief Compares the name of a node with a target value.
 *
 * This function is intended to be used as a comparison callback, for example in search or sort operations.
 *
 * @param data Pointer to the node data to compare.
 * @param target Pointer to the target value to compare against.
 * @return ZOO_TRUE if the node's name matches the target, ZOO_FALSE otherwise.
 */
static ZOO_BOOL node_compare_by_name(const void* data, const void* target)
{
    if (data == NULL || target == NULL)
    {
        return ZOO_FALSE;
    }
    const ZOO_SMB_NODE_STRUCT* node = (const ZOO_SMB_NODE_STRUCT*)data;
    const char* name = (const char*)target;

    return strcmp(node->name, name) == 0;
}

/**
 * @brief Create a new node on the SMB bus.
 * @param bus        Handle to the SMB instance.
 * @param name       Name of the node.
 * @param comm_type  Communication type for the node.
 * @return           Handle to the created node, or NULL on failure.
 */
ZOO_SMB_NODE_HANDLE zoo_smb_create_node(
    IN const char* name,
    IN const char* target,
    IN const char* topic,
    IN ZOO_SMB_NODE_TYPE_ENUM node_type,
    IN ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type)
{
    ZOO_SMB_NODE_STRUCT* node = (ZOO_SMB_NODE_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_NODE_STRUCT));
    if (node == NULL)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for node: %s", name);
        return NULL;
    }
    memset(node, 0, sizeof(ZOO_SMB_NODE_STRUCT));
    snprintf(node->name, sizeof(node->name), "%s", name);  // Ensure null-termination
    node->name[MAX_NODE_NAME_LENGTH - 1] = '\0';  // Ensure null-termination
    snprintf(node->topic, sizeof(node->topic), "%s", topic);  // Ensure null-termination
    node->topic[MAX_NODE_TOPIC_LENGTH - 1] = '\0';  // Ensure null-termination
    snprintf(node->target, sizeof(node->target), "%s", target);  // Ensure null-termination
    node->target[MAX_NODE_TARGET_LENGTH - 1] = '\0';  // Ensure null-termination
    node->node_type = node_type;
    node->transport_type = transport_type;

    node->observers = zoo_list_create(MAX_NODE_OBSERVER_SIZE);
    if (!node->observers)
    {
        ZOO_LOG_ERROR("Failed to create observers list for node: %s", name);
        return NULL;
    }
    node->active = ZOO_FALSE;
    ZOO_LOG_INFO("Node created: %s, type: %d, target: %s, topic: %s", 
        node->name, node->node_type, node->target, node->topic);
    return (ZOO_SMB_NODE_HANDLE)node;
}

/**
 * @brief Destroys a SMB node and releases associated resources.
 *
 * This function deallocates any memory and resources associated with the specified
 * SMB node handle. After calling this function, the node handle should not be used.
 *
 * @param[in] node_handle The handle to the SMB node to be destroyed.
 */
void zoo_smb_destroy_node(IN ZOO_SMB_NODE_HANDLE node_handle)
{
    if (node_handle == NULL)
    {
        ZOO_LOG_ERROR("Observer is NULL");
        return;
    }

    zoo_smb_destroy_node_observer_list(((ZOO_SMB_NODE_STRUCT*)node_handle)->observers);
    zoo_free_to_pool(node_handle);
}

/**
 * @brief Finds a node in the SMB linked list matching the specified criteria.
 *
 * This function searches through the given SMB linked list for a node that matches
 * the specified node type, name, target, and topic.
 *
 * @param list      The handle to the SMB linked list to search.
 * @param node_type The type of node to search for.
 * @param name      The name of the node to match (can be NULL to ignore).
 * @param target    The target of the node to match (can be NULL to ignore).
 * @param topic     The topic of the node to match (can be NULL to ignore).
 *
 * @return ZOO_SMB_NODE_HANDLE to the found node, or NULL if no matching node is found.
 */
ZOO_SMB_NODE_HANDLE zoo_smb_find_node_by_type(IN ZOO_LIST_HANDLE list, IN ZOO_SMB_NODE_TYPE_ENUM node_type, IN const char* name, IN const char* target, IN const char* topic)
{
    for(size_t i = 0; i < zoo_list_size(list); ++i)
    {
        ZOO_SMB_NODE_HANDLE node = (ZOO_SMB_NODE_HANDLE)zoo_list_at(list, i);
        if (node == NULL)
        {
            continue;
        }

        ZOO_SMB_NODE_STRUCT* node_struct = (ZOO_SMB_NODE_STRUCT*)node;
        if ((node_type == ZOO_SMB_NODE_TYPE_MAX || node_struct->node_type == node_type) &&
            (name == NULL || strcmp(node_struct->name, name) == 0) &&
            (target == NULL || strcmp(node_struct->target, target) == 0) &&
            (topic == NULL || strcmp(node_struct->topic, topic) == 0))
        {
            return node;
        } 
    }
    return NULL;
}

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
ZOO_SMB_NODE_HANDLE zoo_smb_find_node(IN ZOO_LIST_HANDLE list, ZOO_SMB_NODE_HANDLE node)
{
    if (list == NULL || node == NULL)
    {
        return NULL;
    }

    // Use the zoo_list_find_if function to search for the node
    return (ZOO_SMB_NODE_HANDLE)zoo_list_find_if(list, node_compare_by_name, node);
}

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
const char* zoo_smb_node_get_name(IN ZOO_SMB_NODE_HANDLE node_handle)
{
    if (node_handle == NULL)
    {
        return NULL;
    }

    return ((ZOO_SMB_NODE_STRUCT*)node_handle)->name;
}