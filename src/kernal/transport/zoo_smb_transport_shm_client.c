/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_SHM_CLIENT
 * File name: zoo_smb_transport_shm_client.c
 * Description: Shared Memory Transport Client Implementation - Optimized
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-23     weiwang.sun       Created
 * 2.0       2025-06-23     weiwang.sun       Sync blocking interface
 * 3.0       2025-06-25     weiwang.sun       Code optimization and cleanup
 ******************************************************************************/

#include "zoo_smb_transport_shm_client.h"
#include "zoo_smb_transport_shm_common.h"
#include "zoo_log.h"
#include "zoo_smb_protocol.h"
#include "zoo_memory_pool.h"
#include <sys/select.h>
#include <sys/eventfd.h>
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

// ==============================================================================
// CONSTANTS AND CONFIGURATION
// ==============================================================================

#define CLIENT_EVENT_TIMEOUT_US 50000       /* 50ms select timeout */
#define CLIENT_REGISTRATION_MAX_RETRIES 10  /* Max registration retries */
#define CLIENT_REGISTRATION_RETRY_MS 100    /* Retry delay in milliseconds */
#define CLIENT_MAX_MESSAGE_SIZE (64 * 1024) /* Maximum message size */

// ==============================================================================
// SIMPLIFIED EVENT LOOP CONFIGURATION
// ==============================================================================

#define CLIENT_HEARTBEAT_INTERVAL_CYCLES 100 /* Heartbeat every 100 cycles (5 seconds) */
#define CLIENT_SELECT_TIMEOUT_MS 50          /* 50ms select timeout */

// ==============================================================================
// INTERNAL FUNCTION DECLARATIONS
// ==============================================================================
static ZOO_ERROR_TYPE client_attach_to_server(ZOO_SMB_SHM_CLIENT_IMPL* impl);
static ZOO_ERROR_TYPE client_register_with_server(ZOO_SMB_SHM_CLIENT_IMPL* impl);
static ZOO_ERROR_TYPE client_process_event_loop(ZOO_SMB_SHM_CLIENT_IMPL* impl);
static ZOO_ERROR_TYPE client_process_server_messages(ZOO_SMB_SHM_CLIENT_IMPL* impl);
static ZOO_ERROR_TYPE client_notify_server(ZOO_SMB_SHM_CLIENT_IMPL* impl);
static void client_cleanup_all_resources(ZOO_SMB_SHM_CLIENT_IMPL* impl);
static ZOO_BOOL client_validate_message_size(size_t size);
static ZOO_ERROR_TYPE client_notify_observers(ZOO_SMB_SHM_CLIENT_IMPL* impl,
                                                  const ZOO_SMB_MSG_STRUCT* message);
static ZOO_ERROR_TYPE client_listen_for_server_messages(ZOO_SMB_SHM_CLIENT_IMPL* impl);
static ZOO_ERROR_TYPE client_handle_heartbeat(ZOO_SMB_SHM_CLIENT_IMPL* impl);

// Add missing function declarations
static ZOO_BOOL client_validate_server_connection(ZOO_SMB_SHM_CLIENT_IMPL* impl);
static ZOO_BOOL client_validate_registration_status(ZOO_SMB_SHM_CLIENT_IMPL* impl);

// ==============================================================================
// PUBLIC API IMPLEMENTATIONS
// ==============================================================================

/**
 * @brief Initialize shared memory client transport
 * @param transport Transport handle to initialize
 * @return ZOO_SMB_OK on success, error code on failure
 * @note Creates client implementation structure and sets up basic configuration
 */
ZOO_ERROR_TYPE shm_client_init(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    ZOO_SMB_VALIDATE_PTR(transport, ZOO_SMB_ERROR_INVALID_PARAM);
    ZOO_SMB_VALIDATE_PTR(transport->config, ZOO_SMB_ERROR_INVALID_PARAM);

    // Allocate client implementation structure
    ZOO_SMB_SHM_CLIENT_IMPL* impl = (ZOO_SMB_SHM_CLIENT_IMPL*)zoo_allocate_from_pool(
        sizeof(ZOO_SMB_SHM_CLIENT_IMPL));
    if (!impl)
    {
        ZOO_LOG_ERROR("Failed to allocate client implementation");
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    // Initialize client state
    memset(impl, 0, sizeof(ZOO_SMB_SHM_CLIENT_IMPL));
    impl->transport = transport;
    impl->is_server = ZOO_FALSE;
    impl->is_started = ZOO_FALSE;
    impl->should_stop = ZOO_FALSE;
    impl->client_id = -1;
    impl->client_shm_id = -1;
    impl->master_shm_id = -1;
    impl->server_event_fd = -1;

    // Generate shared memory key based on transport name
    impl->master_shm_key = shm_generate_key(transport->config->name, 0);

    // Attach implementation to transport and copy callback references
    transport->impl = impl;

    ZOO_LOG_INFO("SHM client transport initialized: %s", transport->config->name);
    return ZOO_SMB_OK;
}

/**
 * @brief Stop shared memory client transport
 * @param impl_ptr Pointer to client implementation
 * @return ZOO_SMB_OK on success, error code on failure
 * @note Sets stop flag to gracefully exit event loop and waits for completion
 */
ZOO_ERROR_TYPE shm_client_stop(void* impl_ptr)
{
    ZOO_SMB_VALIDATE_PTR(impl_ptr, ZOO_SMB_ERROR_INVALID_PARAM);

    ZOO_SMB_SHM_CLIENT_IMPL* impl = (ZOO_SMB_SHM_CLIENT_IMPL*)impl_ptr;

    if (!impl->is_started)
    {
        ZOO_LOG_DEBUG("Client not started, nothing to stop");
        return ZOO_SMB_OK;
    }

    ZOO_LOG_INFO("SHM client stop requested: %s", impl->transport->config->name);

    // Step 1: Signal event loop to stop (but don't clean resources yet)
    impl->should_stop = ZOO_TRUE;

    // Step 2: Update client state in shared memory if still valid
    if (impl->client_info && impl->client_info->state == TRANSPORT_CONN_STATE_CONNECTED)
    {
        impl->client_info->state = TRANSPORT_CONN_STATE_RECONNECTING;
        ZOO_LOG_DEBUG("Client state changed to DISCONNECTING");
    }

    // Step 3: Wait briefly for event loop to exit gracefully
    int wait_cycles = 0;
    const int MAX_WAIT_CYCLES = 100;  // 100 * 10ms = 1 second max wait
    const int WAIT_INTERVAL_MS = 10;

    while (impl->is_started && wait_cycles < MAX_WAIT_CYCLES)
    {
        usleep(WAIT_INTERVAL_MS * 1000);
        wait_cycles++;
    }

    if (impl->is_started)
    {
        ZOO_LOG_WARN("Event loop did not exit gracefully after %dms, forcing stop",
                         MAX_WAIT_CYCLES * WAIT_INTERVAL_MS);
        impl->is_started = ZOO_FALSE;  // Force stop
    }
    else
    {
        ZOO_LOG_DEBUG("Event loop exited gracefully after %dms", wait_cycles * WAIT_INTERVAL_MS);
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Destroy shared memory client instance and cleanup resources
 * @param impl_ptr Pointer to client implementation
 * @note Thread-safe cleanup of all client resources
 */
void shm_client_destroy(void* impl_ptr)
{
    if (!impl_ptr)
    {
        return;
    }

    ZOO_SMB_SHM_CLIENT_IMPL* impl = (ZOO_SMB_SHM_CLIENT_IMPL*)impl_ptr;

    // Stop the transport first if running
    if (impl->is_started)
    {
        ZOO_LOG_DEBUG("Stopping client before destroy");
        shm_client_stop(impl);
    }

    // Small delay to ensure event loop has fully exited
    usleep(50000);  // 50ms

    // Cleanup all allocated resources
    client_cleanup_all_resources(impl);

    // Free implementation structure
    zoo_free_to_pool(impl);
    ZOO_LOG_INFO("SHM client transport destroyed");
}

/**
 * @brief Start shared memory client transport and enter event loop
 * @param impl_ptr Pointer to client implementation
 * @return ZOO_SMB_OK on successful completion, error code on failure
 * @note Blocking function that runs until client is stopped
 */
ZOO_ERROR_TYPE shm_client_start(void* impl_ptr)
{
    ZOO_SMB_VALIDATE_PTR(impl_ptr, ZOO_SMB_ERROR_INVALID_PARAM);

    ZOO_SMB_SHM_CLIENT_IMPL* impl = (ZOO_SMB_SHM_CLIENT_IMPL*)impl_ptr;

    if (impl->is_started)
    {
        ZOO_LOG_WARN("Client already started: %s", impl->transport->config->name);
        return ZOO_SMB_ERROR_ALREADY_STARTED;
    }

    // Phase 1: Connect to server
    ZOO_ERROR_TYPE result = client_attach_to_server(impl);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_ERROR("Failed to attach to server: %s", zoo_smb_get_error_string(result));
        return result;
    }

    // Phase 2: Register with server
    result = client_register_with_server(impl);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_ERROR("Failed to register with server: %s", zoo_smb_get_error_string(result));
        return result;
    }

    // Phase 3: Start event processing
    impl->is_started = ZOO_TRUE;
    impl->should_stop = ZOO_FALSE;

    ZOO_LOG_INFO("SHM client started: %s", impl->transport->config->name);
    return client_process_event_loop(impl);
}

/**
 * @brief Send message through shared memory transport
 * @param impl_ptr Pointer to client implementation
 * @param msg Message data to send
 * @param size Size of message in bytes
 * @param consumer Consumer information (ZOO_SMB_UNUSED in client mode)
 * @return ZOO_SMB_OK on success, error code on failure
 * @note Messages are written to RX ring buffer and server is notified
 */
ZOO_ERROR_TYPE shm_client_send(void* impl_ptr,
                                   const ZOO_SMB_MSG_STRUCT* msg,
                                   const char* receiver)
{
    ZOO_SMB_VALIDATE_PTR(impl_ptr, ZOO_SMB_ERROR_INVALID_PARAM);
    ZOO_SMB_VALIDATE_PTR(msg, ZOO_SMB_ERROR_INVALID_PARAM);
    ZOO_SMB_UNUSED(receiver);  // ZOO_SMB_UNUSED in client mode

    ZOO_SMB_SHM_CLIENT_IMPL* impl = (ZOO_SMB_SHM_CLIENT_IMPL*)impl_ptr;

    // Validate client state
    if (!impl->is_started)
    {
        ZOO_LOG_ERROR("Client transport not started");
        return ZOO_SMB_ERROR_NOT_STARTED;
    }

    if (!impl->client_info || !impl->client_info->rx_ring)
    {
        ZOO_LOG_ERROR("Client resources not initialized");
        return ZOO_SMB_ERROR_SERVICE_UNAVAILABLE;
    }

    uint8_t* msg_buffer = zoo_allocate_from_pool(MAX_TRANSPORT_BUFFER_SIZE);
    if (!msg_buffer)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for message buffer\n");
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    size_t msg_size = zoo_smb_protocol_serialize(msg, msg_buffer, MAX_TRANSPORT_BUFFER_SIZE);
    if (msg_size == 0)
    {
        ZOO_LOG_ERROR("Failed to serialize message\n");
        zoo_free_to_pool(msg_buffer);
        return ZOO_SMB_ERROR_TRANSPORT_SEND_FAILED;
    }

    // Write message to server's receive ring buffer
    ZOO_ERROR_TYPE result = zoo_smb_ring_buffer_write(impl->client_info->rx_ring, msg_buffer, msg_size);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_ERROR("Failed to write message to ring buffer: %s",
                          zoo_smb_get_error_string(result));
        zoo_free_to_pool(msg_buffer);
        return result;
    }

    // Update heartbeat timestamp
    impl->client_info->last_heartbeat = time(NULL);

    // Notify server about new message
    result = client_notify_server(impl);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_WARN("Failed to notify server, relying on polling");
    }

    zoo_free_to_pool(msg_buffer);
    ZOO_LOG_TRACE("Message sent successfully to server");
    return ZOO_SMB_OK;
}

/**
 * @brief Check if client transport is started and ready
 * @param impl_ptr Pointer to client implementation
 * @return ZOO_TRUE if client is started, ZOO_FALSE otherwise
 */
ZOO_BOOL shm_client_is_started(void* impl_ptr)
{
    if (!impl_ptr)
    {
        return ZOO_FALSE;
    }

    ZOO_SMB_SHM_CLIENT_IMPL* impl = (ZOO_SMB_SHM_CLIENT_IMPL*)impl_ptr;
    return impl->is_started;
}

// ==============================================================================
// INTERNAL IMPLEMENTATION FUNCTIONS
// ==============================================================================

/**
 * @brief Attach client to server's shared memory segment
 * @param impl Client implementation
 * @return ZOO_SMB_OK on success, error code on failure
 * @note Locates and validates server's master shared memory segment
 */
static ZOO_ERROR_TYPE client_attach_to_server(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    const int MAX_ATTACH_RETRIES = 10;
    const int RETRY_INTERVAL_MS = 500;

    for (int retry = 0; retry < MAX_ATTACH_RETRIES; retry++)
    {
        // Locate server's master shared memory segment
        impl->master_shm_id = shmget(impl->master_shm_key, 0, 0);
        if (impl->master_shm_id != -1)
        {
            // Found the shared memory, try to attach
            impl->server_header = (SHM_SERVER_HEADER_STRUCT*)shmat(impl->master_shm_id, NULL, 0);
            if (impl->server_header != (void*)-1)
            {
                // Validate server header integrity
                ZOO_ERROR_TYPE result = shm_validate_header(impl->server_header->magic,
                                                                impl->server_header->version);
                if (ZOO_SMB_IS_SUCCESS(result) && impl->server_header->server_running)
                {
                    ZOO_LOG_INFO("Successfully attached to server (SHM ID: %d, retry: %d)",
                                     impl->master_shm_id,
                                     retry);
                    return ZOO_SMB_OK;
                }

                // Invalid header or server not running
                shmdt(impl->server_header);
                impl->server_header = NULL;
            }
        }

        if (retry < MAX_ATTACH_RETRIES - 1)
        {
            ZOO_LOG_DEBUG("Server not found, retrying in %dms... (attempt %d/%d, key: 0x%x)",
                              RETRY_INTERVAL_MS,
                              retry + 1,
                              MAX_ATTACH_RETRIES,
                              impl->master_shm_key);
            usleep(RETRY_INTERVAL_MS * 1000);
        }
    }

    ZOO_LOG_ERROR("Server shared memory not found after %d retries (key: 0x%x): %s",
                      MAX_ATTACH_RETRIES,
                      impl->master_shm_key,
                      strerror(errno));
    return ZOO_SMB_ERROR_SHM_NOT_FOUND;
}

/**
 * @brief Register client with server and create client resources
 * @param impl Client implementation
 * @return ZOO_SMB_OK on success, error code on failure
 * @note Creates client shared memory, ring buffers, and registers with server
 */
static ZOO_ERROR_TYPE client_register_with_server(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    // Allocate client information structure
    impl->client_info = (SHM_CLIENT_INFO_STRUCT*)zoo_allocate_from_pool(
        sizeof(SHM_CLIENT_INFO_STRUCT));
    if (!impl->client_info)
    {
        ZOO_LOG_ERROR("Failed to allocate client info structure");
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    // Initialize client information
    memset(impl->client_info, 0, sizeof(SHM_CLIENT_INFO_STRUCT));
    snprintf(impl->client_info->name, sizeof(impl->client_info->name), "%s", impl->transport->config->name);
    impl->client_info->name[MAX_TRANSPORT_NAME_LENGTH - 1] = '\0';
    impl->client_info->pid = getpid();
    impl->client_info->state = TRANSPORT_CONN_STATE_CONNECTING;
    impl->client_info->connect_time = time(NULL);
    impl->client_info->last_heartbeat = impl->client_info->connect_time;
    impl->client_info->event_fd = INVALID_TRANSPORT_FD;

    // Generate unique shared memory key for this client
    impl->client_info->shm_key = shm_generate_key(impl->client_info->name, impl->client_info->pid);

    // Create event file descriptor for notifications
    int event_fd = eventfd(0, EFD_NONBLOCK);
    if (event_fd == -1)
    {
        ZOO_LOG_ERROR("Failed to create event fd: %s", strerror(errno));
        zoo_free_to_pool(impl->client_info);
        impl->client_info = NULL;
        return ZOO_SMB_ERROR_EVENTFD_CREATION_FAILED;
    }

    // Prepare registration information
    ZOO_SMB_SHM_CLIENT_REGISTRATION reg_info;
    memset(&reg_info, 0, sizeof(reg_info));
    snprintf(reg_info.client_name, sizeof(reg_info.client_name), "%s", impl->client_info->name);
    reg_info.client_pid = impl->client_info->pid;
    reg_info.client_event_fd = event_fd;

    // Create client's shared memory segment
    ZOO_ERROR_TYPE result = shm_create_client_segment(impl->client_info, &reg_info);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_ERROR("Failed to create client segment: %s", zoo_smb_get_error_string(result));
        close(event_fd);
        zoo_free_to_pool(impl->client_info);
        impl->client_info = NULL;
        return result;
    }

    // Initialize client resources (ring buffers, etc.)
    result = shm_init_client_resources(impl->client_info);
    if (ZOO_SMB_IS_ERROR(result))
    {
        ZOO_LOG_ERROR("Failed to initialize client resources: %s", zoo_smb_get_error_string(result));
        shm_cleanup_client_resources(impl->client_info);
        zoo_free_to_pool(impl->client_info);
        impl->client_info = NULL;
        return result;
    }

    ZOO_LOG_DEBUG("Client resources initialized with event fd: %d", impl->client_info->event_fd);

    // Register with server
    int client_id = shm_request_client_registration(
        impl->server_header,
        impl->client_info->name,
        impl->client_info->pid,
        impl->client_info->shm_key,
        impl->client_info->event_fd);

    if (client_id < 0)
    {
        ZOO_LOG_ERROR("Server registration failed");
        shm_cleanup_client_resources(impl->client_info);
        zoo_free_to_pool(impl->client_info);
        impl->client_info = NULL;
        return ZOO_SMB_ERROR_SHM_BUFFER_FULL;
    }

    // Update client state
    impl->client_info->client_id = client_id;
    impl->client_id = client_id;
    impl->client_info->state = TRANSPORT_CONN_STATE_CONNECTED;

    // Obtain server event fd for notifications
    int retry_count = 0;
    while (retry_count < CLIENT_REGISTRATION_MAX_RETRIES)
    {
        if (impl->server_header && impl->server_header->server_event_fd > 0)
        {
            impl->server_event_fd = impl->server_header->server_event_fd;
            ZOO_LOG_DEBUG("Server event fd obtained: %d (after %d retries)",
                              impl->server_event_fd,
                              retry_count);
            break;
        }

        usleep(CLIENT_REGISTRATION_RETRY_MS * 1000);
        retry_count++;
    }

    if (impl->server_event_fd <= 0)
    {
        ZOO_LOG_WARN("Server event fd not available, using alternative notification");
        impl->server_event_fd = -1;
    }

    ZOO_LOG_INFO("Client registered successfully: %s (ID: %d, PID: %d), server_event_fd: %d",
                     impl->client_info->name,
                     client_id,
                     impl->client_info->pid,
                     impl->server_event_fd);

    return ZOO_SMB_OK;
}

/**
 * @brief Simplified main event processing loop for client
 * @param impl Client implementation
 * @return ZOO_SMB_OK on normal exit, error code on failure
 * @note Simple 3-step process: 1) Listen for server messages, 2) Process server messages, 3) Handle heartbeat
 */
static ZOO_ERROR_TYPE client_process_event_loop(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    if (!impl)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_ERROR_TYPE result = ZOO_SMB_OK;
    uint32_t heartbeat_counter = 0;

    ZOO_LOG_INFO("Starting simplified client event loop: %s", impl->transport->config->name);

    while (!impl->should_stop && result == ZOO_SMB_OK)
    {
        // STEP 1: Listen for server messages
        result = client_listen_for_server_messages(impl);
        if (result != ZOO_SMB_OK)
        {
            if (result == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SHM_SYNCHRONIZATION_FAILED && impl->should_stop)
            {
                // Normal exit due to stop request
                result = ZOO_SMB_OK;
            }
            break;
        }

        // STEP 2: Process server messages (only if not stopping)
        if (!impl->should_stop)
        {
            result = client_process_server_messages(impl);
            if (result != ZOO_SMB_OK)
            {
                // Check if error is due to stopping process
                if (impl->should_stop &&
                    (result == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_INVALID_PARAM ||
                     result == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SERVICE_UNAVAILABLE))
                {
                    ZOO_LOG_DEBUG("Message processing error during shutdown, ignoring: %s",
                                      zoo_smb_get_error_string(result));
                    result = ZOO_SMB_OK;
                }
                break;
            }
        }

        // STEP 3: Handle heartbeat (every N cycles, skip if stopping)
        if (!impl->should_stop)
        {
            heartbeat_counter++;
            if (heartbeat_counter >= CLIENT_HEARTBEAT_INTERVAL_CYCLES)
            {
                heartbeat_counter = 0;
                result = client_handle_heartbeat(impl);
                if (result != ZOO_SMB_OK)
                {
                    break;
                }
            }
        }
    }

    // Mark as stopped before cleanup
    impl->is_started = ZOO_FALSE;

    // Send graceful disconnect message if possible
    if (impl->client_info && impl->client_info->state != TRANSPORT_CONN_STATE_DISCONNECTED)
    {
        impl->client_info->state = TRANSPORT_CONN_STATE_DISCONNECTED;
        ZOO_LOG_DEBUG("Client state changed to DISCONNECTED");
    }

    // Handle exit
    if (result != ZOO_SMB_OK && !impl->should_stop)
    {
        ZOO_LOG_ERROR("Client event loop exiting with error: %s", zoo_smb_get_error_string(result));
    }
    else
    {
        ZOO_LOG_INFO("Client event loop exiting normally");
    }

    return result;
}

/**
 * @brief STEP 1: Listen for server messages using select
 * @param impl Client implementation
 * @return ZOO_SMB_OK on success, error code on failure
 */
static ZOO_ERROR_TYPE client_listen_for_server_messages(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    fd_set read_fds;
    struct timeval timeout;
    int max_fd = 0;

    // Setup file descriptor set
    FD_ZERO(&read_fds);
    if (impl->client_info && impl->client_info->event_fd >= 0)
    {
        FD_SET(impl->client_info->event_fd, &read_fds);
        max_fd = impl->client_info->event_fd;
    }

    // Set timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = CLIENT_SELECT_TIMEOUT_MS * 1000;

    // Wait for events
    int select_result = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (select_result < 0)
    {
        if (errno == EINTR)
        {
            return ZOO_SMB_OK;  // Interrupted, continue
        }
        ZOO_LOG_ERROR("Select failed: %s", strerror(errno));
        return ZOO_SMB_ERROR_SHM_SYNCHRONIZATION_FAILED;
    }

    // Clear event fd if signaled
    if (select_result > 0 && impl->client_info && impl->client_info->event_fd >= 0 &&
        FD_ISSET(impl->client_info->event_fd, &read_fds))
    {
        ZOO_LOG_TRACE("Server event received on fd: %d", impl->client_info->event_fd);

        // Drain event fd
        uint64_t event_val;
        while (read(impl->client_info->event_fd, &event_val, sizeof(event_val)) == sizeof(event_val))
        {
            // Continue draining until EAGAIN
        }
    }

    return ZOO_SMB_OK;
}

/**
 * @brief STEP 2: Process all available server messages
 * @param impl Client implementation
 * @return ZOO_SMB_OK on success, error code on failure
 */
static ZOO_ERROR_TYPE client_process_server_messages(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    // Early exit check for shutdown
    if (impl->should_stop)
    {
        return ZOO_SMB_OK;
    }

    // Validate resources are still available
    if (!impl->client_info || !impl->client_info->tx_ring)
    {
        if (impl->should_stop)
        {
            // Resources might be cleaned up during shutdown
            ZOO_LOG_DEBUG("Client resources unavailable during shutdown");
            return ZOO_SMB_OK;
        }
        ZOO_LOG_ERROR("Client resources not initialized");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    while (!impl->should_stop && zoo_smb_ring_buffer_has_data(impl->client_info->tx_ring))
    {
        size_t msg_size = zoo_smb_ring_buffer_get_next_message_size(impl->client_info->tx_ring);
        if (!client_validate_message_size(msg_size))
        {
            ZOO_LOG_ERROR("Invalid message size from server: %zu", msg_size);
            zoo_smb_ring_buffer_clear(impl->client_info->tx_ring);
            return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
        }

        ZOO_LOG_DEBUG("Processing server message of size: %zu", msg_size);
        uint8_t* msg_buffer = (uint8_t*)zoo_allocate_from_pool(msg_size);
        if (!msg_buffer)
        {
            ZOO_LOG_ERROR("Failed to allocate message buffer");
            return ZOO_SMB_ERROR_OUT_OF_MEMORY;
        }

        ZOO_ERROR_TYPE result = zoo_smb_ring_buffer_read(impl->client_info->tx_ring, msg_buffer, msg_size);
        if (ZOO_SMB_IS_ERROR(result))
        {
            zoo_free_to_pool(msg_buffer);
            ZOO_LOG_ERROR("Failed to read message from server: %s", zoo_smb_get_error_string(result));
            return result;
        }

        ZOO_SMB_MSG_STRUCT* msg_out = zoo_smb_default_message();
        if (!msg_out)
        {
            zoo_free_to_pool(msg_buffer);
            ZOO_LOG_ERROR("Failed to create message structure");
            return ZOO_SMB_ERROR_OUT_OF_MEMORY;
        }

        result = zoo_smb_protocol_deserialize(msg_buffer, msg_size, msg_out, sizeof(ZOO_SMB_MSG_STRUCT));
        if (result == ZOO_SMB_OK && !impl->should_stop)
        {
            result = client_notify_observers(impl, msg_out);
        }

        zoo_smb_destroy_message(msg_out);
        zoo_free_to_pool(msg_buffer);

        if (ZOO_SMB_IS_ERROR(result))
        {
            return result;
        }
    }

    return ZOO_SMB_OK;
}

/**
 * @brief STEP 3: Handle heartbeat and connection state validation
 * @param impl Client implementation
 * @return ZOO_SMB_OK on success, error code on connection lost
 */
static ZOO_ERROR_TYPE client_handle_heartbeat(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    // Skip heartbeat if stopping
    if (impl->should_stop)
    {
        return ZOO_SMB_OK;
    }

    if (!impl->client_info)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_LOG_TRACE("Performing heartbeat and connection state check");

    // 1. Validate server connection
    if (!client_validate_server_connection(impl))
    {
        ZOO_LOG_ERROR("Server connection validation failed");
        return ZOO_SMB_ERROR_TRANSPORT_CONNECTION_LOST;
    }

    // 2. Check server running status
    if (impl->server_header && !impl->server_header->server_running)
    {
        ZOO_LOG_ERROR("Server is no longer running");
        return ZOO_SMB_ERROR_TRANSPORT_CONNECTION_LOST;
    }

    // 3. Validate client registration status
    if (!client_validate_registration_status(impl))
    {
        ZOO_LOG_ERROR("Client registration validation failed");
        return ZOO_SMB_ERROR_TRANSPORT_CONNECTION_LOST;
    }

    // 4. Update connection state (only if not disconnecting)
    time_t current_time = time(NULL);
    impl->client_info->last_heartbeat = current_time;

    // Ensure connection state is correct (but don't override DISCONNECTING state)
    if (impl->client_info->state != TRANSPORT_CONN_STATE_CONNECTED &&
        impl->client_info->state != TRANSPORT_CONN_STATE_RECONNECTING)
    {
        ZOO_LOG_WARN("Correcting client connection state from %d to CONNECTED",
                         impl->client_info->state);
        impl->client_info->state = TRANSPORT_CONN_STATE_CONNECTED;
    }

    ZOO_LOG_TRACE("Heartbeat completed successfully");
    return ZOO_SMB_OK;
}

/**
 * @brief Simplified server connection validation
 * @param impl Client implementation
 * @return ZOO_TRUE if server connection is valid, ZOO_FALSE otherwise
 */
static ZOO_BOOL client_validate_server_connection(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    if (!impl->server_header)
    {
        ZOO_LOG_ERROR("Server header is NULL");
        return ZOO_FALSE;
    }

    // Simple magic number check
    if (impl->server_header->magic != ZOO_SMB_MSG_MAGIC_NUMBER)
    {
        ZOO_LOG_ERROR("Server magic number invalid: 0x%x", impl->server_header->magic);
        return ZOO_FALSE;
    }

    ZOO_LOG_TRACE("Server connection validation passed");
    return ZOO_TRUE;
}

/**
 * @brief Simplified client registration validation
 * @param impl Client implementation
 * @return ZOO_TRUE if registration is valid, ZOO_FALSE otherwise
 */
static ZOO_BOOL client_validate_registration_status(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    if (!impl->client_info || impl->client_id < 0)
    {
        ZOO_LOG_ERROR("Invalid client info or client ID: client_id=%d", impl->client_id);
        return ZOO_FALSE;
    }

    // Check if client ID is in valid range
    if (impl->client_id >= ZOO_SMB_SHM_MAX_CLIENTS)
    {
        ZOO_LOG_ERROR("Client ID out of range: %d >= %d", impl->client_id, ZOO_SMB_SHM_MAX_CLIENTS);
        return ZOO_FALSE;
    }

    // Check if ring buffers are still valid
    if (!impl->client_info->rx_ring || !impl->client_info->tx_ring)
    {
        ZOO_LOG_ERROR("Client ring buffers are NULL");
        return ZOO_FALSE;
    }

    ZOO_LOG_TRACE("Client registration validation passed: client_id=%d", impl->client_id);
    return ZOO_TRUE;
}

/**
 * @brief Notify server about new message (simplified)
 * @param impl Client implementation
 * @return ZOO_SMB_OK on success, error code on failure
 */
static ZOO_ERROR_TYPE client_notify_server(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    // Primary method: Event file descriptor
    if (impl->server_event_fd >= 0)
    {
        uint64_t event_val = 1;
        ssize_t write_result = write(impl->server_event_fd, &event_val, sizeof(event_val));
        if (write_result == sizeof(event_val))
        {
            ZOO_LOG_TRACE("Server notified via event fd: %d", impl->server_event_fd);
            return ZOO_SMB_OK;
        }
        else
        {
            ZOO_LOG_WARN("Failed to notify server via event fd: %s", strerror(errno));
        }
    }

    // Fallback: Set flag in shared memory
    if (impl->server_header)
    {
        impl->server_header->has_pending_messages = ZOO_TRUE;
        ZOO_LOG_TRACE("Server notified via shared memory flag");
        return ZOO_SMB_OK;
    }

    ZOO_LOG_DEBUG("No notification mechanism available");
    return ZOO_SMB_ERROR_SERVICE_UNAVAILABLE;
}

/**
 * @brief Cleanup all client resources and disconnect from server
 * @param impl Client implementation
 * @note Sends disconnect message and releases all allocated resources
 */
static void client_cleanup_all_resources(ZOO_SMB_SHM_CLIENT_IMPL* impl)
{
    if (!impl)
    {
        return;
    }

    // Send disconnect notification to server
    if (impl->client_info && impl->client_info->rx_ring && impl->client_id >= 0)
    {
        ZOO_SMB_MSG_HEADER_STRUCT disconnect_msg = {0};
        disconnect_msg.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
        disconnect_msg.msg_type = ZOO_SMB_MSG_TYPE_HB;
        disconnect_msg.payload_size = 0;
        disconnect_msg.timestamp = time(NULL);
        snprintf(disconnect_msg.sender, sizeof(disconnect_msg.sender), "%s", impl->client_info->name);
        disconnect_msg.sender[sizeof(disconnect_msg.sender) - 1] = '\0';

        zoo_smb_ring_buffer_write(impl->client_info->rx_ring, &disconnect_msg, sizeof(disconnect_msg));

        // Notify server of disconnect
        if (impl->server_event_fd >= 0)
        {
            uint64_t event_val = 1;
            ssize_t wr = write(impl->server_event_fd, &event_val, sizeof(event_val));
            if (wr != sizeof(event_val))
            {
                ZOO_LOG_WARN("Failed to notify server via event fd: %s", strerror(errno));
            }
        }
    }

    // Cleanup client-specific resources
    if (impl->client_info)
    {
        shm_cleanup_client_resources(impl->client_info);
        zoo_free_to_pool(impl->client_info);
        impl->client_info = NULL;
    }

    // Detach from server shared memory
    if (impl->server_header)
    {
        if (shmdt(impl->server_header) == -1)
        {
            ZOO_LOG_ERROR("Failed to detach from server shared memory: %s", strerror(errno));
        }
        impl->server_header = NULL;
    }

    // Reset all handles and IDs
    impl->server_event_fd = -1;
    impl->client_id = -1;
    impl->master_shm_id = -1;
    impl->client_shm_id = -1;
}

/**
 * @brief Validate message size against limits
 * @param size Message size to validate
 * @return ZOO_TRUE if size is valid, ZOO_FALSE otherwise
 */
static ZOO_BOOL client_validate_message_size(size_t size)
{
    return (size > 0 && size <= CLIENT_MAX_MESSAGE_SIZE);
}

/**
 * @brief Notify all registered observers about received message
 * @param impl Client implementation
 * @param header Message header
 * @param payload Message payload
 * @param payload_size Size of payload
 * @return ZOO_SMB_OK on success, error code on failure
 */
static ZOO_ERROR_TYPE client_notify_observers(ZOO_SMB_SHM_CLIENT_IMPL* impl,
                                                  const ZOO_SMB_MSG_STRUCT* message)
{
    size_t observer_count = 0;
    TRANSPORT_DATA_OBSERVER_STRUCT** snapshot = NULL;

    ZOO_MUTEX_LOCK(&impl->transport->data_observers_lock);
    observer_count = zoo_list_size(impl->transport->data_observers);
    if (observer_count > 0)
    {
        snapshot = (TRANSPORT_DATA_OBSERVER_STRUCT**)zoo_allocate_from_pool(sizeof(TRANSPORT_DATA_OBSERVER_STRUCT*) * observer_count);
    }

    for (size_t i = 0; i < observer_count; i++)
    {
        TRANSPORT_DATA_OBSERVER_STRUCT* observer = (TRANSPORT_DATA_OBSERVER_STRUCT*)zoo_list_at(impl->transport->data_observers, i);
        if (snapshot)
        {
            snapshot[i] = observer;
        }
        else if (observer && observer->handler)
        {
            observer->handler(observer->user_data, message);
        }
    }
    ZOO_MUTEX_UNLOCK(&impl->transport->data_observers_lock);

    if (snapshot)
    {
        for (size_t i = 0; i < observer_count; i++)
        {
            TRANSPORT_DATA_OBSERVER_STRUCT* observer = snapshot[i];
            if (observer && observer->handler)
            {
                observer->handler(observer->user_data, message);
            }
        }
        zoo_free_to_pool(snapshot);
    }

    return ZOO_SMB_OK;
}