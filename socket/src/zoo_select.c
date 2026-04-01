/*******************************************************************************
 * Copyright (C) 2024 ZOO Project
 * All rights reserved.
 * Module: ZOO Socket Select Implementation
 * Component: Cross-platform socket select abstraction
 * File name: zoo_select.c
 * Description: Cross-platform socket select functionality for I/O multiplexing
 ******************************************************************************/

#include "zoo_select.h"
#include "../platform/inc/zoo_error.h"
#include <string.h>
#include <errno.h>

#ifdef ZOO_HAS_POSIX
#include <sys/select.h>
#include <unistd.h>
#endif

#ifdef ZOO_USE_EPOLL
#include <sys/epoll.h>
#include <unistd.h>
#endif

#ifdef ZOO_USE_LWIP
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#endif

/**
 * @brief Internal implementation of the socket set structure
 * 
 * This structure contains the actual data for a socket set, including
 * platform-specific handles like epoll file descriptors. It is kept
 * internal to the implementation file to hide platform details from the API.
 */
struct ZOO_SOCKET_SET_IMPL_STRUCT
{
    ZOO_SOCKET_INFO_STRUCT *sockets[ZOO_SELECT_MAX_SOCKETS]; /**< Array of socket pointers */
    bool read_ready[ZOO_SELECT_MAX_SOCKETS];                 /**< Read readiness flags */
    bool write_ready[ZOO_SELECT_MAX_SOCKETS];                /**< Write readiness flags */
    bool error_ready[ZOO_SELECT_MAX_SOCKETS];                /**< Error flags */
    size_t count;                                            /**< Number of sockets in set */
    int max_fd;                                              /**< Highest file descriptor (for POSIX) */
#ifdef ZOO_USE_EPOLL
    int epoll_fd;                                            /**< Epoll file descriptor */
    bool epoll_initialized;                                  /**< Whether epoll is initialized */
#endif
};

// ==============================================================================
// HELPER FUNCTIONS
// ==============================================================================

/**
 * @brief Convert platform-specific errno to ZOO error code
 * 
 * This function maps standard POSIX error codes to ZOO framework error codes,
 * providing a unified error handling interface across different platforms.
 * 
 * @param error_code The platform-specific error code (typically from errno)
 * @return Corresponding ZOO error code
 * 
 * @details Error code mappings:
 *   - EBADF (Bad file descriptor) -> ZOO_ERROR_INVALID_PARAM
 *   - EINTR (Interrupted system call) -> ZOO_ERROR_TIMEOUT  
 *   - EINVAL (Invalid argument) -> ZOO_ERROR_INVALID_PARAM
 *   - ENOMEM (Out of memory) -> ZOO_ERROR_OUT_OF_MEMORY
 *   - All others -> ZOO_ERROR_GENERAL
 */
static ZOO_ERROR_TYPE platform_to_zoo_error(int error_code)
{
    switch (error_code)
    {
    case EBADF:
        return ZOO_ERROR_INVALID_PARAM;
    case EINTR:
        return ZOO_ERROR_TIMEOUT;
    case EINVAL:
        return ZOO_ERROR_INVALID_PARAM;
    case ENOMEM:
        return ZOO_ERROR_OUT_OF_MEMORY;
    default:
        return ZOO_ERROR_GENERAL;
    }
}

/**
 * @brief Find the index of a socket within a socket set
 * 
 * This helper function searches for a specific socket within a socket set
 * and returns its index position. Used internally for socket set operations
 * like removal and status checking.
 * 
 * @param socket_set Pointer to the socket set to search in
 * @param socket_info Pointer to the socket to find
 * @return Index of the socket (0-based) if found, -1 if not found or invalid parameters
 * 
 * @details
 * - Performs linear search through the socket array
 * - Uses pointer comparison to identify the socket
 * - Returns -1 for NULL parameters or socket not found
 */
static int find_socket_index(const ZOO_SOCKET_SET_STRUCT *socket_set,
                            const ZOO_SOCKET_INFO_STRUCT *socket_info)
{
    if (!socket_set || !socket_info)
        return -1;

    const struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (const struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

    for (size_t i = 0; i < set_impl->count; i++)
    {
        if (set_impl->sockets[i] == socket_info)
        {
            return (int)i;
        }
    }
    return -1;
}

#ifdef ZOO_USE_EPOLL
/**
 * @brief Initialize epoll for a socket set
 * 
 * Creates an epoll file descriptor and sets up the socket set for epoll operations.
 * This provides better scalability than select() for large numbers of sockets.
 * 
 * @param socket_set Pointer to the socket set to initialize epoll for
 * @return ZOO_OK on success, error code on failure
 * 
 * @details
 * - Creates epoll instance with epoll_create1()
 * - Sets epoll_initialized flag to true
 * - Should be called once per socket set before adding sockets
 */
static ZOO_ERROR_TYPE init_epoll(ZOO_SOCKET_SET_STRUCT *socket_set)
{
    if (!socket_set)
        return ZOO_ERROR_INVALID_PARAM;

    struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

    if (set_impl->epoll_initialized)
        return ZOO_OK; // Already initialized

    set_impl->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (set_impl->epoll_fd < 0)
    {
        return platform_to_zoo_error(errno);
    }

    set_impl->epoll_initialized = true;
    return ZOO_OK;
}

/**
 * @brief Add a socket to epoll monitoring
 * 
 * Adds a socket to the epoll instance for monitoring specified events.
 * This is called internally when sockets are added to an epoll-enabled set.
 * 
 * @param socket_set Pointer to the socket set with epoll
 * @param socket_info Pointer to the socket to add
 * @param events Events to monitor (EPOLLIN, EPOLLOUT, etc.)
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_ERROR_TYPE add_socket_to_epoll(ZOO_SOCKET_SET_STRUCT *socket_set,
                                          ZOO_SOCKET_INFO_STRUCT *socket_info,
                                          uint32_t events)
{
    if (!socket_set || !socket_info)
        return ZOO_ERROR_INVALID_PARAM;

    struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

    if (!set_impl->epoll_initialized)
    {
        ZOO_ERROR_TYPE ret = init_epoll(socket_set);
        if (ret != ZOO_OK)
            return ret;
    }

    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = socket_info; // Store socket_info pointer for easy lookup

    if (epoll_ctl(set_impl->epoll_fd, EPOLL_CTL_ADD, socket_info->fd, &ev) < 0)
    {
        return platform_to_zoo_error(errno);
    }

    return ZOO_OK;
}

/**
 * @brief Remove a socket from epoll monitoring
 * 
 * Removes a socket from the epoll instance when it's removed from the socket set.
 * 
 * @param socket_set Pointer to the socket set with epoll
 * @param socket_info Pointer to the socket to remove
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_ERROR_TYPE remove_socket_from_epoll(ZOO_SOCKET_SET_STRUCT *socket_set,
                                               ZOO_SOCKET_INFO_STRUCT *socket_info)
{
    if (!socket_set || !socket_info)
        return ZOO_ERROR_INVALID_PARAM;

    struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

    if (!set_impl->epoll_initialized)
        return ZOO_ERROR_INVALID_PARAM;

    if (epoll_ctl(set_impl->epoll_fd, EPOLL_CTL_DEL, socket_info->fd, NULL) < 0)
    {
        return platform_to_zoo_error(errno);
    }

    return ZOO_OK;
}

/**
 * @brief Cleanup epoll resources for a socket set
 * 
 * Closes the epoll file descriptor and resets epoll state.
 * Called when socket set is cleared or destroyed.
 * 
 * @param socket_set Pointer to the socket set to cleanup
 */
static void cleanup_epoll(ZOO_SOCKET_SET_STRUCT *socket_set)
{
    if (!socket_set)
        return;

    struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

    if (set_impl->epoll_initialized)
    {
        if (set_impl->epoll_fd >= 0)
        {
            close(set_impl->epoll_fd);
            set_impl->epoll_fd = -1;
        }
        set_impl->epoll_initialized = false;
    }
}
#endif /* ZOO_USE_EPOLL */

// ==============================================================================
// SOCKET SET MANAGEMENT FUNCTIONS
// ==============================================================================

/**
 * @brief Initialize a socket set structure for monitoring operations
 * 
 * This function prepares a socket set for use by clearing all fields and
 * setting initial values. Must be called before adding sockets to the set.
 * 
 * @param socket_set Pointer to the socket set structure to initialize
 * @return ZOO_OK on success, ZOO_ERROR_INVALID_PARAM if socket_set is NULL
 * 
 * @details
 * - Zeros out the entire structure using memset
 * - Sets count to 0 (no sockets initially)
 * - Sets max_fd to -1 (invalid file descriptor)
 * - Clears all readiness flags
 * 
 * @note This function must be called before using any other socket set operations
 */
ZOO_ERROR_TYPE zoo_socket_set_init(ZOO_SOCKET_SET_STRUCT *socket_set)
{
    if (!socket_set)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

    memset(set_impl, 0, sizeof(struct ZOO_SOCKET_SET_IMPL_STRUCT));
    set_impl->count = 0;
    set_impl->max_fd = -1;

#ifdef ZOO_USE_EPOLL
    set_impl->epoll_fd = -1;
    set_impl->epoll_initialized = false;
#endif

    return ZOO_OK;
}

/**
 * @brief Add a socket to the monitoring set
 * 
 * This function adds a socket to the socket set for monitoring during select
 * operations. The socket will be included in future select calls on this set.
 * 
 * @param socket_set Pointer to the socket set to add the socket to
 * @param socket_info Pointer to the socket to add
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if any parameter is NULL
 * @retval ZOO_ERROR_OUT_OF_MEMORY if socket set is full (ZOO_SELECT_MAX_SOCKETS limit)
 * @retval ZOO_OK if socket is already in set (duplicate add is not an error)
 * 
 * @details
 * - Checks for duplicate sockets before adding
 * - Updates the max_fd field for POSIX select optimization
 * - Initializes readiness flags to false for the new socket
 * - Increments the socket count
 * 
 * @note Socket set capacity is limited to ZOO_SELECT_MAX_SOCKETS (1024 by default)
 */
ZOO_ERROR_TYPE zoo_socket_set_add(ZOO_SOCKET_SET_STRUCT *socket_set,
                                  ZOO_SOCKET_INFO_STRUCT *socket_info)
{
    if (!socket_set || !socket_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

    if (set_impl->count >= ZOO_SELECT_MAX_SOCKETS)
    {
        return ZOO_ERROR_OUT_OF_MEMORY;
    }

    // Check if socket is already in the set
    if (find_socket_index(socket_set, socket_info) >= 0)
    {
        return ZOO_OK; // Already exists, not an error
    }

    // Add socket to the set
    set_impl->sockets[set_impl->count] = socket_info;
    set_impl->read_ready[set_impl->count] = false;
    set_impl->write_ready[set_impl->count] = false;
    set_impl->error_ready[set_impl->count] = false;
    set_impl->count++;

    // Update max_fd for POSIX systems
    if (socket_info->fd > set_impl->max_fd)
    {
        set_impl->max_fd = socket_info->fd;
    }

#ifdef ZOO_USE_EPOLL
    // Add to epoll with all events for flexibility
    uint32_t epoll_events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
    ZOO_ERROR_TYPE epoll_ret = add_socket_to_epoll(socket_set, socket_info, epoll_events);
    if (epoll_ret != ZOO_OK)
    {
        // Rollback the socket addition if epoll fails
        set_impl->count--;
        return epoll_ret;
    }
#endif

    return ZOO_OK;
}

/**
 * @brief Remove a socket from the monitoring set
 * 
 * This function removes a specific socket from the socket set, so it will
 * no longer be monitored in future select operations on this set.
 * 
 * @param socket_set Pointer to the socket set to remove the socket from
 * @param socket_info Pointer to the socket to remove
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if any parameter is NULL
 * @retval ZOO_ERROR_NOT_FOUND if socket is not in the set
 * 
 * @details
 * - Finds the socket using linear search
 * - Shifts remaining elements to fill the gap (maintains array compactness)
 * - Recalculates max_fd by scanning all remaining sockets
 * - Decrements socket count
 * - Preserves readiness flags for remaining sockets
 * 
 * @performance O(n) operation due to array shifting and max_fd recalculation
 */
ZOO_ERROR_TYPE zoo_socket_set_remove(ZOO_SOCKET_SET_STRUCT *socket_set,
                                     ZOO_SOCKET_INFO_STRUCT *socket_info)
{
    if (!socket_set || !socket_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

    int index = find_socket_index(socket_set, socket_info);
    if (index < 0)
    {
        return ZOO_ERROR_NOT_FOUND;
    }

#ifdef ZOO_USE_EPOLL
    // Remove from epoll first
    if (set_impl->epoll_initialized)
    {
        remove_socket_from_epoll(socket_set, socket_info);
        // Continue even if epoll removal fails - we still want to remove from our set
    }
#endif

    // Shift remaining elements down
    for (size_t i = (size_t)index; i < set_impl->count - 1; i++)
    {
        set_impl->sockets[i] = set_impl->sockets[i + 1];
        set_impl->read_ready[i] = set_impl->read_ready[i + 1];
        set_impl->write_ready[i] = set_impl->write_ready[i + 1];
        set_impl->error_ready[i] = set_impl->error_ready[i + 1];
    }

    set_impl->count--;

    // Recalculate max_fd
    set_impl->max_fd = -1;
    for (size_t i = 0; i < set_impl->count; i++)
    {
        if (set_impl->sockets[i]->fd > set_impl->max_fd)
        {
            set_impl->max_fd = set_impl->sockets[i]->fd;
        }
    }

    return ZOO_OK;
}

/**
 * @brief Clear all sockets from the monitoring set
 * 
 * This function removes all sockets from the socket set and resets it to
 * the initial empty state, equivalent to calling zoo_socket_set_init().
 * 
 * @param socket_set Pointer to the socket set to clear
 * @return ZOO_OK on success, ZOO_ERROR_INVALID_PARAM if socket_set is NULL
 * 
 * @details
 * - Resets count to 0
 * - Sets max_fd to -1 (invalid)
 * - Zeros out all socket pointers and readiness flags
 * - More efficient than removing sockets one by one
 * 
 * @note After this operation, the socket set is ready for reuse
 */
ZOO_ERROR_TYPE zoo_socket_set_clear(ZOO_SOCKET_SET_STRUCT *socket_set)
{
    if (!socket_set)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

#ifdef ZOO_USE_EPOLL
    cleanup_epoll(socket_set);
#endif

    set_impl->count = 0;
    set_impl->max_fd = -1;
    memset(set_impl->sockets, 0, sizeof(set_impl->sockets));
    memset(set_impl->read_ready, 0, sizeof(set_impl->read_ready));
    memset(set_impl->write_ready, 0, sizeof(set_impl->write_ready));
    memset(set_impl->error_ready, 0, sizeof(set_impl->error_ready));

    return ZOO_OK;
}

/**
 * @brief Get the number of sockets currently in the socket set
 * 
 * This function returns the current count of sockets in the socket set.
 * Useful for checking set capacity and iterating through sockets.
 * 
 * @param socket_set Pointer to the socket set to query
 * @return Number of sockets in the set, or 0 if socket_set is NULL
 * 
 * @details
 * - Safe to call with NULL pointer (returns 0)
 * - Count reflects active sockets, not capacity
 * - Maximum possible value is ZOO_SELECT_MAX_SOCKETS
 * 
 * @note This is a constant-time O(1) operation
 */
size_t zoo_socket_set_count(const ZOO_SOCKET_SET_STRUCT *socket_set)
{
    if (!socket_set)
    {
        return 0;
    }
    const struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (const struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;
    return set_impl->count;
}

// ==============================================================================
// SELECT OPERATIONS
// ==============================================================================

/**
 * @brief Monitor multiple sockets for I/O readiness using platform-optimal mechanisms
 * 
 * This is the core select function that monitors multiple socket sets for different
 * types of I/O readiness events. It uses the most efficient mechanism available on
 * the current platform (POSIX select() or lwIP select()).
 * 
 * @param read_set Pointer to socket set to monitor for read readiness (can be NULL)
 * @param write_set Pointer to socket set to monitor for write readiness (can be NULL)  
 * @param error_set Pointer to socket set to monitor for error conditions (can be NULL)
 * @param timeout_ms Timeout in milliseconds (-1 = infinite, 0 = immediate return)
 * @param result Pointer to result structure to fill with statistics (can be NULL)
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if all socket sets are NULL
 * @retval ZOO_ERROR_TIMEOUT if timeout occurred with no ready sockets
 * @retval Platform-specific error codes for system failures
 * 
 * @details
 * Platform Implementations:
 * - POSIX: Uses select() system call with fd_set structures
 * - lwIP: Uses lwip_select() with similar semantics
 * 
 * Operation:
 * 1. Clears all readiness flags in the socket sets
 * 2. Builds platform-specific file descriptor sets
 * 3. Calculates appropriate timeout structure
 * 4. Calls platform select function
 * 5. Updates readiness flags based on results
 * 6. Fills result structure with statistics
 * 
 * @note At least one socket set must be non-NULL
 * @warning Readiness flags are cleared before each call - check them immediately after
 */
ZOO_ERROR_TYPE zoo_socket_select_multiple(ZOO_SOCKET_SET_STRUCT *read_set,
                                          ZOO_SOCKET_SET_STRUCT *write_set,
                                          ZOO_SOCKET_SET_STRUCT *error_set,
                                          int timeout_ms,
                                          ZOO_SELECT_RESULT_STRUCT *result)
{
    if (!read_set && !write_set && !error_set)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    struct ZOO_SOCKET_SET_IMPL_STRUCT *read_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)read_set;
    struct ZOO_SOCKET_SET_IMPL_STRUCT *write_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)write_set;
    struct ZOO_SOCKET_SET_IMPL_STRUCT *error_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)error_set;

    // Initialize result structure
    if (result)
    {
        memset(result, 0, sizeof(ZOO_SELECT_RESULT_STRUCT));
    }

    // Clear readiness flags in all sets
    if (read_impl)
    {
        memset(read_impl->read_ready, 0, sizeof(read_impl->read_ready));
    }
    if (write_impl)
    {
        memset(write_impl->write_ready, 0, sizeof(write_impl->write_ready));
    }
    if (error_impl)
    {
        memset(error_impl->error_ready, 0, sizeof(error_impl->error_ready));
    }

#ifdef ZOO_USE_EPOLL
    // Epoll implementation - more efficient for large numbers of sockets
    struct epoll_event events[ZOO_EPOLL_MAX_EVENTS];
    int timeout_epoll = timeout_ms;
    int ready_count = 0;
    
    // We need to use one of the socket sets for epoll operations
    // Priority: read_set > write_set > error_set
    ZOO_SOCKET_SET_STRUCT *primary_set = read_set ? read_set : 
                                         (write_set ? write_set : error_set);
    struct ZOO_SOCKET_SET_IMPL_STRUCT *primary_impl = (struct ZOO_SOCKET_SET_IMPL_STRUCT *)primary_set;
    
    if (!primary_impl->epoll_initialized)
    {
        // Initialize epoll if not already done
        ZOO_ERROR_TYPE init_ret = init_epoll(primary_set);
        if (init_ret != ZOO_OK)
        {
            // Fall back to select() if epoll initialization fails
            goto use_select_fallback;
        }
        
        // Add all sockets from all sets to epoll
        if (read_impl && read_impl != primary_impl)
        {
            for (size_t i = 0; i < read_impl->count; i++)
            {
                add_socket_to_epoll(primary_set, read_impl->sockets[i], 
                                   EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP);
            }
        }
        if (write_impl && write_impl != primary_impl)
        {
            for (size_t i = 0; i < write_impl->count; i++)
            {
                add_socket_to_epoll(primary_set, write_impl->sockets[i], 
                                   EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP);
            }
        }
        if (error_impl && error_impl != primary_impl)
        {
            for (size_t i = 0; i < error_impl->count; i++)
            {
                add_socket_to_epoll(primary_set, error_impl->sockets[i], 
                                   EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP);
            }
        }
    }

    ready_count = epoll_wait(primary_impl->epoll_fd, events, ZOO_EPOLL_MAX_EVENTS, timeout_epoll);
    
    if (ready_count < 0)
    {
        return platform_to_zoo_error(errno);
    }
    else if (ready_count == 0)
    {
        if (result) result->timeout_occurred = true;
        return ZOO_ERROR_TIMEOUT;
    }

    // Process epoll events and update socket sets
    size_t total_ready = 0;
    
    for (int i = 0; i < ready_count; i++)
    {
        ZOO_SOCKET_INFO_STRUCT *socket_info = (ZOO_SOCKET_INFO_STRUCT *)events[i].data.ptr;
        uint32_t epoll_events = events[i].events;
        
        // Check read readiness
        if ((epoll_events & EPOLLIN) && read_impl)
        {
            int idx = find_socket_index(read_set, socket_info);
            if (idx >= 0)
            {
                read_impl->read_ready[idx] = true;
                total_ready++;
                if (result) result->read_ready_count++;
            }
        }
        
        // Check write readiness
        if ((epoll_events & EPOLLOUT) && write_impl)
        {
            int idx = find_socket_index(write_set, socket_info);
            if (idx >= 0)
            {
                write_impl->write_ready[idx] = true;
                total_ready++;
                if (result) result->write_ready_count++;
            }
        }
        
        // Check error conditions
        if ((epoll_events & (EPOLLERR | EPOLLHUP)) && error_impl)
        {
            int idx = find_socket_index(error_set, socket_info);
            if (idx >= 0)
            {
                error_impl->error_ready[idx] = true;
                total_ready++;
                if (result) result->error_ready_count++;
            }
        }
    }
    
    if (result) result->ready_count = total_ready;
    return ZOO_OK;

use_select_fallback:
#endif /* ZOO_USE_EPOLL */

#ifdef ZOO_HAS_POSIX
    fd_set read_fds, write_fds, error_fds;
    fd_set *read_fds_ptr = NULL, *write_fds_ptr = NULL, *error_fds_ptr = NULL;
    int max_fd = -1;
    struct timeval tv;
    struct timeval *tv_ptr = NULL;

    // Initialize fd_sets
    if (read_impl && read_impl->count > 0)
    {
        FD_ZERO(&read_fds);
        read_fds_ptr = &read_fds;
        for (size_t i = 0; i < read_impl->count; i++)
        {
            int fd = read_impl->sockets[i]->fd;
            if (fd >= 0)
            {
                FD_SET(fd, &read_fds);
                if (fd > max_fd) max_fd = fd;
            }
        }
    }

    if (write_impl && write_impl->count > 0)
    {
        FD_ZERO(&write_fds);
        write_fds_ptr = &write_fds;
        for (size_t i = 0; i < write_impl->count; i++)
        {
            int fd = write_impl->sockets[i]->fd;
            if (fd >= 0)
            {
                FD_SET(fd, &write_fds);
                if (fd > max_fd) max_fd = fd;
            }
        }
    }

    if (error_impl && error_impl->count > 0)
    {
        FD_ZERO(&error_fds);
        error_fds_ptr = &error_fds;
        for (size_t i = 0; i < error_impl->count; i++)
        {
            int fd = error_impl->sockets[i]->fd;
            if (fd >= 0)
            {
                FD_SET(fd, &error_fds);
                if (fd > max_fd) max_fd = fd;
            }
        }
    }

    // Set up timeout
    if (timeout_ms >= 0)
    {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tv_ptr = &tv;
    }

    // Perform select
    int select_result = select(max_fd + 1, read_fds_ptr, write_fds_ptr, error_fds_ptr, tv_ptr);

    if (select_result < 0)
    {
        return platform_to_zoo_error(errno);
    }
    else if (select_result == 0)
    {
        if (result) result->timeout_occurred = true;
        return ZOO_ERROR_TIMEOUT;
    }

    // Check results and update readiness flags
    size_t total_ready = 0;

    if (read_impl && read_fds_ptr)
    {
        for (size_t i = 0; i < read_impl->count; i++)
        {
            int fd = read_impl->sockets[i]->fd;
            if (fd >= 0 && FD_ISSET(fd, &read_fds))
            {
                read_impl->read_ready[i] = true;
                total_ready++;
                if (result) result->read_ready_count++;
            }
        }
    }

    if (write_impl && write_fds_ptr)
    {
        for (size_t i = 0; i < write_impl->count; i++)
        {
            int fd = write_impl->sockets[i]->fd;
            if (fd >= 0 && FD_ISSET(fd, &write_fds))
            {
                write_impl->write_ready[i] = true;
                total_ready++;
                if (result) result->write_ready_count++;
            }
        }
    }

    if (error_impl && error_fds_ptr)
    {
        for (size_t i = 0; i < error_impl->count; i++)
        {
            int fd = error_impl->sockets[i]->fd;
            if (fd >= 0 && FD_ISSET(fd, &error_fds))
            {
                error_impl->error_ready[i] = true;
                total_ready++;
                if (result) result->error_ready_count++;
            }
        }
    }

    if (result) result->ready_count = total_ready;

#endif /* ZOO_HAS_POSIX */

#ifdef ZOO_USE_LWIP
    // lwIP select implementation
    fd_set read_fds, write_fds, error_fds;
    fd_set *read_fds_ptr = NULL, *write_fds_ptr = NULL, *error_fds_ptr = NULL;
    int max_fd = -1;
    struct timeval tv;
    struct timeval *tv_ptr = NULL;

    // Initialize fd_sets for lwIP
    if (read_impl && read_impl->count > 0)
    {
        FD_ZERO(&read_fds);
        read_fds_ptr = &read_fds;
        for (size_t i = 0; i < read_impl->count; i++)
        {
            int fd = read_impl->sockets[i]->fd;
            if (fd >= 0)
            {
                FD_SET(fd, &read_fds);
                if (fd > max_fd) max_fd = fd;
            }
        }
    }

    if (write_impl && write_impl->count > 0)
    {
        FD_ZERO(&write_fds);
        write_fds_ptr = &write_fds;
        for (size_t i = 0; i < write_impl->count; i++)
        {
            int fd = write_impl->sockets[i]->fd;
            if (fd >= 0)
            {
                FD_SET(fd, &write_fds);
                if (fd > max_fd) max_fd = fd;
            }
        }
    }

    if (error_impl && error_impl->count > 0)
    {
        FD_ZERO(&error_fds);
        error_fds_ptr = &error_fds;
        for (size_t i = 0; i < error_impl->count; i++)
        {
            int fd = error_impl->sockets[i]->fd;
            if (fd >= 0)
            {
                FD_SET(fd, &error_fds);
                if (fd > max_fd) max_fd = fd;
            }
        }
    }

    // Set up timeout for lwIP
    if (timeout_ms >= 0)
    {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tv_ptr = &tv;
    }

    // Perform lwIP select (lwIP provides select() function similar to POSIX)
    int select_result = lwip_select(max_fd + 1, read_fds_ptr, write_fds_ptr, error_fds_ptr, tv_ptr);

    if (select_result < 0)
    {
        return platform_to_zoo_error(errno);
    }
    else if (select_result == 0)
    {
        if (result) result->timeout_occurred = true;
        return ZOO_ERROR_TIMEOUT;
    }

    // Check results and update readiness flags for lwIP
    size_t total_ready = 0;

    if (read_impl && read_fds_ptr)
    {
        for (size_t i = 0; i < read_impl->count; i++)
        {
            int fd = read_impl->sockets[i]->fd;
            if (fd >= 0 && FD_ISSET(fd, &read_fds))
            {
                read_impl->read_ready[i] = true;
                total_ready++;
                if (result) result->read_ready_count++;
            }
        }
    }

    if (write_impl && write_fds_ptr)
    {
        for (size_t i = 0; i < write_impl->count; i++)
        {
            int fd = write_impl->sockets[i]->fd;
            if (fd >= 0 && FD_ISSET(fd, &write_fds))
            {
                write_impl->write_ready[i] = true;
                total_ready++;
                if (result) result->write_ready_count++;
            }
        }
    }

    if (error_impl && error_fds_ptr)
    {
        for (size_t i = 0; i < error_impl->count; i++)
        {
            int fd = error_impl->sockets[i]->fd;
            if (fd >= 0 && FD_ISSET(fd, &error_fds))
            {
                error_impl->error_ready[i] = true;
                total_ready++;
                if (result) result->error_ready_count++;
            }
        }
    }

    if (result) result->ready_count = total_ready;

#endif /* ZOO_USE_LWIP */

    return ZOO_OK;
}

/**
 * @brief Check if a specific socket in a set is ready for reading
 * 
 * This function checks the read readiness flag for a specific socket within
 * a socket set that was previously used in a select operation.
 * 
 * @param socket_set Pointer to socket set that was used in select operation
 * @param socket_info Pointer to the socket to check
 * @return true if socket is ready for reading, false otherwise
 * 
 * @details
 * - Must be called after zoo_socket_select_multiple()
 * - Returns false for NULL parameters or socket not in set
 * - Readiness is based on the last select operation
 * - Thread-safe for read-only access
 * 
 * @note Readiness flags are cleared on each new select operation
 */
bool zoo_socket_set_is_read_ready(const ZOO_SOCKET_SET_STRUCT *socket_set,
                                  const ZOO_SOCKET_INFO_STRUCT *socket_info)
{
    if (!socket_set || !socket_info)
        return false;

    const struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (const struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;
    int index = find_socket_index(socket_set, socket_info);
    return (index >= 0) ? set_impl->read_ready[index] : false;
}

/**
 * @brief Check if a specific socket in a set is ready for writing
 * 
 * This function checks the write readiness flag for a specific socket within
 * a socket set that was previously used in a select operation.
 * 
 * @param socket_set Pointer to socket set that was used in select operation
 * @param socket_info Pointer to the socket to check
 * @return true if socket is ready for writing, false otherwise
 * 
 * @details
 * - Must be called after zoo_socket_select_multiple()
 * - Returns false for NULL parameters or socket not in set
 * - Readiness is based on the last select operation
 * - Write readiness typically means send buffer has space
 * 
 * @note For most sockets, write readiness is usually true unless buffer is full
 */
bool zoo_socket_set_is_write_ready(const ZOO_SOCKET_SET_STRUCT *socket_set,
                                   const ZOO_SOCKET_INFO_STRUCT *socket_info)
{
    if (!socket_set || !socket_info)
        return false;

    const struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (const struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;
    int index = find_socket_index(socket_set, socket_info);
    return (index >= 0) ? set_impl->write_ready[index] : false;
}

/**
 * @brief Check if a specific socket in a set has an error condition
 * 
 * This function checks the error flag for a specific socket within
 * a socket set that was previously used in a select operation.
 * 
 * @param socket_set Pointer to socket set that was used in select operation
 * @param socket_info Pointer to the socket to check
 * @return true if socket has an error condition, false otherwise
 * 
 * @details
 * - Must be called after zoo_socket_select_multiple()
 * - Returns false for NULL parameters or socket not in set
 * - Error conditions include connection reset, network unreachable, etc.
 * - Should be checked before attempting I/O operations
 * 
 * @note Error conditions usually indicate the socket should be closed
 */
bool zoo_socket_set_has_error(const ZOO_SOCKET_SET_STRUCT *socket_set,
                              const ZOO_SOCKET_INFO_STRUCT *socket_info)
{
    if (!socket_set || !socket_info)
        return false;

    const struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (const struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;
    int index = find_socket_index(socket_set, socket_info);
    return (index >= 0) ? set_impl->error_ready[index] : false;
}

// ==============================================================================
// CONVENIENCE FUNCTIONS
// ==============================================================================

/**
 * @brief Wait for specific events on a single socket (convenience function)
 * 
 * This convenience function monitors a single socket for the specified events
 * without requiring manual socket set management. It's a simplified interface
 * for the common case of monitoring one socket.
 * 
 * @param socket_info Pointer to the socket to monitor
 * @param events Bitmask of events to monitor (ZOO_SELECT_EVENT_READ, etc.)
 * @param timeout_ms Timeout in milliseconds (-1 = infinite, 0 = immediate)
 * @param ready_events Pointer to receive which events are ready (can be NULL)
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if socket_info is NULL
 * @retval ZOO_ERROR_TIMEOUT if timeout occurred with no ready events
 * 
 * @details
 * Operation:
 * 1. Creates temporary socket sets based on requested events
 * 2. Adds the socket to appropriate sets
 * 3. Calls zoo_socket_select_multiple()
 * 4. Returns which events are ready via ready_events parameter
 * 
 * Event Flags:
 * - ZOO_SELECT_EVENT_READ: Monitor for read readiness
 * - ZOO_SELECT_EVENT_WRITE: Monitor for write readiness  
 * - ZOO_SELECT_EVENT_ERROR: Monitor for error conditions
 * - ZOO_SELECT_EVENT_ALL: Monitor for all events
 * 
 * @note More efficient than zoo_socket_select_multiple() for single sockets
 */
ZOO_ERROR_TYPE zoo_socket_wait_for_events(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                          ZOO_SELECT_EVENT_ENUM events,
                                          int timeout_ms,
                                          ZOO_SELECT_EVENT_ENUM *ready_events)
{
    if (!socket_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    if (ready_events)
    {
        *ready_events = 0;
    }

    ZOO_SOCKET_SET_STRUCT read_set, write_set, error_set;
    ZOO_SOCKET_SET_STRUCT *read_ptr = NULL, *write_ptr = NULL, *error_ptr = NULL;
    ZOO_SELECT_RESULT_STRUCT result;

    // Initialize sets based on requested events
    if (events & ZOO_SELECT_EVENT_READ)
    {
        zoo_socket_set_init(&read_set);
        zoo_socket_set_add(&read_set, socket_info);
        read_ptr = &read_set;
    }

    if (events & ZOO_SELECT_EVENT_WRITE)
    {
        zoo_socket_set_init(&write_set);
        zoo_socket_set_add(&write_set, socket_info);
        write_ptr = &write_set;
    }

    if (events & ZOO_SELECT_EVENT_ERROR)
    {
        zoo_socket_set_init(&error_set);
        zoo_socket_set_add(&error_set, socket_info);
        error_ptr = &error_set;
    }

    ZOO_ERROR_TYPE ret = zoo_socket_select_multiple(read_ptr, write_ptr, error_ptr, timeout_ms, &result);

    if (ret == ZOO_OK && ready_events)
    {
        if (read_ptr && zoo_socket_set_is_read_ready(read_ptr, socket_info))
        {
            *ready_events |= ZOO_SELECT_EVENT_READ;
        }
        if (write_ptr && zoo_socket_set_is_write_ready(write_ptr, socket_info))
        {
            *ready_events |= ZOO_SELECT_EVENT_WRITE;
        }
        if (error_ptr && zoo_socket_set_has_error(error_ptr, socket_info))
        {
            *ready_events |= ZOO_SELECT_EVENT_ERROR;
        }
    }

    return ret;
}

/**
 * @brief Check if a socket is immediately ready for reading (non-blocking)
 * 
 * This convenience function performs an immediate (timeout=0) check to see
 * if a socket is ready for reading without blocking the caller.
 * 
 * @param socket_info Pointer to the socket to check
 * @param is_ready Pointer to bool to receive the readiness result
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if any parameter is NULL
 * 
 * @details
 * - Uses zoo_socket_wait_for_events() with 0 timeout internally
 * - Converts ZOO_ERROR_TIMEOUT to false readiness (not an error)
 * - Suitable for polling-style I/O checking
 * - Does not block the calling thread
 * 
 * @note This is equivalent to calling zoo_socket_wait_for_events() with
 *       ZOO_SELECT_EVENT_READ and timeout_ms=0
 */
ZOO_ERROR_TYPE zoo_socket_is_read_ready(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                       bool *is_ready)
{
    if (!socket_info || !is_ready)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ZOO_SELECT_EVENT_ENUM ready_events;
    ZOO_ERROR_TYPE ret = zoo_socket_wait_for_events(socket_info, ZOO_SELECT_EVENT_READ, 0, &ready_events);

    if (ret == ZOO_ERROR_TIMEOUT)
    {
        *is_ready = false;
        return ZOO_OK;
    }
    else if (ret == ZOO_OK)
    {
        *is_ready = (ready_events & ZOO_SELECT_EVENT_READ) != 0;
        return ZOO_OK;
    }

    return ret;
}

/**
 * @brief Check if a socket is immediately ready for writing (non-blocking)
 * 
 * This convenience function performs an immediate (timeout=0) check to see
 * if a socket is ready for writing without blocking the caller.
 * 
 * @param socket_info Pointer to the socket to check
 * @param is_ready Pointer to bool to receive the readiness result
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if any parameter is NULL
 * 
 * @details
 * - Uses zoo_socket_wait_for_events() with 0 timeout internally
 * - Converts ZOO_ERROR_TIMEOUT to false readiness (not an error)
 * - Write readiness typically means the send buffer has space
 * - Suitable for checking before non-blocking send operations
 * 
 * @note Most sockets are ready for writing unless the send buffer is full
 *       or the connection is closed/broken
 */
ZOO_ERROR_TYPE zoo_socket_is_write_ready(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                        bool *is_ready)
{
    if (!socket_info || !is_ready)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ZOO_SELECT_EVENT_ENUM ready_events;
    ZOO_ERROR_TYPE ret = zoo_socket_wait_for_events(socket_info, ZOO_SELECT_EVENT_WRITE, 0, &ready_events);

    if (ret == ZOO_ERROR_TIMEOUT)
    {
        *is_ready = false;
        return ZOO_OK;
    }
    else if (ret == ZOO_OK)
    {
        *is_ready = (ready_events & ZOO_SELECT_EVENT_WRITE) != 0;
        return ZOO_OK;
    }

    return ret;
}

/**
 * @brief Check if a socket has error conditions pending (non-blocking)
 * 
 * This convenience function performs an immediate (timeout=0) check to see
 * if a socket has any error conditions or exceptional events pending.
 * 
 * @param socket_info Pointer to the socket to check
 * @param has_error Pointer to bool to receive the error status result
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if any parameter is NULL
 * 
 * @details
 * - Uses zoo_socket_wait_for_events() with 0 timeout internally
 * - Converts ZOO_ERROR_TIMEOUT to false error status (not an error)
 * - Error conditions include connection failures, socket closure, etc.
 * - Suitable for checking socket health before operations
 * 
 * @note Error events may include out-of-band data on some platforms
 *       in addition to actual socket errors
 */
ZOO_ERROR_TYPE zoo_socket_has_error(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                   bool *has_error)
{
    if (!socket_info || !has_error)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ZOO_SELECT_EVENT_ENUM ready_events;
    ZOO_ERROR_TYPE ret = zoo_socket_wait_for_events(socket_info, ZOO_SELECT_EVENT_ERROR, 0, &ready_events);

    if (ret == ZOO_ERROR_TIMEOUT)
    {
        *has_error = false;
        return ZOO_OK;
    }
    else if (ret == ZOO_OK)
    {
        *has_error = (ready_events & ZOO_SELECT_EVENT_ERROR) != 0;
        return ZOO_OK;
    }

    return ret;
}

// ==============================================================================
// PLATFORM-SPECIFIC UTILITIES  
// ==============================================================================

/**
 * @brief Check if epoll is available and being used
 * 
 * This function returns whether the current platform supports and is using
 * epoll for socket operations. This is determined at compile time.
 * 
 * @return true if epoll is available and enabled, false otherwise
 * 
 * @details
 * - Returns true on Linux systems with ZOO_USE_EPOLL defined
 * - Returns false on other platforms or when epoll support is disabled
 * - This is a compile-time constant for performance
 * 
 * @note Useful for diagnostics, performance tuning, and testing
 */
bool zoo_socket_epoll_available(void)
{
#ifdef ZOO_USE_EPOLL
    return true;
#else
    return false;
#endif
}

// ==============================================================================
// ITERATOR FUNCTIONS
// ==============================================================================

/**
 * @brief Get the first ready socket from a socket set for iteration
 *
 * This function begins iteration through ready sockets in a socket set that
 * was used in a select operation. Use with zoo_socket_set_get_next_ready()
 * to iterate through all ready sockets.
 *
 * @param socket_set Pointer to the socket set to iterate through
 * @param index Pointer to index variable (will be initialized to 0)
 * @return Pointer to the first ready ZOO_SOCKET_INFO_STRUCT in the set,
 *         or NULL if no sockets are ready or parameters are invalid
 * 
 * @details
 * - Sets *index to 0 and calls zoo_socket_set_get_next_ready()
 * - A socket is considered "ready" if any of its flags are set:
 *   read_ready, write_ready, or error_ready
 * - Must be called after a successful select operation
 * 
 * @note The index parameter is required and will be modified by this function
 *       to track iteration state for subsequent calls to get_next_ready()
 */
ZOO_SOCKET_INFO_STRUCT *zoo_socket_set_get_first_ready(const ZOO_SOCKET_SET_STRUCT *socket_set,
                                                       size_t *index)
{
    if (!socket_set || !index)
        return NULL;

    *index = 0;
    return zoo_socket_set_get_next_ready(socket_set, index);
}

/**
 * @brief Get the next ready socket from a socket set during iteration
 *
 * This function continues iteration through ready sockets in a socket set.
 * Use after zoo_socket_set_get_first_ready() to get subsequent ready sockets.
 *
 * @param socket_set Pointer to the socket set to iterate through  
 * @param index Pointer to index variable (updated by this function)
 * @return Pointer to the next ready ZOO_SOCKET_INFO_STRUCT in the set,
 *         or NULL if no more ready sockets or parameters are invalid
 * 
 * @details  
 * - Continues from the current *index position
 * - Increments *index for each socket checked
 * - A socket is considered "ready" if any of its flags are set:
 *   read_ready, write_ready, or error_ready
 * - Returns NULL when all sockets have been checked
 * 
 * @note The index parameter tracks iteration state and is modified by this
 *       function. Do not modify index between calls during iteration.
 * 
 * @example
 * @code
 * size_t index;
 * ZOO_SOCKET_INFO_STRUCT *socket = zoo_socket_set_get_first_ready(set, &index);
 * while (socket) {
 *     // Process ready socket
 *     socket = zoo_socket_set_get_next_ready(set, &index);
 * }
 * @endcode
 */
ZOO_SOCKET_INFO_STRUCT *zoo_socket_set_get_next_ready(const ZOO_SOCKET_SET_STRUCT *socket_set,
                                                      size_t *index)
{
    if (!socket_set || !index)
        return NULL;

    const struct ZOO_SOCKET_SET_IMPL_STRUCT *set_impl = (const struct ZOO_SOCKET_SET_IMPL_STRUCT *)socket_set;

    while (*index < set_impl->count)
    {
        size_t current = *index;
        (*index)++;

        // Check if this socket is ready for any operation
        if (set_impl->read_ready[current] || 
            set_impl->write_ready[current] || 
            set_impl->error_ready[current])
        {
            return set_impl->sockets[current];
        }
    }

    return NULL;
}
