/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: LOG
 * Component id: LOG
 * File name: zoo_log.c
 * Description: Logging module implementation for the ZOO library
 *              Provides configurable logging capabilities with different
 *              levels, targets, and formatting options.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun       created
 * 1.1       2025-07-29     GitHub Copilot    implementation completed
 ******************************************************************************/

#include "zoo_log.h"
#include "zoo.h"
#include "zoo_error.h"
#include "zoo_types.h"

// Standard C library headers (available on all platforms)
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#if ZOO_HAS_POSIX
#include <unistd.h>
#include <sys/stat.h>
#endif

#if defined(ZOO_OS_WINDOWS)
#include <io.h>
#endif

// ==============================================================================
// INTERNAL CONSTANTS AND MACROS
// ==============================================================================

/** Maximum length for a single log message */
#define MAX_LOG_MESSAGE_SIZE 4096

/** Maximum length for timestamp string */
#define MAX_TIMESTAMP_SIZE 64

/** Maximum length for thread ID string */
#define MAX_THREAD_ID_SIZE 32

/** Buffer size for log entry formatting */
#define LOG_ENTRY_BUFFER_SIZE (MAX_LOG_MESSAGE_SIZE + MAX_TIMESTAMP_SIZE + MAX_THREAD_ID_SIZE + 256)

/** Color codes for console output */
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_BOLD "\033[1m"

// ==============================================================================
// INTERNAL TYPE DEFINITIONS
// ==============================================================================

/**
 * @brief Internal log context structure
 *
 * This structure maintains the internal state of the logging system,
 * including configuration, file handles, and synchronization objects.
 */
typedef struct
{
    ZOO_LOG_CONFIG_STRUCT config; /**< Current log configuration */
#if ZOO_HAS_FILESYSTEM
    FILE *log_file;               /**< File handle for log file output */
#endif
    ZOO_MUTEX_T mutex;            /**< Cross-platform mutex */
    ZOO_BOOL initialized;       /**< Whether the log system is initialized */
#if ZOO_HAS_FILESYSTEM
    ZOO_USIZE_T current_file_size; /**< Current size of the log file */
#endif
} zoo_log_context_t;

// ==============================================================================
// INTERNAL GLOBAL VARIABLES
// ==============================================================================

/** Global log context - single instance for the entire system */
static zoo_log_context_t g_log_context = {
    .config = ZOO_LOG_CONFIG_STRUCT_DEFAULT,
#if ZOO_HAS_FILESYSTEM
    .log_file = NULL,
#endif
    .initialized = false,
#if ZOO_HAS_FILESYSTEM
    .current_file_size = 0
#endif
};

// ==============================================================================
// INTERNAL HELPER FUNCTIONS
// ==============================================================================

/**
 * @brief Get color code for log level
 *
 * Returns the appropriate ANSI color code for the given log level.
 * Used for colorized console output.
 *
 * @param level Log level to get color for
 * @return String containing ANSI color code
 */
static ZOO_STRING get_level_color(ZOO_LOG_LEVEL_ENUM level)
{
    switch (level)
    {
    case ZOO_LOG_LEVEL_TRACE:
        return COLOR_CYAN;
    case ZOO_LOG_LEVEL_DEBUG:
        return COLOR_BLUE;
    case ZOO_LOG_LEVEL_INFO:
        return COLOR_GREEN;
    case ZOO_LOG_LEVEL_WARN:
        return COLOR_YELLOW;
    case ZOO_LOG_LEVEL_ERROR:
        return COLOR_RED;
    case ZOO_LOG_LEVEL_FATAL:
        return COLOR_BOLD COLOR_RED;
    default:
        return COLOR_RESET;
    }
}

/**
 * @brief Get string representation of log level
 *
 * Converts log level enumeration to human-readable string.
 *
 * @param level Log level to convert
 * @return String representation of log level
 */
static ZOO_STRING get_level_string(ZOO_LOG_LEVEL_ENUM level)
{
    switch (level)
    {
    case ZOO_LOG_LEVEL_TRACE:
        return "TRACE";
    case ZOO_LOG_LEVEL_DEBUG:
        return "DEBUG";
    case ZOO_LOG_LEVEL_INFO:
        return "INFO ";
    case ZOO_LOG_LEVEL_WARN:
        return "WARN ";
    case ZOO_LOG_LEVEL_ERROR:
        return "ERROR";
    case ZOO_LOG_LEVEL_FATAL:
        return "FATAL";
    default:
        return "UNKN ";
    }
}

/**
 * @brief Get current timestamp as formatted string
 *
 * Formats the current system time into a string suitable for log entries.
 *
 * @param buffer Buffer to store formatted timestamp
 * @param buffer_size Size of the buffer
 * @return Number of characters written to buffer
 */
static ZOO_S32_T get_timestamp_string(ZOO_PCHAR_T buffer, ZOO_USIZE_T buffer_size)
{
    return zoo_get_timestamp_string(buffer, buffer_size);
}

/**
 * @brief Get current thread ID as string
 *
 * Retrieves the current thread identifier and formats it as a string.
 *
 * @param buffer Buffer to store thread ID string
 * @param buffer_size Size of the buffer
 * @return Number of characters written to buffer
 */
static ZOO_S32_T get_thread_id_string(ZOO_PCHAR_T buffer, ZOO_USIZE_T buffer_size)
{
    return zoo_get_thread_id_string(buffer, buffer_size);
}

/**
 * @brief Extract filename from full path
 *
 * Returns just the filename portion of a full file path.
 *
 * @param filepath Full file path
 * @return Pointer to filename portion
 */
static ZOO_STRING extract_filename(ZOO_STRING filepath)
{
    if (!filepath)
    {
        return "unknown";
    }

    ZOO_STRING filename = strrchr(filepath, '/');
    if (filename)
    {
        return filename + 1;
    }

    filename = strrchr(filepath, '\\');
    if (filename)
    {
        return filename + 1;
    }

    return filepath;
}

/**
 * @brief Rotate log files when size limit is reached
 *
 * When the current log file exceeds the maximum size, this function
 * creates backup files and starts a new log file.
 *
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_INT32 rotate_log_file(void)
{
    if (!g_log_context.config.file_path || g_log_context.config.max_backup_files == 0)
    {
        return ZOO_OK;
    }

    // Close current log file
    if (g_log_context.log_file)
    {
        fclose(g_log_context.log_file);
        g_log_context.log_file = NULL;
    }

    // Rotate backup files
    char old_name[512];
    char new_name[512];

    // Remove the oldest backup file
    snprintf(old_name, sizeof(old_name), "%s.%zu",
             g_log_context.config.file_path,
             g_log_context.config.max_backup_files);
    unlink(old_name);

    // Rename existing backup files
    for (ZOO_USIZE i = g_log_context.config.max_backup_files; i > 1; i--)
    {
        snprintf(old_name, sizeof(old_name), "%s.%zu",
                 g_log_context.config.file_path, i - 1);
        snprintf(new_name, sizeof(new_name), "%s.%zu",
                 g_log_context.config.file_path, i);
        rename(old_name, new_name);
    }

    // Rename current log file to .1
    snprintf(new_name, sizeof(new_name), "%s.1", g_log_context.config.file_path);
    rename(g_log_context.config.file_path, new_name);

    // Open new log file
    g_log_context.log_file = fopen(g_log_context.config.file_path, "w");
    if (!g_log_context.log_file)
    {
        return -1;
    }

    g_log_context.current_file_size = 0;
    return ZOO_OK;
}

/**
 * @brief Get file size
 *
 * Returns the size of the specified file in bytes.
 *
 * @param filepath Path to the file
 * @return File size in bytes, or 0 if file doesn't exist or error occurred
 */
static ZOO_USIZE_T get_file_size(ZOO_STRING_T filepath)
{
    if (!filepath)
    {
        return 0;
    }

    struct stat st;
    if (stat(filepath, &st) == 0)
    {
        return st.st_size;
    }

    return 0;
}

/**
 * @brief Write log entry to file
 *
 * Writes a formatted log entry to the log file, handling file rotation
 * if necessary.
 *
 * @param entry Formatted log entry string
 * @return ZOO_OK on success, error code on failure
 */
static ZOO_INT32 write_to_file(ZOO_STRING entry)
{
    if (!g_log_context.log_file || !entry)
    {
        return -1;
    }

    ZOO_USIZE entry_len = strlen(entry);

    // Check if we need to rotate the log file
    if (g_log_context.config.max_file_size > 0 &&
        g_log_context.current_file_size + entry_len > g_log_context.config.max_file_size)
    {

        ZOO_INT32 result = rotate_log_file();
        if (result != ZOO_OK)
        {
            return result;
        }
    }

    // Write to file
    ZOO_USIZE written = fwrite(entry, 1, entry_len, g_log_context.log_file);
    if (written != entry_len)
    {
        return -1;
    }

    fflush(g_log_context.log_file);
    g_log_context.current_file_size += written;

    return ZOO_OK;
}

/**
 * @brief Write log entry to console
 *
 * Writes a formatted log entry to stdout, with optional color coding.
 *
 * @param entry Formatted log entry string
 * @param level Log level for color selection
 */
static void write_to_console(ZOO_STRING entry, ZOO_LOG_LEVEL_ENUM level)
{
    if (!entry)
    {
        return;
    }

    if (g_log_context.config.color && isatty(STDOUT_FILENO))
    {
        printf("%s%s%s", get_level_color(level), entry, COLOR_RESET);
    }
    else
    {
        printf("%s", entry);
    }

    fflush(stdout);
}

/**
 * @brief Write log entry to syslog
 *
 * Writes a log entry to the system log facility.
 *
 * @param entry Log message content
 * @param level Log level for syslog priority mapping
 */
static void write_to_syslog(ZOO_STRING entry, ZOO_LOG_LEVEL_ENUM level)
{
#ifdef __linux__
    if (!entry)
    {
        return;
    }

    ZOO_S32 priority;
    switch (level)
    {
    case ZOO_LOG_LEVEL_TRACE:
    case ZOO_LOG_LEVEL_DEBUG:
        priority = LOG_DEBUG;
        break;
    case ZOO_LOG_LEVEL_INFO:
        priority = LOG_INFO;
        break;
    case ZOO_LOG_LEVEL_WARN:
        priority = LOG_WARNING;
        break;
    case ZOO_LOG_LEVEL_ERROR:
        priority = LOG_ERR;
        break;
    case ZOO_LOG_LEVEL_FATAL:
        priority = LOG_CRIT;
        break;
    default:
        priority = LOG_INFO;
        break;
    }

    syslog(priority, "%s", entry);
#endif
}

/**
 * @brief Core logging function
 *
 * This is the main logging function that handles formatting and output
 * of log messages to the configured targets.
 *
 * @param level Log level of the message
 * @param file Source file name
 * @param line_num Line number in source file
 * @param func_name Function name
 * @param format Printf-style format string
 * @param args Variable arguments list
 */
static void log_message(ZOO_LOG_LEVEL_ENUM level, ZOO_STRING file, ZOO_S32 line_num,
                        ZOO_STRING func_name, ZOO_STRING format, va_list args)
{
    // Check if logging is enabled and level is appropriate
    if (!g_log_context.initialized ||
        level < g_log_context.config.level ||
        level >= ZOO_LOG_LEVEL_OFF)
    {
        return;
    }

    // Thread safety
    (void)ZOO_MUTEX_LOCK(&g_log_context.mutex);

    char log_buffer[LOG_ENTRY_BUFFER_SIZE];
    char message_buffer[MAX_LOG_MESSAGE_SIZE];
    char timestamp_buffer[MAX_TIMESTAMP_SIZE];
    char thread_id_buffer[MAX_THREAD_ID_SIZE];

    // Format the actual message
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);

    // Build the complete log entry
    ZOO_S32 offset = 0;

    // Add timestamp if enabled
    if (g_log_context.config.timestamp)
    {
        get_timestamp_string(timestamp_buffer, sizeof(timestamp_buffer));
        offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                           "[%s] ", timestamp_buffer);
    }

    // Add thread ID if enabled
    if (g_log_context.config.thread_id)
    {
        get_thread_id_string(thread_id_buffer, sizeof(thread_id_buffer));
        offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                           "%s ", thread_id_buffer);
    }

    // Add log level
    offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                       "[%s] ", get_level_string(level));

    // Add file and line info if enabled
    if (g_log_context.config.file && file)
    {
        offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                           "%s:%d ", extract_filename(file), line_num);
    }

    // Add function name if enabled
    if (g_log_context.config.function_name && func_name)
    {
        offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                           "%s() ", func_name);
    }

    // Add the actual message
    offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                       "- %s\n", message_buffer);

    // Output to configured targets
    switch (g_log_context.config.target)
    {
    case ZOO_LOG_TARGET_CONSOLE:
        write_to_console(log_buffer, level);
        break;
    case ZOO_LOG_TARGET_SYSLOG:
        write_to_syslog(message_buffer, level);
        break;
    case ZOO_LOG_TARGET_BOTH:
        write_to_console(log_buffer, level);
#if ZOO_HAS_FILESYSTEM
        if (g_log_context.log_file)
        {
            write_to_file(log_buffer);
        }
#endif
        break;
    }

    (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);
}

// ==============================================================================
// PUBLIC API IMPLEMENTATION
// ==============================================================================

/**
 * @brief Initialize the logging system
 *
 * Sets up the logging system according to the provided configuration.
 * Must be called before any other logging functions.
 *
 * @param config Pointer to log configuration structure (NULL for default)
 * @return ZOO_OK on success, error code on failure
 */
ZOO_INT32 zoo_log_init(const ZOO_LOG_CONFIG_STRUCT *config)
{
    (void)ZOO_MUTEX_LOCK(&g_log_context.mutex);

    // If already initialized, shutdown first
    if (g_log_context.initialized)
    {
        (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);
        zoo_log_shutdown();
        (void)ZOO_MUTEX_LOCK(&g_log_context.mutex);
    }

    // Set configuration
    if (config)
    {
        g_log_context.config = *config;
    }
    else
    {
        ZOO_LOG_CONFIG_STRUCT default_config = ZOO_LOG_CONFIG_STRUCT_DEFAULT;
        g_log_context.config = default_config;
    }

#if ZOO_HAS_FILESYSTEM
    // Open log file if file output is enabled
    if (g_log_context.config.target == ZOO_LOG_TARGET_BOTH &&
        g_log_context.config.file_path)
    {

        ZOO_STRING mode = g_log_context.config.append ? "a" : "w";
        g_log_context.log_file = fopen(g_log_context.config.file_path, mode);

        if (!g_log_context.log_file)
        {
            (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);
            return -1;
        }

        // Get current file size for rotation management
        g_log_context.current_file_size = get_file_size(g_log_context.config.file_path);
    }
#endif

#if ZOO_HAS_SYSLOG
    // Initialize syslog if needed
    if (g_log_context.config.target == ZOO_LOG_TARGET_SYSLOG)
    {
        openlog("zoo_smb", LOG_PID | LOG_CONS, LOG_USER);
    }
#endif

    // Initialize platform-specific mutex if needed
    (void)ZOO_MUTEX_INIT(&g_log_context.mutex);

    g_log_context.initialized = true;
    (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);

    return ZOO_OK;
}

/**
 * @brief Shutdown the logging system
 *
 * Closes any open log files and releases resources used by the logging system.
 */
void zoo_log_shutdown(void)
{
    (void)ZOO_MUTEX_LOCK(&g_log_context.mutex);

    if (!g_log_context.initialized)
    {
        (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);
        return;
    }

#if ZOO_HAS_FILESYSTEM
    // Close log file
    if (g_log_context.log_file)
    {
        fclose(g_log_context.log_file);
        g_log_context.log_file = NULL;
    }
#endif

#if ZOO_HAS_SYSLOG
    // Close syslog
    if (g_log_context.config.target == ZOO_LOG_TARGET_SYSLOG)
    {
        closelog();
    }
#endif

    // Cleanup platform-specific mutex
    g_log_context.initialized = false;
#if ZOO_HAS_FILESYSTEM
    g_log_context.current_file_size = 0;
#endif

    (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);
    (void)ZOO_MUTEX_DESTROY(&g_log_context.mutex);
}

/**
 * @brief Set the minimum log level
 *
 * Only messages at or above this level will be output.
 *
 * @param level New minimum log level
 */
void zoo_log_set_level(ZOO_LOG_LEVEL_ENUM level)
{
    (void)ZOO_MUTEX_LOCK(&g_log_context.mutex);
    g_log_context.config.level = level;
    (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);
}

/**
 * @brief Set the log output target
 *
 * Changes where log messages are sent.
 *
 * @param target New log output target
 */
void zoo_log_set_target(ZOO_LOG_TARGET_ENUM target)
{
    (void)ZOO_MUTEX_LOCK(&g_log_context.mutex);
    g_log_context.config.target = target;
    (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);
}

/**
 * @brief Get the current log configuration
 *
 * Returns a pointer to the current log configuration structure.
 *
 * @return Pointer to current log configuration (read-only)
 */
const ZOO_LOG_CONFIG_STRUCT *zoo_log_get_config(void)
{
    return &g_log_context.config;
}

/**
 * @brief Set the log configuration
 *
 * Updates the current log configuration with the provided values.
 *
 * @param config Pointer to new configuration (NULL to reset to default)
 */
void zoo_log_set_config(const ZOO_LOG_CONFIG_STRUCT *config)
{
    if (config)
    {
        (void)ZOO_MUTEX_LOCK(&g_log_context.mutex);
        g_log_context.config = *config;
        (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);
    }
    else
    {
        ZOO_LOG_CONFIG_STRUCT default_config = ZOO_LOG_CONFIG_STRUCT_DEFAULT;
        (void)ZOO_MUTEX_LOCK(&g_log_context.mutex);
        g_log_context.config = default_config;
        (void)ZOO_MUTEX_UNLOCK(&g_log_context.mutex);
    }
}

/**
 * @brief Log a trace level message
 *
 * Trace messages are used for very detailed debugging information.
 *
 * @param file Source file name
 * @param line_num Line number in source file
 * @param func_name Function name
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void zoo_log_trace(ZOO_STRING_T file, ZOO_S32_T line_num, ZOO_STRING_T func_name, ZOO_STRING_T format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(ZOO_LOG_LEVEL_TRACE, file, line_num, func_name, format, args);
    va_end(args);
}

/**
 * @brief Log a debug level message
 *
 * Debug messages provide detailed information about program execution.
 *
 * @param file Source file name
 * @param line_num Line number in source file
 * @param func_name Function name
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void zoo_log_debug(ZOO_STRING_T file, ZOO_S32_T line_num, ZOO_STRING_T func_name, ZOO_STRING_T format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(ZOO_LOG_LEVEL_DEBUG, file, line_num, func_name, format, args);
    va_end(args);
}

/**
 * @brief Log an info level message
 *
 * Info messages provide general information about program operation.
 *
 * @param file Source file name
 * @param line_num Line number in source file
 * @param func_name Function name
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void zoo_log_info(ZOO_STRING file, ZOO_S32 line_num, ZOO_STRING func_name, ZOO_STRING format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(ZOO_LOG_LEVEL_INFO, file, line_num, func_name, format, args);
    va_end(args);
}

/**
 * @brief Log a warning level message
 *
 * Warning messages indicate potentially problematic situations.
 *
 * @param file Source file name
 * @param line_num Line number in source file
 * @param func_name Function name
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void zoo_log_warn(ZOO_STRING file, int line_num, ZOO_STRING func_name, ZOO_STRING format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(ZOO_LOG_LEVEL_WARN, file, line_num, func_name, format, args);
    va_end(args);
}

/**
 * @brief Log an error level message
 *
 * Error messages indicate error conditions that affect program operation.
 *
 * @param file Source file name
 * @param line_num Line number in source file
 * @param func_name Function name
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void zoo_log_error(ZOO_STRING file, int line_num, ZOO_STRING func_name, ZOO_STRING format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(ZOO_LOG_LEVEL_ERROR, file, line_num, func_name, format, args);
    va_end(args);
}

/**
 * @brief Log a fatal level message
 *
 * Fatal messages indicate severe error conditions.
 *
 * @param file Source file name
 * @param line_num Line number in source file
 * @param func_name Function name
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void zoo_log_fatal(ZOO_STRING file, int line_num, ZOO_STRING func_name, ZOO_STRING format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(ZOO_LOG_LEVEL_FATAL, file, line_num, func_name, format, args);
    va_end(args);
}

/**
 * @brief Log an error message with error code
 *
 * Logs an error message along with a ZOO_INT32 error code.
 *
 * @param error Error code to include in the message
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void zoo_log_error_code(ZOO_INT32 error, ZOO_STRING format, ...)
{
    va_list args;
    va_start(args, format);

    char extended_format[MAX_LOG_MESSAGE_SIZE];
    snprintf(extended_format, sizeof(extended_format), "[Error Code: %d] %s", error, format);

    log_message(ZOO_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, extended_format, args);
    va_end(args);
}

/**
 * @brief Log a fatal message with error code
 *
 * Logs a fatal message along with a ZOO_INT32 error code.
 *
 * @param error Error code to include in the message
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void zoo_log_fatal_code(ZOO_INT32 error, ZOO_STRING format, ...)
{
    va_list args;
    va_start(args, format);

    char extended_format[MAX_LOG_MESSAGE_SIZE];
    snprintf(extended_format, sizeof(extended_format), "[Error Code: %d] %s", error, format);

    log_message(ZOO_LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, extended_format, args);
    va_end(args);
}
