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
