
 /*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_BUS
 * File name: ZOO_SMB_service_discovery.c
 * Description: Service discovery implementation using transport layer
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 * 2.0       2025-05-25     weiwang.sun         optimized version
 ******************************************************************************/
#include "zoo_smb_service_discovery.h"
#include "zoo_types.h"
#include "zoo_smb_routing_engine.h"
#include "zoo_memory_pool.h"
#include "zoo_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdatomic.h>

#define DISCOVERY_TASK_SUBMIT_MAX_RETRIES 100
#define DISCOVERY_TASK_SUBMIT_BACKOFF_MS 10

/**
 * @brief Service discovery context structure
 */
typedef struct ZOO_SMB_SERVICE_DISCOVERY_STRUCT
{
    const BROADCAST_CONFIG_STRUCT* config;              /**< Discovery port */
    ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager;     /**< Service manager handle */
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager; /**< Transport manager handle */
    TRANSPORT_DATA_OBSERVER_HANDLE observer;            /**< Transport data observer handle */
    ZOO_LIST_HANDLE incoming_service_list;        /**< List of discovered services */
    ZOO_LIST_HANDLE outgoing_service_list;        /**< List of services to broadcast */
    atomic_bool running;
    atomic_bool worker_active;
} ZOO_SMB_SERVICE_DISCOVERY_STRUCT;

/**
 * @brief Checks the online status of a service
 *
 * This function evaluates the online status of a service by comparing its
 * current timestamp against the provided time.
 *
 * @param service Pointer to service information structure to check
 * @param current_time Current timestamp to compare against
 */
static ZOO_BOOL check_service_online_status_changed(ZOO_SMB_SERVICE_STRUCT* service, time_t current_time)
{
    ZOO_SMB_VALIDATE_PTR(service, ZOO_FALSE);
    time_t time_diff = current_time - service->last_seen;
    if (time_diff > service->broadcast_interval_ms / 1000 * 3)
    {
        if (service->is_online)
        {
            service->is_online = ZOO_FALSE;
            ZOO_LOG_WARN("[DISC] Service offline: %s at %s:%d (last seen: %ld seconds ago)", service->name, service->address, service->port, time_diff);
            return ZOO_TRUE;
        }
    }
    return ZOO_FALSE;
}

/**
 * @brief Checks if the transport type requires multicast information
 * @param transport_type Transport type to check
 * @return ZOO_TRUE if multicast info is needed, ZOO_FALSE otherwise
 */
static ZOO_BOOL is_multicast_transport(ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type)
{
    return (transport_type == ZOO_SMB_TRANSPORT_TYPE_UDP);
}

/**
 * @brief Parses a raw message and extracts service information with multicast support
 *
 * @param message Pointer to the message structure to be parsed
 * @param broadcast_interval_ms Default broadcast interval in milliseconds
 * @return Parsed service structure or NULL on failure
 */
ZOO_SMB_SERVICE_STRUCT* parse_payload_message(const ZOO_SMB_MSG_STRUCT* message, ZOO_U32 broadcast_interval_ms)
{
    ZOO_SMB_VALIDATE_PTR(message, NULL);
    ZOO_SMB_SERVICE_STRUCT* service = NULL;
    char name[MAX_SERVICE_NAME_LENGTH];                                          /**< Service name */
    char topic[MAX_SERVICE_TOPIC_LENGTH];                                        /**< Service topic */
    char address[MAX_SERVICE_ADDRESS_LENGTH];                                    /**< Service address */
    char multicast_addr[MAX_SERVICE_ADDRESS_LENGTH] = {0};                       /**< Multicast group address */
    ZOO_U16 port = 0;                                                           /**< Service port */
    ZOO_U16 multicast_port = 0;                                                 /**< Multicast group port */
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type = ZOO_SMB_TRANSPORT_TYPE_DEFAULT; /**< Transport type. */

    // Try to parse with extended multicast support first
    ZOO_BOOL parse_success = zoo_smb_protocol_parse_discovery_payload_message_ex(
        message, name, address, &port, topic, &transport_type, multicast_addr, &multicast_port);

    if (parse_success)
    {
        service = zoo_smb_create_service(
            ZOO_SMB_SERVICE_TYPE_REGISTRATION, name, topic, address, port, multicast_addr, multicast_port, transport_type, broadcast_interval_ms);

        // Set multicast information if available and applicable
        if (service && is_multicast_transport(transport_type) && strlen(multicast_addr) > 0)
        {
            // Store multicast information in service (you may need to extend ZOO_SMB_SERVICE_STRUCT)
            // For now, we'll log it
            ZOO_LOG_DEBUG("[DISC] Service '%s' uses multicast group %s:%d",
                              name,
                              multicast_addr,
                              multicast_port);
        }
    }
    return service;
}

/**
 * @brief Callback function to handle service update messages
 *
 * This function is called when a service update message is received.
 * It validates the message and updates the service information in the context.
 *
 * @param from_address Address of the sender (ZOO_SMB_UNUSED)
 * @param payload Pointer to the received message buffer
 * @param payload_size Size of the received message buffer in bytes
 * @param user_data Pointer to user data (context)
 */
static void handle_received_message_cb(
    void* user_data,
    const ZOO_SMB_MSG_STRUCT* message)
{
    ZOO_SMB_SERVICE_DISCOVERY_STRUCT* discover = (ZOO_SMB_SERVICE_DISCOVERY_STRUCT*)user_data;
    ZOO_SMB_VALIDATE_PTR_RETURN_VOID(discover);
    ZOO_SMB_VALIDATE_PTR_RETURN_VOID(message);

    if (message->header.msg_type != ZOO_SMB_MSG_TYPE_HB)
    {
        return;
    }

    ZOO_SMB_SERVICE_STRUCT* service = parse_payload_message(message, discover->config->interval_ms);
    ZOO_SMB_SERVICE_HANDLE exist_service = zoo_smb_service_manager_get_service(discover->service_manager, service->name, ZOO_SMB_SERVICE_TYPE_REGISTRATION);
    if (!exist_service)
    {
        zoo_smb_service_manager_register_service(discover->service_manager, ZOO_SMB_SERVICE_TYPE_REGISTRATION, service);
    }
    else
    {
        zoo_smb_service_set_last_seen(exist_service, service->last_seen);
    }
    zoo_smb_destroy_service(service);
}

/**
 * @brief Sends service information over the specified transport with multicast support.
 *
 * This function sends service-related information using the provided transport handle.
 * For UDP multicast services, it includes multicast group information in the broadcast.
 *
 * @param transport_manager The transport manager handle
 * @param service The service structure containing service information
 */
static void broadcast_single_service(
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager,
    const ZOO_SMB_SERVICE_STRUCT* service)
{
    ZOO_SMB_VALIDATE_PTR_RETURN_VOID(transport_manager);
    ZOO_SMB_VALIDATE_PTR_RETURN_VOID(service);

    ZOO_SMB_MSG_STRUCT* message = zoo_smb_create_message(ZOO_SMB_MSG_TYPE_HB, service->name, service->topic, NULL, 0, 0, 0);

    // For UDP multicast services, we need to include multicast group information
    ZOO_STRING multicast_addr = service->multicast_address;  // Assuming service->address is the multicast group
    ZOO_U16 multicast_port = service->multicast_port;        // Assuming service->port is the multicast port

    // Use extended protocol function for multicast services
    zoo_smb_protocol_make_discovery_payload_message_ex(
        message,
        service->name,
        service->address,  // Service's own address (if different from multicast)
        service->port,     // Service's own port (if different from multicast)
        service->topic,
        service->transport_type,
        multicast_addr,   // Multicast group address
        multicast_port);  // Multicast group port

    ZOO_LOG_DEBUG("[DISC] Broadcasting multicast service '%s' - group: %s:%d, transport: %d",
                      service->name,
                      multicast_addr,
                      multicast_port,
                      service->transport_type);

    zoo_smb_transport_manager_broadcast_message(transport_manager, (const ZOO_SMB_SERVICE_HANDLE)service, message);
    zoo_smb_destroy_message(message);
}

/**
 * @brief Sends information about all available SMB services
 *
 * This function broadcasts service information for all currently registered SMB services
 * in the system through the provided discovery context.
 *
 * @param context Pointer to the service discovery context structure containing service information
 */
static void broadcast_all_service(ZOO_SMB_SERVICE_DISCOVERY_STRUCT* discover)
{
    ZOO_SMB_VALIDATE_PTR_RETURN_VOID(discover);
    time_t current_time = time(NULL);
    for (ZOO_USIZE i = 0; i < zoo_list_size(discover->outgoing_service_list); i++)
    {
        ZOO_SMB_SERVICE_STRUCT* service =
            (ZOO_SMB_SERVICE_STRUCT*)zoo_list_at(discover->outgoing_service_list, i);

        if (service != NULL && service->is_online)
        {
            time_t next_broadcast = service->last_seen + (service->broadcast_interval_ms / 1000);
            if (current_time >= next_broadcast)
            {
                broadcast_single_service(discover->transport_manager, service);
                service->last_seen = current_time;
            }
        }
    }
}

/**
 * @brief Handles the online status of a discovered SMB service.
 *
 * This function processes the provided discovery structure to determine
 * and handle the online status of an SMB service. It may update internal
 * states, notify other components, or trigger specific actions based on
 * the service's availability.
 *
 * @param discover Pointer to a ZOO_SMB_SERVICE_DISCOVERY_STRUCT structure
 *                 containing information about the discovered SMB service.
 */
static void handle_service_online_status(ZOO_SMB_SERVICE_DISCOVERY_STRUCT* discover)
{
    time_t current_time = time(NULL);
    for (ZOO_USIZE i = 0; i < zoo_list_size(discover->incoming_service_list); i++)
    {
        ZOO_SMB_SERVICE_STRUCT* service = (ZOO_SMB_SERVICE_STRUCT*)zoo_list_at(discover->incoming_service_list, i);
        check_service_online_status_changed(service, current_time);
    }
}

/**
 * @brief Task function for handling service discovery.
 *
 * This function is intended to be run as a task/thread and is responsible for
 * performing service discovery operations.
 *
 * @param user_data    Pointer to user-defined data passed to the task.
 * @param argument     Pointer to additional arguments for the task.
 * @param argment_len  Length of the argument data.
 */
static ZOO_ERROR_TYPE discovery_task(void* user_data, void* argument)
{
    ZOO_SMB_SERVICE_DISCOVERY_STRUCT* discover = (ZOO_SMB_SERVICE_DISCOVERY_STRUCT*)user_data;
    ZOO_SMB_VALIDATE_PTR(discover, ZOO_SMB_ERROR_INVALID_PARAM);
    ZOO_SMB_UNUSED(argument);
    atomic_store(&discover->worker_active, true);
    while (atomic_load(&discover->running))
    {
        // Broadcast all service information
        broadcast_all_service(discover);

        // Check online status of services
        handle_service_online_status(discover);

        // Notify observers about service updates
        ZOO_SMB_SLEEP_MS(discover->config->interval_ms);
    }

    atomic_store(&discover->worker_active, false);

    return ZOO_SMB_OK;
}

/**
 * @brief Create service discovery instance
 * @param service_name Service name
 * @param address Multicast address
 * @param port Port number
 * @param memory_pool Memory pool handle
 * @return ZOO_SMB_SERVICE_DISCOVERY_HANDLE Service discovery handle, NULL on failure
 */
ZOO_SMB_SERVICE_DISCOVERY_HANDLE zoo_smb_create_service_discovery(
    const ZOO_SMB_CONFIG_STRUCT* config,
    ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager,
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager)
{
    ZOO_LOG_DEBUG("[DISC] Creating service discovery instance");
    ZOO_SMB_SERVICE_DISCOVERY_STRUCT* discovery = (ZOO_SMB_SERVICE_DISCOVERY_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_SERVICE_DISCOVERY_STRUCT));
    if (discovery == NULL)
    {
        ZOO_LOG_ERROR("[DISC] Failed to allocate memory for service discovery context");
        return NULL;
    }

    discovery->config = &config->broadcast;
    discovery->service_manager = service_manager;
    discovery->transport_manager = transport_manager;
    discovery->outgoing_service_list = zoo_smb_service_manager_get_service_list(
        discovery->service_manager, ZOO_SMB_SERVICE_TYPE_BROADCAST);
    discovery->incoming_service_list = zoo_smb_service_manager_get_service_list(
        discovery->service_manager, ZOO_SMB_SERVICE_TYPE_REGISTRATION);
    atomic_init(&discovery->running, false);
    atomic_init(&discovery->worker_active, false);
    discovery->observer = create_transport_data_observer(handle_received_message_cb, discovery);
    if (!discovery->observer)
    {
        ZOO_LOG_ERROR("[DISC] Failed to register incoming data observer");
        zoo_free_to_pool(discovery);
        return NULL;
    }

    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_transport_manager_get_broadcast_transport(discovery->transport_manager);
    if (ZOO_SMB_OK != zoo_smb_transport_manager_register_incoming_data_observer(discovery->transport_manager, transport, discovery->observer))
    {
        ZOO_LOG_ERROR("[DISC] Failed to register incoming data observer");
        destroy_transport_data_observer(discovery->observer);
        zoo_free_to_pool(discovery);
        return NULL;
    }
    ZOO_LOG_INFO("[DISC] Service discovery context created successfully");
    return discovery;
}

/**
 * @brief Destroy service discovery instance
 * @param discovery_handle Service discovery handle
 * @return void
 */
void zoo_smb_destroy_service_discovery(ZOO_SMB_SERVICE_DISCOVERY_HANDLE discovery_handle)
{
    ZOO_LOG_DEBUG("[DISC] Destroying service discovery");
    if (discovery_handle)
    {
        ZOO_SMB_SERVICE_DISCOVERY_STRUCT* discovery = (ZOO_SMB_SERVICE_DISCOVERY_STRUCT*)discovery_handle;
        zoo_smb_stop_service_discovery(discovery_handle);
        ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_transport_manager_get_broadcast_transport(discovery->transport_manager);
        zoo_smb_transport_manager_unregister_incoming_data_observer(discovery->transport_manager, transport, discovery->observer);
        destroy_transport_data_observer(discovery->observer);
        zoo_free_to_pool(discovery);
    }
}

/**
 * @brief Start service discovery service
 * @param discovery_handle Service discovery handle
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_start_service_discovery(
    ZOO_SMB_SERVICE_DISCOVERY_HANDLE discovery_handle)
{
    ZOO_SMB_SERVICE_DISCOVERY_STRUCT* discovery = (ZOO_SMB_SERVICE_DISCOVERY_STRUCT*)discovery_handle;
    ZOO_SMB_VALIDATE_PTR(discovery, ZOO_SMB_ERROR_INVALID_PARAM);

    if (atomic_load(&discovery->running))
    {
        ZOO_LOG_WARN("[DISC] Service discovery is already running");
        return ZOO_SMB_OK;
    }

    atomic_store(&discovery->running, true);
    int retry = 0;
    while (ZOO_SMB_OK != zoo_thread_pool_submit_task("discovery_task", discovery_task, discovery, NULL, ZOO_TRUE))
    {
        retry++;
        if (retry >= DISCOVERY_TASK_SUBMIT_MAX_RETRIES)
        {
            atomic_store(&discovery->running, false);
            ZOO_LOG_ERROR("[DISC] Failed to start discovery task after %d retries", retry);
            return ZOO_SMB_ERROR_OPERATION_FAILED;
        }
        ZOO_LOG_WARN("[DISC] Failed to start discovery task, retry=%d", retry);
        ZOO_SMB_SLEEP_MS(DISCOVERY_TASK_SUBMIT_BACKOFF_MS);
    }
    ZOO_LOG_INFO("[DISC] Service discovery started successfully");
    return ZOO_SMB_OK;
}

/**
 * @brief Stop service discovery process
 * @param discovery_handle Service discovery handle
 * @return void
 */
void zoo_smb_stop_service_discovery(ZOO_SMB_SERVICE_DISCOVERY_HANDLE discovery_handle)
{
    ZOO_SMB_SERVICE_DISCOVERY_STRUCT* discovery = (ZOO_SMB_SERVICE_DISCOVERY_STRUCT*)discovery_handle;
    ZOO_SMB_VALIDATE_PTR_RETURN_VOID(discovery);
    atomic_store(&discovery->running, false);

    ZOO_U32 wait_budget_ms = discovery->config ? (discovery->config->interval_ms + 200U) : 1200U;
    ZOO_U32 wait_steps = (wait_budget_ms / 10U) + 1U;
    for (ZOO_U32 i = 0; i < wait_steps && atomic_load(&discovery->worker_active); ++i)
    {
        ZOO_SMB_SLEEP_MS(10);
    }

    ZOO_LOG_DEBUG("[DISC] Service discovery stopped successfully");
}