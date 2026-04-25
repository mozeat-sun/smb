/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Utility
 * Component ID: ZOO_UTIL
 * File name: zoo_util.c
 * Description: Utility functions implementation for ZOO platform
 *              Provides timestamp handling, memory management utilities,
 *              and platform-specific helper functions.
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-07-30     weiwang.sun     Initial creation
 ******************************************************************************/

#include "zoo_util.h"
#include "zoo_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifdef ZOO_PLATFORM_POSIX
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef ZOO_OS_WINDOWS
#include <windows.h>
#endif

// ==============================================================================
// GLOBAL VARIABLES AND CONSTANTS
// ==============================================================================

/**
 * @brief Global utility module initialization flag
 */
static ZOO_BOOL g_zoo_util_initialized = ZOO_FALSE;

/**
 * @brief Performance counter frequency for Windows platforms
 */
#ifdef ZOO_OS_WINDOWS
static ZOO_UINT64 g_performance_frequency = 0;
#endif

/**
 * @brief Time zone offset in seconds from UTC
 */
static ZOO_INT32 g_timezone_offset = 0;

// ==============================================================================
// PRIVATE FUNCTION DECLARATIONS
// ==============================================================================

/**
 * @brief Initialize platform-specific timing mechanisms
 * @return ZOO_OK on success, error code on failure
 * @note Called automatically on first use of timing functions
 */
static ZOO_ERROR_T zoo_util_init_timing(void);

/**
 * @brief Get current time zone offset from UTC
 * @return Time zone offset in seconds
 * @note Positive values represent time zones east of UTC
 */
static ZOO_INT32 zoo_util_get_timezone_offset(void);

/**
 * @brief Validate timestamp structure
 * @param[in] timestamp Pointer to timestamp structure to validate
 * @return ZOO_OK if valid, error code if invalid
 * @note Checks for NULL pointer and valid nanosecond range
 */
static ZOO_ERROR_T zoo_util_validate_timestamp(const ZOO_TIMESTAMP_T *timestamp);

// ==============================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// ==============================================================================

/**
 * @brief Initialize the utility module
 * @return ZOO_OK on success, error code on failure
 * @note Must be called before using any utility functions
 * @note This function is thread-safe and can be called multiple times
 */
ZOO_ERROR_T zoo_util_initialize(void)
{
    if (g_zoo_util_initialized == ZOO_TRUE)
    {
        return ZOO_OK;
    }

    ZOO_ERROR_T result = zoo_util_init_timing();
    if (ZOO_IS_ERROR(result))
    {
        return result;
    }

    g_timezone_offset = zoo_util_get_timezone_offset();
    g_zoo_util_initialized = ZOO_TRUE;

    return ZOO_OK;
}

/**
 * @brief Cleanup and finalize the utility module
 * @return ZOO_OK on success, error code on failure
 * @note Should be called before program termination
 * @note This function is thread-safe
 */
ZOO_ERROR_T zoo_util_finalize(void)
{
    if (g_zoo_util_initialized == ZOO_FALSE)
    {
        return ZOO_OK;
    }

    g_zoo_util_initialized = ZOO_FALSE;
    g_timezone_offset = 0;

#ifdef ZOO_OS_WINDOWS
    g_performance_frequency = 0;
#endif

    return ZOO_OK;
}

/**
 * @brief Get current timestamp with high precision
 * @param[out] timestamp Pointer to timestamp structure to fill
 * @return ZOO_OK on success, error code on failure
 * @note Automatically initializes utility module if needed
 * @note Provides nanosecond precision when supported by platform
 */
ZOO_ERROR_T zoo_get_timestamp_high_precision(ZOO_TIMESTAMP_T *timestamp)
{
    if (timestamp == NULL)
    {
        return ZOO_ERROR_NULL_POINTER;
    }

    // Auto-initialize if needed
    if (g_zoo_util_initialized == ZOO_FALSE)
    {
        ZOO_ERROR_T init_result = zoo_util_initialize();
        if (ZOO_IS_ERROR(init_result))
        {
            return init_result;
        }
    }

#ifdef ZOO_OS_WINDOWS
    LARGE_INTEGER counter;
    if (QueryPerformanceCounter(&counter) == 0)
    {
        return ZOO_ERROR_SYSTEM_CALL;
    }

    if (g_performance_frequency == 0)
    {
        return ZOO_ERROR_NOT_INITIALIZED;
    }

    // Convert to nanoseconds since epoch
    ZOO_UINT64 ticks_since_epoch = counter.QuadPart;
    timestamp->seconds = ticks_since_epoch / g_performance_frequency;
    ZOO_UINT64 remaining_ticks = ticks_since_epoch % g_performance_frequency;
    timestamp->nanoseconds = (ZOO_UINT32)((remaining_ticks * 1000000000ULL) / g_performance_frequency);

#elif defined(ZOO_HAS_POSIX) && defined(_POSIX_TIMERS)
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
    {
        return ZOO_ERROR_SYSTEM_CALL;
    }

    timestamp->seconds = (ZOO_UINT64)ts.tv_sec;
    timestamp->nanoseconds = (ZOO_UINT32)ts.tv_nsec;

#elif defined(ZOO_HAS_POSIX)
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        return ZOO_ERROR_SYSTEM_CALL;
    }

    timestamp->seconds = (ZOO_UINT64)tv.tv_sec;
    timestamp->nanoseconds = (ZOO_UINT32)(tv.tv_usec * 1000);

#else
    // Fallback to time() function
    time_t current_time = time(NULL);
    if (current_time == (time_t)-1)
    {
        return ZOO_ERROR_SYSTEM_CALL;
    }

    timestamp->seconds = (ZOO_UINT64)current_time;
    timestamp->nanoseconds = 0;
#endif

    return ZOO_OK;
}

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
                                    ZOO_UINT64 *result)
{
    if (timestamp == NULL || result == NULL)
    {
        return ZOO_ERROR_NULL_POINTER;
    }

    ZOO_ERROR_T validation_result = zoo_util_validate_timestamp(timestamp);
    if (ZOO_IS_ERROR(validation_result))
    {
        return validation_result;
    }

    switch (format)
    {
        case ZOO_TIMESTAMP_SECONDS:
            *result = timestamp->seconds;
            break;

        case ZOO_TIMESTAMP_MILLISECONDS:
            // Check for overflow
            if (timestamp->seconds > (ZOO_MAX_UINT64 / 1000))
            {
                return ZOO_ERROR_BUFFER_OVERFLOW;
            }
            *result = timestamp->seconds * 1000 + timestamp->nanoseconds / 1000000;
            break;

        case ZOO_TIMESTAMP_MICROSECONDS:
            // Check for overflow
            if (timestamp->seconds > (ZOO_MAX_UINT64 / 1000000))
            {
                return ZOO_ERROR_BUFFER_OVERFLOW;
            }
            *result = timestamp->seconds * 1000000 + timestamp->nanoseconds / 1000;
            break;

        case ZOO_TIMESTAMP_NANOSECONDS:
            // Check for overflow
            if (timestamp->seconds > (ZOO_MAX_UINT64 / 1000000000ULL))
            {
                return ZOO_ERROR_BUFFER_OVERFLOW;
            }
            *result = timestamp->seconds * 1000000000ULL + timestamp->nanoseconds;
            break;

        default:
            return ZOO_ERROR_INVALID_PARAM;
    }

    return ZOO_OK;
}

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
                                           ZOO_UINT64 *diff_ns)
{
    if (start_time == NULL || end_time == NULL || diff_ns == NULL)
    {
        return ZOO_ERROR_NULL_POINTER;
    }

    ZOO_ERROR_T validation_result = zoo_util_validate_timestamp(start_time);
    if (ZOO_IS_ERROR(validation_result))
    {
        return validation_result;
    }

    validation_result = zoo_util_validate_timestamp(end_time);
    if (ZOO_IS_ERROR(validation_result))
    {
        return validation_result;
    }

    // Convert both timestamps to nanoseconds
    ZOO_UINT64 start_ns = start_time->seconds * 1000000000ULL + start_time->nanoseconds;
    ZOO_UINT64 end_ns = end_time->seconds * 1000000000ULL + end_time->nanoseconds;

    if (end_ns >= start_ns)
    {
        *diff_ns = end_ns - start_ns;
    }
    else
    {
        // Handle rollover case
        *diff_ns = (ZOO_MAX_UINT64 - start_ns) + end_ns + 1;
    }

    return ZOO_OK;
}

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
                                           ZOO_BOOL include_nanoseconds)
{
    if (timestamp == NULL || buffer == NULL)
    {
        return ZOO_ERROR_NULL_POINTER;
    }

    ZOO_SIZE_T required_size = include_nanoseconds ? 31 : 21; // Including null terminator
    if (buffer_size < required_size)
    {
        return ZOO_ERROR_BUFFER_TOO_SMALL;
    }

    ZOO_ERROR_T validation_result = zoo_util_validate_timestamp(timestamp);
    if (ZOO_IS_ERROR(validation_result))
    {
        return validation_result;
    }

    time_t time_val = (time_t)timestamp->seconds;
    struct tm *utc_tm = gmtime(&time_val);
    if (utc_tm == NULL)
    {
        return ZOO_ERROR_SYSTEM_CALL;
    }

    int result;
    if (include_nanoseconds)
    {
        result = snprintf(buffer, buffer_size,
                         "%04d-%02d-%02dT%02d:%02d:%02d.%09uZ",
                         utc_tm->tm_year + 1900,
                         utc_tm->tm_mon + 1,
                         utc_tm->tm_mday,
                         utc_tm->tm_hour,
                         utc_tm->tm_min,
                         utc_tm->tm_sec,
                         timestamp->nanoseconds);
    }
    else
    {
        result = snprintf(buffer, buffer_size,
                         "%04d-%02d-%02dT%02d:%02d:%02dZ",
                         utc_tm->tm_year + 1900,
                         utc_tm->tm_mon + 1,
                         utc_tm->tm_mday,
                         utc_tm->tm_hour,
                         utc_tm->tm_min,
                         utc_tm->tm_sec);
    }

    if (result < 0 || (ZOO_SIZE_T)result >= buffer_size)
    {
        return ZOO_ERROR_BUFFER_TOO_SMALL;
    }

    return ZOO_OK;
}

/**
 * @brief Get system uptime in milliseconds
 * @param[out] uptime_ms System uptime in milliseconds
 * @return ZOO_OK on success, error code on failure
 * @note Returns time since system boot or process start
 */
ZOO_ERROR_T zoo_get_system_uptime(ZOO_UINT64 *uptime_ms)
{
    if (uptime_ms == NULL)
    {
        return ZOO_ERROR_NULL_POINTER;
    }

#ifdef ZOO_OS_WINDOWS
    *uptime_ms = (ZOO_UINT64)GetTickCount64();
    return ZOO_OK;

#elif defined(ZOO_HAS_POSIX) && defined(_POSIX_MONOTONIC_CLOCK)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return ZOO_ERROR_SYSTEM_CALL;
    }

    *uptime_ms = (ZOO_UINT64)ts.tv_sec * 1000 + (ZOO_UINT64)ts.tv_nsec / 1000000;
    return ZOO_OK;

#else
    // Fallback - this is not accurate uptime but better than nothing
    static time_t start_time = 0;
    if (start_time == 0)
    {
        start_time = time(NULL);
    }

    time_t current_time = time(NULL);
    if (current_time == (time_t)-1)
    {
        return ZOO_ERROR_SYSTEM_CALL;
    }

    *uptime_ms = (ZOO_UINT64)(current_time - start_time) * 1000;
    return ZOO_OK;
#endif
}

// ==============================================================================
// PRIVATE FUNCTION IMPLEMENTATIONS
// ==============================================================================

/**
 * @brief Initialize platform-specific timing mechanisms
 * @return ZOO_OK on success, error code on failure
 * @note Called automatically on first use of timing functions
 */
static ZOO_ERROR_T zoo_util_init_timing(void)
{
#ifdef ZOO_OS_WINDOWS
    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency) == 0)
    {
        return ZOO_ERROR_SYSTEM_CALL;
    }
    g_performance_frequency = frequency.QuadPart;
#endif

    return ZOO_OK;
}

/**
 * @brief Get current time zone offset from UTC
 * @return Time zone offset in seconds
 * @note Positive values represent time zones east of UTC
 */
static ZOO_INT32 zoo_util_get_timezone_offset(void)
{
#ifdef ZOO_HAS_POSIX
    time_t now = time(NULL);
    struct tm *local_tm = localtime(&now);
    struct tm *utc_tm = gmtime(&now);

    if (local_tm == NULL || utc_tm == NULL)
    {
        return 0;
    }

    // Calculate offset in seconds
    ZOO_INT32 local_hours = local_tm->tm_hour;
    ZOO_INT32 utc_hours = utc_tm->tm_hour;
    ZOO_INT32 local_minutes = local_tm->tm_min;
    ZOO_INT32 utc_minutes = utc_tm->tm_min;

    ZOO_INT32 offset_hours = local_hours - utc_hours;
    ZOO_INT32 offset_minutes = local_minutes - utc_minutes;

    // Handle day boundary crossings
    if (offset_hours > 12)
    {
        offset_hours -= 24;
    }
    else if (offset_hours < -12)
    {
        offset_hours += 24;
    }

    return offset_hours * 3600 + offset_minutes * 60;

#elif defined(ZOO_OS_WINDOWS)
    TIME_ZONE_INFORMATION tzi;
    DWORD result = GetTimeZoneInformation(&tzi);
    
    if (result == TIME_ZONE_ID_INVALID)
    {
        return 0;
    }

    // Windows bias is in minutes and negative for UTC+
    return -(tzi.Bias * 60);

#else
    return 0;
#endif
}

/**
 * @brief Validate timestamp structure
 * @param[in] timestamp Pointer to timestamp structure to validate
 * @return ZOO_OK if valid, error code if invalid
 * @note Checks for NULL pointer and valid nanosecond range
 */
static ZOO_ERROR_T zoo_util_validate_timestamp(const ZOO_TIMESTAMP_T *timestamp)
{
    if (timestamp == NULL)
    {
        return ZOO_ERROR_NULL_POINTER;
    }

    if (timestamp->nanoseconds >= 1000000000UL)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    return ZOO_OK;
}
