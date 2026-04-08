/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Utilities
 * Component ID: ZOO_UTIL
 * File name: zoo_util.h
 * Description: Main utility header that includes all ZOO utility modules
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-05-14     weiwang.sun     Initial creation
 * 2.0       2025-08-01     AI Assistant    Modularized into separate headers
 ******************************************************************************/
#ifndef ZOO_UTIL_H
#define ZOO_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

// ==============================================================================
// CORE ZOO INCLUDES
// ==============================================================================
#include "zoo.h"
#include "zoo_error.h"

// ==============================================================================
// MODULAR UTILITY INCLUDES
// ==============================================================================
#include "zoo_timestamp.h" // Timestamp and time management utilities
#include "zoo_string.h"    // String manipulation and safe operations
#include "zoo_memory.h"    // Memory management and safety functions
#include "zoo_math.h"      // Mathematical utilities and floating point ops

// ==============================================================================
// STANDARD LIBRARY INCLUDES
// ==============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

    // ==============================================================================
    // UTILITY MODULE MANAGEMENT FUNCTIONS
    // ==============================================================================

    /**
     * @brief Initialize the utility module
     * @return ZOO_OK on success, error code on failure
     * @note Must be called before using any utility functions
     * @note This function is thread-safe and can be called multiple times
     */
    ZOO_ERROR_T zoo_util_initialize(void);

    /**
     * @brief Cleanup and finalize the utility module
     * @return ZOO_OK on success, error code on failure
     * @note Should be called before program termination
     * @note This function is thread-safe
     */
    ZOO_ERROR_T zoo_util_finalize(void);

    // ==============================================================================
    // ADVANCED TIMESTAMP FUNCTIONS
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
                                             ZOO_SIZE buffer_size,
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

#endif /* ZOO_UTIL_H */
