/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Utilities
 * Component ID: ZOO_TIMESTAMP
 * File name: zoo_timestamp.h
 * Description: Timestamp utilities and time manipulation functions for ZOO
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     AI Assistant    Extracted from zoo_util.h
 ******************************************************************************/
#ifndef ZOO_TIMESTAMP_H
#define ZOO_TIMESTAMP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo.h"
#include "zoo_error.h"
#include <time.h>

#ifdef ZOO_PLATFORM_X86_64
#include <sys/time.h>
#elif defined(ZOO_PLATFORM_ARM_CORTEX_A64) || defined(ZOO_PLATFORM_ARM_CORTEX_A32)
#include <sys/time.h>
#elif defined(ZOO_PLATFORM_ARM_CORTEX_M)
// For ARM Cortex-M, we might need to include platform-specific timer headers
// This depends on the specific RTOS or bare-metal implementation
#endif

// ==============================================================================
// TIMESTAMP DEFINITIONS
// ==============================================================================

/**
 * @brief Time conversion constants
 */
#define ZOO_SECONDS_TO_MILLISECONDS 1000ULL
#define ZOO_SECONDS_TO_MICROSECONDS 1000000ULL
#define ZOO_SECONDS_TO_NANOSECONDS 1000000000ULL
#define ZOO_MILLISECONDS_TO_MICROSECONDS 1000ULL
#define ZOO_MILLISECONDS_TO_NANOSECONDS 1000000ULL
#define ZOO_MICROSECONDS_TO_NANOSECONDS 1000ULL

/**
 * @brief Common time intervals in different units
 */
#define ZOO_ONE_SECOND_MS 1000ULL
#define ZOO_ONE_MINUTE_MS 60000ULL
#define ZOO_ONE_HOUR_MS 3600000ULL
#define ZOO_ONE_DAY_MS 86400000ULL

#define ZOO_ONE_SECOND_US 1000000ULL
#define ZOO_ONE_MINUTE_US 60000000ULL
#define ZOO_ONE_HOUR_US 3600000000ULL

#define ZOO_ONE_SECOND_NS 1000000000ULL
#define ZOO_ONE_MINUTE_NS 60000000000ULL

    /**
     * @brief Timestamp format types
     */
    typedef enum
    {
        ZOO_TIMESTAMP_SECONDS = 0,  /**< Timestamp in seconds since epoch */
        ZOO_TIMESTAMP_MILLISECONDS, /**< Timestamp in milliseconds since epoch */
        ZOO_TIMESTAMP_MICROSECONDS, /**< Timestamp in microseconds since epoch */
        ZOO_TIMESTAMP_NANOSECONDS   /**< Timestamp in nanoseconds since epoch */
    } ZOO_TIMESTAMP_FORMAT_T;

    /**
     * @brief Timestamp structure for high precision timing
     */
    typedef struct
    {
        ZOO_UINT64 seconds;     /**< Seconds since epoch */
        ZOO_UINT32 nanoseconds; /**< Nanoseconds part (0-999999999) */
    } ZOO_TIMESTAMP_T;

    // ==============================================================================
    // TIMESTAMP INLINE FUNCTIONS
    // ==============================================================================

    /**
     * @brief Get current timestamp in seconds since Unix epoch
     * @return Current timestamp in seconds
     * @note This function is optimized for performance with inline implementation
     */
    static ZOO_FORCE_INLINE ZOO_UINT64 zoo_get_timestamp_seconds(void)
    {
#if defined(ZOO_PLATFORM_X86_64) || defined(ZOO_PLATFORM_ARM_CORTEX_A64) || defined(ZOO_PLATFORM_ARM_CORTEX_A32)
        return (ZOO_UINT64)time(NULL);
#elif defined(ZOO_PLATFORM_ARM_CORTEX_M)
    // For ARM Cortex-M, this might need to be implemented differently
    // depending on the RTOS or timer implementation
    return 0; // Placeholder - implement based on your platform
#else
    return (ZOO_UINT64)time(NULL);
#endif
    }

    /**
     * @brief Get current timestamp in milliseconds since Unix epoch
     * @return Current timestamp in milliseconds
     * @note Uses high-resolution timer when available
     */
    static ZOO_FORCE_INLINE ZOO_UINT64 zoo_get_timestamp_milliseconds(void)
    {
#if defined(ZOO_PLATFORM_X86_64) || defined(ZOO_PLATFORM_ARM_CORTEX_A64) || defined(ZOO_PLATFORM_ARM_CORTEX_A32)
        struct timeval tv;
        if (gettimeofday(&tv, NULL) == 0)
        {
            return (ZOO_UINT64)(tv.tv_sec) * 1000 + (ZOO_UINT64)(tv.tv_usec) / 1000;
        }
        return zoo_get_timestamp_seconds() * 1000;
#elif defined(ZOO_PLATFORM_ARM_CORTEX_M)
    // For ARM Cortex-M, implement based on your timer/RTOS
    return zoo_get_timestamp_seconds() * 1000;
#else
    return zoo_get_timestamp_seconds() * 1000;
#endif
    }

    /**
     * @brief Get current timestamp in microseconds since Unix epoch
     * @return Current timestamp in microseconds
     * @note Provides microsecond precision when supported by platform
     */
    static ZOO_FORCE_INLINE ZOO_UINT64 zoo_get_timestamp_microseconds(void)
    {
#if defined(ZOO_PLATFORM_X86_64) || defined(ZOO_PLATFORM_ARM_CORTEX_A64) || defined(ZOO_PLATFORM_ARM_CORTEX_A32)
        struct timeval tv;
        if (gettimeofday(&tv, NULL) == 0)
        {
            return (ZOO_UINT64)(tv.tv_sec) * 1000000 + (ZOO_UINT64)(tv.tv_usec);
        }
        return zoo_get_timestamp_seconds() * 1000000;
#elif defined(ZOO_PLATFORM_ARM_CORTEX_M)
    // For ARM Cortex-M, implement based on your timer/RTOS
    return zoo_get_timestamp_milliseconds() * 1000;
#else
    return zoo_get_timestamp_seconds() * 1000000;
#endif
    }

    /**
     * @brief Get current timestamp in nanoseconds since Unix epoch
     * @return Current timestamp in nanoseconds
     * @note Uses clock_gettime for nanosecond precision when available
     */
    static ZOO_FORCE_INLINE ZOO_UINT64 zoo_get_timestamp_nanoseconds(void)
    {
#if defined(ZOO_PLATFORM_X86_64) || defined(ZOO_PLATFORM_ARM_CORTEX_A64) || defined(ZOO_PLATFORM_ARM_CORTEX_A32)
#ifdef _POSIX_C_SOURCE
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
        {
            return (ZOO_UINT64)(ts.tv_sec) * 1000000000ULL + (ZOO_UINT64)(ts.tv_nsec);
        }
#endif
        return zoo_get_timestamp_microseconds() * 1000;
#elif defined(ZOO_PLATFORM_ARM_CORTEX_M)
    // For ARM Cortex-M, implement based on your timer/RTOS
    return zoo_get_timestamp_microseconds() * 1000;
#else
    return zoo_get_timestamp_microseconds() * 1000;
#endif
    }

    /**
     * @brief Get high-precision timestamp structure
     * @param timestamp Pointer to timestamp structure to fill
     * @return ZOO_OK on success, error code on failure
     * @note Provides the highest precision available on the platform
     */
    static ZOO_FORCE_INLINE int zoo_get_timestamp_precise(ZOO_TIMESTAMP_T *timestamp)
    {
        if (timestamp == NULL)
        {
            return -1;
        }

#if defined(ZOO_PLATFORM_X86_64) || defined(ZOO_PLATFORM_ARM_CORTEX_A64) || defined(ZOO_PLATFORM_ARM_CORTEX_A32)
#ifdef _POSIX_C_SOURCE
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
        {
            timestamp->seconds = (ZOO_UINT64)ts.tv_sec;
            timestamp->nanoseconds = (ZOO_UINT32)ts.tv_nsec;
            return ZOO_OK;
        }
#endif

        // Fallback to gettimeofday
        struct timeval tv;
        if (gettimeofday(&tv, NULL) == 0)
        {
            timestamp->seconds = (ZOO_UINT64)tv.tv_sec;
            timestamp->nanoseconds = (ZOO_UINT32)(tv.tv_usec * 1000);
            return ZOO_OK;
        }
#endif

        // Final fallback
        timestamp->seconds = zoo_get_timestamp_seconds();
        timestamp->nanoseconds = 0;
        return ZOO_OK;
    }

    /**
     * @brief Get timestamp with specified format
     * @param format Desired timestamp format
     * @return Timestamp in the specified format
     * @note Convenient wrapper for different timestamp precision needs
     */
    static ZOO_FORCE_INLINE ZOO_UINT64 zoo_get_timestamp(ZOO_TIMESTAMP_FORMAT_T format)
    {
        switch (format)
        {
        case ZOO_TIMESTAMP_SECONDS:
            return zoo_get_timestamp_seconds();
        case ZOO_TIMESTAMP_MILLISECONDS:
            return zoo_get_timestamp_milliseconds();
        case ZOO_TIMESTAMP_MICROSECONDS:
            return zoo_get_timestamp_microseconds();
        case ZOO_TIMESTAMP_NANOSECONDS:
            return zoo_get_timestamp_nanoseconds();
        default:
            return zoo_get_timestamp_seconds();
        }
    }

    /**
     * @brief Calculate elapsed time between two timestamps
     * @param start_timestamp Start timestamp in nanoseconds
     * @param end_timestamp End timestamp in nanoseconds
     * @return Elapsed time in nanoseconds
     * @note Handles timestamp overflow correctly
     */
    static ZOO_FORCE_INLINE ZOO_UINT64 zoo_calculate_elapsed_time(ZOO_UINT64 start_timestamp, ZOO_UINT64 end_timestamp)
    {
        if (end_timestamp >= start_timestamp)
        {
            return end_timestamp - start_timestamp;
        }
        // Handle overflow case
        return (ZOO_MAX_UINT64 - start_timestamp) + end_timestamp + 1;
    }

    /**
     * @brief Format timestamp to human-readable string
     * @param timestamp Timestamp in seconds since epoch
     * @param buffer Buffer to store formatted string
     * @param buffer_size Size of the buffer
     * @return ZOO_OK on success, error code on failure
     * @note Uses ISO 8601 format: YYYY-MM-DD HH:MM:SS
     */
    static ZOO_FORCE_INLINE int zoo_format_timestamp(ZOO_UINT64 timestamp, char *buffer, ZOO_SIZE_T buffer_size)
    {
        if (buffer == NULL || buffer_size < 20)
        {
            return -1;
        }

        time_t time_val = (time_t)timestamp;
        struct tm *tm_info = localtime(&time_val);

        if (tm_info == NULL)
        {
            return -1;
        }

        int result = snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d",
                              tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                              tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

        return (result > 0 && (ZOO_SIZE_T)result < buffer_size) ? ZOO_OK : -1;
    }

/**
 * @brief Check if timeout exceeded using timestamp comparison
 * @param start_time Start timestamp
 * @param timeout_ms Timeout value in milliseconds
 * @return 1 if timeout exceeded, 0 otherwise
 */
#define ZOO_IS_TIMEOUT(start_time, timeout_ms) \
    (ZOO_TIME_ELAPSED_MS(start_time) > (timeout_ms))

    // ==============================================================================
    // ADVANCED TIMESTAMP FUNCTIONS (Implementation in .c file)
    // ==============================================================================

    /**
     * @brief Get current timestamp with high precision
     * @param[out] timestamp Pointer to timestamp structure to fill
     * @return ZOO_OK on success, error code on failure
     * @note Automatically initializes utility module if needed
     * @note Provides nanosecond precision when supported by platform
     */
    ZOO_ERROR_T zoo_get_timestamp_high_precision(ZOO_TIMESTAMP_T *timestamp);

    /**
     * @brief Convert timestamp to specified format
     * @param[in] timestamp Source timestamp structure
     * @param[in] format Desired output format
     * @param[out] result Converted timestamp value
     * @return ZOO_OK on success, error code on failure
     * @note Handles overflow protection for different formats
     */
    ZOO_ERROR_T zoo_convert_timestamp(const ZOO_TIMESTAMP_T *timestamp,
                                      ZOO_TIMESTAMP_FORMAT_T format,
                                      ZOO_UINT64 *result);

    /**
     * @brief Calculate time difference between two timestamps
     * @param[in] start_time Start timestamp
     * @param[in] end_time End timestamp
     * @param[out] diff_ns Time difference in nanoseconds
     * @return ZOO_OK on success, error code on failure
     * @note Handles timestamp rollover correctly
     */
    ZOO_ERROR_T zoo_calculate_time_difference(const ZOO_TIMESTAMP_T *start_time,
                                              const ZOO_TIMESTAMP_T *end_time,
                                              ZOO_UINT64 *diff_ns);

    /**
     * @brief Format timestamp to ISO 8601 string representation
     * @param[in] timestamp Source timestamp
     * @param[out] buffer Output buffer for formatted string
     * @param[in] buffer_size Size of output buffer
     * @param[in] include_nanoseconds Whether to include nanosecond precision
     * @return ZOO_OK on success, error code on failure
     * @note Format: YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ (with nanoseconds)
     * @note Format: YYYY-MM-DDTHH:MM:SSZ (without nanoseconds)
     */
    ZOO_ERROR_T zoo_format_timestamp_iso8601(const ZOO_TIMESTAMP_T *timestamp,
                                             char *buffer,
                                             ZOO_SIZE_T buffer_size,
                                             ZOO_BOOL include_nanoseconds);

    /**
     * @brief Get system uptime in milliseconds
     * @param[out] uptime_ms System uptime in milliseconds
     * @return ZOO_OK on success, error code on failure
     * @note Returns time since system boot or process start
     */
    ZOO_ERROR_T zoo_get_system_uptime(ZOO_UINT64 *uptime_ms);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_TIMESTAMP_H */
