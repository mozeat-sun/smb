/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_SHM_SERVER
 * File name: zoo_smb_transport_shm_server.c
 * Description: Shared Memory Transport Server Implementation
 *              Manages client connections, message routing, and sender mappings
 *              for high-performance inter-process communication.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-23     weiwang.sun       Created
 * 2.0       2025-06-25     weiwang.sun       Added sender mapping support
 * 3.0       2025-07-28     weiwang.sun       Enhanced client management
 ******************************************************************************/

#include "zoo_smb_transport_shm_server.h"
#include "zoo_smb_transport_shm_common.h"
#include "zoo_log.h"
#include "zoo_smb_protocol.h"
#include "zoo_memory_pool.h"
#include <sys/select.h>
#include <sys/eventfd.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

// ==============================================================================
// CONSTANTS
// ==============================================================================

/** @brief Maximum messages processed per cycle to prevent starvation */
#define MAX_MESSAGES_PER_CYCLE 10

/** @brief Maximum consecutive errors before client disconnection */
#define MAX_CONSECUTIVE_ERRORS 3

/** @brief Client heartbeat timeout in seconds */
#define HEARTBEAT_TIMEOUT_SECONDS 30

/** @brief Server stop timeout in seconds */
#define SERVER_STOP_TIMEOUT_SECONDS 10

// ==============================================================================
// INTERNAL FUNCTION DECLARATIONS
// ==============================================================================

static ZOO_ERROR_TYPE shm_server_create_master_segment(SHM_SERVER_IMPL_STRUCT* impl);
static ZOO_ERROR_TYPE shm_server_init_event_fd(SHM_SERVER_IMPL_STRUCT* impl);
static ZOO_ERROR_TYPE shm_server_accept_new_clients(SHM_SERVER_IMPL_STRUCT* impl);
static ZOO_ERROR_TYPE shm_server_process_client_messages(SHM_SERVER_IMPL_STRUCT* impl,
                                                             SHM_CLIENT_INFO_STRUCT* client);
static void shm_server_cleanup_disconnected_clients(SHM_SERVER_IMPL_STRUCT* impl);
static void shm_server_update_client_state(SHM_SERVER_IMPL_STRUCT* impl,
                                           SHM_CLIENT_INFO_STRUCT* client,
                                           TRANSPORT_CONNECTION_STATE_ENUM new_state);
static ZOO_ERROR_TYPE shm_validate_and_register_client(SHM_SERVER_IMPL_STRUCT* impl,
                                                           ZOO_SMB_SHM_CLIENT_REGISTRATION* reg_request);

// ==============================================================================
// CLIENT MANAGEMENT HELPERS
// ==============================================================================

/**
 * @brief Find sender mapping by sender name
 * @param impl Server implementation instance
 * @param sender_name Sender name to find
 * @return Pointer to sender mapping structure, NULL if not found
 */
static SHM_SENDER_MAPPING_STRUCT* shm_find_sender_mapping(SHM_SERVER_IMPL_STRUCT* impl,
                                                          const char* sender_name)
{
    if (!impl || !impl->sender_mapping_list || !sender_name)
        return NULL;

    for (size_t i = 0; i < zoo_list_size(impl->sender_mapping_list); i++)
    {
        SHM_SENDER_MAPPING_STRUCT* mapping = (SHM_SENDER_MAPPING_STRUCT*)zoo_list_at(impl->sender_mapping_list, i);
        if (mapping && strcmp(mapping->sender_name, sender_name) == 0)
        {
            return mapping;
        }
    }
    return NULL;
}

/**
 * @brief Register or update sender mapping
 * @param impl Server implementation instance
 * @param sender_name Logical sender name
 * @param client_info Physical client that owns this sender
 * @return Error code indicating success or failure
 */
static ZOO_ERROR_TYPE shm_register_sender_mapping(SHM_SERVER_IMPL_STRUCT* impl,
                                                      const char* sender_name,
                                                      SHM_CLIENT_INFO_STRUCT* client_info)
{
    if (!impl || !sender_name || !client_info)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    // Check if sender already exists
    SHM_SENDER_MAPPING_STRUCT* existing_mapping = shm_find_sender_mapping(impl, sender_name);
    if (existing_mapping)
    {
        // Update existing mapping
        if (existing_mapping->client_info != client_info)
        {
            ZOO_LOG_INFO("Sender '%s' moved from client '%s' to client '%s'",
                             sender_name,
                             existing_mapping->client_info->name,
                             client_info->name);
            existing_mapping->client_info = client_info;
        }
        existing_mapping->last_message_time = time(NULL);
        existing_mapping->message_count++;
        return ZOO_SMB_OK;
    }

    // Create new mapping
    SHM_SENDER_MAPPING_STRUCT* new_mapping = (SHM_SENDER_MAPPING_STRUCT*)zoo_allocate_from_pool(
        sizeof(SHM_SENDER_MAPPING_STRUCT));
    if (!new_mapping)
    {
        ZOO_LOG_ERROR("Failed to allocate sender mapping for: %s", sender_name);
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    memset(new_mapping, 0, sizeof(SHM_SENDER_MAPPING_STRUCT));
    (void)snprintf(new_mapping->sender_name,
                   sizeof(new_mapping->sender_name),
                   "%s",
                   sender_name);
    new_mapping->client_info = client_info;
    new_mapping->registration_time = time(NULL);
    new_mapping->last_message_time = new_mapping->registration_time;
    new_mapping->message_count = 1;

    ZOO_ERROR_TYPE result = zoo_list_push_back(impl->sender_mapping_list, new_mapping);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_ERROR("Failed to add sender mapping to list: %s", zoo_smb_get_error_string(result));
        zoo_free_to_pool(new_mapping);
        return result;
    }

    ZOO_LOG_INFO("Registered new sender mapping: '%s' -> client '%s' (physical client: %s)",
                     sender_name,
                     client_info->name,
                     client_info->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Clean up sender mappings for disconnected client
 * @param impl Server implementation instance
 * @param disconnected_client Client that disconnected
 */
static void shm_cleanup_sender_mappings_for_client(SHM_SERVER_IMPL_STRUCT* impl,
                                                   SHM_CLIENT_INFO_STRUCT* disconnected_client)
{
    if (!impl || !impl->sender_mapping_list || !disconnected_client)
        return;

    size_t i = 0;
    size_t cleaned_count = 0;

    while (i < zoo_list_size(impl->sender_mapping_list))
    {
        SHM_SENDER_MAPPING_STRUCT* mapping = (SHM_SENDER_MAPPING_STRUCT*)zoo_list_at(impl->sender_mapping_list, i);
        if (mapping && mapping->client_info == disconnected_client)
        {
            ZOO_LOG_INFO("Removing sender mapping: '%s' (client disconnected: %s)",
                             mapping->sender_name,
                             disconnected_client->name);
            zoo_list_erase(impl->sender_mapping_list, i);
            zoo_free_to_pool(mapping);
            cleaned_count++;
        }
        else
        {
            i++;
        }
    }

    if (cleaned_count > 0)
    {
        ZOO_LOG_INFO("Cleaned %zu sender mappings for disconnected client: %s",
                         cleaned_count,
                         disconnected_client->name);
    }
}

/**
 * @brief Find target client by receiver/sender name (improved with mapping)
 * @param impl Server implementation instance
 * @param receiver Target receiver/sender name
 * @return Pointer to client info structure, NULL if not found
 */
static SHM_CLIENT_INFO_STRUCT* shm_find_target_client(SHM_SERVER_IMPL_STRUCT* impl,
                                                      const char* receiver)
{
    if (!impl || !receiver)
        return NULL;

    // First try to find by sender mapping (logical sender -> physical client)
    SHM_SENDER_MAPPING_STRUCT* mapping = shm_find_sender_mapping(impl, receiver);
    if (mapping && mapping->client_info &&
        mapping->client_info->state == TRANSPORT_CONN_STATE_CONNECTED)
    {
        ZOO_LOG_DEBUG("Found target client via sender mapping: '%s' -> client '%s'",
                          receiver,
                          mapping->client_info->name);
        return mapping->client_info;
    }

    // Fallback: Try to find by physical client name
    for (size_t i = 0; i < zoo_list_size(impl->client_list); i++)
    {
        SHM_CLIENT_INFO_STRUCT* client = (SHM_CLIENT_INFO_STRUCT*)zoo_list_at(impl->client_list, i);
        if (client &&
            client->state == TRANSPORT_CONN_STATE_CONNECTED &&
            strcmp(client->name, receiver) == 0)
        {
            ZOO_LOG_DEBUG("Found target client via physical name: '%s'", receiver);
            return client;
        }
    }

    return NULL;
}

/**
 * @brief Log currently available clients for debugging
 * @param impl Server implementation instance
 */
static void shm_log_available_clients(SHM_SERVER_IMPL_STRUCT* impl)
{
    if (!impl->client_list)
        return;

    ZOO_LOG_DEBUG("Available clients:");
    for (size_t i = 0; i < zoo_list_size(impl->client_list); i++)
    {
        SHM_CLIENT_INFO_STRUCT* client = (SHM_CLIENT_INFO_STRUCT*)zoo_list_at(impl->client_list, i);
        if (client)
        {
            ZOO_LOG_DEBUG("  Client[%zu]: name=%s, event_fd=%d, state=%d",
                              i,
                              client->name,
                              client->event_fd,
                              client->state);
        }
    }
}

/**
 * @brief Log currently available senders for debugging
 * @param impl Server implementation instance
 */
static void shm_log_available_senders(SHM_SERVER_IMPL_STRUCT* impl)
{
    if (!impl->sender_mapping_list)
        return;

    ZOO_LOG_DEBUG("Available senders (%zu total):", zoo_list_size(impl->sender_mapping_list));
    for (size_t i = 0; i < zoo_list_size(impl->sender_mapping_list); i++)
    {
        SHM_SENDER_MAPPING_STRUCT* mapping = (SHM_SENDER_MAPPING_STRUCT*)zoo_list_at(impl->sender_mapping_list, i);
        if (mapping)
        {
            ZOO_LOG_DEBUG("  Sender[%zu]: name='%s' -> client='%s' (msgs=%u)",
                              i,
                              mapping->sender_name,
                              mapping->client_info ? mapping->client_info->name : "NULL",
                              mapping->message_count);
        }
    }
}

// ==============================================================================
// MESSAGE SENDING FUNCTIONS
// ==============================================================================

/**
 * @brief Send message to specific target client (enhanced with sender mapping)
 * @param impl Server implementation instance
 * @param msg Message to send
 * @param receiver Target receiver/sender name
 * @return Error code indicating success or failure
 */
static ZOO_ERROR_TYPE shm_send_to_target_client(SHM_SERVER_IMPL_STRUCT* impl,
                                                    const ZOO_SMB_MSG_STRUCT* msg,
                                                    const char* receiver)
{
    SHM_CLIENT_INFO_STRUCT* target_client = shm_find_target_client(impl, receiver);
    if (!target_client)
    {
        ZOO_LOG_ERROR("Target client not found for receiver: '%s'", receiver);
        ZOO_LOG_DEBUG("Available clients:");
        shm_log_available_clients(impl);
        ZOO_LOG_DEBUG("Available senders:");
        shm_log_available_senders(impl);
        return ZOO_SMB_ERROR_SERVICE_NOT_FOUND;
    }

    uint8_t* msg_buffer = zoo_allocate_from_pool(MAX_TRANSPORT_BUFFER_SIZE);
    if (!msg_buffer)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for message buffer");
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    size_t msg_size = zoo_smb_protocol_serialize(msg, msg_buffer, MAX_TRANSPORT_BUFFER_SIZE);
    if (msg_size == 0)
    {
        ZOO_LOG_ERROR("Failed to serialize message");
        zoo_free_to_pool(msg_buffer);
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    ZOO_ERROR_TYPE result = zoo_smb_ring_buffer_write(target_client->tx_ring, msg_buffer, msg_size);
    if (ZOO_SMB_IS_SUCCESS(result))
    {
        shm_notify_peer(target_client->event_fd);
        ZOO_LOG_DEBUG("Message sent successfully: receiver='%s' -> physical_client='%s' (size=%zu)",
                          receiver,
                          target_client->name,
                          msg_size);
    }
    else
    {
        ZOO_LOG_ERROR("Failed to write message to client %s: %s",
                          target_client->name,
                          zoo_smb_get_error_string(result));
    }

    zoo_free_to_pool(msg_buffer);
    return result;
}

/**
 * @brief Broadcast message to all connected clients
 * @param impl Server implementation instance
 * @param msg Message to broadcast
 * @return Error code indicating success or failure
 */
static ZOO_ERROR_TYPE shm_broadcast_to_all_clients(SHM_SERVER_IMPL_STRUCT* impl,
                                                       const ZOO_SMB_MSG_STRUCT* msg)
{
    uint8_t* msg_buffer = zoo_allocate_from_pool(MAX_TRANSPORT_BUFFER_SIZE);
    if (!msg_buffer)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for message buffer");
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    size_t msg_size = zoo_smb_protocol_serialize(msg, msg_buffer, MAX_TRANSPORT_BUFFER_SIZE);
    if (msg_size == 0)
    {
        ZOO_LOG_ERROR("Failed to serialize message");
        zoo_free_to_pool(msg_buffer);
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    int sent_count = 0;
    for (size_t i = 0; i < zoo_list_size(impl->client_list); i++)
    {
        SHM_CLIENT_INFO_STRUCT* client = (SHM_CLIENT_INFO_STRUCT*)zoo_list_at(impl->client_list, i);
        if (client && client->state == TRANSPORT_CONN_STATE_CONNECTED)
        {
            ZOO_ERROR_TYPE result = zoo_smb_ring_buffer_write(client->tx_ring, msg_buffer, msg_size);
            if (ZOO_SMB_IS_SUCCESS(result))
            {
                shm_notify_peer(client->event_fd);
                sent_count++;
                ZOO_LOG_DEBUG("Broadcast message sent to client: %s", client->name);
            }
            else
            {
                ZOO_LOG_WARN("Failed to broadcast to client %s: %s",
                                 client->name,
                                 zoo_smb_get_error_string(result));
            }
        }
    }

    zoo_free_to_pool(msg_buffer);
    ZOO_LOG_DEBUG("Broadcast completed: sent to %d clients", sent_count);
    return (sent_count > 0) ? ZOO_SMB_OK : ZOO_SMB_ERROR_PUBLICATION_NO_SUBSCRIBERS;
}

// ==============================================================================
// SERVER INITIALIZATION FUNCTIONS
// ==============================================================================

/**
 * @brief Create master shared memory segment for server
 * @param impl Server implementation instance
 * @return Error code indicating success or failure
 */
static ZOO_ERROR_TYPE shm_server_create_master_segment(SHM_SERVER_IMPL_STRUCT* impl)
{
    size_t master_size = sizeof(SHM_SERVER_HEADER_STRUCT) +
                         (ZOO_SMB_SHM_MAX_PENDING_REGISTRATIONS * sizeof(ZOO_SMB_SHM_CLIENT_REGISTRATION));

    // Try to create new shared memory segment
    impl->common.master_shm_id = shmget(impl->common.master_shm_key, master_size, IPC_CREAT | IPC_EXCL | 0666);
    if (impl->common.master_shm_id == -1)
    {
        if (errno == EEXIST)
        {
            // Remove existing segment and retry
            ZOO_LOG_WARN("Master shared memory already exists, removing...");
            int old_id = shmget(impl->common.master_shm_key, 0, 0);
            if (old_id != -1)
            {
                shmctl(old_id, IPC_RMID, NULL);
            }
            impl->common.master_shm_id = shmget(impl->common.master_shm_key, master_size, IPC_CREAT | 0666);
        }

        if (impl->common.master_shm_id == -1)
        {
            ZOO_LOG_ERROR("Failed to create master shared memory: %s", strerror(errno));
            return ZOO_SMB_ERROR_SHM_CREATE_FAILED;
        }
    }

    // Attach to shared memory
    impl->common.server_header = (SHM_SERVER_HEADER_STRUCT*)shmat(impl->common.master_shm_id, NULL, 0);
    if (impl->common.server_header == (void*)-1)
    {
        ZOO_LOG_ERROR("Failed to attach master shared memory: %s", strerror(errno));
        shmctl(impl->common.master_shm_id, IPC_RMID, NULL);
        return ZOO_SMB_ERROR_SHM_ATTACH_FAILED;
    }

    // Initialize server header
    memset(impl->common.server_header, 0, master_size);
    impl->common.server_header->magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    impl->common.server_header->version = ZOO_SMB_PROTOCOL_VERSION;
    impl->common.server_header->client_count = 0;
    impl->common.server_header->max_clients = ZOO_SMB_SHM_MAX_CLIENTS;
    impl->common.server_header->server_start_time = time(NULL);
    impl->common.server_header->server_running = ZOO_TRUE;
    impl->common.server_header->has_pending_messages = ZOO_FALSE;
    impl->common.server_header->server_event_fd = -1;

    ZOO_LOG_INFO("Server master shared memory created (ID: %d, Key: 0x%08X)",
                     impl->common.master_shm_id,
                     impl->common.master_shm_key);
    return ZOO_SMB_OK;
}

/**
 * @brief Initialize server event file descriptor
 * @param impl Server implementation instance
 * @return Error code indicating success or failure
 */
static ZOO_ERROR_TYPE shm_server_init_event_fd(SHM_SERVER_IMPL_STRUCT* impl)
{
    impl->server_event_fd = eventfd(0, EFD_NONBLOCK);
    if (impl->server_event_fd == -1)
    {
        ZOO_LOG_ERROR("Failed to create server event fd: %s", strerror(errno));
        return ZOO_SMB_ERROR_EVENTFD_CREATION_FAILED;
    }

    if (impl->common.server_header)
    {
        impl->common.server_header->server_event_fd = impl->server_event_fd;
        ZOO_LOG_DEBUG("Server event fd created and stored in header: %d", impl->server_event_fd);
    }

    return ZOO_SMB_OK;
}

// ==============================================================================
// CLIENT STATE MANAGEMENT
// ==============================================================================

/**
 * @brief Update client connection state
 * @param impl Server implementation instance
 * @param client Client information structure
 * @param new_state New connection state
 */
static void shm_server_update_client_state(SHM_SERVER_IMPL_STRUCT* impl,
                                           SHM_CLIENT_INFO_STRUCT* client,
                                           TRANSPORT_CONNECTION_STATE_ENUM new_state)
{
    ZOO_SMB_UNUSED(impl);
    if (!client || client->state == new_state)
        return;

    TRANSPORT_CONNECTION_STATE_ENUM old_state = client->state;
    client->state = new_state;

    ZOO_LOG_INFO("Client '%s' state changed: %d -> %d", client->name, old_state, new_state);
}

/**
 * @brief Clean up disconnected clients (enhanced with sender mapping cleanup)
 * @param impl Server implementation instance
 */
static void shm_server_cleanup_disconnected_clients(SHM_SERVER_IMPL_STRUCT* impl)
{
    if (!impl || !impl->client_list)
        return;

    time_t now = time(NULL);
    size_t i = 0;
    size_t cleaned_count = 0;

    while (i < zoo_list_size(impl->client_list))
    {
        SHM_CLIENT_INFO_STRUCT* client = (SHM_CLIENT_INFO_STRUCT*)zoo_list_at(impl->client_list, i);
        ZOO_BOOL need_cleanup = ZOO_FALSE;

        if (!client)
        {
            zoo_list_erase(impl->client_list, i);
            cleaned_count++;
            continue;
        }

        // Check for explicit disconnection
        if (client->state == TRANSPORT_CONN_STATE_DISCONNECTED)
        {
            need_cleanup = ZOO_TRUE;
        }
        // Check for heartbeat timeout
        else if (client->last_heartbeat > 0 &&
                 (now - client->last_heartbeat > HEARTBEAT_TIMEOUT_SECONDS))
        {
            ZOO_LOG_WARN("Client '%s' heartbeat timeout (%lds), marking as disconnected",
                             client->name,
                             now - client->last_heartbeat);
            shm_server_update_client_state(impl, client, TRANSPORT_CONN_STATE_DISCONNECTED);
            need_cleanup = ZOO_TRUE;
        }

        if (need_cleanup)
        {
            ZOO_LOG_INFO("Cleaning up disconnected client: %s (event_fd=%d, id=%d)",
                             client->name,
                             client->event_fd,
                             client->client_id);

            // **ADD THIS**: Clean up sender mappings for this client
            shm_cleanup_sender_mappings_for_client(impl, client);

            shm_cleanup_client_resources(client);
            zoo_list_erase(impl->client_list, i);
            zoo_free_to_pool(client);
            cleaned_count++;
        }
        else
        {
            i++;
        }
    }

    // Update client count in server header
    if (impl->common.server_header)
        impl->common.server_header->client_count = zoo_list_size(impl->client_list);

    if (cleaned_count > 0)
        ZOO_LOG_INFO("Cleaned %zu disconnected clients", cleaned_count);
}

// ==============================================================================
// MESSAGE PROCESSING
// ==============================================================================

/**
 * @brief Process single message from client (enhanced with sender mapping)
 * @param impl Server implementation instance
 * @param client Client information structure
 * @return Error code indicating success or failure
 */
static ZOO_ERROR_TYPE shm_process_single_message(SHM_SERVER_IMPL_STRUCT* impl,
                                                     SHM_CLIENT_INFO_STRUCT* client)
{
    if (!impl || !client || !client->rx_ring)
    {
        ZOO_LOG_ERROR("Invalid parameters in shm_process_single_message");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Get the next message payload size
    size_t msg_size = zoo_smb_ring_buffer_get_next_message_size(client->rx_ring);
    if (msg_size == 0)
    {
        ZOO_LOG_TRACE("No complete message available in RX ring buffer for client: %s", client->name);
        return ZOO_SMB_OK;  // No complete message available
    }

    if (msg_size > MAX_TRANSPORT_BUFFER_SIZE)
    {
        ZOO_LOG_ERROR("Message size %zu exceeds maximum %d from client %s",
                          msg_size,
                          MAX_TRANSPORT_BUFFER_SIZE,
                          client->name);
        zoo_smb_ring_buffer_clear(client->rx_ring);
        return ZOO_SMB_ERROR_INVALID_SIZE;
    }

    ZOO_LOG_DEBUG("Processing message from client %s: payload_size=%zu",
                      client->name,
                      msg_size);

    // Allocate message buffer
    uint8_t* msg_buffer = (uint8_t*)zoo_allocate_from_pool(msg_size);
    if (!msg_buffer)
    {
        ZOO_LOG_ERROR("Failed to allocate message buffer of size %zu for client %s",
                          msg_size,
                          client->name);
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    // Read message payload
    ZOO_ERROR_TYPE result = zoo_smb_ring_buffer_read(client->rx_ring, msg_buffer, msg_size);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_ERROR("Failed to read message from client %s: %s",
                          client->name,
                          zoo_smb_get_error_string(result));
        zoo_free_to_pool(msg_buffer);
        return result;
    }

    // Deserialize message
    ZOO_SMB_MSG_STRUCT* msg_out = zoo_smb_default_message();
    if (!msg_out)
    {
        ZOO_LOG_ERROR("Failed to get default message structure");
        zoo_free_to_pool(msg_buffer);
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    result = zoo_smb_protocol_deserialize(msg_buffer, msg_size, msg_out, sizeof(ZOO_SMB_MSG_STRUCT));
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_ERROR("Failed to deserialize message from client %s: %s",
                          client->name,
                          zoo_smb_get_error_string(result));
        zoo_smb_destroy_message(msg_out);
        zoo_free_to_pool(msg_buffer);
        return result;
    }

    // Update heartbeat
    client->last_heartbeat = time(NULL);

    result = shm_register_sender_mapping(impl, msg_out->header.sender, client);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_WARN("Failed to register sender mapping '%s' -> client '%s': %s",
                         msg_out->header.sender,
                         client->name,
                         zoo_smb_get_error_string(result));
        // Don't fail the message processing for this
    }

    ZOO_LOG_DEBUG("Processing valid message: sender='%s', type=%d, physical_client='%s'",
                      msg_out->header.sender,
                      msg_out->header.msg_type,
                      client->name);

    // Notify observers

    ZOO_LIST_HANDLE observers = impl->common.transport->data_observers;
    size_t observer_count = 0;
    TRANSPORT_DATA_OBSERVER_STRUCT** snapshot = NULL;

    ZOO_MUTEX_LOCK(&impl->common.transport->data_observers_lock);
    observer_count = zoo_list_size(observers);
    if (observer_count > 0)
    {
        snapshot = (TRANSPORT_DATA_OBSERVER_STRUCT**)zoo_allocate_from_pool(sizeof(TRANSPORT_DATA_OBSERVER_STRUCT*) * observer_count);
    }

    for (size_t i = 0; i < observer_count; i++)
    {
        TRANSPORT_DATA_OBSERVER_STRUCT* observer = (TRANSPORT_DATA_OBSERVER_STRUCT*)zoo_list_at(observers, i);
        if (snapshot)
        {
            snapshot[i] = observer;
        }
    }
    ZOO_MUTEX_UNLOCK(&impl->common.transport->data_observers_lock);

    if (observer_count > 0 && snapshot == NULL)
    {
        ZOO_LOG_ERROR("Failed to allocate observer snapshot");
        zoo_free_to_pool(msg_buffer);
        zoo_smb_destroy_message(msg_out);
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    if (snapshot)
    {
        for (size_t i = 0; i < observer_count; i++)
        {
            TRANSPORT_DATA_OBSERVER_STRUCT* observer = snapshot[i];
            if (observer && observer->handler)
            {
                observer->handler(observer->user_data, msg_out);
            }
        }
        zoo_free_to_pool(snapshot);
    }

    zoo_free_to_pool(msg_buffer);
    zoo_smb_destroy_message(msg_out);
    return ZOO_SMB_OK;
}

/**
 * @brief Process messages from specific client with error handling
 * @param impl Server implementation instance
 * @param client Client information structure
 * @return Error code indicating success or failure
 */
static ZOO_ERROR_TYPE shm_server_process_client_messages(SHM_SERVER_IMPL_STRUCT* impl,
                                                             SHM_CLIENT_INFO_STRUCT* client)
{
    if (!impl || !client || !client->rx_ring)
    {
        ZOO_LOG_ERROR("Invalid parameters in shm_server_process_client_messages");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (!zoo_smb_ring_buffer_is_valid(client->rx_ring))
    {
        ZOO_LOG_ERROR("Client %s has invalid RX ring buffer", client->name);
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }

    if (zoo_smb_ring_buffer_is_empty(client->rx_ring))
    {
        ZOO_LOG_TRACE("No messages in RX ring buffer for client: %s", client->name);
        return ZOO_SMB_OK;
    }

    ZOO_LOG_DEBUG("Client %s has messages to process", client->name);

    int processed_count = 0;
    while (!zoo_smb_ring_buffer_is_empty(client->rx_ring) && processed_count < MAX_MESSAGES_PER_CYCLE)
    {
        ZOO_ERROR_TYPE result = shm_process_single_message(impl, client);
        if (ZOO_SMB_IS_ERROR(result))
        {
            ZOO_LOG_ERROR("Error processing message from client %s: %s",
                              client->name,
                              zoo_smb_get_error_string(result));
            break;
        }
        processed_count++;
    }

    if (processed_count > 0)
    {
        ZOO_LOG_TRACE("Processed %d messages from client %s", processed_count, client->name);
    }

    return ZOO_SMB_OK;
}

// ==============================================================================
// CLIENT REGISTRATION
// ==============================================================================

/**
 * @brief Validate and register a new client
 * @param impl Server implementation instance
 * @param reg_request Client registration request
 * @return Error code indicating success or failure
 */
static ZOO_ERROR_TYPE shm_validate_and_register_client(SHM_SERVER_IMPL_STRUCT* impl,
                                                           ZOO_SMB_SHM_CLIENT_REGISTRATION* reg_request)
{
    if (!impl || !reg_request)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    // Check if client is already registered
    for (size_t i = 0; i < zoo_list_size(impl->client_list); i++)
    {
        SHM_CLIENT_INFO_STRUCT* existing_client = (SHM_CLIENT_INFO_STRUCT*)zoo_list_at(impl->client_list, i);
        if (existing_client &&
            existing_client->pid == reg_request->client_pid &&
            strcmp(existing_client->name, reg_request->client_name) == 0)
        {
            ZOO_LOG_DEBUG("Client already registered: name=%s, pid=%d",
                              reg_request->client_name,
                              reg_request->client_pid);
            reg_request->status = ZOO_SMB_REGISTRATION_SUCCESS;
            return ZOO_SMB_OK;
        }
    }

    // Validate client registration
    ZOO_ERROR_TYPE validation_result = shm_validate_client_registration(
        impl->common.server_header, reg_request->client_name, reg_request->client_pid, reg_request->client_shm_key);

    if (ZOO_SMB_IS_ERROR(validation_result))
    {
        ZOO_LOG_ERROR("Client registration validation failed: %s",
                          zoo_smb_get_error_string(validation_result));
        reg_request->status = ZOO_SMB_REGISTRATION_FAILED;
        return validation_result;
    }

    // Attach to client shared memory
    int client_shm_id = shmget(reg_request->client_shm_key, 0, 0);
    if (client_shm_id == -1)
    {
        ZOO_LOG_ERROR("Failed to find client shared memory: name=%s, key=0x%08x",
                          reg_request->client_name,
                          reg_request->client_shm_key);
        reg_request->status = ZOO_SMB_REGISTRATION_FAILED;
        return ZOO_SMB_ERROR_SHM_NOT_FOUND;
    }

    void* client_shm_addr = shmat(client_shm_id, NULL, 0);
    if (client_shm_addr == (void*)-1)
    {
        ZOO_LOG_ERROR("Failed to attach to client shared memory: name=%s",
                          reg_request->client_name);
        reg_request->status = ZOO_SMB_REGISTRATION_FAILED;
        return ZOO_SMB_ERROR_SHM_ATTACH_FAILED;
    }

    // Get shared memory size
    struct shmid_ds shm_info;
    if (shmctl(client_shm_id, IPC_STAT, &shm_info) == -1)
    {
        ZOO_LOG_ERROR("Failed to get shared memory info: %s", strerror(errno));
        shmdt(client_shm_addr);
        reg_request->status = ZOO_SMB_REGISTRATION_FAILED;
        return ZOO_SMB_ERROR_SHM_DATA_CORRUPTED;
    }

    // Create and initialize client info structure
    SHM_CLIENT_INFO_STRUCT* client_info = (SHM_CLIENT_INFO_STRUCT*)zoo_allocate_from_pool(
        sizeof(SHM_CLIENT_INFO_STRUCT));
    if (!client_info)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for client info");
        shmdt(client_shm_addr);
        reg_request->status = ZOO_SMB_REGISTRATION_FAILED;
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    memset(client_info, 0, sizeof(SHM_CLIENT_INFO_STRUCT));
    strncpy(client_info->name, reg_request->client_name, sizeof(client_info->name) - 1);
    client_info->name[sizeof(client_info->name) - 1] = '\0';
    client_info->client_id = impl->next_client_id++;
    client_info->pid = reg_request->client_pid;
    client_info->event_fd = -1;
    client_info->shm_id = client_shm_id;
    client_info->shm_addr = client_shm_addr;
    client_info->shm_size = shm_info.shm_segsz;
    client_info->state = TRANSPORT_CONN_STATE_CONNECTING;
    client_info->last_heartbeat = time(NULL);

    ZOO_LOG_DEBUG("Client info created: name=%s, id=%d, pid=%d, event_fd=%d, shm_size=%zu",
                      client_info->name,
                      client_info->client_id,
                      client_info->pid,
                      client_info->event_fd,
                      client_info->shm_size);

    // Initialize client header
    SHM_CLIENT_HEADER_STRUCT* client_header = (SHM_CLIENT_HEADER_STRUCT*)client_shm_addr;
    client_header->magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    client_header->version = ZOO_SMB_PROTOCOL_VERSION;
    strncpy(client_header->client_name, reg_request->client_name, sizeof(client_header->client_name) - 1);
    client_header->client_name[sizeof(client_header->client_name) - 1] = '\0';

    ZOO_LOG_INFO("Initializing client header for %s", reg_request->client_name);

    // Initialize ring buffers
    uint8_t* base_addr = (uint8_t*)client_shm_addr;
    size_t client_header_size = sizeof(SHM_CLIENT_HEADER_STRUCT);
    size_t ring_buffer_size = (client_info->shm_size - client_header_size) / 2;

    ZOO_LOG_DEBUG("Ring buffer layout: total_size=%zu, header_size=%zu, ring_size=%zu each",
                      client_info->shm_size,
                      client_header_size,
                      ring_buffer_size);

    void* tx_ring_addr = base_addr + client_header_size;
    client_info->tx_ring = zoo_smb_ring_buffer_create_in_memory(tx_ring_addr, ring_buffer_size, ZOO_TRUE);
    if (!client_info->tx_ring)
    {
        ZOO_LOG_ERROR("Failed to create TX ring buffer for client: %s", client_info->name);
        shmdt(client_shm_addr);
        zoo_free_to_pool(client_info);
        reg_request->status = ZOO_SMB_REGISTRATION_FAILED;
        return ZOO_SMB_ERROR_RINGBUF_INIT_FAILED;
    }

    void* rx_ring_addr = base_addr + client_header_size + ring_buffer_size;
    client_info->rx_ring = zoo_smb_ring_buffer_create_in_memory(rx_ring_addr, ring_buffer_size, ZOO_TRUE);
    if (!client_info->rx_ring)
    {
        ZOO_LOG_ERROR("Failed to create RX ring buffer for client: %s", client_info->name);
        zoo_smb_ring_buffer_destroy(client_info->tx_ring);
        shmdt(client_shm_addr);
        zoo_free_to_pool(client_info);
        reg_request->status = ZOO_SMB_REGISTRATION_FAILED;
        return ZOO_SMB_ERROR_RINGBUF_INIT_FAILED;
    }

    ZOO_LOG_DEBUG("Initialized ring buffers for client %s: TX=%p (size=%zu), RX=%p (size=%zu)",
                      client_info->name,
                      tx_ring_addr,
                      ring_buffer_size,
                      rx_ring_addr,
                      ring_buffer_size);

    // Update client state to connected
    shm_server_update_client_state(impl, client_info, TRANSPORT_CONN_STATE_CONNECTED);

    // Add to client list
    ZOO_ERROR_TYPE result = zoo_list_push_back(impl->client_list, client_info);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_ERROR("Failed to add client to list: %s", zoo_smb_get_error_string(result));
        shm_cleanup_client_resources(client_info);
        zoo_free_to_pool(client_info);
        reg_request->status = ZOO_SMB_REGISTRATION_FAILED;
        return result;
    }

    // Update server header and complete registration
    if (impl->common.server_header)
        impl->common.server_header->client_count = zoo_list_size(impl->client_list);

    reg_request->status = ZOO_SMB_REGISTRATION_SUCCESS;

    ZOO_LOG_INFO("Client registration completed successfully: name=%s, id=%d, pid=%d, event_fd=%d, clients=%zu/%d",
                     client_info->name,
                     client_info->client_id,
                     client_info->pid,
                     client_info->event_fd,
                     zoo_list_size(impl->client_list),
                     impl->common.server_header->max_clients);

    // Clear the registration slot
    memset(reg_request, 0, sizeof(ZOO_SMB_SHM_CLIENT_REGISTRATION));
    reg_request->status = ZOO_SMB_REGISTRATION_EMPTY;

    return ZOO_SMB_OK;
}

/**
 * @brief Accept new client registrations
 * @param impl Server implementation instance
 * @return Error code indicating success or failure
 */
static ZOO_ERROR_TYPE shm_server_accept_new_clients(SHM_SERVER_IMPL_STRUCT* impl)
{
    if (!impl || !impl->common.server_header)
        return ZOO_SMB_ERROR_INVALID_PARAM;

    if (impl->common.server_header->client_count >= impl->common.server_header->max_clients)
        return ZOO_SMB_OK;

    ZOO_SMB_SHM_CLIENT_REGISTRATION* reg_area = (ZOO_SMB_SHM_CLIENT_REGISTRATION*)((uint8_t*)impl->common.server_header + sizeof(SHM_SERVER_HEADER_STRUCT));

    int processed_count = 0;

    for (uint32_t i = 0; i < ZOO_SMB_SHM_MAX_PENDING_REGISTRATIONS; i++)
    {
        ZOO_SMB_SHM_CLIENT_REGISTRATION* reg_request = &reg_area[i];

        if (reg_request->status != ZOO_SMB_REGISTRATION_PENDING)
            continue;

        ZOO_LOG_DEBUG("Processing registration slot %d: client_name=%s, pid=%d",
                          i,
                          reg_request->client_name,
                          reg_request->client_pid);

        ZOO_ERROR_TYPE result = shm_validate_and_register_client(impl, reg_request);
        processed_count++;

        if (ZOO_SMB_IS_ERROR(result))
        {
            ZOO_LOG_ERROR("Failed to process registration for client %s: %s",
                              reg_request->client_name,
                              zoo_smb_get_error_string(result));
        }
    }

    if (processed_count > 0)
    {
        ZOO_LOG_DEBUG("Registration processing complete: processed=%d", processed_count);
    }

    return ZOO_SMB_OK;
}

// ==============================================================================
// PUBLIC INTERFACE
// ==============================================================================

/**
 * @brief Send message to consumer via shared memory transport
 * @param impl_ptr Server implementation pointer
 * @param msg Message to send
 * @param receiver Target receiver name (NULL for broadcast)
 * @return Error code indicating success or failure
 */
ZOO_ERROR_TYPE shm_server_send(void* impl_ptr, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver)
{
    ZOO_SMB_VALIDATE_PTR(impl_ptr, ZOO_SMB_ERROR_INVALID_PARAM);
    ZOO_SMB_VALIDATE_PTR(msg, ZOO_SMB_ERROR_INVALID_PARAM);

    SHM_SERVER_IMPL_STRUCT* impl = (SHM_SERVER_IMPL_STRUCT*)impl_ptr;

    if (!impl->common.is_started)
    {
        ZOO_LOG_ERROR("SHM server transport is not started: %s",
                          impl->common.transport->config->name);
        return ZOO_SMB_ERROR_NOT_STARTED;
    }

    return receiver ? shm_send_to_target_client(impl, msg, receiver)
                    : shm_broadcast_to_all_clients(impl, msg);
}

/**
 * @brief Check if server is started
 * @param impl_ptr Server implementation pointer
 * @return ZOO_TRUE if server is running, ZOO_FALSE otherwise
 */
ZOO_BOOL shm_server_is_started(void* impl_ptr)
{
    if (!impl_ptr)
        return ZOO_FALSE;

    SHM_SERVER_IMPL_STRUCT* impl = (SHM_SERVER_IMPL_STRUCT*)impl_ptr;
    return impl->common.is_started && !impl->common.should_stop;
}

/**
 * @brief Initialize shared memory server transport
 */
ZOO_ERROR_TYPE shm_server_init(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    ZOO_SMB_VALIDATE_PTR(transport, ZOO_SMB_ERROR_INVALID_PARAM);
    ZOO_SMB_VALIDATE_PTR(transport->config, ZOO_SMB_ERROR_INVALID_PARAM);

    SHM_SERVER_IMPL_STRUCT* impl = (SHM_SERVER_IMPL_STRUCT*)zoo_allocate_from_pool(
        sizeof(SHM_SERVER_IMPL_STRUCT));
    if (!impl)
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;

    memset(impl, 0, sizeof(SHM_SERVER_IMPL_STRUCT));
    impl->common.transport = transport;
    impl->common.is_server = ZOO_TRUE;
    impl->common.is_started = ZOO_FALSE;
    impl->common.should_stop = ZOO_FALSE;
    impl->next_client_id = 1;
    impl->server_event_fd = -1;

    // Create client list
    impl->client_list = zoo_list_create(ZOO_SMB_SHM_MAX_CLIENTS);
    if (!impl->client_list)
    {
        zoo_free_to_pool(impl);
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    // Create sender mapping list (can have more senders than clients)
    impl->sender_mapping_list = zoo_list_create(ZOO_SMB_SHM_MAX_CLIENTS * 4);
    if (!impl->sender_mapping_list)
    {
        zoo_list_destroy(impl->client_list);
        zoo_free_to_pool(impl);
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    impl->common.master_shm_key = shm_generate_key(transport->config->name, 0);
    transport->impl = impl;

    ZOO_LOG_INFO("SHM server transport initialized: %s", transport->config->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Start shared memory server
 * @param impl_ptr Server implementation pointer
 * @return Error code indicating success or failure
 */
ZOO_ERROR_TYPE shm_server_start(void* impl_ptr)
{
    ZOO_SMB_VALIDATE_PTR(impl_ptr, ZOO_SMB_ERROR_INVALID_PARAM);

    SHM_SERVER_IMPL_STRUCT* impl = (SHM_SERVER_IMPL_STRUCT*)impl_ptr;

    if (impl->common.is_started)
    {
        ZOO_LOG_INFO("SHM server already started: %s", impl->common.transport->config->name);
        return ZOO_SMB_ERROR_ALREADY_STARTED;
    }

    ZOO_ERROR_TYPE result = shm_server_create_master_segment(impl);
    if (ZOO_SMB_IS_ERROR(result))
        return result;

    result = shm_server_init_event_fd(impl);
    if (ZOO_SMB_IS_ERROR(result))
    {
        if (impl->common.server_header)
        {
            shmdt(impl->common.server_header);
            impl->common.server_header = NULL;
        }
        if (impl->common.master_shm_id != -1)
        {
            shmctl(impl->common.master_shm_id, IPC_RMID, NULL);
            impl->common.master_shm_id = -1;
        }
        return result;
    }

    impl->common.is_started = ZOO_TRUE;
    impl->common.should_stop = ZOO_FALSE;

    if (impl->common.server_header)
        impl->common.server_header->server_running = ZOO_TRUE;

    ZOO_LOG_INFO("SHM server started: %s", impl->common.transport->config->name);

    // Main server loop
    while (!impl->common.should_stop)
    {
        shm_server_cleanup_disconnected_clients(impl);
        shm_server_accept_new_clients(impl);

        // Wait for events or timeout
        fd_set read_fds;
        struct timeval timeout = {.tv_sec = 0, .tv_usec = 50000};
        int max_fd = 0;

        FD_ZERO(&read_fds);
        if (impl->server_event_fd >= 0)
        {
            FD_SET(impl->server_event_fd, &read_fds);
            max_fd = impl->server_event_fd;
        }

        int select_result = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result > 0 && impl->server_event_fd >= 0 &&
            FD_ISSET(impl->server_event_fd, &read_fds))
        {
            uint64_t event_val;
            ssize_t bytes_read = read(impl->server_event_fd, &event_val, sizeof(event_val));
            if (bytes_read < 0)
            {
                ZOO_LOG_WARN("server eventfd read failed: %s", strerror(errno));
            }
        }

        // Process messages from all connected clients
        for (size_t i = 0; i < zoo_list_size(impl->client_list); i++)
        {
            SHM_CLIENT_INFO_STRUCT* client = (SHM_CLIENT_INFO_STRUCT*)zoo_list_at(impl->client_list, i);
            if (client && client->state == TRANSPORT_CONN_STATE_CONNECTED)
            {
                shm_server_process_client_messages(impl, client);
            }
        }
    }

    impl->common.is_started = ZOO_FALSE;
    if (impl->common.server_header)
        impl->common.server_header->server_running = ZOO_FALSE;

    ZOO_LOG_INFO("SHM server stopped: %s", impl->common.transport->config->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Stop shared memory server
 * @param impl_ptr Server implementation pointer
 * @return Error code indicating success or failure
 */
ZOO_ERROR_TYPE shm_server_stop(void* impl_ptr)
{
    ZOO_SMB_VALIDATE_PTR(impl_ptr, ZOO_SMB_ERROR_INVALID_PARAM);

    SHM_SERVER_IMPL_STRUCT* impl = (SHM_SERVER_IMPL_STRUCT*)impl_ptr;

    if (!impl->common.is_started)
    {
        ZOO_LOG_INFO("SHM server already stopped: %s",
                         impl->common.transport ? impl->common.transport->config->name : "unknown");
        return ZOO_SMB_OK;
    }

    ZOO_LOG_INFO("Stopping SHM server: %s", impl->common.transport->config->name);
    impl->common.should_stop = ZOO_TRUE;

    if (impl->common.server_header)
        impl->common.server_header->server_running = ZOO_FALSE;

    // Wait for server to stop gracefully
    time_t stop_start_time = time(NULL);
    while (impl->common.is_started &&
           (time(NULL) - stop_start_time) < SERVER_STOP_TIMEOUT_SECONDS)
    {
        usleep(100000);
    }

    // Disconnect all remaining clients
    if (impl->client_list)
    {
        for (size_t i = 0; i < zoo_list_size(impl->client_list); i++)
        {
            SHM_CLIENT_INFO_STRUCT* client = (SHM_CLIENT_INFO_STRUCT*)zoo_list_at(impl->client_list, i);
            if (client && client->state == TRANSPORT_CONN_STATE_CONNECTED)
            {
                shm_server_update_client_state(impl, client, TRANSPORT_CONN_STATE_DISCONNECTED);
            }
        }
    }

    impl->common.should_stop = ZOO_FALSE;
    ZOO_LOG_INFO("SHM server stop completed: %s", impl->common.transport->config->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Destroy shared memory server instance (enhanced)
 * @param impl_ptr Server implementation pointer
 */
void shm_server_destroy(void* impl_ptr)
{
    if (!impl_ptr)
        return;

    SHM_SERVER_IMPL_STRUCT* impl = (SHM_SERVER_IMPL_STRUCT*)impl_ptr;

    // Stop server if still running
    shm_server_stop(impl);

    // Close event file descriptor
    if (impl->server_event_fd >= 0)
    {
        close(impl->server_event_fd);
        impl->server_event_fd = -1;
    }

    // Clean up all sender mappings
    if (impl->sender_mapping_list)
    {
        while (!zoo_list_empty(impl->sender_mapping_list))
        {
            SHM_SENDER_MAPPING_STRUCT* mapping = (SHM_SENDER_MAPPING_STRUCT*)zoo_list_pop_front(impl->sender_mapping_list);
            if (mapping)
            {
                zoo_free_to_pool(mapping);
            }
        }
        zoo_list_destroy(impl->sender_mapping_list);
    }

    // Clean up all clients
    if (impl->client_list)
    {
        while (!zoo_list_empty(impl->client_list))
        {
            SHM_CLIENT_INFO_STRUCT* client = (SHM_CLIENT_INFO_STRUCT*)zoo_list_pop_front(impl->client_list);
            if (client)
            {
                shm_cleanup_client_resources(client);
                zoo_free_to_pool(client);
            }
        }
        zoo_list_destroy(impl->client_list);
    }

    // Clean up shared memory
    if (impl->common.server_header)
    {
        impl->common.server_header->server_running = ZOO_FALSE;
        shmdt(impl->common.server_header);
        shmctl(impl->common.master_shm_id, IPC_RMID, NULL);
        ZOO_LOG_INFO("Server shared memory removed (ID: %d)", impl->common.master_shm_id);
    }

    zoo_free_to_pool(impl);
    ZOO_LOG_INFO("SHM server transport destroyed");
}
