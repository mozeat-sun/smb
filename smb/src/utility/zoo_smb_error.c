/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_ERROR
 * File name: zoo_smb_error.c
 * Description: Implementation of the comprehensive error handling system
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun       created
 * 2.0       2025-06-23     weiwang.sun    Re-implemented for structured errors
 ******************************************************************************/

#include "zoo_smb_error.h"
#include "zoo_smb_types.h"
#include "../../log/inc/zoo_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// ==============================================================================
// THREAD-LOCAL ERROR STORAGE
// ==============================================================================

/**
 * @brief Thread-local variable to store the last SMB error code.
 *
 * This variable holds the most recent error code of type ZOO_ERROR_TYPE
 * encountered in the current thread. It is initialized to ZOO_SMB_OK.
 * The use of __thread ensures that each thread has its own instance of this variable,
 * preventing race conditions and ensuring thread safety when tracking errors.
 */
static __thread ZOO_ERROR_TYPE last_error = ZOO_SMB_OK;

// ==============================================================================
// GLOBAL ERROR CALLBACK MANAGEMENT
// ==============================================================================

// ==============================================================================
// ERROR STRING MAPPING TABLES
// ==============================================================================

/**
 * @brief Error category string mappings
 */
static const char* const category_strings[] = {
    [ZOO_SMB_CATEGORY_GENERAL] = "General",
    [ZOO_SMB_CATEGORY_MEMORY] = "Memory",
    [ZOO_SMB_CATEGORY_THREAD] = "Thread",
    [ZOO_SMB_CATEGORY_NETWORK] = "Network",
    [ZOO_SMB_CATEGORY_PROTOCOL] = "Protocol",
    [ZOO_SMB_CATEGORY_TRANSPORT] = "Transport",
    [ZOO_SMB_CATEGORY_SERVICE] = "Service",
    [ZOO_SMB_CATEGORY_CONFIG] = "Config",
    [ZOO_SMB_CATEGORY_FILESYSTEM] = "FileSystem",
    [ZOO_SMB_CATEGORY_SECURITY] = "Security",
    [ZOO_SMB_CATEGORY_DATA_STRUCTURE] = "DataStructure",
    [ZOO_SMB_CATEGORY_HANDSHAKE] = "Handshake",
    [ZOO_SMB_CATEGORY_ROUTING] = "Routing",
    [ZOO_SMB_CATEGORY_LOGGING] = "Logging",
    [ZOO_SMB_CATEGORY_SHARED_MEMORY] = "SharedMemory",
    [ZOO_SMB_CATEGORY_RING_BUFFER] = "RingBuffer",
    [ZOO_SMB_CATEGORY_SUBSCRIPTION] = "Subscription",
    [ZOO_SMB_CATEGORY_PUBLICATION] = "Publication",
    [ZOO_SMB_CATEGORY_DISCOVERY] = "Discovery",
    [ZOO_SMB_CATEGORY_PERFORMANCE] = "Performance",
    [ZOO_SMB_CATEGORY_VALIDATION] = "Validation",
    [ZOO_SMB_CATEGORY_SERIALIZATION] = "Serialization",
    [ZOO_SMB_CATEGORY_COMPRESSION] = "Compression",
    [ZOO_SMB_CATEGORY_ENCRYPTION] = "Encryption",
    [ZOO_SMB_CATEGORY_STATISTICS] = "Statistics",
    [ZOO_SMB_CATEGORY_MAINTENANCE] = "Maintenance",
    [ZOO_SMB_CATEGORY_RESOURCE] = "Resource",
    [ZOO_SMB_CATEGORY_TIMEOUT] = "Timeout",
    [ZOO_SMB_CATEGORY_COMPATIBILITY] = "Compatibility",
    [ZOO_SMB_CATEGORY_EXTENSION] = "Extension"};

/**
 * @brief Error severity string mappings
 */
static const char* const severity_strings[] = {
    [ZOO_SMB_SEVERITY_INFO] = "Info",
    [ZOO_SMB_SEVERITY_WARNING] = "Warning",
    [ZOO_SMB_SEVERITY_ERROR] = "Error",
    [ZOO_SMB_SEVERITY_CRITICAL] = "Critical",
    [ZOO_SMB_SEVERITY_FATAL] = "Fatal"};

// ==============================================================================
// ERROR STRING HELPER FUNCTIONS
// ==============================================================================

/**
 * @brief Get general error string based on subcategory and specific code
 */
static const char* get_general_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_GENERAL_PARAM:
            switch (specific)
            {
                case 1:
                    return "Invalid parameter";
                case 2:
                    return "Null pointer";
                case 3:
                    return "Buffer too small";
                case 4:
                    return "Invalid size";
                case 5:
                    return "Out of range";
                default:
                    return "Parameter validation error";
            }
        case ZOO_SMB_GENERAL_STATE:
            switch (specific)
            {
                case 1:
                    return "Not initialized";
                case 2:
                    return "Already initialized";
                case 3:
                    return "Invalid state";
                case 4:
                    return "Not started";
                case 5:
                    return "Already started";
                default:
                    return "State management error";
            }
        case ZOO_SMB_GENERAL_OPERATION:
            switch (specific)
            {
                case 1:
                    return "Operation not supported";
                case 2:
                    return "Operation not implemented";
                case 3:
                    return "Operation failed";
                case 4:
                    return "Operation interrupted";
                default:
                    return "Operation execution error";
            }
        case ZOO_SMB_GENERAL_RESOURCE:
            switch (specific)
            {
                case 1:
                    return "Resource not found";
                case 2:
                    return "Resource already exists";
                case 3:
                    return "Resource busy";
                case 4:
                    return "Resource temporarily unavailable";
                default:
                    return "Resource access error";
            }
        default:
            return "General error";
    }
}

/**
 * @brief Get memory error string based on subcategory and specific code
 */
static const char* get_memory_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_MEMORY_ALLOCATION:
            switch (specific)
            {
                case 1:
                    return "Out of memory";
                case 2:
                    return "Memory allocation failed";
                case 3:
                    return "Memory free failed";
                default:
                    return "Memory allocation error";
            }
        case ZOO_SMB_MEMORY_POOL:
            switch (specific)
            {
                case 1:
                    return "Memory pool creation failed";
                case 2:
                    return "Memory pool destruction failed";
                case 3:
                    return "Memory pool corrupted";
                case 4:
                    return "Memory pool exhausted";
                default:
                    return "Memory pool error";
            }
        case ZOO_SMB_MEMORY_CORRUPTION:
            switch (specific)
            {
                case 1:
                    return "Buffer overflow";
                case 2:
                    return "Buffer underflow";
                case 3:
                    return "Invalid memory block";
                case 4:
                    return "Double free detected";
                default:
                    return "Memory corruption error";
            }
        case ZOO_SMB_MEMORY_LEAK:
            return "Memory leak detected";
        default:
            return "Memory error";
    }
}

/**
 * @brief Get transport error string based on subcategory and specific code
 */
static const char* get_transport_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_TRANSPORT_LIFECYCLE:
            switch (specific)
            {
                case 1:
                    return "Transport creation failed";
                case 2:
                    return "Transport initialization failed";
                case 3:
                    return "Transport start failed";
                case 4:
                    return "Transport stop failed";
                case 5:
                    return "Transport destruction failed";
                default:
                    return "Transport lifecycle error";
            }
        case ZOO_SMB_TRANSPORT_COMMUNICATION:
            switch (specific)
            {
                case 1:
                    return "Transport send failed";
                case 2:
                    return "Transport receive failed";
                case 3:
                    return "Message too large for transport";
                default:
                    return "Transport communication error";
            }
        case ZOO_SMB_TRANSPORT_CONNECTION:
            switch (specific)
            {
                case 1:
                    return "Transport connection failed";
                case 2:
                    return "Transport disconnected";
                case 3:
                    return "Transport connection lost";
                default:
                    return "Transport connection error";
            }
        case ZOO_SMB_TRANSPORT_CONSUMER:
            switch (specific)
            {
                case 1:
                    return "Transport consumer not found";
                case 2:
                    return "Transport consumer already exists";
                default:
                    return "Transport consumer error";
            }
        default:
            return "Transport error";
    }
}

/**
 * @brief Get handshake error string based on subcategory and specific code
 */
static const char* get_handshake_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_HANDSHAKE_PROCESS:
            switch (specific)
            {
                case 1:
                    return "Handshake failed";
                case 2:
                    return "Handshake rejected";
                case 3:
                    return "Handshake version mismatch";
                default:
                    return "Handshake process error";
            }
        case ZOO_SMB_HANDSHAKE_MESSAGE:
            switch (specific)
            {
                case 1:
                    return "Invalid handshake message";
                case 2:
                    return "Handshake message format error";
                case 3:
                    return "Handshake message send failed";
                default:
                    return "Handshake message error";
            }
        case ZOO_SMB_HANDSHAKE_TIMEOUT:
            switch (specific)
            {
                case 1:
                    return "Handshake timeout";
                case 2:
                    return "Handshake acknowledgment timeout";
                default:
                    return "Handshake timeout error";
            }
        default:
            return "Handshake error";
    }
}

/**
 * @brief Get shared memory error string based on subcategory and specific code
 */
static const char* get_shm_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_SHM_LIFECYCLE:
            switch (specific)
            {
                case 1:
                    return "Shared memory creation failed";
                case 2:
                    return "Shared memory attach failed";
                case 3:
                    return "Shared memory detach failed";
                case 4:
                    return "Shared memory destruction failed";
                default:
                    return "Shared memory lifecycle error";
            }
        case ZOO_SMB_SHM_ACCESS:
            switch (specific)
            {
                case 1:
                    return "Shared memory not found";
                case 2:
                    return "Shared memory already exists";
                case 3:
                    return "Shared memory permission denied";
                default:
                    return "Shared memory access error";
            }
        case ZOO_SMB_SHM_SYNCHRONIZATION:
            switch (specific)
            {
                case 1:
                    return "Shared memory lock failed";
                case 2:
                    return "Shared memory unlock failed";
                case 3:
                    return "Shared memory semaphore creation failed";
                case 4:
                    return "Shared memory synchronization failed";
                case 5:
                    return "Event file descriptor creation failed";
                default:
                    return "Shared memory synchronization error";
            }
        case ZOO_SMB_SHM_DATA:
            switch (specific)
            {
                case 1:
                    return "Shared memory buffer full";
                case 2:
                    return "Invalid shared memory message";
                case 3:
                    return "Shared memory version mismatch";
                default:
                    return "Shared memory data error";
            }
        default:
            return "Shared memory error";
    }
}

/**
 * @brief Get ring buffer error string based on subcategory and specific code
 */
static const char* get_ringbuf_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_RINGBUF_LIFECYCLE:
            switch (specific)
            {
                case 1:
                    return "Ring buffer creation failed";
                case 2:
                    return "Ring buffer destruction failed";
                case 3:
                    return "Ring buffer initialization failed";
                default:
                    return "Ring buffer lifecycle error";
            }
        case ZOO_SMB_RINGBUF_OPERATION:
            switch (specific)
            {
                case 1:
                    return "Ring buffer write failed";
                case 2:
                    return "Ring buffer read failed";
                case 3:
                    return "Ring buffer full";
                case 4:
                    return "Ring buffer empty";
                default:
                    return "Ring buffer operation error";
            }
        case ZOO_SMB_RINGBUF_STATE:
            switch (specific)
            {
                case 1:
                    return "Ring buffer not initialized";
                case 2:
                    return "Ring buffer invalid state";
                case 3:
                    return "Ring buffer not owner";
                default:
                    return "Ring buffer state error";
            }
        case ZOO_SMB_RINGBUF_DATA:
            switch (specific)
            {
                case 1:
                    return "Ring buffer corrupted";
                case 2:
                    return "Ring buffer overflow";
                case 3:
                    return "Ring buffer underflow";
                case 4:
                    return "Ring buffer invalid magic number";
                default:
                    return "Ring buffer data integrity error";
            }
        default:
            return "Ring buffer error";
    }
}

/**
 * @brief Get timeout error string based on subcategory and specific code
 */
static const char* get_timeout_error_string(uint32_t subcategory, uint32_t specific)
{
    specific = specific; // Avoid ZOO_SMB_UNUSED variable warning
    switch (subcategory)
    {
        case ZOO_SMB_TIMEOUT_OPERATION:
            return "Operation timeout";
        case ZOO_SMB_TIMEOUT_CONNECTION:
            return "Connection timeout";
        case ZOO_SMB_TIMEOUT_RESPONSE:
            return "Response timeout";
        case ZOO_SMB_TIMEOUT_HEARTBEAT:
            return "Heartbeat timeout";
        default:
            return "Timeout error";
    }
}

/**
 * @brief Get thread error string based on subcategory and specific code
 */
static const char* get_thread_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_THREAD_LIFECYCLE:
            switch (specific)
            {
                case 1: return "Thread creation failed";
                case 2: return "Thread join failed";
                case 3: return "Thread detach failed";
                default: return "Thread lifecycle error";
            }
        case ZOO_SMB_THREAD_SYNCHRONIZATION:
            switch (specific)
            {
                case 1: return "Mutex initialization failed";
                case 2: return "Mutex lock failed";
                case 3: return "Mutex unlock failed";
                case 4: return "Condition variable initialization failed";
                case 5: return "Condition variable wait failed";
                case 6: return "Condition variable signal failed";
                default: return "Thread synchronization error";
            }
        default:
            return "Thread error";
    }
}

/**
 * @brief Get network error string based on subcategory and specific code
 */
static const char* get_network_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_NETWORK_SOCKET:
            switch (specific)
            {
                case 1: return "Socket creation failed";
                case 2: return "Socket bind failed";
                case 3: return "Socket listen failed";
                case 4: return "Socket accept failed";
                case 5: return "Socket connect failed";
                default: return "Socket error";
            }
        case ZOO_SMB_NETWORK_IO:
            switch (specific)
            {
                case 1: return "Network send failed";
                case 2: return "Network receive failed";
                case 3: return "Network I/O error";
                default: return "Network I/O error";
            }
        default:
            return "Network error";
    }
}

/**
 * @brief Get protocol error string based on subcategory and specific code
 */
static const char* get_protocol_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_PROTOCOL_MESSAGE:
            switch (specific)
            {
                case 1: return "Invalid message format";
                case 2: return "Message too large";
                case 3: return "Message corrupted";
                case 4: return "Message incomplete";
                default: return "Protocol message error";
            }
        case ZOO_SMB_PROTOCOL_VERSION:
            switch (specific)
            {
                case 1: return "Version mismatch";
                case 2: return "Version not supported";
                default: return "Protocol version error";
            }
        case ZOO_SMB_PROTOCOL_SEQUENCE:
            switch (specific)
            {
                case 1: return "Message sequence error";
                case 2: return "Duplicate message";
                default: return "Protocol sequence error";
            }
        default:
            return "Protocol error";
    }
}

/**
 * @brief Get service error string based on subcategory and specific code
 */
static const char* get_service_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_SERVICE_LIFECYCLE:
            switch (specific)
            {
                case 1: return "Service start failed";
                case 2: return "Service stop failed";
                case 3: return "Service restart failed";
                default: return "Service lifecycle error";
            }
        case ZOO_SMB_SERVICE_REGISTRATION:
            switch (specific)
            {
                case 1: return "Service registration failed";
                case 2: return "Service unregistration failed";
                case 3: return "Service already registered";
                default: return "Service registration error";
            }
        case ZOO_SMB_SERVICE_DISCOVERY:
            switch (specific)
            {
                case 1: return "Service not found";
                case 2: return "Service unavailable";
                default: return "Service discovery error";
            }
        default:
            return "Service error";
    }
}

/**
 * @brief Get config error string based on subcategory and specific code
 */
static const char* get_config_error_string(uint32_t subcategory, uint32_t specific)
{
    switch (subcategory)
    {
        case ZOO_SMB_CONFIG_FILE:
            switch (specific)
            {
                case 1: return "Configuration file not found";
                case 2: return "Configuration file read failed";
                case 3: return "Configuration file write failed";
                default: return "Configuration file error";
            }
        case ZOO_SMB_CONFIG_PARSE:
            switch (specific)
            {
                case 1: return "Configuration parse failed";
                case 2: return "Invalid configuration format";
                default: return "Configuration parse error";
            }
        case ZOO_SMB_CONFIG_VALIDATION:
            switch (specific)
            {
                case 1: return "Invalid configuration value";
                case 2: return "Missing required configuration";
                default: return "Configuration validation error";
            }
        default:
            return "Configuration error";
    }
}

// ==============================================================================
// PUBLIC API IMPLEMENTATION
// ==============================================================================

/**
 * @brief Get human-readable error string
 */
const char* zoo_smb_get_error_string(ZOO_ERROR_TYPE error)
{
    if (ZOO_SMB_IS_SUCCESS(error))
    {
        return "Success";
    }

    uint32_t category = ZOO_SMB_GET_CATEGORY(error);
    uint32_t subcategory = ZOO_SMB_GET_SUBCATEGORY(error);
    uint32_t specific = ZOO_SMB_GET_SPECIFIC(error);

    switch (category)
    {
        case ZOO_SMB_CATEGORY_GENERAL:
            return get_general_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_MEMORY:
            return get_memory_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_THREAD:
            return get_thread_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_NETWORK:
            return get_network_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_PROTOCOL:
            return get_protocol_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_TRANSPORT:
            return get_transport_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_SERVICE:
            return get_service_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_CONFIG:
            return get_config_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_HANDSHAKE:
            return get_handshake_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_SHARED_MEMORY:
            return get_shm_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_RING_BUFFER:
            return get_ringbuf_error_string(subcategory, specific);
        case ZOO_SMB_CATEGORY_TIMEOUT:
            return get_timeout_error_string(subcategory, specific);
        default:
            return "Unknown error category";
    }
}

/**
 * @brief Get error category name
 */
const char* zoo_smb_get_category_string(ZOO_SMB_ERROR_CATEGORY category)
{
    if (category < sizeof(category_strings) / sizeof(category_strings[0]) &&
        category_strings[category] != NULL)
    {
        return category_strings[category];
    }
    return "Unknown Category";
}

/**
 * @brief Get error severity name as string
 */
const char* zoo_smb_get_error_severity_string(ZOO_SMB_ERROR_SEVERITY severity)
{
    if (severity < sizeof(severity_strings) / sizeof(severity_strings[0]))
    {
        return severity_strings[severity];
    }
    return "Unknown Severity";
}

/**
 * @brief Get error severity for given error code
 */
ZOO_SMB_ERROR_SEVERITY zoo_smb_get_error_severity(ZOO_ERROR_TYPE error)
{
    if (ZOO_SMB_IS_SUCCESS(error))
    {
        return ZOO_SMB_SEVERITY_INFO;
    }

    uint32_t category = ZOO_SMB_GET_CATEGORY(error);
    uint32_t subcategory = ZOO_SMB_GET_SUBCATEGORY(error);

    switch (category)
    {
        case ZOO_SMB_CATEGORY_MEMORY:
            return (subcategory == ZOO_SMB_MEMORY_CORRUPTION) ? ZOO_SMB_SEVERITY_CRITICAL : ZOO_SMB_SEVERITY_ERROR;
        case ZOO_SMB_CATEGORY_SECURITY:
        case ZOO_SMB_CATEGORY_PROTOCOL:
            return ZOO_SMB_SEVERITY_CRITICAL;
        case ZOO_SMB_CATEGORY_TIMEOUT:
        case ZOO_SMB_CATEGORY_RESOURCE:
            return ZOO_SMB_SEVERITY_WARNING;
        case ZOO_SMB_CATEGORY_GENERAL:
            return (subcategory == ZOO_SMB_GENERAL_RESOURCE) ? ZOO_SMB_SEVERITY_WARNING : ZOO_SMB_SEVERITY_ERROR;
        default:
            return ZOO_SMB_SEVERITY_ERROR;
    }
}

/**
 * @brief Set last error for current thread
 */
void zoo_smb_set_last_error(ZOO_ERROR_TYPE error)
{
    last_error = error;
}

/**
 * @brief Get last error for current thread
 */
ZOO_ERROR_TYPE zoo_smb_get_last_error(void)
{
    return last_error;
}

/**
 * @brief Clear last error for current thread
 */
void zoo_smb_clear_last_error(void)
{
    last_error = ZOO_SMB_OK;
}
