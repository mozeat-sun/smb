/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_REACTOR
 * File name: zoo_smb_reactor.h
 * Description: Unified reactor abstraction for SMB transports
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/

#ifndef ZOO_SMB_REACTOR_H
#define ZOO_SMB_REACTOR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_error.h"
#include <stdint.h>

#if defined(__linux__)
#include <sys/epoll.h>
#endif

    /**
     * @brief Add or modify fd in reactor backend.
     *
     * @param reactor_fd Backend reactor descriptor (e.g. epoll fd)
     * @param watched_fd Target socket/fd
     * @param events Backend event mask
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE zoo_smb_reactor_watch_fd(int reactor_fd, int watched_fd, uint32_t events);

    /**
     * @brief Remove fd from reactor backend.
     *
     * @param reactor_fd Backend reactor descriptor
     * @param watched_fd Target socket/fd
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE zoo_smb_reactor_unwatch_fd(int reactor_fd, int watched_fd);

    /**
     * @brief Wait for events from reactor backend.
     *
     * @param reactor_fd Backend reactor descriptor
     * @param out_events Event array buffer (backend-specific layout)
     * @param max_events Capacity of out_events array
     * @param timeout_ms Wait timeout in milliseconds
     * @param ready_count Output ready event count
     * @return ZOO_SMB_OK on success, error code otherwise
     */
    ZOO_ERROR_TYPE zoo_smb_reactor_wait(
        int reactor_fd,
        void* out_events,
        int max_events,
        int timeout_ms,
        int* ready_count);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_REACTOR_H */
