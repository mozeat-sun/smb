/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_SHM_COMMON
 * File name: zoo_smb_transport_shm_common.c
 * Description: Shared Memory Transport Common Implementation
 *              Implements shared functionality for SHM transport including
 *              client segment creation, resource management, validation,
 *              and registration handling.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-23     weiwang.sun       Created
 * 2.0       2025-07-28     weiwang.sun       Enhanced validation functions
 ******************************************************************************/

#include "zoo_smb_transport_shm_common.h"
#include "zoo_log.h"
#include "zoo_smb_protocol.h"
#include "zoo_memory_pool.h"
#include "zoo.h"
#include <sys/shm.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>

/**
 * @brief Creates a shared memory segment for a SMB client.
 *
 * This function initializes and creates a shared memory segment for the specified
 * SMB client. It sets up the necessary resources and data structures required for
 * inter-process communication between the client and the server using shared memory.
 *
 * @param client Pointer to a SHM_CLIENT_INFO_STRUCT structure containing information
 *               about the client for which the shared memory segment is to be created.
 *
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
 *         Possible values include success or specific error codes related to shared memory
 *         creation or initialization failures.
 */
ZOO_ERROR_TYPE shm_create_client_segment(SHM_CLIENT_INFO_STRUCT* client,
                                             ZOO_SMB_SHM_CLIENT_REGISTRATION* reg_info)
{
    if (!client || !reg_info)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Calculate required size for client segment
    size_t segment_size = ZOO_SMB_SHM_CLIENT_SIZE;

    // Create shared memory segment
    int shm_id = shmget(client->shm_key, segment_size, IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id == -1)
    {
        if (errno == EEXIST)
        {
            ZOO_LOG_WARN("Client shared memory already exists, removing...");
            int old_id = shmget(client->shm_key, 0, 0);
            if (old_id != -1)
            {
                shmctl(old_id, IPC_RMID, NULL);
            }
            shm_id = shmget(client->shm_key, segment_size, IPC_CREAT | 0666);
        }

        if (shm_id == -1)
        {
            ZOO_LOG_ERROR("Failed to create client shared memory: %s", strerror(errno));
            return ZOO_SMB_ERROR_SHM_CREATE_FAILED;
        }
    }

    // Attach to shared memory
    void* shm_addr = shmat(shm_id, NULL, 0);
    if (shm_addr == (void*)-1)
    {
        ZOO_LOG_ERROR("Failed to attach client shared memory: %s", strerror(errno));
        shmctl(shm_id, IPC_RMID, NULL);
        return ZOO_SMB_ERROR_SHM_ATTACH_FAILED;
    }

    // Initialize client shared memory
    memset(shm_addr, 0, segment_size);

    // Set client info
    client->shm_id = shm_id;
    client->shm_addr = shm_addr;
    client->shm_size = segment_size;

    ZOO_LOG_DEBUG("Client shared memory created successfully: name=%s, ID=%d, key=0x%08x, size=%zu, addr=%p",
                      client->name,
                      shm_id,
                      client->shm_key,
                      segment_size,
                      shm_addr);

    return ZOO_SMB_OK;
}

/**
 * @brief Initialize client resources (ring buffers, event fd).
 *
 * This function sets up the eventfd and ring buffers for the client in shared memory.
 *
 * @param client Pointer to the client info structure.
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE shm_init_client_resources(SHM_CLIENT_INFO_STRUCT* client)
{
    if (!client || !client->shm_addr)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Set event fd (should be created before calling this function)
    if (client->event_fd <= 0)
    {
        client->event_fd = eventfd(0, EFD_NONBLOCK);
        if (client->event_fd == -1)
        {
            ZOO_LOG_ERROR("Failed to create client event fd: %s", strerror(errno));
            return ZOO_SMB_ERROR_EVENTFD_CREATION_FAILED;
        }
    }

    // Calculate ring buffer positions
    uint8_t* base_addr = (uint8_t*)client->shm_addr;
    size_t header_size = sizeof(SHM_CLIENT_HEADER_STRUCT);
    size_t ring_size = (client->shm_size - header_size) / 2;

    ZOO_LOG_DEBUG("Client ring buffer layout: total_size=%zu, header_size=%zu, ring_size=%zu each",
                      client->shm_size,
                      header_size,
                      ring_size);

    // Create TX ring buffer (server -> client)
    void* tx_addr = base_addr + header_size;
    client->tx_ring = zoo_smb_ring_buffer_create_in_memory(tx_addr, ring_size, ZOO_TRUE);
    if (!client->tx_ring)
    {
        ZOO_LOG_ERROR("Failed to create TX ring buffer for client: %s", client->name);
        close(client->event_fd);
        client->event_fd = -1;
        return ZOO_SMB_ERROR_RINGBUF_CREATE_FAILED;
    }

    // Create RX ring buffer (client -> server)
    void* rx_addr = base_addr + header_size + ring_size;
    client->rx_ring = zoo_smb_ring_buffer_create_in_memory(rx_addr, ring_size, ZOO_TRUE);
    if (!client->rx_ring)
    {
        ZOO_LOG_ERROR("Failed to create RX ring buffer for client: %s", client->name);
        zoo_smb_ring_buffer_destroy(client->tx_ring);
        client->tx_ring = NULL;
        close(client->event_fd);
        client->event_fd = -1;
        return ZOO_SMB_ERROR_RINGBUF_CREATE_FAILED;
    }

    ZOO_LOG_DEBUG("Initialized client resources: TX ring=%p (size=%zu), RX ring=%p (size=%zu), event_fd=%d",
                      tx_addr,
                      ring_size,
                      rx_addr,
                      ring_size,
                      client->event_fd);
    return ZOO_SMB_OK;
}

/**
 * @brief Clean up client resources.
 *
 * This function releases all resources associated with the client, including ring buffers,
 * event fd, and shared memory segment.
 *
 * @param client Pointer to the client info structure.
 */
void shm_cleanup_client_resources(SHM_CLIENT_INFO_STRUCT* client)
{
    if (!client)
        return;

    // Destroy ring buffers
    if (client->tx_ring)
    {
        zoo_smb_ring_buffer_destroy(client->tx_ring);
        client->tx_ring = NULL;
    }

    if (client->rx_ring)
    {
        zoo_smb_ring_buffer_destroy(client->rx_ring);
        client->rx_ring = NULL;
    }

    // Close event fd
    if (client->event_fd > 0)
    {
        close(client->event_fd);
        client->event_fd = -1;
    }

    // Detach from shared memory
    if (client->shm_addr)
    {
        if (shmdt(client->shm_addr) == -1)
        {
            ZOO_LOG_ERROR("Failed to detach client shared memory: %s", strerror(errno));
        }
        client->shm_addr = NULL;
    }

    // Remove shared memory segment if we own it
    if (client->shm_id > 0)
    {
        if (shmctl(client->shm_id, IPC_RMID, NULL) == -1)
        {
            ZOO_LOG_ERROR("Failed to remove client shared memory: %s", strerror(errno));
        }
        client->shm_id = -1;
    }
}

/**
 * @brief Validate shared memory header basic fields.
 *
 * Checks the magic number and version fields of a shared memory header.
 *
 * @param magic Magic number to validate.
 * @param version Version number to validate.
 * @return ZOO_ERROR_TYPE Returns an error code if validation fails.
 */
ZOO_ERROR_TYPE shm_validate_header(uint32_t magic, uint32_t version)
{
    if (magic != ZOO_SMB_MSG_MAGIC_NUMBER)
    {
        ZOO_LOG_ERROR("Invalid magic number: 0x%08x, expected: 0x%08x",
                          magic,
                          ZOO_SMB_MSG_MAGIC_NUMBER);
        return ZOO_SMB_ERROR_SHM_DATA_INVALID_MAGIC;
    }

    if (version != ZOO_SMB_PROTOCOL_VERSION)
    {
        ZOO_LOG_ERROR("Version mismatch: %u, expected: %u",
                          version,
                          ZOO_SMB_PROTOCOL_VERSION);
        return ZOO_SMB_ERROR_SHM_DATA_VERSION_MISMATCH;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Validate complete server header structure.
 *
 * Validates the fields of a server shared memory header.
 *
 * @param header Pointer to the server header structure.
 * @return ZOO_ERROR_TYPE Returns an error code if validation fails.
 */
ZOO_ERROR_TYPE shm_validate_server_header(const SHM_SERVER_HEADER_STRUCT* header)
{
    if (!header)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Validate basic header fields
    ZOO_ERROR_TYPE result = shm_validate_header(header->magic, header->version);
    if (ZOO_SMB_IS_ERROR(result))
    {
        return result;
    }

    // Validate server specific fields
    if (header->max_clients == 0 || header->max_clients > ZOO_SMB_SHM_MAX_CLIENTS)
    {
        ZOO_LOG_ERROR("Invalid max_clients: %u", header->max_clients);
        return ZOO_SMB_ERROR_SHM_DATA_CORRUPTED;
    }

    if (header->client_count > header->max_clients)
    {
        ZOO_LOG_ERROR("Client count exceeds maximum: %u > %u",
                          header->client_count,
                          header->max_clients);
        return ZOO_SMB_ERROR_SHM_DATA_CORRUPTED;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Validate client header structure.
 *
 * Validates the fields of a client shared memory header.
 *
 * @param header Pointer to the client header structure.
 * @return ZOO_ERROR_TYPE Returns an error code if validation fails.
 */
ZOO_ERROR_TYPE shm_validate_client_header(const SHM_CLIENT_HEADER_STRUCT* header)
{
    if (!header)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Validate basic header fields
    ZOO_ERROR_TYPE result = shm_validate_header(header->magic, header->version);
    if (ZOO_SMB_IS_ERROR(result))
    {
        return result;
    }

    // Validate client specific fields
    if (strlen(header->client_name) == 0)
    {
        ZOO_LOG_ERROR("Empty client name");
        return ZOO_SMB_ERROR_INVALID_NAME;
    }

    if (header->pid <= 0)
    {
        ZOO_LOG_ERROR("Invalid client PID: %d", header->pid);
        return ZOO_SMB_ERROR_PROCESS_NOT_FOUND;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Generate shared memory key based on name.
 *
 * Generates a unique key for shared memory based on the provided name and segment id.
 *
 * @param name Name string to hash.
 * @param segment_id Segment identifier.
 * @return key_t The generated shared memory key.
 */
key_t shm_generate_key(const char* name, int segment_id)
{
    if (!name)
    {
        return -1;
    }

    // Simple hash algorithm based on name
    uint32_t hash = 0;
    size_t name_len = strlen(name);

    for (size_t i = 0; i < name_len; i++)
    {
        hash = ((hash << 5) + hash) + (uint32_t)name[i];
    }

    // Combine with base key and segment ID
    key_t key = ZOO_SMB_SHM_KEY_BASE + (hash & 0xFFFF) + (segment_id << 16);

    ZOO_LOG_DEBUG("Generated SHM key: 0x%08x for name '%s', segment %d (hash: 0x%08x)",
                      key,
                      name,
                      segment_id,
                      hash);

    return key;
}

/**
 * @brief Check if process is still alive.
 *
 * Uses kill(2) with signal 0 to check if a process exists.
 *
 * @param pid Process ID to check.
 * @return ZOO_TRUE if the process exists, ZOO_FALSE otherwise.
 */
ZOO_BOOL shm_is_process_alive(pid_t pid)
{
    if (pid <= 0)
        return ZOO_FALSE;

    return (kill(pid, 0) == 0);
}

/**
 * @brief Notify peer through event fd.
 *
 * Writes to the event fd to notify the peer process.
 *
 * @param event_fd The event file descriptor.
 */
void shm_notify_peer(int event_fd)
{
    if (event_fd < 0)
    {
        ZOO_LOG_DEBUG("Invalid event fd for notification: %d", event_fd);
        return;
    }

    int flags = fcntl(event_fd, F_GETFL);
    if (flags == -1)
    {
        if (errno == EBADF)
        {
            ZOO_LOG_WARN("Event fd %d is invalid (closed or corrupted)", event_fd);
            return;
        }
        else
        {
            ZOO_LOG_WARN("Failed to check event fd %d flags: %s", event_fd, strerror(errno));
        }
    }

    uint64_t event_val = 1;
    ssize_t result = write(event_fd, &event_val, sizeof(event_val));

    if (result != sizeof(event_val))
    {
        if (errno == EBADF)
        {
            ZOO_LOG_WARN("Event fd %d is bad file descriptor", event_fd);
            return;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            ZOO_LOG_DEBUG("Event fd %d would block, notification skipped", event_fd);
            return;
        }
        else
        {
            ZOO_LOG_WARN("Failed to notify peer via event fd %d: %s", event_fd, strerror(errno));
            return;
        }
    }

    ZOO_LOG_TRACE("Successfully notified peer via event fd: %d", event_fd);
}

/**
 * @brief Notify client through event fd.
 *
 * Calls shm_notify_peer for the client's event fd.
 *
 * @param client Pointer to the client info structure.
 */
void shm_notify_client(SHM_CLIENT_INFO_STRUCT* client)
{
    if (client && client->event_fd > 0)
    {
        shm_notify_peer(client->event_fd);
    }
}

/**
 * @brief Request client registration with server.
 *
 * Attempts to register a client with the server by filling a registration slot.
 *
 * @param server_header Pointer to the server header structure.
 * @param client_name Name of the client.
 * @param client_pid PID of the client process.
 * @param client_shm_key Shared memory key for the client.
 * @param client_event_fd Event fd for the client.
 * @return int Assigned client ID on success, -1 on failure.
 */
int shm_request_client_registration(SHM_SERVER_HEADER_STRUCT* server_header,
                                    const char* client_name,
                                    pid_t client_pid,
                                    key_t client_shm_key,
                                    int client_event_fd)
{
    if (!server_header || !client_name)
    {
        return -1;
    }

    // Validate registration request
    ZOO_ERROR_TYPE validation_result = shm_validate_client_registration(
        server_header, client_name, client_pid, client_shm_key);
    if (ZOO_SMB_IS_ERROR(validation_result))
    {
        ZOO_LOG_ERROR("Client registration validation failed: %s",
                          zoo_smb_get_error_string(validation_result));
        return -1;
    }

    // Try to acquire registration lock
    ZOO_LOG_DEBUG("Attempting to acquire registration lock for client: %s", client_name);

    int attempts = 0;
    const int max_attempts = 100;
    const int retry_delay_us = 10000;  // 10ms

    while (attempts < max_attempts)
    {
        // Use GCC builtin atomic compare-and-swap (platform-specific)
        // TODO(smb-transport): Replace with zoo.h atomic operations when CAS is available.
        if (__sync_bool_compare_and_swap(&server_header->registration_lock, 0, 1))
        {
            // Lock acquired
            break;
        }
        ZOO_SMB_SLEEP_US(retry_delay_us);
        attempts++;
    }

    if (attempts >= max_attempts)
    {
        ZOO_LOG_ERROR("Failed to acquire registration lock after %d attempts", max_attempts);
        return -1;
    }

    ZOO_LOG_DEBUG("Registration lock acquired after %d attempts", attempts);

    // Find an available client slot
    int assigned_id = -1;

    // Get registration area from shared memory
    uint8_t* shm_base = (uint8_t*)server_header + sizeof(SHM_SERVER_HEADER_STRUCT);
    ZOO_SMB_SHM_CLIENT_REGISTRATION* reg_area = (ZOO_SMB_SHM_CLIENT_REGISTRATION*)shm_base;

    // Find an empty registration slot
    for (uint32_t slot = 0; slot < ZOO_SMB_SHM_MAX_PENDING_REGISTRATIONS; slot++)
    {
        ZOO_SMB_SHM_CLIENT_REGISTRATION* reg_slot = &reg_area[slot];

        if (reg_slot->status == ZOO_SMB_REGISTRATION_EMPTY)
        {
            // Fill the registration request
            memset(reg_slot, 0, sizeof(ZOO_SMB_SHM_CLIENT_REGISTRATION));
            reg_slot->status = ZOO_SMB_REGISTRATION_PENDING;
            snprintf(reg_slot->client_name, MAX_TRANSPORT_NAME_LENGTH, "%s", client_name);
            reg_slot->client_name[MAX_TRANSPORT_NAME_LENGTH - 1] = '\0';
            reg_slot->client_pid = client_pid;
            reg_slot->client_shm_key = client_shm_key;
            reg_slot->client_shm_size = ZOO_SMB_SHM_CLIENT_SIZE;
            reg_slot->client_event_fd = client_event_fd;
            reg_slot->timestamp = time(NULL);

            // Assign client ID
            assigned_id = server_header->next_available_id++;

            // Update server statistics
            server_header->client_count++;

            ZOO_LOG_INFO("Client registration successful: %s (ID: %d, PID: %d, Key: 0x%08x, EventFD: %d, Count: %d/%d)",
                             client_name,
                             assigned_id,
                             client_pid,
                             client_shm_key,
                             client_event_fd,
                             server_header->client_count,
                             server_header->max_clients);
            break;
        }
    }

    if (assigned_id == -1)
    {
        ZOO_LOG_ERROR("No available registration slots for client: %s", client_name);
    }

    // Release registration lock
    // TODO(smb-transport): Replace with zoo.h atomic operations when CAS is available.
    __sync_bool_compare_and_swap(&server_header->registration_lock, 1, 0);
    ZOO_LOG_DEBUG("Registration lock released");

    return assigned_id;
}

/**
 * @brief Create or attach to shared memory segment.
 *
 * Attempts to create or attach to a shared memory segment with the given key and size.
 *
 * @param key Shared memory key.
 * @param size Size of the segment.
 * @param flags Flags for shmget.
 * @param shm_id Output: pointer to store the shared memory id.
 * @param shm_addr Output: pointer to store the attached address.
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE shm_create_or_attach_segment(key_t key, size_t size, int flags, int* shm_id, void** shm_addr)
{
    if (!shm_id || !shm_addr)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Try to create/get shared memory segment
    *shm_id = shmget(key, size, flags);
    if (*shm_id == -1)
    {
        ZOO_LOG_ERROR("Failed to create/get shared memory segment (key=0x%08x, size=%zu): %s",
                          key,
                          size,
                          strerror(errno));
        return ZOO_SMB_ERROR_SHM_CREATE_FAILED;
    }

    // Attach to the segment
    *shm_addr = shmat(*shm_id, NULL, 0);
    if (*shm_addr == (void*)-1)
    {
        ZOO_LOG_ERROR("Failed to attach to shared memory segment (ID=%d): %s",
                          *shm_id,
                          strerror(errno));
        return ZOO_SMB_ERROR_SHM_ATTACH_FAILED;
    }

    return ZOO_SMB_OK;
}


/**
 * @brief Validates a client registration request.
 *
 * Performs comprehensive validation of a client registration request.
 *
 * @param server_header Pointer to the server header structure.
 * @param client_name Client name to validate.
 * @param client_pid Client process ID.
 * @param client_shm_key Client shared memory key.
 * @return ZOO_ERROR_TYPE Success or specific validation error.
 */
ZOO_ERROR_TYPE shm_validate_client_registration(const SHM_SERVER_HEADER_STRUCT* server_header,
                                                    const char* client_name,
                                                    pid_t client_pid,
                                                    key_t client_shm_key)
{
    // Validate parameters
    if (!server_header || !client_name)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Check if server is running
    if (!server_header->server_running)
    {
        ZOO_LOG_WARN("Registration rejected: server not running");
        return ZOO_SMB_ERROR_SERVICE_UNAVAILABLE;
    }

    // Check server capacity
    if (server_header->client_count >= server_header->max_clients)
    {
        ZOO_LOG_WARN("Registration rejected: server at capacity (%d/%d)",
                         server_header->client_count,
                         server_header->max_clients);
        return ZOO_SMB_ERROR_SHM_BUFFER_FULL;
    }

    // Validate client name
    size_t name_len = strlen(client_name);
    if (name_len == 0 || name_len >= MAX_TRANSPORT_NAME_LENGTH)
    {
        ZOO_LOG_WARN("Registration rejected: invalid client name length (%zu)", name_len);
        return ZOO_SMB_ERROR_INVALID_NAME;
    }

    // Check for invalid characters in name
    for (size_t i = 0; i < name_len; i++)
    {
        char c = client_name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
        {
            ZOO_LOG_WARN("Registration rejected: invalid character in client name: '%c'", c);
            return ZOO_SMB_ERROR_INVALID_NAME;
        }
    }

    // Validate process ID
    if (client_pid <= 0)
    {
        ZOO_LOG_WARN("Registration rejected: invalid process ID: %d", client_pid);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Check if process exists
    if (kill(client_pid, 0) != 0)
    {
        if (errno == ESRCH)
        {
            ZOO_LOG_WARN("Registration rejected: client process does not exist: %d", client_pid);
            return ZOO_SMB_ERROR_PROCESS_NOT_FOUND;
        }
        else if (errno == EPERM)
        {
            // Process exists but we don't have permission to signal it
            // This is acceptable for registration purposes
            ZOO_LOG_DEBUG("Client process exists but no signal permission: %d", client_pid);
        }
        else
        {
            ZOO_LOG_WARN("Registration rejected: error checking client process %d: %s",
                             client_pid,
                             strerror(errno));
            return ZOO_SMB_ERROR_PROCESS_CHECK_FAILED;
        }
    }

    // Validate shared memory key
    if (client_shm_key == 0 || client_shm_key == -1)
    {
        ZOO_LOG_WARN("Registration rejected: invalid shared memory key: 0x%08x", client_shm_key);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Try to access the client's shared memory to verify it exists
    int client_shm_id = shmget(client_shm_key, 0, 0);
    if (client_shm_id == -1)
    {
        ZOO_LOG_WARN("Registration rejected: cannot access client shared memory (key=0x%08x): %s",
                         client_shm_key,
                         strerror(errno));
        return ZOO_SMB_ERROR_SHM_NOT_FOUND;
    }

    ZOO_LOG_DEBUG("Client registration validation passed: name='%s', pid=%d, key=0x%08x",
                      client_name,
                      client_pid,
                      client_shm_key);

    return ZOO_SMB_OK;
}
