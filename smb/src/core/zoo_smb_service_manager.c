/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SERVICE_MANAGER
 * File name: zoo_smb_service_manager.c
 * Description: Implementation of service manager for ZOO SMB.
 *              Provides creation, destruction, registration, lookup, and removal of services.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-02     weiwang.sun       created
 ******************************************************************************/
#include "zoo_smb_service_manager.h"
#include "../../memory_pool/inc/zoo_memory_pool.h"
#include "zoo_smb_port_manager.h"
#include "../../thread_pool/inc/zoo_thread_pool.h"
#include "../../buffer/inc/zoo_list.h"
#include "../../log/inc/zoo_log.h"
#include "zoo_util.h"
#include <string.h>
#include <stdlib.h>

#define SERVICE_INDEX_BUCKET_COUNT 257

typedef struct ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT
{
    char name[MAX_SERVICE_NAME_LENGTH];
    ZOO_SMB_SERVICE_HANDLE service;
} ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT;

/**
 * @brief Internal structure for service manager.
 */
typedef struct ZOO_SMB_SERVICE_MANAGER_STRUCT
{
    ZOO_LIST_HANDLE service_list[ZOO_SMB_SERVICE_TYPE_MAX];
    ZOO_LIST_HANDLE registration_index_buckets[SERVICE_INDEX_BUCKET_COUNT];
    ZOO_LIST_HANDLE observer_list; /**< List of observers registered with the service manager */
    const ZOO_SMB_CONFIG_STRUCT* config; /**< Pointer to the local machine configuration */
    ZOO_MUTEX_T service_lock;
    ZOO_COND_T service_cond;
} ZOO_SMB_SERVICE_MANAGER_STRUCT;

/**
 * @brief Calculate hash bucket index for service name.
 * @param name Service name string
 * @return ZOO_USIZE Hash bucket index
 */
static ZOO_USIZE service_index_hash(const char* name)
{
    if (!name)
    {
        return 0;
    }

    ZOO_USIZE hash = 5381;
    while (*name)
    {
        hash = ((hash << 5) + hash) + (unsigned char)(*name);
        name++;
    }

    return hash % SERVICE_INDEX_BUCKET_COUNT;
}

/**
 * @brief Compare service index entry against target service name.
 * @param data Pointer to service index entry
 * @param target Pointer to target service name
 * @return ZOO_TRUE if names match, otherwise ZOO_FALSE
 */
static ZOO_BOOL compare_service_index_entry_by_name(const void* data, const void* target)
{
    const ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT* entry = (const ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT*)data;
    const char* name = (const char*)target;

    if (!entry || !name)
    {
        return ZOO_FALSE;
    }

    return strcmp(entry->name, name) == 0;
}

/**
 * @brief Remove registration index entry for the specified service name.
 * @param manager Service manager handle
 * @param service_name Service name string
 * @return void
 */
static void service_index_remove_locked(
    ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
    const char* service_name)
{
    if (!manager || !service_name)
    {
        return;
    }

    ZOO_USIZE bucket = service_index_hash(service_name);
    ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT*)zoo_list_remove_if(
        manager->registration_index_buckets[bucket],
        compare_service_index_entry_by_name,
        service_name);

    if (entry)
    {
        zoo_free_to_pool(entry);
    }
}

/**
 * @brief Insert or update registration index entry for a service.
 * @param manager Service manager handle
 * @param service Service handle to index
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
static ZOO_ERROR_TYPE service_index_upsert_locked(
    ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!manager || !service)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_USIZE bucket = service_index_hash(service->name);
    ZOO_LIST_HANDLE bucket_list = manager->registration_index_buckets[bucket];
    if (!bucket_list)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT*)zoo_list_find_if(
        bucket_list,
        compare_service_index_entry_by_name,
        service->name);

    if (entry)
    {
        entry->service = service;
        return ZOO_SMB_OK;
    }

    entry = (ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT));
    if (!entry)
    {
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    memset(entry, 0, sizeof(ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT));
    snprintf(entry->name, sizeof(entry->name), "%s", service->name);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->service = service;

    ZOO_ERROR_TYPE ret = zoo_list_push_back(bucket_list, entry);
    if (ret != ZOO_SMB_OK)
    {
        zoo_free_to_pool(entry);
        return ret;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Find service handle from registration index.
 * @param manager Service manager handle
 * @param service_name Service name string
 * @return ZOO_SMB_SERVICE_HANDLE Service handle, NULL if not found
 */
static ZOO_SMB_SERVICE_HANDLE service_index_find_locked(
    ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
    const char* service_name)
{
    if (!manager || !service_name)
    {
        return NULL;
    }

    ZOO_USIZE bucket = service_index_hash(service_name);
    ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT*)zoo_list_find_if(
        manager->registration_index_buckets[bucket],
        compare_service_index_entry_by_name,
        service_name);

    return entry ? entry->service : NULL;
}

/**
 * @brief Destroy all service registration index buckets and entries.
 * @param manager Service manager handle
 * @return void
 */
static void destroy_service_index(ZOO_SMB_SERVICE_MANAGER_HANDLE manager)
{
    if (!manager)
    {
        return;
    }

    for (ZOO_USIZE i = 0; i < SERVICE_INDEX_BUCKET_COUNT; i++)
    {
        ZOO_LIST_HANDLE bucket = manager->registration_index_buckets[i];
        if (!bucket)
        {
            continue;
        }

        while (!zoo_list_empty(bucket))
        {
            ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_SERVICE_INDEX_ENTRY_STRUCT*)zoo_list_pop_front(bucket);
            if (entry)
            {
                zoo_free_to_pool(entry);
            }
        }

        zoo_list_destroy(bucket);
        manager->registration_index_buckets[i] = NULL;
    }
}

/**
 * @brief Compares the name of a service with a target name.
 *
 * This function is intended to be used as a comparison callback, for example in search or filter operations.
 *
 * @param data Pointer to the service object whose name will be compared.
 * @param target Pointer to the target name or object to compare against.
 * @return ZOO_TRUE if the service name matches the target; ZOO_FALSE otherwise.
 */
static ZOO_BOOL compare_service_by_name(const void* data, const void* target)
{
    const ZOO_SMB_SERVICE_STRUCT* service = (const ZOO_SMB_SERVICE_STRUCT*)data;
    if (!service || !target)
    {
        return ZOO_FALSE;
    }
    return (strcmp(service->name, (const char*)target) == 0);
}

/**
 * @brief Finds and returns a handle to a SMB service managed by the service manager.
 *
 * This function searches for an available SMB service and returns its handle.
 * The criteria for selecting the service may depend on internal logic such as
 * service availability, load balancing, or specific service attributes.
 *
 * @return ZOO_SMB_SERVICE_HANDLE Handle to the found SMB service, or an invalid handle
 *         if no suitable service is found.
 */
static ZOO_SMB_SERVICE_HANDLE manager_find_service(
    ZOO_LIST_HANDLE service_list,
    const char* service_name,
    const char* topic,
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type)
{
    for (size_t i = 0; i < zoo_list_size(service_list); i++)
    {
        ZOO_SMB_SERVICE_STRUCT* service = (ZOO_SMB_SERVICE_STRUCT*)zoo_list_at(service_list, i);
        if (service && strcmp(service->name, service_name) == 0 && service->transport_type == transport_type && strcmp(service->topic, topic) == 0)
        {
            return (ZOO_SMB_SERVICE_HANDLE)service;
        }
    }
    return NULL;
}

/**
 * @brief Notifies all registered observers about a service event.
 *
 * This function iterates through the list of observers registered for service notifications
 * and sends them the relevant event or status update. It is typically called internally
 * whenever a significant change occurs in the service state that observers need to be aware of.
 *
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the notification process.
 */
static ZOO_ERROR_TYPE notify_service_observers(
    ZOO_LIST_HANDLE observers,
    const ZOO_SMB_SERVICE_STRUCT* service)
{
    if (!observers || !service)
    {
        ZOO_LOG_ERROR("notify_service_observers: invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    for (size_t i = 0; i < zoo_list_size(observers); i++)
    {
        ZOO_SMB_SERVICE_OBSERVER_STRUCT* observer = (ZOO_SMB_SERVICE_OBSERVER_STRUCT*)zoo_list_at(observers, i);
        if (observer && observer->handler_cb)
        {
            observer->handler_cb((const ZOO_SMB_SERVICE_HANDLE)service, observer->user_data);
        }
    }
    return ZOO_SMB_OK;
}

/**
 * @brief Initializes the broadcast service for the SMB manager.
 *
 * This function sets up and starts the broadcast service, which is responsible
 * for announcing the presence of the SMB service on the network.
 *
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the initialization.
 */
ZOO_ERROR_TYPE init_broadcast_service(ZOO_SMB_SERVICE_MANAGER_HANDLE manager, const ZOO_SMB_CONFIG_STRUCT* config)
{
    ZOO_SMB_SERVICE_STRUCT* service = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY, 
        "Discovery", "discovery/service",
         config->broadcast.address, 
         config->broadcast.port, 
         NULL, 
         0, 
         ZOO_SMB_TRANSPORT_TYPE_UDP_BROADCAST, 
         config->broadcast.interval_ms);
    if (service)
    {
        zoo_list_push_back(manager->service_list[ZOO_SMB_SERVICE_TYPE_DISCOVERY], service);
    }
    return ZOO_SMB_OK;
}

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
ZOO_SMB_SERVICE_MANAGER_HANDLE zoo_smb_create_service_manager(const ZOO_SMB_CONFIG_STRUCT* config)
{
    ZOO_SMB_SERVICE_MANAGER_HANDLE manager = (ZOO_SMB_SERVICE_MANAGER_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_SERVICE_MANAGER_STRUCT));
    if (!manager || !config)
    {
        ZOO_LOG_ERROR("zoo_smb_create_service_manager: invalid parameters");
        return NULL;
    }

    memset(manager, 0, sizeof(ZOO_SMB_SERVICE_MANAGER_STRUCT));
    if (!ZOO_MUTEX_INIT(&manager->service_lock) || !ZOO_COND_INIT(&manager->service_cond))
    {
        ZOO_LOG_ERROR("failed to initialize service manager synchronization primitives");
        zoo_free_to_pool(manager);
        return NULL;
    }

    for (size_t i = 0; i < ZOO_SMB_SERVICE_TYPE_MAX; i++)
    {
        manager->service_list[i] = zoo_list_create(512);  // initial capacity
        if (!manager->service_list[i])
        {
            ZOO_LOG_ERROR("failed to create service list");
            ZOO_COND_DESTROY(&manager->service_cond);
            ZOO_MUTEX_DESTROY(&manager->service_lock);
            zoo_free_to_pool(manager);
            return NULL;
        }
    }

    for (size_t i = 0; i < SERVICE_INDEX_BUCKET_COUNT; i++)
    {
        manager->registration_index_buckets[i] = zoo_list_create(16);
        if (!manager->registration_index_buckets[i])
        {
            ZOO_LOG_ERROR("failed to create registration service index bucket");
            for (size_t j = 0; j < i; j++)
            {
                zoo_list_destroy(manager->registration_index_buckets[j]);
                manager->registration_index_buckets[j] = NULL;
            }
            for (size_t j = 0; j < ZOO_SMB_SERVICE_TYPE_MAX; j++)
            {
                if (manager->service_list[j])
                {
                    zoo_list_destroy(manager->service_list[j]);
                }
            }
            ZOO_COND_DESTROY(&manager->service_cond);
            ZOO_MUTEX_DESTROY(&manager->service_lock);
            zoo_free_to_pool(manager);
            return NULL;
        }
    }

    manager->observer_list = zoo_list_create(512);  // initial capacity
    if (!manager->observer_list)
    {
        ZOO_LOG_ERROR("failed to create observer list");
        for (size_t i = 0; i < ZOO_SMB_SERVICE_TYPE_MAX; i++)
        {
            if (manager->service_list[i])
            {
                zoo_list_destroy(manager->service_list[i]);
            }
        }
        ZOO_COND_DESTROY(&manager->service_cond);
        ZOO_MUTEX_DESTROY(&manager->service_lock);
        zoo_free_to_pool(manager);
        return NULL;
    }

    if (init_broadcast_service(manager, config) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("failed to initialize broadcast service");
        zoo_list_destroy(manager->observer_list);
        for (size_t i = 0; i < ZOO_SMB_SERVICE_TYPE_MAX; i++)
        {
            zoo_list_destroy(manager->service_list[i]);
        }
        destroy_service_index(manager);
        ZOO_COND_DESTROY(&manager->service_cond);
        ZOO_MUTEX_DESTROY(&manager->service_lock);
        zoo_free_to_pool(manager);
        return NULL;
    }

    manager->config = config;  // Store the configuration pointer
    ZOO_LOG_DEBUG("manager created and initialized successfully");
    return manager;
}

/**
 * @brief Destroys the specified SMB service manager instance and releases associated resources.
 *
 * This function cleans up and deallocates any resources held by the given
 * ZOO_SMB_SERVICE_MANAGER_HANDLE. After calling this function, the handle
 * should not be used in any further operations.
 *
 * @param manager The handle to the SMB service manager to be destroyed.
 */
void zoo_smb_destroy_service_manager(ZOO_SMB_SERVICE_MANAGER_HANDLE manager)
{
    if (!manager)
    {
        ZOO_LOG_WARN("manager is NULL");
        return;
    }

    for (size_t j = 0; j < ZOO_SMB_SERVICE_TYPE_MAX; j++)
    {
        for (size_t i = 0; i < zoo_list_size(manager->service_list[j]); ++i)
        {
            ZOO_SMB_SERVICE_HANDLE service = (ZOO_SMB_SERVICE_HANDLE)zoo_list_at(manager->service_list[j], i);
            zoo_smb_destroy_service(service);
        }
    }

    for (size_t i = 0; i < ZOO_SMB_SERVICE_TYPE_MAX; i++)
    {
        zoo_list_destroy(manager->service_list[i]);
    }

    zoo_list_destroy(manager->observer_list);
    destroy_service_index(manager);
    ZOO_COND_DESTROY(&manager->service_cond);
    ZOO_MUTEX_DESTROY(&manager->service_lock);
    zoo_free_to_pool(manager);
    ZOO_LOG_DEBUG("manager destroyed");
}

/**
 * @brief Creates or initializes a new SMB service within the Zoo SMB Service Manager.
 *
 * This function is responsible for setting up a new service instance, configuring
 * necessary parameters, and registering it with the service manager. The exact
 * behavior and required parameters depend on the implementation details.
 *
 * @param ... Parameters required to create the SMB service (to be specified).
 * @return void
 */
ZOO_SMB_SERVICE_HANDLE zoo_smb_service_manager_make_service(
    ZOO_SMB_SERVICE_MANAGER_HANDLE manager,
    ZOO_SMB_NODE_HANDLE node)
{
    ZOO_SMB_SERVICE_TYPE_ENUM service_type = ZOO_SMB_SERVICE_TYPE_MAX;
    if (node->node_type == ZOO_SMB_NODE_TYPE_SERVER || node->node_type == ZOO_SMB_NODE_TYPE_PUBLISHER)
    {
        service_type = ZOO_SMB_SERVICE_TYPE_BROADCAST;
    }
    else if (node->node_type == ZOO_SMB_NODE_TYPE_SUBSCRIBER || node->node_type == ZOO_SMB_NODE_TYPE_CLIENT)
    {
        service_type = ZOO_SMB_SERVICE_TYPE_REGISTRATION;
    }
    else
    {
        ZOO_LOG_ERROR("Unsupported node type for service creation: %d", node->node_type);
        return NULL;
    }

    ZOO_LOG_DEBUG("Creating service for node: %s, target:%s type: %d", node->name, node->target, service_type);
    ZOO_SMB_SERVICE_HANDLE service = manager_find_service(manager->service_list[service_type], node->target, node->topic, node->transport_type);
    if (!service && service_type == ZOO_SMB_SERVICE_TYPE_BROADCAST)
    {
        const char * multicast_address = manager->config->multicast.address;
        int multicast_port = manager->config->multicast.port;
        int service_port = zoo_smb_find_available_port(manager->config->local_machine.port_min, manager->config->local_machine.port_max);
        const char* service_address = manager->config->local_machine.address;
        service = zoo_smb_create_service(
            service_type, 
            node->target, 
            node->topic, 
            service_address, 
            service_port, 
            multicast_address, 
            multicast_port,
            node->transport_type, 
            manager->config->broadcast.interval_ms);
        if (service)
        {
            ZOO_LOG_INFO("service %s, service_type:%d added", node->target, service_type);
            zoo_list_push_back(manager->service_list[service_type], service);
        }
    }

    return service;
}

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
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!manager || !service)
    {
        ZOO_LOG_ERROR("invalid parameters, manager: %p, service: %p", manager, service);
        return;
    }

    if (service_type >= ZOO_SMB_SERVICE_TYPE_MAX)
    {
        ZOO_LOG_ERROR("invalid service type[%d]", service_type);
        return;
    }

    ZOO_LIST_HANDLE service_list = manager->service_list[service_type];
    ZOO_MUTEX_LOCK(&manager->service_lock);
    if (NULL == zoo_list_find_if(service_list, compare_service_by_name, service->name))
    {
        ZOO_LOG_INFO("New service %s, service_type:%d topic:%s registered", service->name, service_type, service->topic);
        ZOO_SMB_SERVICE_HANDLE new_service = zoo_smb_service_duplicate(service);
        zoo_list_push_back(service_list, new_service);
        if (service_type == ZOO_SMB_SERVICE_TYPE_REGISTRATION)
        {
            (void)service_index_upsert_locked(manager, new_service);
        }
        ZOO_COND_BROADCAST(&manager->service_cond);
        if (service_type == ZOO_SMB_SERVICE_TYPE_REGISTRATION)
        {
            zoo_thread_pool_submit_task(
                "service_manager_notify", (ZOO_TASK_FUNC)notify_service_observers, manager->observer_list, new_service, ZOO_FALSE);
        }
    }
    ZOO_MUTEX_UNLOCK(&manager->service_lock);
}

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
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!manager || !service)
    {
        ZOO_LOG_ERROR("zoo_smb_service_manager_unregister_service: invalid parameters");
        return;
    }

    if (service_type >= ZOO_SMB_SERVICE_TYPE_MAX)
    {
        ZOO_LOG_ERROR("zoo_smb_service_manager_unregister_service: invalid service type");
        return;
    }

    ZOO_LIST_HANDLE service_list = manager->service_list[service_type];
    if (!service_list)
    {
        ZOO_LOG_ERROR("zoo_smb_service_manager_unregister_service: service list is NULL");
        return;
    }

    ZOO_MUTEX_LOCK(&manager->service_lock);
    if (service_type == ZOO_SMB_SERVICE_TYPE_REGISTRATION)
    {
        service_index_remove_locked(manager, service->name);
    }
    zoo_list_remove(service_list, service);
    ZOO_COND_BROADCAST(&manager->service_cond);
    ZOO_MUTEX_UNLOCK(&manager->service_lock);
    ZOO_LOG_INFO("service %s, service_type:%d removed", service->name, service_type);
}

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
    ZOO_SMB_SERVICE_TYPE_ENUM service_type)
{
    if (!manager || !service_name)
    {
        ZOO_LOG_ERROR("invalid parameters, manager: %p, service_name: %p", manager, service_name);
        return NULL;
    }

    if (service_type >= ZOO_SMB_SERVICE_TYPE_MAX)
    {
        ZOO_LOG_ERROR("invalid service type[%d]", service_type);
        return NULL;
    }

    ZOO_MUTEX_LOCK(&manager->service_lock);
    ZOO_SMB_SERVICE_HANDLE service = NULL;
    if (service_type == ZOO_SMB_SERVICE_TYPE_REGISTRATION)
    {
        service = service_index_find_locked(manager, service_name);
    }
    else
    {
        service = zoo_list_find_if(manager->service_list[service_type], compare_service_by_name, service_name);
    }
    ZOO_MUTEX_UNLOCK(&manager->service_lock);
    return service;
}

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
    uint32_t timeout_ms)
{
    ZOO_LOG_DEBUG("service_name=%s, timeout_ms=%u", service_name, timeout_ms);
    if (!manager || !service_name)
    {
        return NULL;
    }

    uint64_t start_time = zoo_get_timestamp_milliseconds();
    ZOO_MUTEX_LOCK(&manager->service_lock);
    ZOO_SMB_SERVICE_HANDLE service = service_index_find_locked(manager, service_name);

    while (!service)
    {
        uint64_t elapsed = zoo_get_timestamp_milliseconds() - start_time;
        if (elapsed >= timeout_ms)
        {
            break;
        }

        uint32_t wait_ms = (uint32_t)(timeout_ms - elapsed);
        if (!ZOO_COND_WAIT_TIMEOUT(&manager->service_cond, &manager->service_lock, wait_ms))
        {
            break;
        }

        service = service_index_find_locked(manager, service_name);
    }
    ZOO_MUTEX_UNLOCK(&manager->service_lock);
    return service;
}

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
    ZOO_SMB_SERVICE_TYPE_ENUM service_type)
{
    return manager ? manager->service_list[service_type] : NULL;
}

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
    ZOO_SMB_SERVICE_OBSERVER_HANDLE observer)
{
    if (!manager || !observer)
    {
        ZOO_LOG_WARN("invalid parameters");
        return;
    }

    zoo_list_push_back(manager->observer_list, observer);
    ZOO_LOG_INFO("observer registered");
}

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
    ZOO_SMB_SERVICE_OBSERVER_HANDLE observer)
{
    if (!manager || !observer)
    {
        ZOO_LOG_WARN("invalid parameters");
        return;
    }

    zoo_list_remove(manager->observer_list, observer);
    ZOO_LOG_INFO("observer unregistered");
}