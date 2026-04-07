/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SERVICE
 * File name: zoo_smb_service.c
 * Description: Implementation of service information management for ZOO SMB
 *              Provides creation, registration, update, query, and destruction
 *              of SMB service information structures.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-01     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_service.h"
#include "zoo_memory_pool.h"
#include "zoo_list.h"
#include "zoo_log.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Creates and initializes a new ZOO_SMB_SERVICE_STRUCT instance.
 *
 * @param name           The name of the service.
 * @param topic          The topic associated with the service.
 * @param transport_type The transport protocol type used by the service.
 * @return ZOO_SMB_SERVICE_HANDLE Pointer to the newly created service info, or NULL on failure.
 */
ZOO_SMB_SERVICE_HANDLE zoo_smb_create_service(
    ZOO_SMB_SERVICE_TYPE_ENUM service_type,
    const char* name,
    const char* topic,
    const char* address,
    int port,
    const char* multicast_address,
    int multicast_port,
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type,
    uint32_t broadcast_interval_ms)
{
    if (!name || !topic)
    {
        ZOO_LOG_ERROR("zoo_smb_create_service: name or topic is NULL");
        return NULL;
    }
    ZOO_SMB_SERVICE_HANDLE info = (ZOO_SMB_SERVICE_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_SERVICE_STRUCT));
    if (!info)
    {
        ZOO_LOG_ERROR("zoo_smb_create_service: allocation failed");
        return NULL;
    }
    memset(info, 0, sizeof(ZOO_SMB_SERVICE_STRUCT));
    if (service_type == ZOO_SMB_SERVICE_TYPE_DISCOVERY ||
        service_type == ZOO_SMB_SERVICE_TYPE_BROADCAST)
    {
        info->is_server = ZOO_TRUE;  // Discovery service is always a server
    }

    info->service_type = service_type;
    snprintf(info->name, sizeof(info->name), "%s", name);
    info->name[sizeof(info->name) - 1] = '\0';  // Ensure null-termination
    snprintf(info->topic, sizeof(info->topic), "%s", topic);
    info->topic[sizeof(info->topic) - 1] = '\0';  // Ensure null-termination
    snprintf(info->address, sizeof(info->address), "%s", address);
    info->address[sizeof(info->address) - 1] = '\0';  // Ensure null-termination
    info->port = port;
    snprintf(info->multicast_address, sizeof(info->multicast_address), "%s", multicast_address);
    info->multicast_address[sizeof(info->multicast_address) - 1] = '\0';  // Ensure null-termination
    info->multicast_port = multicast_port;
    info->transport_type = transport_type;
    info->is_online = ZOO_TRUE;
    info->last_seen = time(NULL);
    info->broadcast_interval_ms = broadcast_interval_ms < 1000 ? 1000 : broadcast_interval_ms;  // Ensure minimum interval of 1 second
    ZOO_LOG_DEBUG(
        "Service:'%s',topic:'%s', addr=%s:%d, type=%d, multicast=%s:%d",
        info->name,
        info->topic,
        info->address,
        info->port,
        info->transport_type,
        info->multicast_address,
        info->multicast_port);
    return info;
}

/**
 * @brief Registers SMB service information with the Zoo SMB service.
 *
 * @param service_list The linked list handle for service registry.
 * @param service The service info handle to register.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the registration.
 */
ZOO_ERROR_TYPE zoo_smb_register_service(
    ZOO_LIST_HANDLE service_list,
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!service_list || !service)
    {
        ZOO_LOG_ERROR("zoo_smb_register_service: invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    if (zoo_list_push_back(service_list, service) != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("zoo_smb_register_service: failed to register service");
        return ZOO_SMB_ERROR_OPERATION_FAILED;
    }
    ZOO_LOG_INFO("zoo_smb_register_service: service '%s' registered", service->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Unregisters the SMB service information from the Zoo service registry.
 *
 * @param service_list The linked list handle for service registry.
 * @param service The service info handle to unregister.
 */
void zoo_smb_unregister_service(
    ZOO_LIST_HANDLE service_list,
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!service_list || !service)
    {
        ZOO_LOG_ERROR("zoo_smb_unregister_service: invalid parameters");
        return;
    }
    if (zoo_list_remove(service_list, service) == ZOO_SMB_OK)
    {
        ZOO_LOG_INFO("zoo_smb_unregister_service: service '%s' unregistered", service->name);
    }
    else
    {
        ZOO_LOG_WARN("zoo_smb_unregister_service: service '%s' not found", service->name);
    }
}

/**
 * @brief Destroys and frees resources associated with a SMB service info structure.
 *
 * @param service Pointer to the SMB service info structure to be destroyed.
 */
void zoo_smb_destroy_service(
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!service)
    {
        ZOO_LOG_WARN("service is NULL");
        return;
    }
    ZOO_LOG_DEBUG("destroying service '%s'", service->name);
    zoo_free_to_pool(service);
}

/**
 * @brief Destroys the SMB service information and removes associated resources.
 *
 * @param service_list The linked list handle for service registry.
 * @param service The service info handle to destroy and remove.
 */
void zoo_smb_destroy_service_and_remove(
    ZOO_LIST_HANDLE service_list,
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!service_list || !service)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return;
    }
    zoo_smb_unregister_service(service_list, service);
    zoo_smb_destroy_service(service);
}

/**
 * @brief Destroys and frees all memory associated with a linked list of SMB service information.
 *
 * This function iterates through the provided service_list, releasing any resources
 * and memory allocated for each node in the list. After calling this function,
 * the service_list handle should not be used.
 *
 * @param service_list The handle to the head of the linked list containing SMB service information.
 */
void zoo_smb_destroy_service_list(ZOO_LIST_HANDLE service_list)
{
    ZOO_LOG_DEBUG("destroying service list %p", service_list);

    for (size_t i = 0; i < zoo_list_size(service_list); ++i)
    {
        ZOO_SMB_SERVICE_HANDLE info = (ZOO_SMB_SERVICE_HANDLE)zoo_list_at(service_list, i);
        if (info)
        {
            zoo_smb_destroy_service(info);
        }
    }
    zoo_free_to_pool(service_list);
}

/**
 * @brief Finds and retrieves SMB service information by name.
 *
 * @param service_list The linked list handle for service registry.
 * @param service_name The name of the service to find.
 * @return ZOO_SMB_SERVICE_HANDLE Handle to the SMB service information, or NULL if not found.
 */
ZOO_SMB_SERVICE_HANDLE zoo_smb_find_service(
    ZOO_LIST_HANDLE service_list,
    const char* service_name)
{
    if (!service_list || !service_name)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return NULL;
    }
    size_t size = zoo_list_size(service_list);
    for (size_t i = 0; i < size; ++i)
    {
        ZOO_SMB_SERVICE_HANDLE info = (ZOO_SMB_SERVICE_HANDLE)zoo_list_at(service_list, i);
        if (info && strcmp(info->name, service_name) == 0)
        {
            ZOO_LOG_DEBUG("found service '%s'", service_name);
            return info;
        }
    }
    ZOO_LOG_WARN("service '%s' not found", service_name);
    return NULL;
}

/**
 * @brief Copies information from one zoo SMB service structure to another.
 *
 * @param from Pointer to the source zoo SMB service structure.
 * @param to   Pointer to the destination zoo SMB service structure.
 */
void zoo_smb_service_copy(
    const ZOO_SMB_SERVICE_HANDLE from,
    ZOO_SMB_SERVICE_HANDLE to)
{
    if (!from || !to)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return;
    }
    memcpy(to, from, sizeof(ZOO_SMB_SERVICE_STRUCT));
}

/**
 * @brief Creates a duplicate of the given ZOO_SMB_SERVICE_HANDLE.
 *
 * This function returns a new handle that represents the same SMB service as the original,
 * allowing for independent management of the duplicated handle.
 *
 * @param handle The original ZOO_SMB_SERVICE_HANDLE to duplicate.
 * @return A new ZOO_SMB_SERVICE_HANDLE that is a duplicate of the input handle, or NULL on failure.
 */
ZOO_SMB_SERVICE_HANDLE zoo_smb_service_duplicate(
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!service)
    {
        ZOO_LOG_ERROR("service is NULL");
        return NULL;
    }
    ZOO_SMB_SERVICE_HANDLE duplicate = (ZOO_SMB_SERVICE_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_SERVICE_STRUCT));
    if (!duplicate)
    {
        ZOO_LOG_ERROR("allocation failed");
        return NULL;
    }
    zoo_smb_service_copy(service, duplicate);
    ZOO_LOG_DEBUG("duplicated service '%s'", service->name);
    return duplicate;
}

/**
 * @brief Sets the address information for the SMB service.
 *
 * @param service The service info handle.
 * @param address      The address string to set.
 */
void zoo_smb_service_set_address(
    ZOO_SMB_SERVICE_HANDLE service,
    const char* address)
{
    if (!service || !address)
    {
        ZOO_LOG_ERROR("invalid parameters");
        return;
    }
    snprintf(service->address, sizeof(service->address), "%s", address);
    service->address[sizeof(service->address) - 1] = '\0';
    ZOO_LOG_DEBUG("set address '%s' for service '%s'", address, service->name);
}

/**
 * @brief Sets the port information for the SMB service.
 *
 * @param service The service info handle.
 * @param port         The port number to be set for the SMB service.
 */
void zoo_smb_service_set_port(
    ZOO_SMB_SERVICE_HANDLE service,
    int port)
{
    if (!service)
    {
        ZOO_LOG_ERROR("service is NULL");
        return;
    }
    service->port = port;
    ZOO_LOG_DEBUG("set port %d for service '%s'", port, service->name);
}

/**
 * @brief Sets the broadcast interval for the SMB service information.
 *
 * @param service         The service info handle.
 * @param broadcast_interval_ms The broadcast interval in milliseconds.
 */
void zoo_smb_service_set_broadcast_interval(
    ZOO_SMB_SERVICE_HANDLE service,
    uint32_t broadcast_interval_ms)
{
    if (!service)
    {
        ZOO_LOG_ERROR("service is NULL");
        return;
    }
    service->broadcast_interval_ms = broadcast_interval_ms;
    ZOO_LOG_DEBUG("set interval %u ms for service '%s'", broadcast_interval_ms, service->name);
}

/**
 * @brief Sets the last seen timestamp for the SMB service.
 *
 * @param service The service info handle.
 * @param last_seen    The last seen timestamp to set.
 */
void zoo_smb_service_set_last_seen(
    ZOO_SMB_SERVICE_HANDLE service,
    uint64_t last_seen)
{
    if (!service)
    {
        ZOO_LOG_ERROR("service is NULL");
        return;
    }
    service->last_seen = last_seen;
    ZOO_LOG_DEBUG("set last_seen=%llu for service '%s'", last_seen, service->name);
}

/**
 * @brief Sets the online status for the SMB service information.
 *
 * @param service The service info handle.
 * @param is_online    The online status to set.
 */
void zoo_smb_service_set_online_status(
    ZOO_SMB_SERVICE_HANDLE service,
    ZOO_BOOL is_online)
{
    if (!service)
    {
        ZOO_LOG_ERROR("service is NULL");
        return;
    }
    service->is_online = is_online;
    ZOO_LOG_DEBUG("set online=%d for service '%s'", is_online, service->name);
}

/**
 * @brief Sets the transport type for the SMB service information.
 *
 * @param service   The service info handle.
 * @param transport_type The transport type to set.
 */
void zoo_smb_service_set_transport_type(
    ZOO_SMB_SERVICE_HANDLE service,
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type)
{
    if (!service)
    {
        ZOO_LOG_ERROR("service is NULL");
        return;
    }
    service->transport_type = transport_type;
    ZOO_LOG_DEBUG("set transport_type=%d for service '%s'", transport_type, service->name);
}

/**
 * @brief Retrieves the name of the SMB service.
 *
 * @param service The service info handle.
 * @return const char* Pointer to the name string, or NULL if service is NULL.
 */
const char* zoo_smb_service_get_name(
    ZOO_SMB_SERVICE_HANDLE service)
{
    if (!service)
    {
        ZOO_LOG_ERROR("service is NULL");
        return NULL;
    }
    return service->name;
}

/**
 * @brief Sets the type of the specified SMB service.
 *
 * This function assigns a new service type to the given SMB service handle.
 *
 * @param service        The handle to the SMB service whose type is to be set.
 * @param service_type   The type to assign to the SMB service. This should be a value from the ZOO_SMB_SERVICE_TYPE_ENUM enumeration.
 */
void zoo_smb_service_set_type(ZOO_SMB_SERVICE_HANDLE service, ZOO_SMB_SERVICE_TYPE_ENUM service_type)
{
    if (!service)
    {
        ZOO_LOG_ERROR("service is NULL");
        return;
    }
    service->service_type = service_type;
    ZOO_LOG_DEBUG("set service type %d for service '%s'", service_type, service->name);
}
