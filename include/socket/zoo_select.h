/*******************************************************************************
 * Copyright (C) 2024 ZOO Project
 * All rights reserved.
 * Module: ZOO Socket Select
 * Component: Cross-platform socket select abstraction
 * File name: zoo_select.h
 * Description: Cross-platform socket select functionality for I/O multiplexing
 ******************************************************************************/

#ifndef ZOO_SELECT_H
#define ZOO_SELECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zoo_socket.h"
#include <stdbool.h>
#include <stddef.h>

// ==============================================================================
// CONSTANTS AND MACROS
// ==============================================================================

/**
 * @brief Maximum number of sockets that can be monitored simultaneously
 */
#define ZOO_SELECT_MAX_SOCKETS 1024

/**
 * @brief Infinite timeout value for select operations
 */
#define ZOO_SELECT_TIMEOUT_INFINITE -1

/**
 * @brief No timeout - immediate return
 */
#define ZOO_SELECT_TIMEOUT_IMMEDIATE 0

/**
 * @brief Maximum number of epoll events to process in one call
 */
#define ZOO_EPOLL_MAX_EVENTS 256

// ==============================================================================
// TYPE DEFINITIONS
// ==============================================================================

/**
 * @brief Socket set structure for monitoring multiple sockets
 * This is an opaque structure that abstracts platform-specific fd_set
 */
typedef struct ZOO_SOCKET_SET_IMPL_STRUCT ZOO_SOCKET_SET_STRUCT;

/**
 * @brief Select result structure containing readiness information
 */
typedef struct
{
    size_t ready_count;                /**< Total number of ready sockets */
    size_t read_ready_count;           /**< Number of sockets ready for reading */
    size_t write_ready_count;          /**< Number of sockets ready for writing */
    size_t error_ready_count;          /**< Number of sockets with errors */
    bool timeout_occurred;             /**< True if operation timed out */
} ZOO_SELECT_RESULT_STRUCT;

/**
 * @brief Select events to monitor
 */
typedef enum
{
    ZOO_SELECT_EVENT_READ = 0x01,      /**< Monitor for read readiness */
    ZOO_SELECT_EVENT_WRITE = 0x02,     /**< Monitor for write readiness */
    ZOO_SELECT_EVENT_ERROR = 0x04,     /**< Monitor for error conditions */
    ZOO_SELECT_EVENT_ALL = 0x07        /**< Monitor for all events */
} ZOO_SELECT_EVENT_ENUM;

// ==============================================================================
// SOCKET SET MANAGEMENT FUNCTIONS
// ==============================================================================

/**
 * @brief Initialize a socket set for monitoring
 * 
 * This function initializes a socket set structure that can be used to monitor
 * multiple sockets for I/O readiness.
 * 
 * @param socket_set Pointer to socket set structure to initialize
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if socket_set is NULL
 */
ZOO_ERROR_TYPE zoo_socket_set_init(ZOO_SOCKET_SET_STRUCT *socket_set);

/**
 * @brief Add a socket to the monitoring set
 * 
 * Adds a socket to the set for monitoring. The socket will be checked for
 * the specified events during select operations.
 * 
 * @param socket_set Pointer to socket set
 * @param socket_info Pointer to socket to add
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 * @retval ZOO_ERROR_NO_MEMORY if socket set is full
 */
ZOO_ERROR_TYPE zoo_socket_set_add(ZOO_SOCKET_SET_STRUCT *socket_set,
                                  ZOO_SOCKET_INFO_STRUCT *socket_info);

/**
 * @brief Remove a socket from the monitoring set
 * 
 * @param socket_set Pointer to socket set
 * @param socket_info Pointer to socket to remove
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 * @retval ZOO_ERROR_NOT_FOUND if socket not in set
 */
ZOO_ERROR_TYPE zoo_socket_set_remove(ZOO_SOCKET_SET_STRUCT *socket_set,
                                     ZOO_SOCKET_INFO_STRUCT *socket_info);

/**
 * @brief Clear all sockets from the set
 * 
 * @param socket_set Pointer to socket set to clear
 * @return ZOO_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_socket_set_clear(ZOO_SOCKET_SET_STRUCT *socket_set);

/**
 * @brief Get the number of sockets in the set
 * 
 * @param socket_set Pointer to socket set
 * @return Number of sockets in set, or 0 if socket_set is NULL
 */
size_t zoo_socket_set_count(const ZOO_SOCKET_SET_STRUCT *socket_set);

// ==============================================================================
// SELECT OPERATIONS
// ==============================================================================

/**
 * @brief Monitor multiple sockets for I/O readiness
 * 
 * This function monitors a set of sockets for I/O readiness using the most
 * efficient mechanism available on the platform:
 * - Linux: epoll() (for better scalability with many sockets)
 * - POSIX: select() (fallback or when epoll unavailable)
 * - lwIP: lwip_select() (embedded systems)
 * 
 * @param read_set Pointer to set of sockets to monitor for read readiness (can be NULL)
 * @param write_set Pointer to set of sockets to monitor for write readiness (can be NULL)
 * @param error_set Pointer to set of sockets to monitor for error conditions (can be NULL)
 * @param timeout_ms Timeout in milliseconds (-1 for infinite, 0 for immediate)
 * @param result Pointer to result structure to fill (can be NULL)
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_TIMEOUT if timeout occurred with no ready sockets
 * @retval ZOO_ERROR_INVALID_PARAM if all sets are NULL
 * 
 * @note On Linux, epoll is used automatically for better performance with large socket counts.
 *       The implementation gracefully falls back to select() if epoll initialization fails.
 */
ZOO_ERROR_TYPE zoo_socket_select_multiple(ZOO_SOCKET_SET_STRUCT *read_set,
                                          ZOO_SOCKET_SET_STRUCT *write_set,
                                          ZOO_SOCKET_SET_STRUCT *error_set,
                                          int timeout_ms,
                                          ZOO_SELECT_RESULT_STRUCT *result);

/**
 * @brief Check if a specific socket is ready for reading
 * 
 * @param socket_set Pointer to socket set that was used in select
 * @param socket_info Pointer to socket to check
 * @return true if socket is ready for reading, false otherwise
 */
bool zoo_socket_set_is_read_ready(const ZOO_SOCKET_SET_STRUCT *socket_set,
                                  const ZOO_SOCKET_INFO_STRUCT *socket_info);

/**
 * @brief Check if a specific socket is ready for writing
 * 
 * @param socket_set Pointer to socket set that was used in select
 * @param socket_info Pointer to socket to check
 * @return true if socket is ready for writing, false otherwise
 */
bool zoo_socket_set_is_write_ready(const ZOO_SOCKET_SET_STRUCT *socket_set,
                                   const ZOO_SOCKET_INFO_STRUCT *socket_info);

/**
 * @brief Check if a specific socket has an error condition
 * 
 * @param socket_set Pointer to socket set that was used in select
 * @param socket_info Pointer to socket to check
 * @return true if socket has an error condition, false otherwise
 */
bool zoo_socket_set_has_error(const ZOO_SOCKET_SET_STRUCT *socket_set,
                              const ZOO_SOCKET_INFO_STRUCT *socket_info);

// ==============================================================================
// CONVENIENCE FUNCTIONS
// ==============================================================================

/**
 * @brief Wait for a single socket to become ready for specified events
 * 
 * This is a convenience function that monitors a single socket for the specified
 * events. It's equivalent to creating a socket set, adding the socket, and calling
 * zoo_socket_select_multiple.
 * 
 * @param socket_info Pointer to socket to monitor
 * @param events Events to monitor (combination of ZOO_SELECT_EVENT_* flags)
 * @param timeout_ms Timeout in milliseconds
 * @param ready_events Pointer to variable to receive ready events (can be NULL)
 * @return ZOO_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_socket_wait_for_events(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                          ZOO_SELECT_EVENT_ENUM events,
                                          int timeout_ms,
                                          ZOO_SELECT_EVENT_ENUM *ready_events);

/**
 * @brief Check if a socket is immediately ready for reading (non-blocking)
 * 
 * @param socket_info Pointer to socket to check
 * @param is_ready Pointer to bool to receive result
 * @return ZOO_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_socket_is_read_ready(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                       bool *is_ready);

/**
 * @brief Check if a socket is immediately ready for writing (non-blocking)
 * 
 * @param socket_info Pointer to socket to check
 * @param is_ready Pointer to bool to receive result
 * @return ZOO_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_socket_is_write_ready(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                        bool *is_ready);

/**
 * @brief Check if a socket has an error condition (non-blocking)
 * 
 * @param socket_info Pointer to socket to check
 * @param has_error Pointer to bool to receive result
 * @return ZOO_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_socket_has_error(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                   bool *has_error);

// ==============================================================================
// PLATFORM-SPECIFIC UTILITIES
// ==============================================================================

/**
 * @brief Check if epoll is available and being used
 * 
 * This function returns whether the current platform supports and is using
 * epoll for socket operations. Useful for diagnostics and performance tuning.
 * 
 * @return true if epoll is available and enabled, false otherwise
 * @note This is a compile-time constant on most platforms
 */
bool zoo_socket_epoll_available(void);

// ==============================================================================
// ITERATOR FUNCTIONS
// ==============================================================================

/**
 * @brief Get the first ready socket from a socket set
 * 
 * Use this function in combination with zoo_socket_set_get_next_ready to iterate
 * through all ready sockets after a select operation.
 * 
 * @param socket_set Pointer to socket set
 * @param index Pointer to index variable (initialize to 0 before first call)
 * @return Pointer to first ready socket, or NULL if no ready sockets
 */
ZOO_SOCKET_INFO_STRUCT *zoo_socket_set_get_first_ready(const ZOO_SOCKET_SET_STRUCT *socket_set,
                                                       size_t *index);

/**
 * @brief Get the next ready socket from a socket set
 * 
 * @param socket_set Pointer to socket set
 * @param index Pointer to index variable (updated by this function)
 * @return Pointer to next ready socket, or NULL if no more ready sockets
 */
ZOO_SOCKET_INFO_STRUCT *zoo_socket_set_get_next_ready(const ZOO_SOCKET_SET_STRUCT *socket_set,
                                                      size_t *index);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SELECT_H */
