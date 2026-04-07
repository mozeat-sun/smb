/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_CONSUMER
 * File name: zoo_smb_consumer.c
 * Description: Implementation of SMB consumer interface for ZOO Soft Message Bus
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_consumer.h"
#include "zoo_smb_error.h"
#include "../../memory_pool/inc/zoo_memory_pool.h"
#include "../../log/inc/zoo_log.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief Create a new SMB consumer handle.
 *
 * This function allocates and initializes a new SMB consumer structure,
 * which can be used to interact with the SMB transport layer.
 *
 * @param name           Name of the consumer.
 * @param fd             Unique identifier for the consumer.
 * @param addr           Pointer to the consumer's address.
 * @return ZOO_SMB_CONSUMER_HANDLE Handle to the newly created SMB consumer, or NULL on failure.
 */
ZOO_SMB_CONSUMER_HANDLE zoo_smb_create_consumer(
    const char* name,
    int fd,
    struct sockaddr_in* addr)
{
    if (!name || name[0] == '\0' || fd < 0 || !addr)
    {
        ZOO_LOG_ERROR("invalid create_consumer arguments");
        return NULL;
    }
    ZOO_SMB_CONSUMER_HANDLE consumer = (ZOO_SMB_CONSUMER_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_CONSUMER_STRUCT));
    if (!consumer)
    {
        ZOO_LOG_ERROR("allocation failed");
        return NULL;
    }
    memset(consumer, 0, sizeof(ZOO_SMB_CONSUMER_STRUCT));
    snprintf(consumer->name, sizeof(consumer->name), "%s", name);
    consumer->name[MAX_CONSUMER_NAME_LENGTH - 1] = '\0';
    consumer->fd = fd;
    memcpy(&consumer->addr, addr, sizeof(struct sockaddr_in));
    ZOO_LOG_INFO("created consumer '%s'", consumer->name);
    return consumer;
}

/**
 * @brief Finds a SMB consumer handle in the linked list by its name.
 *
 * This function searches through the provided linked list of SMB consumer handles
 * and returns the handle that matches the specified name.
 *
 * @param list Pointer to the head of the linked list of SMB consumer handles.
 * @param name The name of the SMB consumer to search for.
 * @return ZOO_SMB_CONSUMER_HANDLE The handle to the SMB consumer if found, or NULL if not found.
 */
ZOO_SMB_CONSUMER_HANDLE zoo_smb_consumer_find_by_name(ZOO_LIST_HANDLE list, const char* name)
{
    if (!list)
    {
        ZOO_LOG_ERROR("invalid parameters: list is NULL");
        return NULL;
    }

    if (!name)
    {
        return NULL;
    }

    for (size_t i = 0; i < zoo_list_size(list); i++)
    {
        ZOO_SMB_CONSUMER_HANDLE consumer = (ZOO_SMB_CONSUMER_HANDLE)zoo_list_at(list, i);
        if (consumer && strcmp(consumer->name, name) == 0)
        {
            ZOO_LOG_DEBUG("found consumer '%s'", consumer->name);
            return consumer;
        }
    }

    ZOO_LOG_TRACE("consumer '%s' not found", name);
    return NULL;
}

ZOO_SMB_CONSUMER_HANDLE zoo_smb_find_consumer_by_name(ZOO_LIST_HANDLE list, const char* name)
{
    return zoo_smb_consumer_find_by_name(list, name);
}

ZOO_SMB_CONSUMER_HANDLE zoo_smb_find_consumer_by_fd(ZOO_LIST_HANDLE list, int32_t fd)
{
    if (!list)
    {
        return NULL;
    }

    for (size_t i = 0; i < zoo_list_size(list); i++)
    {
        ZOO_SMB_CONSUMER_HANDLE consumer = (ZOO_SMB_CONSUMER_HANDLE)zoo_list_at(list, i);
        if (consumer && consumer->fd == fd)
        {
            return consumer;
        }
    }

    return NULL;
}

/**
 * @brief Creates a new SMB consumer, initializes it with the given parameters, and inserts it into the provided linked list.
 *
 * @param list             The linked list handle where the new consumer will be inserted.
 * @param name             The name of the consumer to be created.
 * @param fd               The file descriptor for the consumer.
 * @param addr             The address of the consumer.
 *
 * @return ZOO_SMB_CONSUMER_HANDLE
 *         Handle to the newly created SMB consumer, or NULL on failure.
 */
ZOO_SMB_CONSUMER_HANDLE zoo_smb_create_consumer_and_insert(ZOO_LIST_HANDLE list,
                                                           const char* name,
                                                           int fd,
                                                           struct sockaddr_in* addr)
{
    ZOO_SMB_CONSUMER_HANDLE consumer = zoo_smb_create_consumer(name, fd, addr);
    if (!consumer)
    {
        ZOO_LOG_ERROR("failed to create consumer");
        return NULL;
    }

    if (zoo_list_push_back(list, consumer) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("failed to insert consumer into list");
        zoo_smb_destroy_consumer(consumer);
        return NULL;
    }
    ZOO_LOG_DEBUG("Created consumer '%s'", name);
    return consumer;
}

/**
 * @brief Set the ID for the SMB consumer.
 *
 * This function assigns a unique identifier to the SMB consumer instance.
 *
 * @param consumer SMB consumer handle.
 * @param id       Identifier to set for the consumer.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE zoo_smb_consumer_set_fd(
    ZOO_SMB_CONSUMER_HANDLE consumer,
    int32_t fd)
{
    if (!consumer)
    {
        ZOO_LOG_ERROR(" consumer is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    consumer->fd = fd;
    ZOO_LOG_DEBUG("set fd for consumer '%s'", consumer->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Retrieves the unique identifier for the specified SMB consumer.
 *
 * @param consumer The handle to the SMB consumer instance.
 * @return int32_t The unique identifier associated with the given consumer.
 */
int32_t zoo_smb_consumer_get_fd(ZOO_SMB_CONSUMER_HANDLE consumer)
{
    return consumer ? consumer->fd : -1;
}

/**
 * @brief Destroy a SMB consumer instance and release associated resources.
 *
 * This function cleans up and deallocates any resources held by the specified
 * SMB consumer handle. After calling this function, the consumer handle should
 * not be used in any further operations.
 *
 * @param consumer SMB consumer handle to be destroyed.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 *         Returns ZOO_SMB_OK on success, or an appropriate error code on failure.
 */
ZOO_ERROR_TYPE zoo_smb_destroy_consumer(ZOO_SMB_CONSUMER_HANDLE consumer)
{
    if (!consumer)
    {
        ZOO_LOG_ERROR("consumer is NULL");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    ZOO_LOG_INFO("destroy consumer '%s'", consumer->name);
    zoo_free_to_pool(consumer);
    return ZOO_SMB_OK;
}