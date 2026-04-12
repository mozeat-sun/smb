/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_REACTOR
 * File name: zoo_smb_reactor.c
 * Description: Unified reactor abstraction for SMB transports
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/

#include "zoo_smb_reactor.h"
#include "zoo_smb_error.h"
#include <errno.h>

#if defined(__linux__)
#include <sys/epoll.h>
#endif

/**
 * @brief Watches (registers or modifies) a file descriptor in the reactor.
 *
 * On Linux, uses epoll to add or modify the watched file descriptor for the specified events.
 * If the file descriptor is not already registered, it will be added; otherwise, it is modified.
 *
 * @param reactor_fd   The epoll file descriptor representing the reactor.
 * @param watched_fd   The file descriptor to watch for events.
 * @param events       Bitmask of events to watch (e.g., EPOLLIN, EPOLLOUT).
 * @return ZOO_SMB_OK on success, ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED on error, or ZOO_SMB_ERROR_NOT_SUPPORTED if not on Linux.
 */
ZOO_ERROR_TYPE zoo_smb_reactor_watch_fd(int reactor_fd, int watched_fd, uint32_t events)
{
#if defined(__linux__)
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = watched_fd;

    if (epoll_ctl(reactor_fd, EPOLL_CTL_MOD, watched_fd, &ev) == -1)
    {
        if (errno == ENOENT)
        {
            if (epoll_ctl(reactor_fd, EPOLL_CTL_ADD, watched_fd, &ev) == -1)
            {
                return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
            }
            return ZOO_SMB_OK;
        }
        return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
    }

    return ZOO_SMB_OK;
#else
    (void)reactor_fd;
    (void)watched_fd;
    (void)events;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Unwatches (removes) a file descriptor from the reactor.
 *
 * On Linux, uses epoll to remove the watched file descriptor from the reactor.
 *
 * @param reactor_fd   The epoll file descriptor representing the reactor.
 * @param watched_fd   The file descriptor to remove from watching.
 * @return ZOO_SMB_OK on success, ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED on error, or ZOO_SMB_ERROR_NOT_SUPPORTED if not on Linux.
 */
ZOO_ERROR_TYPE zoo_smb_reactor_unwatch_fd(int reactor_fd, int watched_fd)
{
#if defined(__linux__)
    if (epoll_ctl(reactor_fd, EPOLL_CTL_DEL, watched_fd, NULL) == -1)
    {
        if (errno != ENOENT)
        {
            return ZOO_SMB_ERROR_TRANSPORT_INIT_FAILED;
        }
    }
    return ZOO_SMB_OK;
#else
    (void)reactor_fd;
    (void)watched_fd;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Waits for events on the reactor and returns ready events.
 *
 * On Linux, uses epoll_wait to wait for events on registered file descriptors.
 * Fills out_events with up to max_events ready events and sets ready_count to the number of events.
 *
 * @param reactor_fd   The epoll file descriptor representing the reactor.
 * @param out_events   Pointer to an array of epoll_event structures to receive events.
 * @param max_events   Maximum number of events to return.
 * @param timeout_ms   Timeout in milliseconds (-1 for infinite).
 * @param ready_count  Pointer to int to receive the number of ready events.
 * @return ZOO_SMB_OK on success, ZOO_SMB_ERROR_INVALID_PARAM for bad arguments, ZOO_SMB_ERROR_NETWORK_IO_ERROR on error, or ZOO_SMB_ERROR_NOT_SUPPORTED if not on Linux.
 */
ZOO_ERROR_TYPE zoo_smb_reactor_wait(
    int reactor_fd,
    void* out_events,
    int max_events,
    int timeout_ms,
    int* ready_count)
{
    if (!out_events || max_events <= 0 || !ready_count)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

#if defined(__linux__)
    int n = epoll_wait(reactor_fd, (struct epoll_event*)out_events, max_events, timeout_ms);
    if (n < 0)
    {
        if (errno == EINTR)
        {
            *ready_count = 0;
            return ZOO_SMB_OK;
        }
        return ZOO_SMB_ERROR_NETWORK_IO_ERROR;
    }

    *ready_count = n;
    return ZOO_SMB_OK;
#else
    (void)reactor_fd;
    (void)timeout_ms;
    *ready_count = 0;
    return ZOO_SMB_ERROR_NOT_SUPPORTED;
#endif
}
