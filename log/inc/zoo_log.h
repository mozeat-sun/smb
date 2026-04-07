/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: LOG
 * Component id: zoo_log
 * File name: zoo_log.h
 * Description: Logging module for the ZOO library
 *              Provides configurable logging capabilities with different
 *              levels, targets, and formatting options.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_LOG_H
#define ZOO_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "../../platform/inc/zoo_types.h"
#include "zoo.h"
#include <stdarg.h>

    // ==============================================================================
    // TYPE DEFINITIONS
    // ==============================================================================

    /**
     * @brief Log level enumeration
     *
     * Defines the various logging levels available in the system.
     * Lower numeric values indicate more verbose logging.
     */
    typedef enum
    {
        ZOO_LOG_LEVEL_TRACE = 0, /**< Trace level - most verbose debugging information */
        ZOO_LOG_LEVEL_DEBUG = 1, /**< Debug level - detailed debugging information */
        ZOO_LOG_LEVEL_INFO = 2,  /**< Info level - general information messages */
        ZOO_LOG_LEVEL_WARN = 3,  /**< Warning level - potentially harmful situations */
        ZOO_LOG_LEVEL_ERROR = 4, /**< Error level - error events but application continues */
        ZOO_LOG_LEVEL_FATAL = 5, /**< Fatal level - severe error events leading to abort */
        ZOO_LOG_LEVEL_OFF = 6    /**< Off level - turn off logging completely */
    } ZOO_LOG_LEVEL_ENUM;

    /**
     * @brief Log output target enumeration
     *
     * Defines where log messages should be output.
     */
    typedef enum
    {
        ZOO_LOG_TARGET_CONSOLE = 0, /**< Output to console/stdout */
        ZOO_LOG_TARGET_SYSLOG = 1,   /**< Output to system log (syslog) */
        ZOO_LOG_TARGET_BOTH = 2,    /**< Output to both console and file */
    } ZOO_LOG_TARGET_ENUM;

    /**
     * @brief Log configuration structure
     *
     * Contains all configuration parameters for the logging system.
     * This structure defines how the logging system should behave,
     * including output targets, formatting options, and file management.
     */
    typedef struct
    {
        ZOO_LOG_LEVEL_ENUM level;   /**< Minimum log level to output */
        ZOO_LOG_TARGET_ENUM target; /**< Where to output log messages */
        ZOO_STRING file_path;     /**< Path to log file (if file output enabled) */
        ZOO_USIZE max_file_size;  /**< Maximum log file size in bytes */
        ZOO_USIZE max_backup_files; /**< Maximum number of backup files to keep */
        ZOO_BOOL append;          /**< Whether to append to existing file */
        ZOO_BOOL timestamp;       /**< Whether to include timestamp in log messages */
        ZOO_BOOL thread_id;       /**< Whether to include thread ID in log messages */
        ZOO_BOOL file;            /**< Whether to include filename in log messages */
        ZOO_BOOL function_name;   /**< Whether to include function name in log messages */
        ZOO_BOOL color;           /**< Whether to use color output (console only) */
    } ZOO_LOG_CONFIG_STRUCT;

// ==============================================================================
// DEFAULT CONFIGURATION
// ==============================================================================

/**
 * @brief Default configuration initializer for ZOO_LOG_CONFIG_STRUCT
 *
 * This macro provides sensible default values for log configuration.
 * Usage: ZOO_LOG_CONFIG_STRUCT config = ZOO_LOG_CONFIG_STRUCT_DEFAULT;
 */
#define ZOO_LOG_CONFIG_STRUCT_DEFAULT {               \
    ZOO_LOG_LEVEL_INFO,     /* level */               \
    ZOO_LOG_TARGET_CONSOLE, /* target */              \
    "zoo_smb.log",              /* file_path */           \
    1048576,                    /* max_file_size (1MB) */ \
    5,                          /* max_backup_files */    \
    ZOO_TRUE,                   /* append */              \
    ZOO_TRUE,                   /* timestamp */           \
    ZOO_TRUE,                   /* thread_id */           \
    ZOO_TRUE,                   /* file */                \
    ZOO_TRUE,                   /* function_name */       \
    ZOO_TRUE                    /* color */               \
}

    // ==============================================================================
    // CORE LOGGING FUNCTIONS
    // ==============================================================================

    /**
     * @brief Initialize the logging system
     *
     * This function must be called before any other logging functions.
     * It sets up the logging system according to the provided configuration.
     *
     * @param config Pointer to log configuration structure
     * @return ZOO_OK on success, error code on failure
     *
     * @note If config is NULL, default configuration will be used
     * @note This function is not thread-safe and should be called during initialization
     */
    ZOO_INT32 zoo_log_init(const ZOO_LOG_CONFIG_STRUCT *config);

    /**
     * @brief Shutdown the logging system
     *
     * Closes any open log files and releases resources used by the logging system.
     * After calling this function, no further logging operations should be performed
     * until zoo_log_init() is called again.
     *
     * @note This function is not thread-safe and should be called during shutdown
     */
    void zoo_log_shutdown(void);

    // ==============================================================================
    // CONFIGURATION FUNCTIONS
    // ==============================================================================

    /**
     * @brief Set the minimum log level
     *
     * Only messages at or above this level will be output.
     * This can be used to dynamically change the verbosity of logging.
     *
     * @param level New minimum log level
     */
    void zoo_log_set_level(ZOO_LOG_LEVEL_ENUM level);

    /**
     * @brief Set the log output target
     *
     * Changes where log messages are sent (console, file, or both).
     *
     * @param target New log output target
     */
    void zoo_log_set_target(ZOO_LOG_TARGET_ENUM target);

    /**
     * @brief Get the current log configuration
     *
     * Returns a pointer to the current log configuration structure.
     * The returned pointer should not be modified or freed.
     *
     * @return Pointer to current log configuration (read-only)
     */
    const ZOO_LOG_CONFIG_STRUCT *zoo_log_get_config(void);

    /**
     * @brief Set the log configuration
     *
     * Updates the current log configuration with the provided values.
     * A copy of the configuration is made internally.
     *
     * @param config Pointer to new configuration, or NULL to reset to default
     */
    void zoo_log_set_config(const ZOO_LOG_CONFIG_STRUCT *config);

    // ==============================================================================
    // LOGGING FUNCTIONS
    // ==============================================================================

    /**
     * @brief Log a trace level message
     *
     * Trace messages are typically used for very detailed debugging information
     * that is only useful when investigating specific issues.
     *
     * @param file Source file name (typically __FILE__)
     * @param line_num Line number in source file (typically __LINE__)
     * @param func_name Function name (typically __func__)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    void zoo_log_trace(ZOO_STRING file, ZOO_S32 line_num, ZOO_STRING func_name, ZOO_STRING format, ...);

    /**
     * @brief Log a debug level message
     *
     * Debug messages provide detailed information about program execution
     * that is useful for troubleshooting and development.
     *
     * @param file Source file name (typically __FILE__)
     * @param line_num Line number in source file (typically __LINE__)
     * @param func_name Function name (typically __func__)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    void zoo_log_debug(ZOO_STRING file, ZOO_S32 line_num, ZOO_STRING func_name, ZOO_STRING format, ...);

    /**
     * @brief Log an info level message
     *
     * Info messages provide general information about program operation
     * that is useful for understanding normal program flow.
     *
     * @param file Source file name (typically __FILE__)
     * @param line_num Line number in source file (typically __LINE__)
     * @param func_name Function name (typically __func__)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    void zoo_log_info(ZOO_STRING file, ZOO_S32 line_num, ZOO_STRING func_name, ZOO_STRING format, ...);

    /**
     * @brief Log a warning level message
     *
     * Warning messages indicate potentially problematic situations
     * that don't prevent the program from continuing.
     *
     * @param file Source file name (typically __FILE__)
     * @param line_num Line number in source file (typically __LINE__)
     * @param func_name Function name (typically __func__)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    void zoo_log_warn(ZOO_STRING_T file, ZOO_S32_T line_num, ZOO_STRING_T func_name, ZOO_STRING_T format, ...);

    /**
     * @brief Log an error level message
     *
     * Error messages indicate error conditions that affect program operation
     * but allow the program to continue running.
     *
     * @param file Source file name (typically __FILE__)
     * @param line_num Line number in source file (typically __LINE__)
     * @param func_name Function name (typically __func__)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    void zoo_log_error(ZOO_STRING_T file, ZOO_S32_T line_num, ZOO_STRING_T func_name, ZOO_STRING_T format, ...);

    /**
     * @brief Log a fatal level message
     *
     * Fatal messages indicate severe error conditions that may cause
     * the program to terminate or become unstable.
     *
     * @param file Source file name (typically __FILE__)
     * @param line_num Line number in source file (typically __LINE__)
     * @param func_name Function name (typically __func__)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    void zoo_log_fatal(ZOO_STRING_T file, ZOO_S32_T line_num, ZOO_STRING_T func_name, ZOO_STRING_T format, ...);

    // ==============================================================================
    // ERROR CODE LOGGING FUNCTIONS
    // ==============================================================================

    /**
     * @brief Log an error message with error code
     *
     * Logs an error message along with a ZOO_INT32 code.
     * The error code will be translated to a human-readable string.
     *
     * @param error Error code to include in the message
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    void zoo_log_error_code(ZOO_INT32 error, ZOO_STRING format, ...);

    /**
     * @brief Log a fatal message with error code
     *
     * Logs a fatal message along with a ZOO_INT32 code.
     * The error code will be translated to a human-readable string.
     *
     * @param error Error code to include in the message
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    void zoo_log_fatal_code(ZOO_INT32 error, ZOO_STRING format, ...);

// ==============================================================================
// CONVENIENCE MACROS
// ==============================================================================

/**
 * @brief Convenience macro for trace logging
 *
 * Automatically fills in file, line, and function information.
 * Usage: ZOO_LOG_TRACE("Debug info: %d", value);
 */
#define ZOO_LOG_TRACE(fmt, ...) \
    zoo_log_trace(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/**
 * @brief Convenience macro for debug logging
 *
 * Automatically fills in file, line, and function information.
 * Usage: ZOO_LOG_DEBUG("Debug info: %d", value);
 */
#define ZOO_LOG_DEBUG(fmt, ...) \
    zoo_log_debug(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/**
 * @brief Convenience macro for info logging
 *
 * Automatically fills in file, line, and function information.
 * Usage: ZOO_LOG_INFO("Operation completed: %s", operation_name);
 */
#define ZOO_LOG_INFO(fmt, ...) \
    zoo_log_info(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/**
 * @brief Convenience macro for warning logging
 *
 * Automatically fills in file, line, and function information.
 * Usage: ZOO_LOG_WARN("Potential issue detected: %s", issue_desc);
 */
#define ZOO_LOG_WARN(fmt, ...) \
    zoo_log_warn(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/**
 * @brief Convenience macro for error logging
 *
 * Automatically fills in file, line, and function information.
 * Usage: ZOO_LOG_ERROR("Failed to process: %s", error_desc);
 */
#define ZOO_LOG_ERROR(fmt, ...) \
    zoo_log_error(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/**
 * @brief Convenience macro for fatal logging
 *
 * Automatically fills in file, line, and function information.
 * Usage: ZOO_LOG_FATAL("Critical error: %s", error_desc);
 */
#define ZOO_LOG_FATAL(fmt, ...) \
    zoo_log_fatal(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// ==============================================================================
// CONDITIONAL LOGGING MACROS
// ==============================================================================

/**
 * @brief Conditional logging macros (only log if condition is true)
 */
#define ZOO_LOG_TRACE_IF(condition, fmt, ...)  \
    do                                             \
    {                                              \
        if (condition)                             \
            ZOO_LOG_TRACE(fmt, ##__VA_ARGS__); \
    } while (0)

#define ZOO_LOG_DEBUG_IF(condition, fmt, ...)  \
    do                                             \
    {                                              \
        if (condition)                             \
            ZOO_LOG_DEBUG(fmt, ##__VA_ARGS__); \
    } while (0)

#define ZOO_LOG_INFO_IF(condition, fmt, ...)  \
    do                                            \
    {                                             \
        if (condition)                            \
            ZOO_LOG_INFO(fmt, ##__VA_ARGS__); \
    } while (0)

#define ZOO_LOG_WARN_IF(condition, fmt, ...)  \
    do                                            \
    {                                             \
        if (condition)                            \
            ZOO_LOG_WARN(fmt, ##__VA_ARGS__); \
    } while (0)

#define ZOO_LOG_ERROR_IF(condition, fmt, ...)  \
    do                                             \
    {                                              \
        if (condition)                             \
            ZOO_LOG_ERROR(fmt, ##__VA_ARGS__); \
    } while (0)

#define ZOO_LOG_FATAL_IF(condition, fmt, ...)  \
    do                                             \
    {                                              \
        if (condition)                             \
            ZOO_LOG_FATAL(fmt, ##__VA_ARGS__); \
    } while (0)

// ==============================================================================
// UTILITY MACROS
// ==============================================================================

/**
 * @brief Log function entry
 *
 * Useful for tracing function calls during debugging.
 */
#define ZOO_LOG_FUNCTION_ENTRY() \
    ZOO_LOG_TRACE("Entering function: %s", __func__)

/**
 * @brief Log function exit
 *
 * Useful for tracing function calls during debugging.
 */
#define ZOO_LOG_FUNCTION_EXIT() \
    ZOO_LOG_TRACE("Exiting function: %s", __func__)

/**
 * @brief Log function exit with return value
 *
 * Useful for tracing function calls and return values during debugging.
 */
#define ZOO_LOG_FUNCTION_EXIT_WITH_RESULT(result) \
    ZOO_LOG_TRACE("Exiting function: %s, result: %d", __func__, (int)(result))

#ifdef __cplusplus
}
#endif

#endif