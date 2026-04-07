/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT
 * File name: zoo_smb_transport.c
 * Description: Implementation of the transport layer interface for ZOO SMB
 ******************************************************************************/

#include "zoo_smb_transport.h"
#include "../../memory_pool/inc/zoo_memory_pool.h"
#include "../../thread_pool/inc/zoo_thread_pool.h"
#include "zoo_smb_transport_udp.h"
#include "zoo_smb_transport_udp_broadcast.h"
#include "zoo_smb_transport_tcp.h"
#include "zoo_smb_transport_shm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
/* Static array for registered transport ops */
static ZOO_SMB_TRANSPORT_OPS_STRUCT* g_transport_ops[ZOO_SMB_TRANSPORT_TYPE_MAX] = {0};

static ZOO_BOOL transport_should_retry_send(ZOO_ERROR_TYPE err)
{
    return (err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED ||
            err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_NETWORK_SEND_FAILED ||
            err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_TIMEOUT);
}

/**
 * @brief Task handler for transport operations.
 *
 * This function is intended to be run as a task/thread and handles transport-related
 * operations. It receives user-specific data and an argument buffer.
 *
 * @param user_data Pointer to user-defined data passed to the task.
 * @param argument Pointer to an argument buffer for the task.
 * @param argment_len Length of the argument buffer in bytes.
 */
static ZOO_ERROR_TYPE transport_task(void* user_data, void* argument)
{
    ZOO_SMB_UNUSED(argument);
    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)user_data;
    return g_transport_ops[transport->config->type]->start(transport->impl);
}

/**
 * @brief Internal helper to allocate and initialize a transport structure.
 *
 * @param config Pointer to the transport configuration.
 * @return Pointer to the allocated and partially initialized transport struct, or NULL on failure.
 */
static ZOO_SMB_TRANSPORT_STRUCT* zoo_smb_transport_alloc_and_init(const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config)
{
    if (!config)
    {
        ZOO_LOG_ERROR("Invalid config:%p", config);
        return NULL;
    }

    if (config->type <= ZOO_SMB_TRANSPORT_TYPE_MIN || config->type >= ZOO_SMB_TRANSPORT_TYPE_MAX)
    {
        ZOO_LOG_ERROR("Invalid transport type:%d, range(%d, %d)", config->type, ZOO_SMB_TRANSPORT_TYPE_MIN, ZOO_SMB_TRANSPORT_TYPE_MAX);
        return NULL;
    }

    ZOO_LOG_INFO("Transport config: name=%s, type=%d, address=%s, port=%d",
                     config->name,
                     config->type,
                     config->address,
                     config->port);

    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_TRANSPORT_STRUCT));
    if (!transport)
    {
        ZOO_LOG_ERROR("Failed to allocate transport");
        return NULL;
    }

    memset(transport, 0, sizeof(ZOO_SMB_TRANSPORT_STRUCT));
    snprintf(transport->name, sizeof(transport->name), "%s", config->name);
    transport->name[sizeof(transport->name) - 1] = '\0';

    return transport;
}

/**
 * @brief Internal helper to create all observer and consumer lists for a transport.
 *
 * @param transport Pointer to the transport struct.
 * @return ZOO_TRUE on success, ZOO_FALSE on failure.
 */
static ZOO_BOOL zoo_smb_transport_create_lists(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (!ZOO_MUTEX_INIT(&transport->data_observers_lock))
    {
        ZOO_LOG_ERROR("Failed to initialize data_observers_lock");
        return ZOO_FALSE;
    }

    transport->data_observers = zoo_list_create(MAX_TRANSPORT_DATA_OBSERVER_SIZE);
    if (!transport->data_observers)
    {
        ZOO_LOG_ERROR("Failed to create data_observers list");
        ZOO_MUTEX_DESTROY(&transport->data_observers_lock);
        return ZOO_FALSE;
    }
    return ZOO_TRUE;
}

/**
 * @brief Internal helper to allocate and copy transport config.
 *
 * @param transport Pointer to the transport struct.
 * @param config Pointer to the source config.
 * @return ZOO_TRUE on success, ZOO_FALSE on failure.
 */
static ZOO_BOOL zoo_smb_transport_copy_config(ZOO_SMB_TRANSPORT_STRUCT* transport, const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config)
{
    transport->config = (ZOO_SMB_TRANSPORT_CONFIG_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_TRANSPORT_CONFIG_STRUCT));
    if (!transport->config)
    {
        ZOO_LOG_ERROR("Failed to allocate config");
        return ZOO_FALSE;
    }
    memcpy(transport->config, config, sizeof(ZOO_SMB_TRANSPORT_CONFIG_STRUCT));
    return ZOO_TRUE;
}

/**
 * @brief Internal helper to initialize the transport implementation.
 *
 * @param transport Pointer to the transport struct.
 * @param config Pointer to the transport config.
 * @return ZOO_ERROR_TYPE error code.
 */
static ZOO_ERROR_TYPE zoo_smb_transport_init_impl(ZOO_SMB_TRANSPORT_STRUCT* transport, const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config)
{
    switch (config->type)
    {
        case ZOO_SMB_TRANSPORT_TYPE_UDP:
            zoo_smb_transport_udp_init();
            break;
        case ZOO_SMB_TRANSPORT_TYPE_UDP_BROADCAST:
            zoo_smb_transport_udp_broadcast_init();
            break;
        case ZOO_SMB_TRANSPORT_TYPE_TCP:
            zoo_smb_transport_tcp_init();
            break;
        case ZOO_SMB_TRANSPORT_TYPE_SHM:
            zoo_smb_transport_shm_init();
            break;
        default:
            ZOO_LOG_ERROR("Unsupported transport type %d", config->type);
            return ZOO_SMB_ERROR_NOT_SUPPORTED;
    }
    return g_transport_ops[config->type]->init(transport);
}

/**
 * @brief Clean up a partially created transport structure.
 *
 * @param transport Pointer to the transport struct.
 */
static void zoo_smb_transport_cleanup(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (!transport)
        return;
    if (transport->config)
        zoo_free_to_pool(transport->config);
    if (transport->data_observers)
        zoo_list_destroy(transport->data_observers);
    ZOO_MUTEX_DESTROY(&transport->data_observers_lock);
    zoo_free_to_pool(transport);
}

/**
 * @brief Free all resources associated with a transport implementation.
 *
 * @param transport Pointer to the transport struct.
 */
static void zoo_smb_transport_free_impl(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    if (g_transport_ops[transport->config->type] && g_transport_ops[transport->config->type]->destroy)
    {
        g_transport_ops[transport->config->type]->destroy(transport->impl);
        ZOO_LOG_INFO("transport implementation destroyed for '%s'", transport->name);
    }
}

/**
 * @brief Register a transport implementation.
 *
 * @param type Transport type.
 * @param ops Pointer to the transport operations structure.
 */
void zoo_smb_transport_register(
    ZOO_SMB_TRANSPORT_TYPE_ENUM type,
    ZOO_SMB_TRANSPORT_OPS_STRUCT* ops)
{
    if (!ops || type <= ZOO_SMB_TRANSPORT_TYPE_MIN || type >= ZOO_SMB_TRANSPORT_TYPE_MAX)
    {
        ZOO_LOG_ERROR("Invalid parameters, type=%d", type);
        return;
    }
    ZOO_LOG_INFO("Registering transport type %d", type);
    if (type >= 0 && type < ZOO_SMB_TRANSPORT_TYPE_MAX)
        g_transport_ops[type] = ops;
}

/**
 * @brief Create a transport instance.
 *
 * @param config Pointer to the transport configuration.
 * @return Handle to the created transport instance, or NULL on failure.
 */
ZOO_SMB_TRANSPORT_HANDLE zoo_smb_create_transport(const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* config)
{
    ZOO_SMB_TRANSPORT_STRUCT* transport = zoo_smb_transport_alloc_and_init(config);
    if (!transport)
    {
        ZOO_LOG_ERROR("transport allocation/init failed");
        return NULL;
    }

    if (!zoo_smb_transport_create_lists(transport))
    {
        ZOO_LOG_ERROR("list creation failed");
        zoo_free_to_pool(transport);
        return NULL;
    }

    if (!zoo_smb_transport_copy_config(transport, config))
    {
        ZOO_LOG_ERROR("config copy failed");
        zoo_smb_transport_cleanup(transport);
        return NULL;
    }

    ZOO_ERROR_TYPE err = zoo_smb_transport_init_impl(transport, config);
    if (err != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("transport implementation init failed, err=%d", err);
        zoo_smb_transport_cleanup(transport);
        return NULL;
    }

    ZOO_LOG_INFO("transport '%s' created successfully", transport->name);
    return (ZOO_SMB_TRANSPORT_HANDLE)transport;
}

/**
 * @brief Destroy a transport instance and release all associated resources.
 *
 * This function cleans up all memory and resources associated with the given
 * transport handle, including observer lists, consumer lists, and transport
 * implementation. After calling this function, the handle must not be used.
 *
 * @param handle Handle to the transport instance to destroy.
 */
void zoo_smb_destroy_transport(ZOO_SMB_TRANSPORT_HANDLE handle)
{
    if (!handle)
    {
        ZOO_LOG_WARN("handle is NULL");
        return;
    }

    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;
    zoo_smb_transport_stop(transport);
    zoo_smb_transport_free_impl(transport);
    zoo_smb_transport_cleanup(transport);
}

/**
 * @brief Register a message receive callback (data observer).
 *
 * Registers a callback function to be invoked when a message is received on the transport.
 *
 * @param handle   Transport handle.
 * @param observer Callback function to register.
 * @param user_data User data to pass to the callback.
 * @return Error code.
 */
ZOO_ERROR_TYPE zoo_smb_transport_add_data_observer(
    ZOO_SMB_TRANSPORT_HANDLE handle,
    TRANSPORT_DATA_OBSERVER_HANDLE observer)
{
    if (!handle || !observer)
    {
        ZOO_LOG_ERROR("Invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;

    ZOO_MUTEX_LOCK(&transport->data_observers_lock);
    TRANSPORT_DATA_OBSERVER_HANDLE exist_observer = find_transport_data_observer(transport->data_observers, observer->handler);
    if (!exist_observer)
    {
        add_transport_data_observer(transport->data_observers, observer);
    }
    ZOO_MUTEX_UNLOCK(&transport->data_observers_lock);

    ZOO_LOG_INFO("Observer %p registered successfully", observer);
    return ZOO_SMB_OK;
}

/**
 * @brief Remove a message receive callback (data observer).
 *
 * Unregisters a previously registered message receive callback from the transport.
 *
 * @param handle   Transport handle.
 * @param observer Callback function to remove.
 */
void zoo_smb_transport_remove_data_observer(
    ZOO_SMB_TRANSPORT_HANDLE handle,
    TRANSPORT_DATA_OBSERVER_HANDLE observer)
{
    if (!handle || !observer)
    {
        ZOO_LOG_ERROR("Invalid parameters, handle=%p, observer=%p", handle, observer);
        return;
    }

    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;
    ZOO_MUTEX_LOCK(&transport->data_observers_lock);
    remove_transport_data_observer(transport->data_observers, observer);
    ZOO_MUTEX_UNLOCK(&transport->data_observers_lock);
    ZOO_LOG_DEBUG("Observer %p removed successfully", observer);
}

/**
 * @brief Checks if the SMB transport has been started.
 *
 * This function determines whether the specified SMB transport handle
 * is currently in a started state.
 *
 * @param handle The handle to the SMB transport instance.
 * @return ZOO_TRUE if the transport is started, ZOO_FALSE otherwise.
 */
ZOO_BOOL zoo_smb_transport_is_started(ZOO_SMB_TRANSPORT_HANDLE handle)
{
    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;
    return g_transport_ops[transport->config->type]->is_started(transport->impl);
}

/**
 * @brief Start the transport.
 *
 * @param handle Transport handle.
 * @return Error code.
 */
ZOO_ERROR_TYPE zoo_smb_transport_start(ZOO_SMB_TRANSPORT_HANDLE handle, ZOO_BOOL background)
{
    if (!handle)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;
    if (!g_transport_ops[transport->config->type])
        return ZOO_SMB_ERROR_NOT_SUPPORTED;
    ZOO_LOG_INFO("Starting transport '%s'", transport->name);
    if (background)
        return zoo_thread_pool_submit_task(transport->config->name, transport_task, handle, NULL, ZOO_FALSE);
    else
        return g_transport_ops[transport->config->type]->start(transport->impl);
}

/**
 * @brief Stop the transport.
 *
 * @param handle Transport handle.
 * @return Error code.
 */
ZOO_ERROR_TYPE zoo_smb_transport_stop(ZOO_SMB_TRANSPORT_HANDLE handle)
{
    ZOO_LOG_DEBUG("Stopping transport");
    if (!handle)
        return ZOO_SMB_ERROR_INVALID_PARAM;
    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;
    if (!g_transport_ops[transport->config->type] || !g_transport_ops[transport->config->type]->stop)
        return ZOO_SMB_ERROR_NOT_SUPPORTED;
    return g_transport_ops[transport->config->type]->stop(transport->impl);
}

/**
 * @brief Send a message.
 *
 * @param handle Transport handle.
 * @param msg Pointer to the message data.
 * @param size Size of the message in bytes.
 * @param context Pointer to the context address.
 * @return Error code.
 */
ZOO_ERROR_TYPE zoo_smb_transport_send(
    ZOO_SMB_TRANSPORT_HANDLE handle,
    const ZOO_SMB_MSG_STRUCT* msg,
    const char* receiver)
{
    if (!handle || !msg)
    {
        ZOO_LOG_ERROR("Invalid parameters, handle=%p, msg=%p", handle, msg);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;
    if (!g_transport_ops[transport->config->type] || !g_transport_ops[transport->config->type]->send)
    {
        ZOO_LOG_ERROR("Transport type %d does not support send operation", transport->config->type);
        return ZOO_SMB_ERROR_NOT_SUPPORTED;
    }

    uint32_t max_attempts = (transport->config && transport->config->max_send_attempts > 0)
                                ? transport->config->max_send_attempts
                                : 1u;
    uint32_t retry_interval_us = (transport->config && transport->config->retry_interval_s > 0)
                                     ? (uint32_t)transport->config->retry_interval_s * 1000000u
                                     : 0u;

    ZOO_ERROR_TYPE last_err = ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    for (uint32_t attempt = 0; attempt < max_attempts; ++attempt)
    {
        last_err = g_transport_ops[transport->config->type]->send(transport->impl, msg, receiver);
        if (ZOO_SMB_IS_SUCCESS(last_err))
        {
            return last_err;
        }

        if (attempt + 1u >= max_attempts || !transport_should_retry_send(last_err))
        {
            break;
        }

        if (retry_interval_us > 0)
        {
            usleep(retry_interval_us);
        }
    }

    return last_err;
}

/**
 * @brief Get the transport type.
 *
 * @param handle Transport handle.
 * @return Transport type.
 */
ZOO_SMB_TRANSPORT_TYPE_ENUM zoo_smb_transport_get_type(
    ZOO_SMB_TRANSPORT_HANDLE handle)
{
    if (!handle)
        return ZOO_SMB_TRANSPORT_TYPE_MAX;
    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;
    return transport->config->type;
}

/**
 * @brief Get the transport configuration.
 *
 * @param handle Transport handle.
 * @return Pointer to the transport configuration.
 */
ZOO_SMB_TRANSPORT_CONFIG_STRUCT* zoo_smb_transport_get_config(
    ZOO_SMB_TRANSPORT_HANDLE handle)
{
    if (!handle)
        return NULL;
    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;
    return transport->config;
}

/**
 * @brief Retrieves the name of the SMB transport associated with the given handle.
 *
 * @param handle The handle to the SMB transport instance.
 * @return A constant pointer to a string containing the name of the SMB transport.
 *         The returned string is managed by the transport and must not be freed by the caller.
 */
const char* zoo_smb_transport_get_name(ZOO_SMB_TRANSPORT_HANDLE handle)
{
    ZOO_SMB_TRANSPORT_STRUCT* transport = (ZOO_SMB_TRANSPORT_STRUCT*)handle;
    if (!transport || !transport->config)
        return NULL;
    return transport->config->name;
}
