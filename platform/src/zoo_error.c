/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Platform
 * Component ID: ERROR
 * File name: zoo_error.c
 * Description: Error handling implementation for ZOO
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-05-14     weiwang.sun     Initial creation
 ******************************************************************************/

#include "zoo_error.h"
#include <stdio.h>
#include <string.h>

// ==============================================================================
// ERROR CODE TO STRING MAPPING
// ==============================================================================

/**
 * @brief Error code to string mapping structure
 */
typedef struct
{
    ZOO_ERROR_T code;
    const char *message;
    ZOO_ERROR_SEVERITY_ENUM severity;
    const char *category;
} ZOO_ERROR_INFO_STRUCT;

/**
 * @brief Error information table
 */
static const ZOO_ERROR_INFO_STRUCT error_table[] = {
    // Success
    {ZOO_ERROR_OK, "Success", ZOO_SEVERITY_INFO, "General"},

    // General errors (0 to -99)
    {ZOO_ERROR_GENERAL, "General error", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_INVALID_PARAM, "Invalid parameter", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_NULL_POINTER, "Null pointer", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_NOT_SUPPORTED, "Operation not supported", ZOO_SEVERITY_WARNING, "General"},
    {ZOO_ERROR_TIMEOUT, "Operation timeout", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_NOT_INITIALIZED, "Component not initialized", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_ALREADY_EXISTS, "Resource already exists", ZOO_SEVERITY_WARNING, "General"},
    {ZOO_ERROR_NOT_FOUND, "Resource not found", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_BUFFER_TOO_SMALL, "Buffer too small", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_INVALID_STATE, "Invalid state", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_OPERATION_FAILED, "Operation failed", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_ACCESS_DENIED, "Access denied", ZOO_SEVERITY_ERROR, "General"},
    {ZOO_ERROR_PERMISSION_DENIED, "Permission denied", ZOO_SEVERITY_ERROR, "General"},

    // Memory errors (-100 to -199)
    {ZOO_ERROR_OUT_OF_MEMORY, "Out of memory", ZOO_SEVERITY_CRITICAL, "Memory"},
    {ZOO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed", ZOO_SEVERITY_CRITICAL, "Memory"},
    {ZOO_ERROR_MEMORY_CORRUPTION, "Memory corruption detected", ZOO_SEVERITY_FATAL, "Memory"},
    {ZOO_ERROR_BUFFER_OVERFLOW, "Buffer overflow", ZOO_SEVERITY_CRITICAL, "Memory"},
    {ZOO_ERROR_STACK_OVERFLOW, "Stack overflow", ZOO_SEVERITY_FATAL, "Memory"},

    // File system errors (-200 to -299)
    {ZOO_ERROR_FILE_NOT_FOUND, "File not found", ZOO_SEVERITY_ERROR, "FileSystem"},
    {ZOO_ERROR_FILE_ACCESS_DENIED, "File access denied", ZOO_SEVERITY_ERROR, "FileSystem"},
    {ZOO_ERROR_DIRECTORY_NOT_FOUND, "Directory not found", ZOO_SEVERITY_ERROR, "FileSystem"},
    {ZOO_ERROR_DISK_FULL, "Disk full", ZOO_SEVERITY_CRITICAL, "FileSystem"},

    // Network errors (-300 to -399)
    {ZOO_ERROR_NETWORK_UNREACHABLE, "Network unreachable", ZOO_SEVERITY_ERROR, "Network"},
    {ZOO_ERROR_CONNECTION_TIMEOUT, "Connection timeout", ZOO_SEVERITY_ERROR, "Network"},

    // Threading errors (-400 to -499)
    {ZOO_ERROR_THREAD_CREATE_FAILED, "Thread creation failed", ZOO_SEVERITY_CRITICAL, "Threading"},
    {ZOO_ERROR_MUTEX_LOCK_FAILED, "Mutex lock failed", ZOO_SEVERITY_ERROR, "Threading"},
    {ZOO_ERROR_DEADLOCK_DETECTED, "Deadlock detected", ZOO_SEVERITY_CRITICAL, "Threading"},

    // Hardware errors (-500 to -599)
    {ZOO_ERROR_HARDWARE_FAULT, "Hardware fault", ZOO_SEVERITY_CRITICAL, "Hardware"},
    {ZOO_ERROR_WATCHDOG_TIMEOUT, "Watchdog timeout", ZOO_SEVERITY_CRITICAL, "Hardware"},
    {ZOO_ERROR_POWER_FAILURE, "Power failure", ZOO_SEVERITY_FATAL, "Hardware"},

    // System call errors
    {ZOO_ERROR_SYSTEM_CALL_FAILED, "System call failed", ZOO_SEVERITY_ERROR, "System"},
};

/**
 * @brief Number of entries in error table
 */
static const size_t error_table_size = sizeof(error_table) / sizeof(error_table[0]);

// ==============================================================================
// ERROR HANDLING FUNCTION IMPLEMENTATIONS
// ==============================================================================

/**
 * Converts a ZOO_ERROR_T error code to its corresponding string representation.
 *
 * @param error_code The error code of type ZOO_ERROR_T to be converted.
 * @return A constant pointer to a string describing the error.
 */
const char *zoo_error_to_string(ZOO_ERROR_T error_code)
{
    for (size_t i = 0; i < error_table_size; i++)
    {
        if (error_table[i].code == error_code)
        {
            return error_table[i].message;
        }
    }
    return "Unknown error";
}

/**
 * @brief Retrieves the severity level associated with a given Zoo error code.
 *
 * This function takes a Zoo error code and returns its corresponding severity
 * as a value of the ZOO_ERROR_SEVERITY_ENUM enumeration.
 *
 * @param error_code The error code for which to obtain the severity.
 * @return The severity level of the specified error code.
 */
ZOO_ERROR_SEVERITY_ENUM zoo_error_get_severity(ZOO_ERROR_T error_code)
{
    for (size_t i = 0; i < error_table_size; i++)
    {
        if (error_table[i].code == error_code)
        {
            return error_table[i].severity;
        }
    }
    return ZOO_SEVERITY_ERROR; // Default severity
}

/**
 * @brief Returns a string representing the error category for a given error code.
 *
 * This function maps a ZOO_ERROR_T error code to its corresponding error category
 * as a human-readable string.
 *
 * @param error_code The error code to categorize.
 * @return A constant string describing the error category.
 */
const char *zoo_error_get_category(ZOO_ERROR_T error_code)
{
    for (size_t i = 0; i < error_table_size; i++)
    {
        if (error_table[i].code == error_code)
        {
            return error_table[i].category;
        }
    }
    return "Unknown";
}
