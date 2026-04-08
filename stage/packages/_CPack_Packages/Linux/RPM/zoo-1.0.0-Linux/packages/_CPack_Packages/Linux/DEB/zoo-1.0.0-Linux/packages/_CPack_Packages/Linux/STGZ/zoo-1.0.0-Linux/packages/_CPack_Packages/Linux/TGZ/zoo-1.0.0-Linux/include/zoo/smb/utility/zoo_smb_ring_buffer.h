/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: zoo_smb_ring_buffer
 * File name: zoo_smb_ring_buffer.h
 * Description: High-performance ring buffer implementation for ZOO SMB
 * Features:
 * - Lock-free single producer/single consumer
 * - Memory alignment for cache efficiency
 * - Zero-copy operations where possible
 * - Variable-length message support
 * - Memory barrier support for multi-core systems
 * - Overflow handling strategies
 * - Optimized for shared memory transport
 *
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-16     weiwang.sun       created from ring_queue
 * 2.0       2025-06-23     weiwang.sun    Optimized for SHM transport
 ******************************************************************************/
#ifndef ZOO_SMB_RING_BUFFER_H
#define ZOO_SMB_RING_BUFFER_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo_smb_types.h"
#include "zoo_smb_error.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

    /* ========== Constants ========== */

#define ZOO_SMB_RING_BUFFER_MAGIC 0x52494E47 /* "RING" */
#define ZOO_SMB_RING_BUFFER_VERSION 2
#define ZOO_SMB_RING_BUFFER_MIN_SIZE 1024               /* 1KB minimum */
#define ZOO_SMB_RING_BUFFER_MAX_SIZE (64 * 1024 * 1024) /* 64MB maximum */
#define ZOO_SMB_RING_BUFFER_ALIGNMENT 64                /* Cache line alignment */

/* Message header size */
#define ZOO_SMB_RING_MSG_HEADER ZOO_SMB_MSG_HEADER_STRUCT
#define ZOO_SMB_RING_MSG_HEADER_SIZE sizeof(ZOO_SMB_RING_MSG_HEADER)

    typedef struct ZOO_SMB_RING_BUFFER_STRUCT* ZOO_SMB_RING_BUFFER_HANDLE;

    /* ========== Data Structures ========== */

    /* ========== Function Declarations ========== */

    /**
     * @brief Calculate required memory size for ring buffer
     * @param data_size Desired data area size
     * @return Total required memory size
     */
    size_t zoo_smb_ring_buffer_get_required_size(size_t data_size);

    /**
     * @brief Create new ring buffer in provided memory
     * @param memory Memory pointer where to create the ring buffer
     * @param memory_size Total memory size available
     * @param allow_overwrite Allow overwriting unread data
     * @return Ring buffer handle on success, NULL on failure
     */
    ZOO_SMB_RING_BUFFER_HANDLE zoo_smb_ring_buffer_create_in_memory(
        void* memory,
        size_t memory_size,
        ZOO_BOOL allow_overwrite);

    /**
     * @brief Attach to existing ring buffer in memory
     * @param memory Memory pointer containing existing ring buffer
     * @return Ring buffer handle on success, NULL on failure
     */
    ZOO_SMB_RING_BUFFER_HANDLE zoo_smb_ring_buffer_attach(void* memory);

    /**
     * @brief Destroy ring buffer
     * @param buffer Buffer handle
     */
    void zoo_smb_ring_buffer_destroy(ZOO_SMB_RING_BUFFER_HANDLE buffer);

    /**
     * @brief Write data to ring buffer
     * @param buffer Buffer handle
     * @param data Data to write
     * @param size Data size
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_ring_buffer_write(ZOO_SMB_RING_BUFFER_HANDLE buffer,
                                                 const void* data,
                                                 size_t size);

    /**
     * @brief Read data from ring buffer
     * @param buffer Buffer handle
     * @param data Output data buffer
     * @param size Expected data size
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_ring_buffer_read(ZOO_SMB_RING_BUFFER_HANDLE buffer,
                                                void* data,
                                                size_t size);

    /**
     * @brief Get next message size without consuming it
     * @param buffer Buffer handle
     * @return Message size in bytes, 0 if no message available
     */
    size_t zoo_smb_ring_buffer_get_next_message_size(ZOO_SMB_RING_BUFFER_HANDLE buffer);

    /**
     * @brief Check if buffer has data available
     * @param buffer Buffer handle
     * @return ZOO_TRUE if data is available, ZOO_FALSE otherwise
     */
    ZOO_BOOL zoo_smb_ring_buffer_has_data(ZOO_SMB_RING_BUFFER_HANDLE buffer);

    /**
     * @brief Get available space for writing
     * @param buffer Buffer handle
     * @return Available space in bytes
     */
    uint32_t zoo_smb_ring_buffer_get_available_space(ZOO_SMB_RING_BUFFER_HANDLE buffer);

    /**
     * @brief Check if buffer is full
     * @param buffer Buffer handle
     * @return ZOO_TRUE if full, ZOO_FALSE otherwise
     */
    ZOO_BOOL zoo_smb_ring_buffer_is_full(ZOO_SMB_RING_BUFFER_HANDLE buffer);

    /**
     * @brief Check if buffer is empty
     * @param buffer Buffer handle
     * @return ZOO_TRUE if empty, ZOO_FALSE otherwise
     */
    ZOO_BOOL zoo_smb_ring_buffer_is_empty(ZOO_SMB_RING_BUFFER_HANDLE buffer);

    /**
     * @brief Validate ring buffer integrity
     * @param buffer Buffer handle
     * @return ZOO_TRUE if valid, ZOO_FALSE otherwise
     */
    ZOO_BOOL zoo_smb_ring_buffer_is_valid(ZOO_SMB_RING_BUFFER_HANDLE buffer);

    /**
     * @brief Clear all data in buffer
     * @param buffer Buffer handle
     * @return ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_ring_buffer_clear(ZOO_SMB_RING_BUFFER_HANDLE buffer);

    /**
     * @brief Get the amount of data available for reading in the ring buffer
     * @param buffer Ring buffer handle
     * @return Number of bytes available for reading, 0 if buffer is empty or invalid
     */
    size_t zoo_smb_ring_buffer_get_available_data_size(ZOO_SMB_RING_BUFFER_HANDLE buffer);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_RING_BUFFER_H */