/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_MANAGER
 * File name: zoo_smb_transport_manager.c
 * Description: Implementation of transport manager for ZOO SMB.
 *              Provides creation, destruction, and management of transport handles.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-02     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_transport_manager.h"
#include "zoo_types.h"
#include "zoo_smb_port_manager.h"
#include "zoo_smb_util.h"
#include "zoo_memory_pool.h"
#include "zoo_thread_pool.h"
#include "zoo_list.h"
#include "zoo_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdatomic.h>

#define TRANSPORT_INDEX_BUCKET_COUNT 257
#define TRANSPORT_START_WAIT_STEP_MS 20
#define TRANSPORT_SEND_FAIL_OPEN_THRESHOLD 5
#define TRANSPORT_SEND_OPEN_COOLDOWN_SEC 2

/**
 * @brief Check whether a send error should trip the transport circuit breaker.
 * @param err Transport send error code.
 * @return ZOO_BOOL ZOO_TRUE when the error should open circuit protection.
 */
static ZOO_BOOL transport_send_failure_should_trip_circuit(ZOO_ERROR_TYPE err)
{
    return !((ZOO_ERROR_TYPE)ZOO_SMB_ERROR_QUEUE_FULL == err ||
             (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_TIMEOUT == err ||
             (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_BUSY == err ||
             (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_AGAIN == err ||
             (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_OUT_OF_MEMORY == err ||
             (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_ALLOCATION_FAILED == err ||
             (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_POOL_EXHAUSTED == err ||
             (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SERVICE_UNAVAILABLE == err ||
             (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SERVICE_NOT_FOUND == err ||
             (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SHM_BUFFER_FULL == err);
}

typedef struct ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT
{
    char key[2 * MAX_TRANSPORT_ADDRESS_LENGTH + 40];
    ZOO_SMB_TRANSPORT_HANDLE transport;
    char address[MAX_TRANSPORT_ADDRESS_LENGTH];
    ZOO_U16 port;
    ZOO_SMB_TRANSPORT_TYPE_ENUM type;
    ZOO_BOOL is_server;
    ZOO_U32 consecutive_send_failures;
    ZOO_U64 total_send_success;
    ZOO_U64 total_send_failures;
    ZOO_U64 backpressure_drops;
    ZOO_U64 circuit_open_rejections;
    ZOO_U32 in_flight_sends;
    ZOO_U32 send_high_watermark;
    ZOO_U32 send_low_watermark;
    ZOO_BOOL backpressure_active;
    time_t circuit_open_until;
} ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT;

/**
 * @brief Internal structure for transport manager.
 */
typedef struct ZOO_SMB_TRANSPORT_MANAGER_STRUCT
{
    ZOO_MUTEX_T mutex;
    ZOO_LIST_HANDLE transport_list;
    ZOO_LIST_HANDLE transport_index_buckets[TRANSPORT_INDEX_BUCKET_COUNT];
    ZOO_LIST_HANDLE data_observer_list;
    const ZOO_SMB_CONFIG_STRUCT* global_config;
    ZOO_U32 send_high_watermark;
    ZOO_U32 send_low_watermark;
    atomic_bool backpressure_active;
    atomic_uint in_flight_sends;
    atomic_ullong total_send_success;
    atomic_ullong total_send_failures;
    atomic_ullong backpressure_drops;
    atomic_ullong circuit_open_rejections;
    ZOO_BOOL telemetry_enabled;
} ZOO_SMB_TRANSPORT_MANAGER_STRUCT;

#define LOCK_MANAGER(mgr) ZOO_MUTEX_LOCK(&(mgr)->mutex)
#define UNLOCK_MANAGER(mgr) ZOO_MUTEX_UNLOCK(&(mgr)->mutex)

static void init_transport_config(
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config,
    const ZOO_SMB_CONFIG_STRUCT* global_config,
    ZOO_SMB_SERVICE_HANDLE service);

/**
 * @brief Wait until a transport enters the started state or timeout expires.
 * @param manager Transport manager handle
 * @param transport Transport handle
 * @return ZOO_TRUE if transport started successfully, otherwise ZOO_FALSE
 */
static ZOO_BOOL wait_for_transport_started(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_TRANSPORT_HANDLE transport)
{
    if (!transport)
    {
        return ZOO_FALSE;
    }

    ZOO_INT32 timeout_ms = 5000;
    if (manager && manager->global_config && manager->global_config->sys.max_timeout > 0)
    {
        timeout_ms = manager->global_config->sys.max_timeout;
    }

    ZOO_INT32 elapsed_ms = 0;
    while (!zoo_smb_transport_is_started(transport) && elapsed_ms < timeout_ms)
    {
        ZOO_SMB_SLEEP_MS(TRANSPORT_START_WAIT_STEP_MS);
        elapsed_ms += TRANSPORT_START_WAIT_STEP_MS;
    }

    return zoo_smb_transport_is_started(transport);
}

/**
 * @brief Calculate hash bucket index for transport index key.
 * @param key Transport index key string
 * @return ZOO_USIZE Hash bucket index
 */
static ZOO_USIZE transport_index_hash(const char* key)
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

    return hash % TRANSPORT_INDEX_BUCKET_COUNT;
}

/**
 * @brief Build transport index key from address, port, and transport type.
 * @param out_key Output key buffer
 * @param out_size Output key buffer size
 * @param address Transport address string
 * @param port Transport port
 * @param type Transport type
 * @return void
 */
static void make_transport_index_key(
    char* out_key,
    ZOO_USIZE out_size,
    const char* address,
    ZOO_U16 port,
    ZOO_SMB_TRANSPORT_TYPE_ENUM type,
    ZOO_BOOL is_server)
{
    if (!out_key || out_size == 0)
    {
        return;
    }

    snprintf(out_key,
             out_size,
             "%s|%u|%d|%d",
             address ? address : "",
             (unsigned int)port,
             (int)type,
             (int)is_server);
    out_key[out_size - 1] = '\0';
}

/**
 * @brief Compare transport index entry against target key.
 * @param data Pointer to transport index entry
 * @param target Pointer to target key string
 * @return ZOO_TRUE if keys match, otherwise ZOO_FALSE
 */
static ZOO_BOOL compare_transport_index_entry_by_key(const void* data, const void* target)
{
    const ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* entry = (const ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT*)data;
    const char* key = (const char*)target;

    if (!entry || !key)
    {
        return ZOO_FALSE;
    }

    return strcmp(entry->key, key) == 0;
}

/**
 * @brief Insert or update a transport index entry.
 * @param manager Transport manager handle
 * @param address Transport address string
 * @param port Transport port
 * @param type Transport type
 * @param transport Transport handle to store
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
static ZOO_ERROR_TYPE transport_index_upsert(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    const char* address,
    ZOO_U16 port,
    ZOO_SMB_TRANSPORT_TYPE_ENUM type,
    ZOO_BOOL is_server,
    ZOO_SMB_TRANSPORT_HANDLE transport)
{
    if (!manager || !transport)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    char key[2 * MAX_TRANSPORT_ADDRESS_LENGTH + 40] = {0};
    make_transport_index_key(key, sizeof(key), address, port, type, is_server);
    ZOO_USIZE bucket = transport_index_hash(key);
    ZOO_LIST_HANDLE bucket_list = manager->transport_index_buckets[bucket];
    if (!bucket_list)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    LOCK_MANAGER(manager);

    ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT*)zoo_list_find_if(
        bucket_list,
        compare_transport_index_entry_by_key,
        key);

    if (entry)
    {
        entry->transport = transport;
        if (address)
        {
            snprintf(entry->address, sizeof(entry->address), "%s", address);
            entry->address[sizeof(entry->address) - 1] = '\0';
        }
        entry->port = port;
        entry->type = type;
        entry->is_server = is_server;
        UNLOCK_MANAGER(manager);
        return ZOO_SMB_OK;
    }

    entry = (ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT));
    if (!entry)
    {
        UNLOCK_MANAGER(manager);
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    memset(entry, 0, sizeof(ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT));
    snprintf(entry->key, sizeof(entry->key), "%s", key);
    entry->key[sizeof(entry->key) - 1] = '\0';
    entry->transport = transport;
    snprintf(entry->address, sizeof(entry->address), "%s", address ? address : "");
    entry->address[sizeof(entry->address) - 1] = '\0';
    entry->port = port;
    entry->type = type;
    entry->is_server = is_server;
    entry->consecutive_send_failures = 0;
    entry->total_send_success = 0;
    entry->total_send_failures = 0;
    entry->backpressure_drops = 0;
    entry->circuit_open_rejections = 0;
    entry->in_flight_sends = 0;
    entry->send_high_watermark = manager->send_high_watermark;
    entry->send_low_watermark = manager->send_low_watermark;
    entry->backpressure_active = ZOO_FALSE;
    entry->circuit_open_until = 0;

    ZOO_ERROR_TYPE ret = zoo_list_push_back(bucket_list, entry);
    if (ret != ZOO_SMB_OK)
    {
        zoo_free_to_pool(entry);
    }
    UNLOCK_MANAGER(manager);
    return ret;
}

/**
 * @brief Find transport handle from transport index.
 * @param manager Transport manager handle
 * @param address Transport address string
 * @param port Transport port
 * @param type Transport type
 * @return ZOO_SMB_TRANSPORT_HANDLE Transport handle, NULL if not found
 */
static ZOO_SMB_TRANSPORT_HANDLE transport_index_find(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    const char* address,
    ZOO_U16 port,
    ZOO_SMB_TRANSPORT_TYPE_ENUM type,
    ZOO_BOOL is_server)
{
    if (!manager)
    {
        return NULL;
    }

    char key[2 * MAX_TRANSPORT_ADDRESS_LENGTH + 40] = {0};
    make_transport_index_key(key, sizeof(key), address, port, type, is_server);
    ZOO_USIZE bucket = transport_index_hash(key);

    LOCK_MANAGER(manager);
    ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT*)zoo_list_find_if(
        manager->transport_index_buckets[bucket],
        compare_transport_index_entry_by_key,
        key);
    ZOO_SMB_TRANSPORT_HANDLE transport = entry ? entry->transport : NULL;
    UNLOCK_MANAGER(manager);
    return transport;
}

/**
 * @brief Find transport index entry by transport handle.
 * @param manager Transport manager handle
 * @param transport Transport handle
 * @return Entry pointer if found, otherwise NULL
 */
static ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* transport_index_find_entry_by_transport(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_TRANSPORT_HANDLE transport)
{
    if (!manager || !transport)
    {
        return NULL;
    }

    LOCK_MANAGER(manager);

    for (ZOO_USIZE i = 0; i < TRANSPORT_INDEX_BUCKET_COUNT; i++)
    {
        ZOO_LIST_HANDLE bucket = manager->transport_index_buckets[i];
        if (!bucket)
        {
            continue;
        }

        ZOO_USIZE bucket_size = zoo_list_size(bucket);
        for (ZOO_USIZE j = 0; j < bucket_size; j++)
        {
            ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT*)zoo_list_at(bucket, j);
            if (entry && entry->transport == transport)
            {
                UNLOCK_MANAGER(manager);
                return entry;
            }
        }
    }

    UNLOCK_MANAGER(manager);
    return NULL;
}

/**
 * @brief Destroy all transport index buckets and entries.
 * @param manager Transport manager handle
 * @return void
 */
static void destroy_transport_index(ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager)
{
    if (!manager)
    {
        return;
    }

    for (ZOO_USIZE i = 0; i < TRANSPORT_INDEX_BUCKET_COUNT; i++)
    {
        ZOO_LIST_HANDLE bucket = manager->transport_index_buckets[i];
        if (!bucket)
        {
            continue;
        }

        while (!zoo_list_empty(bucket))
        {
            ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT*)zoo_list_pop_front(bucket);
            if (entry)
            {
                zoo_free_to_pool(entry);
            }
        }

        zoo_list_destroy(bucket);
        manager->transport_index_buckets[i] = NULL;
    }
}

/**
 * @brief Finds a SMB transport handle by its address.
 *
 * This function searches for an existing SMB transport handle that matches
 * the specified address. It is typically used to retrieve a handle for
 * an already established transport connection.
 *
 * @param address The address to search for (type and details depend on implementation).
 * @return ZOO_SMB_TRANSPORT_HANDLE The handle to the transport if found, or NULL/invalid handle if not found.
 */
static ZOO_SMB_TRANSPORT_HANDLE find_transport_by_service(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!manager || !service)
    {
        return NULL;
    }

    ZOO_SMB_TRANSPORT_HANDLE indexed = transport_index_find(
        manager,
        service->address,
        service->port,
        service->transport_type,
        service->is_server);
    if (indexed)
    {
        return indexed;
    }

    for (ZOO_USIZE i = 0; i < zoo_list_size(manager->transport_list); ++i)
    {
        ZOO_SMB_TRANSPORT_HANDLE transport = (ZOO_SMB_TRANSPORT_HANDLE)zoo_list_at(manager->transport_list, i);
        ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config = zoo_smb_transport_get_config(transport);
        if (config &&
            transport &&
            strcmp(config->address, service->address) == 0 &&
            config->port == service->port &&
            config->type == service->transport_type &&
            config->is_server == service->is_server)
        {
            ZOO_LOG_DEBUG("Found transport for address '%s:%d' of type %d", service->address, service->port, service->transport_type);
            (void)transport_index_upsert(
                manager,
                service->address,
                service->port,
                service->transport_type,
                service->is_server,
                transport);
            return transport;
        }
    }
    return NULL;
}

/**
 * @brief Initializes the broadcast transport mechanism for SMB communication.
 *
 * This function sets up the necessary resources and configuration to enable
 * broadcast-based transport according to the provided global SMB configuration.
 *
 * @param global_config Pointer to a constant ZOO_SMB_CONFIG_STRUCT containing
 *        the global configuration parameters for SMB transport.
 *
 * @return ZOO_ERROR_TYPE indicating the result of the initialization.
 *         Returns an appropriate error code if initialization fails.
 */
ZOO_ERROR_TYPE init_broadcast_transport(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager,
    const ZOO_SMB_CONFIG_STRUCT* global_config)
{
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT config = {0};
    ZOO_SMB_SERVICE_HANDLE discovery_service = zoo_smb_service_manager_get_service(
        service_manager, "Discovery", ZOO_SMB_SERVICE_TYPE_DISCOVERY);
    if (discovery_service == NULL)
    {
        ZOO_LOG_ERROR("Discovery service not found");
        return ZOO_SMB_ERROR_SERVICE_NOT_FOUND;
    }

    init_transport_config(
        &config, global_config, discovery_service);
    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&config);
    if (!transport)
    {
        ZOO_LOG_ERROR("Failed to create broadcast transport");
        return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
    }

    ZOO_LOG_INFO("Broadcast transport initialized: %s at %s:%d", config.name, config.address, config.port);
    zoo_list_push_back(manager->transport_list, transport);
    (void)transport_index_upsert(manager, config.address, config.port, config.type, config.is_server, transport);
    return zoo_smb_transport_start(transport, ZOO_TRUE);
}

/**
 * @brief Creates a new transport manager instance.
 *
 * @param config Pointer to the configuration structure for the transport manager.
 * @return A handle to the created transport manager, or NULL on failure.
 */
ZOO_SMB_TRANSPORT_MANAGER_HANDLE zoo_smb_create_transport_manager(
    ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager,
    const ZOO_SMB_CONFIG_STRUCT* config)
{
    if (!config)
    {
        ZOO_LOG_ERROR("config is NULL");
        return NULL;
    }

    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager = (ZOO_SMB_TRANSPORT_MANAGER_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_TRANSPORT_MANAGER_STRUCT));
    if (!manager)
    {
        ZOO_LOG_ERROR("allocation failed");
        return NULL;
    }
    manager->global_config = config;
    manager->send_high_watermark = config->sys.send_high_watermark;
    manager->send_low_watermark = config->sys.send_low_watermark;
    if (manager->send_high_watermark == 0)
    {
        manager->send_high_watermark = config->sys.max_thread_queue_size > 0 ? config->sys.max_thread_queue_size : 128;
    }
    if (manager->send_low_watermark == 0 || manager->send_low_watermark >= manager->send_high_watermark)
    {
        manager->send_low_watermark = manager->send_high_watermark / 2;
        if (manager->send_low_watermark == 0)
        {
            manager->send_low_watermark = 1;
        }
    }
    manager->telemetry_enabled = config->sys.enable_transport_telemetry;
    if (!ZOO_MUTEX_INIT(&manager->mutex))
    {
        ZOO_LOG_ERROR("failed to init manager mutex");
        zoo_free_to_pool(manager);
        return NULL;
    }
    atomic_init(&manager->backpressure_active, ZOO_FALSE);
    atomic_init(&manager->in_flight_sends, 0);
    atomic_init(&manager->total_send_success, 0);
    atomic_init(&manager->total_send_failures, 0);
    atomic_init(&manager->backpressure_drops, 0);
    atomic_init(&manager->circuit_open_rejections, 0);
    manager->transport_list = zoo_list_create(256);
    if (!manager->transport_list)
    {
        ZOO_LOG_ERROR("failed to create transport list");
        ZOO_MUTEX_DESTROY(&manager->mutex);
        zoo_free_to_pool(manager);
        return NULL;
    }

    manager->data_observer_list = zoo_list_create(256);
    if (!manager->data_observer_list)
    {
        ZOO_LOG_ERROR("failed to create data observer list");
        zoo_list_destroy(manager->transport_list);
        ZOO_MUTEX_DESTROY(&manager->mutex);
        zoo_free_to_pool(manager);
        return NULL;
    }

    for (ZOO_USIZE i = 0; i < TRANSPORT_INDEX_BUCKET_COUNT; i++)
    {
        manager->transport_index_buckets[i] = zoo_list_create(16);
        if (!manager->transport_index_buckets[i])
        {
            ZOO_LOG_ERROR("failed to create transport index bucket");
            for (ZOO_USIZE j = 0; j < i; j++)
            {
                zoo_list_destroy(manager->transport_index_buckets[j]);
                manager->transport_index_buckets[j] = NULL;
            }
            zoo_list_destroy(manager->data_observer_list);
            zoo_list_destroy(manager->transport_list);
            ZOO_MUTEX_DESTROY(&manager->mutex);
            zoo_free_to_pool(manager);
            return NULL;
        }
    }

    if (ZOO_SMB_OK != init_broadcast_transport(manager, service_manager, config))
    {
        ZOO_LOG_ERROR("failed to initialize broadcast transport");
        destroy_transport_index(manager);
        zoo_list_destroy(manager->data_observer_list);
        zoo_list_destroy(manager->transport_list);
        ZOO_MUTEX_DESTROY(&manager->mutex);
        zoo_free_to_pool(manager);
        return NULL;
    }

    ZOO_LOG_DEBUG("manager created");
    return manager;
}

/**
 * @brief Destroys the specified SMB transport manager and releases all associated resources.
 *
 * @param manager The handle to the SMB transport manager to destroy.
 */
void zoo_smb_destroy_transport_manager(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager)
{
    if (!manager)
    {
        ZOO_LOG_WARN("manager is NULL");
        return;
    }

    if (manager->transport_list)
    {
        for (size_t i = 0; i < zoo_list_size(manager->transport_list); ++i)
        {
            ZOO_SMB_TRANSPORT_HANDLE transport = (ZOO_SMB_TRANSPORT_HANDLE)zoo_list_at(manager->transport_list, i);
            zoo_smb_destroy_transport(transport);
        }
    }
    destroy_transport_index(manager);
    if (manager->data_observer_list)
    {
        zoo_list_destroy(manager->data_observer_list);
    }
    zoo_list_destroy(manager->transport_list);
    ZOO_MUTEX_DESTROY(&manager->mutex);
    zoo_free_to_pool(manager);
    ZOO_LOG_DEBUG("manager destroyed");
}

/**
 * @brief Initializes the transport configuration structure.
 *
 * This function sets up the given ZOO_SMB_TRANSPORT_CONFIG_STRUCT instance
 * using the provided global configuration and service information.
 *
 * @param config Pointer to the transport configuration structure to initialize.
 * @param global_config Pointer to the global SMB configuration structure.
 * @param service Pointer to the SMB service structure.
 * @param is_server Boolean flag indicating if the configuration is for a server (ZOO_TRUE) or client (ZOO_FALSE).
 */
static void init_transport_config(
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config,
    const ZOO_SMB_CONFIG_STRUCT* global_config,
    ZOO_SMB_SERVICE_HANDLE service)
{
    memset(config, 0, sizeof(ZOO_SMB_TRANSPORT_CONFIG_STRUCT));
    config->type = service->transport_type;
    config->timeout_ms = global_config->sys.max_timeout;
    config->is_server = service->is_server;
    config->retry_interval_s = 5;  // Default retry interval
    snprintf(config->address, sizeof(config->address), "%s", service->address);
    config->address[MAX_TRANSPORT_ADDRESS_LENGTH - 1] = '\0';  // Ensure null-termination
    config->port = service->port;
    strncpy(config->name, service->name, sizeof(config->name) - 1);
    config->name[sizeof(config->name) - 1] = '\0';
    config->max_send_attempts = 3;  // Default maximum send attempts
    snprintf(config->multicast_address, sizeof(config->multicast_address), "%s", service->multicast_address);
    config->multicast_address[MAX_TRANSPORT_ADDRESS_LENGTH - 1] = '\0';  // Ensure null-termination
    config->multicast_port = service->multicast_port;
    ZOO_LOG_INFO(
        "init: type=%d, name='%s', addr='%s':%d,  multicast='%s':%d",
        config->type, config->name, config->address, config->port, config->multicast_address, config->multicast_port);
}

/**
 * @brief Creates and initializes a transport for the specified SMB service.
 *
 * This function is responsible for setting up a new transport instance within the given
 * transport manager for the provided SMB service structure.
 *
 * @param manager Handle to the SMB transport manager.
 * @param service Pointer to the SMB service structure for which the transport is to be created.
 */
ZOO_SMB_TRANSPORT_HANDLE zoo_smb_transport_manager_make_transport(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!manager)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return NULL;
    }

    ZOO_SMB_TRANSPORT_CONFIG_STRUCT config;
    init_transport_config(&config, manager->global_config, service);
    ZOO_SMB_TRANSPORT_HANDLE transport = find_transport_by_service(manager, service);
    if (transport)
    {
        ZOO_LOG_DEBUG("transport for address '%s:%d' already exists", config.address, config.port);
        return transport;
    }

    transport = zoo_smb_create_transport(&config);
    if (!transport)
    {
        ZOO_LOG_ERROR("failed to create transport for service '%s'", config.name);
        return NULL;
    }

    if (zoo_list_push_back(manager->transport_list, transport) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("failed to add transport");
        zoo_smb_destroy_transport(transport);
        return NULL;
    }

    (void)transport_index_upsert(manager, config.address, config.port, config.type, config.is_server, transport);

    return transport;
}

/**
 * @brief Starts the SMB transport using the transport manager.
 *
 * This function initializes and starts the SMB transport layer, allowing
 * communication over the SMB protocol. It should be called before attempting
 * any SMB operations that require network transport.
 *
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
 *         Possible values include success or specific error types defined in ZOO_ERROR_TYPE.
 */
ZOO_ERROR_TYPE zoo_smb_transport_manager_start_transport(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_TRANSPORT_HANDLE transport)
{
    if (!manager || !transport)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_LOG_DEBUG("starting transport '%s'", zoo_smb_transport_get_name(transport));
    if (zoo_smb_transport_start(transport, ZOO_TRUE))
    {
        ZOO_LOG_ERROR("failed to start transport '%s'", zoo_smb_transport_get_name(transport));
        return ZOO_SMB_ERROR_TRANSPORT_START_FAILED;
    }

    if (!wait_for_transport_started(manager, transport))
    {
        ZOO_LOG_ERROR("transport '%s' start wait timed out", zoo_smb_transport_get_name(transport));
        return ZOO_SMB_ERROR_TRANSPORT_START_FAILED;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Retrieves a transport handle for the specified service.
 *
 * @param manager      Handle to the transport manager instance.
 * @param service_info Pointer to a structure containing information about the service.
 * @return ZOO_SMB_TRANSPORT_HANDLE associated with the specified service, or NULL if not found.
 */
ZOO_SMB_TRANSPORT_HANDLE zoo_smb_transport_manager_get_broadcast_transport(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager)
{
    if (!manager)
    {
        ZOO_LOG_ERROR("invalid parameters, manager: %p", manager);
        return NULL;
    }

    return zoo_list_front(manager->transport_list);
}

/**
 * @brief Retrieves the name of the specified SMB transport.
 *
 * This function returns a constant string representing the name of the
 * transport associated with the given ZOO_SMB_TRANSPORT_HANDLE.
 *
 * @param transport The handle to the SMB transport whose name is to be retrieved.
 * @return A constant character pointer to the transport's name. The returned
 *         string is managed internally and must not be freed or modified by the caller.
 */
const char* zoo_smb_transport_manager_get_transport_name(ZOO_SMB_TRANSPORT_HANDLE transport)
{
    if (!transport)
    {
        ZOO_LOG_ERROR("invalid transport handle");
        return NULL;
    }
    return zoo_smb_transport_get_name(transport);
}
/**
 * @brief Registers an observer for incoming SMB transport data.
 *
 * This function allows a client to register a callback or observer that will be notified
 * whenever new data is received by the SMB transport manager.
 *
 * @param manager Transport manager handle.
 * @param transport Transport instance where observer is attached.
 * @param observer Observer callback/context handle.
 * @return ZOO_SMB_OK on success, ZOO_SMB_ERROR_INVALID_PARAM for invalid inputs,
 *         or transport-level registration error.
 */
ZOO_ERROR_TYPE zoo_smb_transport_manager_register_incoming_data_observer(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_TRANSPORT_HANDLE transport,
    TRANSPORT_DATA_OBSERVER_HANDLE observer)
{
    if (!manager || !observer || !transport)
    {
        ZOO_LOG_ERROR("invalid parameters,manager: %p, transport: %p, observer: %p", manager, transport, observer);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    return zoo_smb_transport_add_data_observer(transport, observer);
}

/**
 * @brief Unregisters an observer for incoming data events in the SMB transport manager.
 *
 * This function removes a previously registered observer that was receiving
 * notifications or callbacks when new data arrives via the SMB transport.
 *
 * @param observer The observer instance to unregister.
 *
 * @note After calling this function, the specified observer will no longer
 *       receive incoming data notifications.
 */
void zoo_smb_transport_manager_unregister_incoming_data_observer(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_TRANSPORT_HANDLE transport,
    TRANSPORT_DATA_OBSERVER_HANDLE observer)
{
    if (!manager || !observer || !transport)
    {
        ZOO_LOG_ERROR("invalid parameters, manager: %p, observer: %p, transport: %p", manager, observer, transport);
        return;
    }

    zoo_smb_transport_remove_data_observer(transport, observer);
}

/**
 * @brief Sends one message through selected transport.
 *
 * Security policy checks are enforced before send. Telemetry counters are
 * updated for success/failure and backpressure outcomes.
 *
 * @param manager Transport manager handle.
 * @param transport Transport instance used for send.
 * @param message Message object to transmit.
 * @param receiver Optional receiver identity; may be NULL for multicast/broadcast paths.
 * @return ZOO_SMB_OK on success, or module error code on validation/security/
 *         transport failure.
 */
ZOO_ERROR_TYPE zoo_smb_transport_manager_send_message(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_TRANSPORT_HANDLE transport,
    const ZOO_SMB_MSG_STRUCT* message,
    const char* receiver)
{
    if (!manager || !transport || !message)
    {
        ZOO_LOG_ERROR("invalid parameters, manager: %p, transport: %p, message: %p, receiver: %p",
                          manager,
                          transport,
                          message,
                          receiver);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (manager->global_config && manager->global_config->sys.require_sender_identity && message->header.sender[0] == '\0')
    {
        if (manager->telemetry_enabled)
        {
            atomic_fetch_add(&manager->total_send_failures, 1);
        }
        ZOO_LOG_ERROR("sender identity is required by security policy");
        return ZOO_SMB_ERROR_AUTH_REQUIRED;
    }

    if (manager->global_config && manager->global_config->sys.enforce_encrypted_messages &&
        ((message->header.flags & ZOO_SMB_MSG_FLAG_ENCRYPT) == 0))
    {
        if (manager->telemetry_enabled)
        {
            atomic_fetch_add(&manager->total_send_failures, 1);
        }
        ZOO_LOG_ERROR("non-encrypted message rejected by security policy");
        return ZOO_SMB_ERROR_ENCRYPT_FAILED;
    }

    if (!zoo_smb_transport_is_started(transport))
    {
        const char* transport_name = zoo_smb_transport_get_name(transport);
        ZOO_LOG_ERROR("transport '%s' is not started", transport_name);
        return ZOO_SMB_ERROR_NOT_INITIALIZED;
    }

    ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* entry = transport_index_find_entry_by_transport(manager, transport);
    time_t now = time(NULL);
    LOCK_MANAGER(manager);
    if (entry && entry->circuit_open_until > now)
    {
        if (manager->telemetry_enabled)
        {
            atomic_fetch_add(&manager->circuit_open_rejections, 1);
            atomic_fetch_add(&manager->total_send_failures, 1);
        }
        entry->circuit_open_rejections++;
        entry->total_send_failures++;
        UNLOCK_MANAGER(manager);
        ZOO_LOG_WARN("transport '%s' send circuit open, cooldown remaining=%llds",
                         zoo_smb_transport_get_name(transport),
                         (long long)(entry->circuit_open_until - now));
        return ZOO_SMB_ERROR_TIMEOUT;
    }

    if (entry)
    {
        ZOO_U32 transport_in_flight = ++entry->in_flight_sends;
        ZOO_U32 transport_high = entry->send_high_watermark > 0 ? entry->send_high_watermark : manager->send_high_watermark;
        ZOO_U32 transport_low = entry->send_low_watermark > 0 ? entry->send_low_watermark : manager->send_low_watermark;

        if (transport_in_flight > transport_high)
        {
            --entry->in_flight_sends;
            if (manager->telemetry_enabled)
            {
                atomic_fetch_add(&manager->backpressure_drops, 1);
                atomic_fetch_add(&manager->total_send_failures, 1);
            }
            entry->backpressure_drops++;
            entry->total_send_failures++;
            entry->backpressure_active = ZOO_TRUE;
            UNLOCK_MANAGER(manager);
            ZOO_LOG_WARN("transport '%s' local backpressure active: inflight=%u high=%u low=%u",
                             zoo_smb_transport_get_name(transport),
                             transport_in_flight,
                             transport_high,
                             transport_low);
            return ZOO_SMB_ERROR_QUEUE_FULL;
        }
    }
    UNLOCK_MANAGER(manager);

    ZOO_U32 in_flight = (ZOO_U32)atomic_fetch_add(&manager->in_flight_sends, 1) + 1;
    if (in_flight > manager->send_high_watermark)
    {
        (void)atomic_fetch_sub(&manager->in_flight_sends, 1);
        if (entry)
        {
            LOCK_MANAGER(manager);
            --entry->in_flight_sends;
            entry->backpressure_drops++;
            entry->total_send_failures++;
            entry->backpressure_active = ZOO_TRUE;
            UNLOCK_MANAGER(manager);
        }
        if (manager->telemetry_enabled)
        {
            atomic_fetch_add(&manager->backpressure_drops, 1);
            atomic_fetch_add(&manager->total_send_failures, 1);
        }

        if (!atomic_exchange(&manager->backpressure_active, ZOO_TRUE))
        {
            ZOO_LOG_WARN("transport manager backpressure active: inflight=%u high=%u low=%u",
                             in_flight,
                             manager->send_high_watermark,
                             manager->send_low_watermark);
        }
        return ZOO_SMB_ERROR_QUEUE_FULL;
    }

    ZOO_ERROR_TYPE ret = zoo_smb_transport_send(transport, message, receiver);
    ZOO_U32 remain = (ZOO_U32)atomic_fetch_sub(&manager->in_flight_sends, 1) - 1;
    ZOO_U32 transport_remain = 0;
    if (entry)
    {
        LOCK_MANAGER(manager);
        transport_remain = --entry->in_flight_sends;
        ZOO_U32 transport_low = entry->send_low_watermark > 0 ? entry->send_low_watermark : manager->send_low_watermark;
        if (entry->backpressure_active && transport_remain <= transport_low)
        {
            entry->backpressure_active = ZOO_FALSE;
            UNLOCK_MANAGER(manager);
            ZOO_LOG_INFO("transport '%s' local backpressure released: inflight=%u low=%u",
                             zoo_smb_transport_get_name(transport),
                             transport_remain,
                             transport_low);
        }
        else
        {
            UNLOCK_MANAGER(manager);
        }
    }

    if (atomic_load(&manager->backpressure_active) && remain <= manager->send_low_watermark)
    {
        if (atomic_exchange(&manager->backpressure_active, ZOO_FALSE))
        {
            ZOO_LOG_INFO("transport manager backpressure released: inflight=%u low=%u",
                             remain,
                             manager->send_low_watermark);
        }
    }

    if (ZOO_SMB_IS_SUCCESS(ret))
    {
        if (manager->telemetry_enabled)
        {
            atomic_fetch_add(&manager->total_send_success, 1);
        }
        if (entry)
        {
            LOCK_MANAGER(manager);
            entry->total_send_success++;
            entry->consecutive_send_failures = 0;
            entry->circuit_open_until = 0;
            UNLOCK_MANAGER(manager);
        }
        return ret;
    }

    if (entry)
    {
        LOCK_MANAGER(manager);
        entry->total_send_failures++;
        if (transport_send_failure_should_trip_circuit(ret))
        {
            entry->consecutive_send_failures++;
            if (entry->consecutive_send_failures >= TRANSPORT_SEND_FAIL_OPEN_THRESHOLD)
            {
                entry->circuit_open_until = now + TRANSPORT_SEND_OPEN_COOLDOWN_SEC;
                UNLOCK_MANAGER(manager);
                ZOO_LOG_WARN("transport '%s' send circuit opened for %ds after %u consecutive failures",
                                 zoo_smb_transport_get_name(transport),
                                 TRANSPORT_SEND_OPEN_COOLDOWN_SEC,
                                 entry->consecutive_send_failures);
            }
            else
            {
                UNLOCK_MANAGER(manager);
            }
        }
        else
        {
            entry->consecutive_send_failures = 0;
            UNLOCK_MANAGER(manager);
        }
    }

    if (manager->telemetry_enabled)
    {
        atomic_fetch_add(&manager->total_send_failures, 1);
    }

    return ret;
}

/**
 * @brief Broadcasts a message using the SMB transport manager.
 *
 * This function sends a broadcast message to all connected SMB clients.
 *
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the broadcast operation.
 */
ZOO_ERROR_TYPE zoo_smb_transport_manager_broadcast_message(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    const ZOO_SMB_SERVICE_HANDLE service,
    const ZOO_SMB_MSG_STRUCT* message)
{
    ZOO_SMB_UNUSED(service);
    if (!manager || !message)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_TRANSPORT_HANDLE broadcast_transport = zoo_list_front(manager->transport_list);
    if (!broadcast_transport)
    {
        ZOO_LOG_ERROR("no broadcast transport available");
        return ZOO_SMB_ERROR_NOT_FOUND;
    }

    if (!zoo_smb_transport_is_started(broadcast_transport))
    {
        ZOO_LOG_ERROR("broadcast transport is not started");
        return ZOO_SMB_ERROR_NOT_INITIALIZED;
    }

    return zoo_smb_transport_manager_send_message(manager, broadcast_transport, message, NULL);
}

ZOO_ERROR_TYPE zoo_smb_transport_manager_get_metrics(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_TRANSPORT_MANAGER_METRICS_STRUCT* out_metrics)
{
    if (!manager || !out_metrics)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    out_metrics->total_send_success = (uint64_t)atomic_load(&manager->total_send_success);
    out_metrics->total_send_failures = (uint64_t)atomic_load(&manager->total_send_failures);
    out_metrics->backpressure_drops = (uint64_t)atomic_load(&manager->backpressure_drops);
    out_metrics->circuit_open_rejections = (uint64_t)atomic_load(&manager->circuit_open_rejections);
    out_metrics->in_flight_sends = (uint32_t)atomic_load(&manager->in_flight_sends);
    out_metrics->send_high_watermark = manager->send_high_watermark;
    out_metrics->send_low_watermark = manager->send_low_watermark;
    out_metrics->backpressure_active = atomic_load(&manager->backpressure_active);

    return ZOO_SMB_OK;
}

ZOO_ERROR_TYPE zoo_smb_transport_manager_reset_metrics(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager)
{
    if (!manager)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    atomic_store(&manager->total_send_success, 0);
    atomic_store(&manager->total_send_failures, 0);
    atomic_store(&manager->backpressure_drops, 0);
    atomic_store(&manager->circuit_open_rejections, 0);

    LOCK_MANAGER(manager);
    for (ZOO_USIZE i = 0; i < TRANSPORT_INDEX_BUCKET_COUNT; i++)
    {
        ZOO_LIST_HANDLE bucket = manager->transport_index_buckets[i];
        if (!bucket)
        {
            continue;
        }

        ZOO_USIZE bucket_size = zoo_list_size(bucket);
        for (ZOO_USIZE j = 0; j < bucket_size; j++)
        {
            ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT*)zoo_list_at(bucket, j);
            if (!entry)
            {
                continue;
            }
            entry->consecutive_send_failures = 0;
            entry->total_send_success = 0;
            entry->total_send_failures = 0;
            entry->backpressure_drops = 0;
            entry->circuit_open_rejections = 0;
            entry->in_flight_sends = 0;
            entry->backpressure_active = ZOO_FALSE;
            entry->circuit_open_until = 0;
        }
    }
    UNLOCK_MANAGER(manager);

    return ZOO_SMB_OK;
}

ZOO_ERROR_TYPE zoo_smb_transport_manager_get_transport_metrics(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_TRANSPORT_HANDLE transport,
    ZOO_SMB_TRANSPORT_CHANNEL_METRICS_STRUCT* out_metrics)
{
    if (!manager || !transport || !out_metrics)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* entry = transport_index_find_entry_by_transport(manager, transport);
    if (!entry)
    {
        return ZOO_SMB_ERROR_NOT_FOUND;
    }

    LOCK_MANAGER(manager);
    memset(out_metrics, 0, sizeof(ZOO_SMB_TRANSPORT_CHANNEL_METRICS_STRUCT));
    const char* transport_name = zoo_smb_transport_get_name(transport);
    if (transport_name)
    {
        snprintf(out_metrics->transport_name, sizeof(out_metrics->transport_name), "%s", transport_name);
        out_metrics->transport_name[sizeof(out_metrics->transport_name) - 1] = '\0';
    }
    snprintf(out_metrics->address, sizeof(out_metrics->address), "%s", entry->address);
    out_metrics->address[sizeof(out_metrics->address) - 1] = '\0';
    out_metrics->port = entry->port;
    out_metrics->transport_type = (int)entry->type;
    out_metrics->total_send_success = entry->total_send_success;
    out_metrics->total_send_failures = entry->total_send_failures;
    out_metrics->backpressure_drops = entry->backpressure_drops;
    out_metrics->circuit_open_rejections = entry->circuit_open_rejections;
    out_metrics->in_flight_sends = entry->in_flight_sends;
    out_metrics->send_high_watermark = entry->send_high_watermark;
    out_metrics->send_low_watermark = entry->send_low_watermark;
    out_metrics->backpressure_active = entry->backpressure_active;
    out_metrics->circuit_open = entry->circuit_open_until > time(NULL);
    UNLOCK_MANAGER(manager);

    return ZOO_SMB_OK;
}

ZOO_ERROR_TYPE zoo_smb_transport_manager_set_transport_watermarks(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE manager,
    ZOO_SMB_TRANSPORT_HANDLE transport,
    uint32_t high_watermark,
    uint32_t low_watermark)
{
    if (!manager || !transport || high_watermark == 0)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (low_watermark == 0 || low_watermark >= high_watermark)
    {
        low_watermark = high_watermark / 2;
        if (low_watermark == 0)
        {
            low_watermark = 1;
        }
    }

    ZOO_SMB_TRANSPORT_INDEX_ENTRY_STRUCT* entry = transport_index_find_entry_by_transport(manager, transport);
    if (!entry)
    {
        return ZOO_SMB_ERROR_NOT_FOUND;
    }

    LOCK_MANAGER(manager);
    entry->send_high_watermark = high_watermark;
    entry->send_low_watermark = low_watermark;
    UNLOCK_MANAGER(manager);

    return ZOO_SMB_OK;
}