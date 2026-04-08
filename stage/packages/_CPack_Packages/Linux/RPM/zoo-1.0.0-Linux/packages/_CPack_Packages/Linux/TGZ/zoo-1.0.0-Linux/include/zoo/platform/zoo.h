/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: ZOO
 * Component ID: ZOO
 * File name: zoo.h
 * Description: Main ZOO platform header - includes all platform abstractions
 *              This is the primary header file that should be included by
 *              applications using the ZOO platform abstraction layer
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-05-14     weiwang.sun     Initial creation
 * 1.1       2025-08-01     assistant       Reorganized into modular headers
 ******************************************************************************/
#ifndef ZOO_H
#define ZOO_H

#if defined(__linux__) || defined(__unix__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "zoo_platform.h"
#include "zoo_types.h"
#include "zoo_error.h"

// ==============================================================================
// FUNCTION PARAMETER DIRECTION INDICATORS
// ==============================================================================

/**
 * @brief Function parameter direction indicators
 * @details These macros are used to indicate parameter direction in function declarations
 */

/** @brief Indicates input parameter in function declarations */
#ifndef IN
#define IN
#endif

/** @brief Indicates output parameter in function declarations */
#ifndef OUT
#define OUT
#endif

/** @brief Indicates input/output parameter in function declarations */
#ifndef INOUT
#define INOUT
#endif

/** @brief Indicates function override in derived classes */
#ifndef OVERRIDE
#define OVERRIDE
#endif

    // ==============================================================================
    // BASIC CONSTANTS
    // ==============================================================================

#define ZOO_OK 0
#define ZOO_TRUE 1
#define ZOO_FALSE 0

// ==============================================================================
// VERSION AND BUILD INFORMATION
// ==============================================================================

/** @brief ZOO library version information */
#define ZOO_VERSION_MAJOR 1
#define ZOO_VERSION_MINOR 2
#define ZOO_VERSION_PATCH 0
#define ZOO_VERSION_STRING "1.2.0"

/** @brief Stringify macro for version building */
#define ZOO_STRINGIFY(x) #x
#define ZOO_TOSTRING(x) ZOO_STRINGIFY(x)

/** @brief Build version string at compile time */
#define ZOO_BUILD_VERSION()         \
    ZOO_TOSTRING(ZOO_VERSION_MAJOR) \
    "." ZOO_TOSTRING(ZOO_VERSION_MINOR) "." ZOO_TOSTRING(ZOO_VERSION_PATCH)

#ifdef __cplusplus
}
#endif

#endif /* ZOO_H */