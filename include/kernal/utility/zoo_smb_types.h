/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TYPES
 * File name: zoo_smb_types.h
 * Description: Common type definitions for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_SMB_TYPES_H
#define ZOO_SMB_TYPES_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

    /**
     * @enum ZOO_ERROR_TYPE
     * @brief Enumeration of error types for ZOO SMB operations.
     */
    typedef enum
    {
        ZOO_SMB_NODE_TYPE_CLIENT = 0,
        ZOO_SMB_NODE_TYPE_SERVER = 1,
        ZOO_SMB_NODE_TYPE_PUBLISHER = 2,
        ZOO_SMB_NODE_TYPE_SUBSCRIBER = 3,
        ZOO_SMB_NODE_TYPE_DISCOVERY = 4,
        ZOO_SMB_NODE_TYPE_MAX
    } ZOO_SMB_NODE_TYPE_ENUM;

    /**
     * @enum ZOO_SMB_TRANSPORT_TYPE_ENUM
     * @brief Enumeration of supported SMB transport types.
     */
    typedef enum
    {
        ZOO_SMB_TRANSPORT_TYPE_MIN = 0,
        ZOO_SMB_TRANSPORT_TYPE_UDP = 1,
        ZOO_SMB_TRANSPORT_TYPE_UDP_BROADCAST = 2,
        ZOO_SMB_TRANSPORT_TYPE_TCP = 3,
        ZOO_SMB_TRANSPORT_TYPE_SHM = 4,
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT = ZOO_SMB_TRANSPORT_TYPE_UDP, /**< Default transport type */
        ZOO_SMB_TRANSPORT_TYPE_MAX = 5,
    } ZOO_SMB_TRANSPORT_TYPE_ENUM;

    /**
     * @brief Function pointer type for handling events related to a ZOO_SMB_NODE.
     *
     * This handler is called when a specific event occurs for a node.
     *
     * @param node_handle Handle to the ZOO_SMB_NODE instance associated with the event.
     */
    typedef int (*ZOO_SMB_MSG_HANDLER)(void* user_data, uint32_t msg_id, uint64_t request_id, uint64_t timestamp, const char* sender, const void* payload, size_t payload_size);

#define ZOO_SMB_NODE_TYPE_TO_STRING(type) (                                                             \
    (type) == ZOO_SMB_NODE_TYPE_CLIENT ? "CLIENT" : (type) == ZOO_SMB_NODE_TYPE_SERVER   ? "SERVER"     \
                                                : (type) == ZOO_SMB_NODE_TYPE_PUBLISHER  ? "PUBLISHER"  \
                                                : (type) == ZOO_SMB_NODE_TYPE_SUBSCRIBER ? "SUBSCRIBER" \
                                                : (type) == ZOO_SMB_NODE_TYPE_MAX        ? "MAX"        \
                                                                                         : "UNKNOWN")
#define ZOO_SMB_TRANSPORT_TYPE_TO_STRING(type) (                                                                    \
    (type) == ZOO_SMB_TRANSPORT_TYPE_UDP ? "UDP" : (type) == ZOO_SMB_TRANSPORT_TYPE_UDP_BROADCAST ? "UDP_BROADCAST" \
                                               : (type) == ZOO_SMB_TRANSPORT_TYPE_TCP             ? "TCP"           \
                                               : (type) == ZOO_SMB_TRANSPORT_TYPE_SHM             ? "SHM"           \
                                               : (type) == ZOO_SMB_TRANSPORT_TYPE_MIN             ? "MIN"           \
                                               : (type) == ZOO_SMB_TRANSPORT_TYPE_MAX             ? "MAX"           \
                                                                                                  : "UNKNOWN")

// ==============================================================================
// UTILITY MACROS
// ==============================================================================

/**
 * @brief SMB-specific utility macros
 */
#define ZOO_SMB_UNUSED(x) ZOO_UNUSED(x)
#define ZOO_SMB_SLEEP_MS(ms) ZOO_SLEEP_MS(ms)
#define ZOO_SMB_SLEEP_US(us) ZOO_SLEEP_US(us)

/**
 * @brief Pointer validation macros
 */
#define ZOO_SMB_VALIDATE_PTR(ptr, ret_val) \
    do { \
        if ((ptr) == NULL) { \
            ZOO_LOG_ERROR("NULL pointer validation failed: " #ptr); \
            return (ret_val); \
        } \
    } while(0)

#define ZOO_SMB_VALIDATE_PTR_RETURN_VOID(ptr) \
    do { \
        if ((ptr) == NULL) { \
            ZOO_LOG_ERROR("NULL pointer validation failed: " #ptr); \
            return; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TYPES_H */