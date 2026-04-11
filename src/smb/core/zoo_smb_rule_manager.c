/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_RULE_MANAGER
 * File name: zoo_smb_rule_manager.c
 * Description: Implementation of rule manager for ZOO SMB.
 *              Provides creation, destruction, and management of routing rules.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-02     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_rule_manager.h"
#include "zoo_memory_pool.h"
#include "zoo_list.h"
#include "zoo_log.h"
#include "zoo_smb_util.h"
#include <string.h>
#include <stdlib.h>

#define RULE_INDEX_BUCKET_COUNT 257

typedef struct ZOO_SMB_RULE_INDEX_ENTRY_STRUCT
{
    char key[MAX_RULE_NAME_LENGTH];
    ZOO_SMB_RULE_HANDLE rule;
} ZOO_SMB_RULE_INDEX_ENTRY_STRUCT;

/**
 * @brief Internal structure for rule manager.
 */
typedef struct ZOO_SMB_RULE_MANAGER_STRUCT
{
    ZOO_MUTEX_T mutex;
    ZOO_LIST_HANDLE rule_list;
    ZOO_LIST_HANDLE rule_index_buckets[RULE_INDEX_BUCKET_COUNT];
} ZOO_SMB_RULE_MANAGER_STRUCT;

static ZOO_USIZE rule_index_hash(const char* key)
{
    if (!key)
    {
        return 0;
    }

    ZOO_USIZE hash = 5381;
    while (*key)
    {
        hash = ((hash << 5) + hash) + (unsigned char)(*key);
        key++;
    }
    return hash % RULE_INDEX_BUCKET_COUNT;
}

static ZOO_BOOL compare_rule_index_entry_by_key(const void* data, const void* target)
{
    const ZOO_SMB_RULE_INDEX_ENTRY_STRUCT* entry = (const ZOO_SMB_RULE_INDEX_ENTRY_STRUCT*)data;
    const char* key = (const char*)target;
    if (!entry || !key)
    {
        return ZOO_FALSE;
    }
    return strcmp(entry->key, key) == 0;
}

static ZOO_SMB_RULE_HANDLE rule_index_find_locked(
    ZOO_SMB_RULE_MANAGER_HANDLE manager,
    const char* rule_name)
{
    if (!manager || !rule_name)
    {
        return NULL;
    }

    ZOO_USIZE bucket = rule_index_hash(rule_name);
    ZOO_LIST_HANDLE bucket_list = manager->rule_index_buckets[bucket];
    if (!bucket_list)
    {
        return NULL;
    }

    ZOO_SMB_RULE_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_RULE_INDEX_ENTRY_STRUCT*)zoo_list_find_if(
        bucket_list,
        compare_rule_index_entry_by_key,
        rule_name);
    return entry ? entry->rule : NULL;
}

static ZOO_ERROR_TYPE rule_index_upsert_locked(
    ZOO_SMB_RULE_MANAGER_HANDLE manager,
    const char* rule_name,
    ZOO_SMB_RULE_HANDLE rule)
{
    if (!manager || !rule_name || !rule)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_USIZE bucket = rule_index_hash(rule_name);
    ZOO_LIST_HANDLE bucket_list = manager->rule_index_buckets[bucket];
    if (!bucket_list)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_RULE_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_RULE_INDEX_ENTRY_STRUCT*)zoo_list_find_if(
        bucket_list,
        compare_rule_index_entry_by_key,
        rule_name);
    if (entry)
    {
        entry->rule = rule;
        return ZOO_SMB_OK;
    }

    entry = (ZOO_SMB_RULE_INDEX_ENTRY_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_RULE_INDEX_ENTRY_STRUCT));
    if (!entry)
    {
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    memset(entry, 0, sizeof(ZOO_SMB_RULE_INDEX_ENTRY_STRUCT));
    snprintf(entry->key, sizeof(entry->key), "%s", rule_name);
    entry->key[sizeof(entry->key) - 1] = '\0';
    entry->rule = rule;

    if (zoo_list_push_back(bucket_list, entry) != ZOO_SMB_OK)
    {
        zoo_free_to_pool(entry);
        return ZOO_SMB_ERROR_OPERATION_FAILED;
    }

    return ZOO_SMB_OK;
}

static void rule_index_remove_locked(
    ZOO_SMB_RULE_MANAGER_HANDLE manager,
    const char* rule_name)
{
    if (!manager || !rule_name)
    {
        return;
    }

    ZOO_USIZE bucket = rule_index_hash(rule_name);
    ZOO_LIST_HANDLE bucket_list = manager->rule_index_buckets[bucket];
    if (!bucket_list)
    {
        return;
    }

    ZOO_SMB_RULE_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_RULE_INDEX_ENTRY_STRUCT*)zoo_list_remove_if(
        bucket_list,
        compare_rule_index_entry_by_key,
        rule_name);
    if (entry)
    {
        zoo_free_to_pool(entry);
    }
}

static void destroy_rule_index(ZOO_SMB_RULE_MANAGER_HANDLE manager)
{
    if (!manager)
    {
        return;
    }

    for (ZOO_USIZE i = 0; i < RULE_INDEX_BUCKET_COUNT; i++)
    {
        ZOO_LIST_HANDLE bucket = manager->rule_index_buckets[i];
        if (!bucket)
        {
            continue;
        }

        while (!zoo_list_empty(bucket))
        {
            ZOO_SMB_RULE_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_RULE_INDEX_ENTRY_STRUCT*)zoo_list_pop_front(bucket);
            if (entry)
            {
                zoo_free_to_pool(entry);
            }
        }

        zoo_list_destroy(bucket);
        manager->rule_index_buckets[i] = NULL;
    }
}

/**
 * @brief Finds a rule by its name.
 *
 * Searches for a rule in the rule manager using the specified name and returns
 * a handle to the rule if found.
 *
 * @param name The name of the rule to search for.
 * @return ZOO_SMB_RULE_HANDLE Handle to the found rule, or NULL/invalid handle if not found.
 */
static ZOO_SMB_RULE_HANDLE find_rule_by_name(
    IN ZOO_SMB_RULE_MANAGER_HANDLE manager,
    IN const char* rule_name)
{
    if (!manager || !rule_name)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return NULL;
    }

    ZOO_SMB_RULE_HANDLE indexed_rule = rule_index_find_locked(manager, rule_name);
    if (indexed_rule)
    {
        return indexed_rule;
    }

    for (size_t i = 0; i < zoo_list_size(manager->rule_list); ++i)
    {
        ZOO_SMB_RULE_HANDLE rule = (ZOO_SMB_RULE_HANDLE)zoo_list_at(manager->rule_list, i);
        if (rule && strcmp(rule->name, rule_name) == 0)
        {
            (void)rule_index_upsert_locked(manager, rule_name, rule);
            return rule;
        }
    }
    return NULL;
}

/**
 * @brief Creates a new rule manager instance.
 *
 * @return A handle to the created rule manager, or NULL on failure.
 */
ZOO_SMB_RULE_MANAGER_HANDLE zoo_smb_create_rule_manager(void)
{
    ZOO_SMB_RULE_MANAGER_HANDLE manager = (ZOO_SMB_RULE_MANAGER_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_RULE_MANAGER_STRUCT));
    if (!manager)
    {
        ZOO_LOG_ERROR("allocation failed");
        return NULL;
    }
    memset(manager, 0, sizeof(ZOO_SMB_RULE_MANAGER_STRUCT));
    if (!ZOO_MUTEX_INIT(&manager->mutex))
    {
        zoo_free_to_pool(manager);
        return NULL;
    }

    manager->rule_list = zoo_list_create(256);  // initial capacity
    if (!manager->rule_list)
    {
        ZOO_LOG_ERROR("failed to create rule list");
        ZOO_MUTEX_DESTROY(&manager->mutex);
        zoo_free_to_pool(manager);
        return NULL;
    }

    for (ZOO_USIZE i = 0; i < RULE_INDEX_BUCKET_COUNT; i++)
    {
        manager->rule_index_buckets[i] = zoo_list_create(8);
        if (!manager->rule_index_buckets[i])
        {
            destroy_rule_index(manager);
            zoo_list_destroy(manager->rule_list);
            ZOO_MUTEX_DESTROY(&manager->mutex);
            zoo_free_to_pool(manager);
            return NULL;
        }
    }

    return manager;
}

/**
 * @brief Destroys the specified rule manager and releases all associated resources.
 *
 * @param manager The handle to the rule manager to destroy.
 */
void zoo_smb_destroy_rule_manager(IN ZOO_SMB_RULE_MANAGER_HANDLE manager)
{
    if (!manager)
    {
        ZOO_LOG_WARN("manager is NULL");
        return;
    }

    ZOO_MUTEX_LOCK(&manager->mutex);
    if (manager->rule_list)
    {
        for (size_t i = 0; i < zoo_list_size(manager->rule_list); ++i)
        {
            ZOO_SMB_RULE_HANDLE rule = (ZOO_SMB_RULE_HANDLE)zoo_list_at(manager->rule_list, i);
            zoo_smb_destroy_routing_rule(rule);
        }
    }
    destroy_rule_index(manager);
    zoo_list_destroy(manager->rule_list);
    ZOO_MUTEX_UNLOCK(&manager->mutex);
    ZOO_MUTEX_DESTROY(&manager->mutex);
    zoo_free_to_pool(manager);
}

/**
 * @brief Creates a new SMB rule and returns its handle.
 *
 * This function initializes and creates a new rule within the SMB rule manager.
 *
 * @return ZOO_SMB_RULE_HANDLE Handle to the newly created SMB rule.
 */
ZOO_SMB_RULE_HANDLE zoo_smb_rule_manager_make_rule(IN ZOO_SMB_RULE_MANAGER_HANDLE rule_manager,
                                                   IN ZOO_SMB_NODE_HANDLE node,
                                                   IN ZOO_SMB_SERVICE_HANDLE service,
                                                   IN ZOO_SMB_TRANSPORT_HANDLE transport)
{
    if (!rule_manager || !node || !service || !transport)
    {
        ZOO_LOG_ERROR(" invalid parameters, rule_manager: %p, node: %p, service: %p, transport: %p",
                          rule_manager,
                          node,
                          service,
                          transport);
        return NULL;
    }

    const char* rule_name = zoo_smb_make_name_3(node->target, node->topic, node->transport_type);
    ZOO_MUTEX_LOCK(&rule_manager->mutex);
    ZOO_SMB_RULE_HANDLE rule = find_rule_by_name(rule_manager, rule_name);
    if (rule)
    {
        ZOO_MUTEX_UNLOCK(&rule_manager->mutex);
        ZOO_LOG_DEBUG("rule '%s' already exists", rule_name);
        return rule;
    }

    rule = zoo_smb_create_routing_rule(rule_name, service, transport, node->observers);
    if (!rule)
    {
        ZOO_MUTEX_UNLOCK(&rule_manager->mutex);
        ZOO_LOG_ERROR("failed to create rule '%s'", rule_name);
        return NULL;  // Failed to create rule
    }

    zoo_list_push_back(rule_manager->rule_list, rule);
    (void)rule_index_upsert_locked(rule_manager, rule->name, rule);
    ZOO_MUTEX_UNLOCK(&rule_manager->mutex);
    return rule;
}

/**
 * @brief Retrieves a rule handle by its name from the specified rule manager.
 *
 * This function searches for a rule with the given name within the provided
 * rule manager and returns a handle to the rule if found.
 *
 * @param[in] rule_manager Handle to the rule manager instance.
 * @param[in] rule_name    Name of the rule to retrieve.
 *
 * @return ZOO_SMB_RULE_HANDLE Handle to the rule if found, or NULL if not found.
 */
ZOO_SMB_RULE_HANDLE zoo_smb_rule_manager_get_rule(IN ZOO_SMB_RULE_MANAGER_HANDLE rule_manager, IN const char* rule_name)
{
    if (!rule_manager || !rule_name)
    {
        ZOO_LOG_ERROR("invalid parameters rule_manager: %p, rule_name: %p", rule_manager, rule_name);
        return NULL;
    }

    ZOO_MUTEX_LOCK(&rule_manager->mutex);
    ZOO_SMB_RULE_HANDLE rule = find_rule_by_name(rule_manager, rule_name);
    ZOO_MUTEX_UNLOCK(&rule_manager->mutex);
    return rule;
}

/**
 * @brief Adds a routing rule to the rule manager.
 *
 * @param manager The rule manager handle.
 * @param rule    The rule handle to add.
 * @return ZOO_ERROR_TYPE Returns ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE zoo_smb_rule_manager_add_rule(
    IN ZOO_SMB_RULE_MANAGER_HANDLE manager,
    IN const ZOO_SMB_RULE_HANDLE rule)
{
    if (!manager || !rule)
    {
        ZOO_LOG_ERROR("invalid parameters, manager: %p, rule: %p", manager, rule);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_MUTEX_LOCK(&manager->mutex);
    if (find_rule_by_name(manager, rule->name))
    {
        ZOO_MUTEX_UNLOCK(&manager->mutex);
        return ZOO_SMB_OK;
    }

    if (zoo_list_push_back(manager->rule_list, rule) != ZOO_SMB_OK)
    {
        ZOO_MUTEX_UNLOCK(&manager->mutex);
        ZOO_LOG_ERROR("failed to add rule '%s'", rule->name);
        return ZOO_SMB_ERROR_OPERATION_FAILED;
    }
    (void)rule_index_upsert_locked(manager, rule->name, rule);
    ZOO_MUTEX_UNLOCK(&manager->mutex);
    ZOO_LOG_INFO("rule '%s' added", rule->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Removes a routing rule from the rule manager.
 *
 * @param manager The rule manager handle.
 * @param rule    The rule handle to remove.
 * @return ZOO_ERROR_TYPE Returns ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE zoo_smb_rule_manager_remove_rule(
    IN ZOO_SMB_RULE_MANAGER_HANDLE manager,
    IN ZOO_SMB_RULE_HANDLE rule)
{
    if (!manager || !rule)
    {
        ZOO_LOG_ERROR("invalid parameters, manager: %p, rule: %p", manager, rule);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_MUTEX_LOCK(&manager->mutex);
    if (zoo_list_remove(manager->rule_list, rule) == ZOO_SMB_OK)
    {
        rule_index_remove_locked(manager, rule->name);
        ZOO_MUTEX_UNLOCK(&manager->mutex);
        zoo_smb_destroy_routing_rule(rule);
        return ZOO_SMB_OK;
    }
    else
    {
        ZOO_MUTEX_UNLOCK(&manager->mutex);
        ZOO_LOG_WARN("rule['%s'] not found in list", rule->name);
        return ZOO_SMB_ERROR_NOT_FOUND;
    }
}