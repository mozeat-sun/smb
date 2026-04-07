/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: ZOO
 * Component ID: ERROR
 * File name: zoo_error.h
 * Description: Error handling definitions and abstractions for ZOO
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-05-14     weiwang.sun     Initial creation
 ******************************************************************************/
#ifndef ZOO_ERROR_H
#define ZOO_ERROR_H
#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Standard error codes */
#ifndef ZOO_OK
#define ZOO_OK 0 /**< Success alias */
#endif

// ==============================================================================
// ERROR CODE RANGES - Each module gets 100 error codes
// ==============================================================================

/** @brief Base error code ranges for different modules */
#define ZOO_ERROR_RANGE_GENERAL 0       /**< General errors: 0 to -99 */
#define ZOO_ERROR_RANGE_MEMORY -100     /**< Memory errors: -100 to -199 */
#define ZOO_ERROR_RANGE_FILE -200       /**< File system errors: -200 to -299 */
#define ZOO_ERROR_RANGE_NETWORK -300    /**< Network errors: -300 to -399 */
#define ZOO_ERROR_RANGE_THREAD -400     /**< Threading errors: -400 to -499 */
#define ZOO_ERROR_RANGE_HARDWARE -500   /**< Hardware errors: -500 to -599 */
#define ZOO_ERROR_RANGE_PROTOCOL -600   /**< Protocol errors: -600 to -699 */
#define ZOO_ERROR_RANGE_SECURITY -700   /**< Security errors: -700 to -799 */
#define ZOO_ERROR_RANGE_CONFIG -800     /**< Configuration errors: -800 to -899 */
#define ZOO_ERROR_RANGE_VALIDATION -900 /**< Validation errors: -900 to -999 */
#define ZOO_ERROR_RANGE_REALTIME -1000  /**< Real-time errors: -1000 to -1099 */
#define ZOO_ERROR_RANGE_MATH -1100      /**< Mathematical errors: -1100 to -1199 */
#define ZOO_ERROR_RANGE_SMB -1200       /**< SMB specific errors: -1200 to -1299 */
#define ZOO_ERROR_RANGE_UTIL -1300      /**< Utility errors: -1300 to -1399 */
#define ZOO_ERROR_RANGE_LOG -1400       /**< Logging errors: -1400 to -1499 */
#define ZOO_ERROR_RANGE_PLATFORM -1500  /**< Platform errors: -1500 to -1599 */
#define ZOO_ERROR_RANGE_BUFFER -1600    /**< Buffer errors: -1600 to -1699 */

    // ==============================================================================
    // GENERAL ERROR CODES (0 to -99)
    // ==============================================================================

#define ZOO_ERROR_OK (ZOO_ERROR_RANGE_GENERAL + 0)                 /**< Success */
#define ZOO_ERROR_GENERAL (ZOO_ERROR_RANGE_GENERAL - 1)            /**< General error */
#define ZOO_ERROR_INVALID_PARAM (ZOO_ERROR_RANGE_GENERAL - 2)      /**< Invalid parameter */
#define ZOO_ERROR_NULL_POINTER (ZOO_ERROR_RANGE_GENERAL - 3)       /**< Null pointer */
#define ZOO_ERROR_NOT_SUPPORTED (ZOO_ERROR_RANGE_GENERAL - 4)      /**< Operation not supported */
#define ZOO_ERROR_TIMEOUT (ZOO_ERROR_RANGE_GENERAL - 5)            /**< Operation timeout */
#define ZOO_ERROR_NOT_INITIALIZED (ZOO_ERROR_RANGE_GENERAL - 6)    /**< Component not initialized */
#define ZOO_ERROR_ALREADY_EXISTS (ZOO_ERROR_RANGE_GENERAL - 7)     /**< Resource already exists */
#define ZOO_ERROR_NOT_FOUND (ZOO_ERROR_RANGE_GENERAL - 8)          /**< Resource not found */
#define ZOO_ERROR_BUFFER_TOO_SMALL (ZOO_ERROR_RANGE_GENERAL - 9)   /**< Buffer too small */
#define ZOO_ERROR_INVALID_STATE (ZOO_ERROR_RANGE_GENERAL - 10)     /**< Invalid state */
#define ZOO_ERROR_OPERATION_FAILED (ZOO_ERROR_RANGE_GENERAL - 11)  /**< Operation failed */
#define ZOO_ERROR_ACCESS_DENIED (ZOO_ERROR_RANGE_GENERAL - 12)     /**< Access denied */
#define ZOO_ERROR_PERMISSION_DENIED (ZOO_ERROR_RANGE_GENERAL - 13) /**< Permission denied */
#define ZOO_ERROR_RESOURCE_BUSY (ZOO_ERROR_RANGE_GENERAL - 14)     /**< Resource busy */
#define ZOO_ERROR_NOT_STARTED (ZOO_ERROR_RANGE_GENERAL - 15)       /**< Service not started */
#define ZOO_ERROR_AGAIN (ZOO_ERROR_RANGE_GENERAL - 16)             /**< Try again later */
#define ZOO_ERROR_IN_PROGRESS ZOO_ERROR_AGAIN                       /**< Operation is in progress */

/* Legacy compatibility aliases used by some modules. */
#define ZOO_ERROR_GENERIC ZOO_ERROR_GENERAL

#define ZOO_ERROR_NOT_INIT ZOO_ERROR_NOT_INITIALIZED
#define ZOO_ERROR_ALREADY_INIT ZOO_ERROR_ALREADY_EXISTS
#define ZOO_ERROR_NO_MEMORY ZOO_ERROR_OUT_OF_MEMORY
#define ZOO_ERROR_NOT_CONNECTED ZOO_ERROR_CONNECTION_LOST
#define ZOO_ERROR_ADDRESS_IN_USE ZOO_ERROR_BIND_FAILED
#define ZOO_ERROR_WOULD_BLOCK ZOO_ERROR_RESOURCE_BUSY
#define ZOO_ERROR_NETWORK ZOO_ERROR_NETWORK_ERROR

    // ==============================================================================
    // MEMORY ERROR CODES (-100 to -199)
    // ==============================================================================

#define ZOO_ERROR_OUT_OF_MEMORY (ZOO_ERROR_RANGE_MEMORY - 1)              /**< Out of memory */
#define ZOO_ERROR_MEMORY_ALLOCATION_FAILED (ZOO_ERROR_RANGE_MEMORY - 2)   /**< Memory allocation failed */
#define ZOO_ERROR_MEMORY_DEALLOCATION_FAILED (ZOO_ERROR_RANGE_MEMORY - 3) /**< Memory deallocation failed */
#define ZOO_ERROR_MEMORY_CORRUPTION (ZOO_ERROR_RANGE_MEMORY - 4)          /**< Memory corruption */
#define ZOO_ERROR_MEMORY_LEAK_DETECTED (ZOO_ERROR_RANGE_MEMORY - 5)       /**< Memory leak detected */
#define ZOO_ERROR_HEAP_EXHAUSTED (ZOO_ERROR_RANGE_MEMORY - 6)             /**< Heap exhausted */
#define ZOO_ERROR_STACK_OVERFLOW (ZOO_ERROR_RANGE_MEMORY - 7)             /**< Stack overflow */
#define ZOO_ERROR_BUFFER_OVERFLOW (ZOO_ERROR_RANGE_MEMORY - 8)            /**< Buffer overflow */
#define ZOO_ERROR_BUFFER_UNDERFLOW (ZOO_ERROR_RANGE_MEMORY - 9)           /**< Buffer underflow */
#define ZOO_ERROR_MEMORY_ALIGNMENT (ZOO_ERROR_RANGE_MEMORY - 10)          /**< Memory alignment error */
#define ZOO_ERROR_INVALID_ADDRESS (ZOO_ERROR_RANGE_MEMORY - 11)           /**< Invalid memory address */

    // ==============================================================================
    // FILE SYSTEM ERROR CODES (-200 to -299)
    // ==============================================================================

#define ZOO_ERROR_FILE_NOT_FOUND (ZOO_ERROR_RANGE_FILE - 1)      /**< File not found */
#define ZOO_ERROR_FILE_EXISTS (ZOO_ERROR_RANGE_FILE - 2)         /**< File already exists */
#define ZOO_ERROR_FILE_ACCESS_DENIED (ZOO_ERROR_RANGE_FILE - 3)  /**< File access denied */
#define ZOO_ERROR_FILE_READ_ERROR (ZOO_ERROR_RANGE_FILE - 4)     /**< File read error */
#define ZOO_ERROR_FILE_WRITE_ERROR (ZOO_ERROR_RANGE_FILE - 5)    /**< File write error */
#define ZOO_ERROR_FILE_CORRUPT (ZOO_ERROR_RANGE_FILE - 6)        /**< File corrupted */
#define ZOO_ERROR_DIRECTORY_NOT_FOUND (ZOO_ERROR_RANGE_FILE - 7) /**< Directory not found */
#define ZOO_ERROR_DISK_FULL (ZOO_ERROR_RANGE_FILE - 8)           /**< Disk full */
#define ZOO_ERROR_PATH_TOO_LONG (ZOO_ERROR_RANGE_FILE - 9)       /**< Path too long */
#define ZOO_ERROR_INVALID_PATH (ZOO_ERROR_RANGE_FILE - 10)       /**< Invalid path */
#define ZOO_ERROR_IO_ERROR (ZOO_ERROR_RANGE_FILE - 11)           /**< I/O error */

    // ==============================================================================
    // NETWORK ERROR CODES (-300 to -399)
    // ==============================================================================

#define ZOO_ERROR_NETWORK_ERROR (ZOO_ERROR_RANGE_NETWORK - 1)          /**< Network error */
#define ZOO_ERROR_CONNECTION_LOST (ZOO_ERROR_RANGE_NETWORK - 2)        /**< Connection lost */
#define ZOO_ERROR_NETWORK_UNREACHABLE (ZOO_ERROR_RANGE_NETWORK - 3)    /**< Network unreachable */
#define ZOO_ERROR_HOST_UNREACHABLE (ZOO_ERROR_RANGE_NETWORK - 4)       /**< Host unreachable */
#define ZOO_ERROR_CONNECTION_REFUSED (ZOO_ERROR_RANGE_NETWORK - 5)     /**< Connection refused */
#define ZOO_ERROR_CONNECTION_TIMEOUT (ZOO_ERROR_RANGE_NETWORK - 6)     /**< Connection timeout */
#define ZOO_ERROR_CONNECTION_RESET (ZOO_ERROR_RANGE_NETWORK - 7)       /**< Connection reset */
#define ZOO_ERROR_SOCKET_ERROR (ZOO_ERROR_RANGE_NETWORK - 8)           /**< Socket error */
#define ZOO_ERROR_BIND_FAILED (ZOO_ERROR_RANGE_NETWORK - 9)            /**< Bind failed */
#define ZOO_ERROR_LISTEN_FAILED (ZOO_ERROR_RANGE_NETWORK - 10)         /**< Listen failed */
#define ZOO_ERROR_ACCEPT_FAILED (ZOO_ERROR_RANGE_NETWORK - 11)         /**< Accept failed */
#define ZOO_ERROR_DNS_RESOLUTION_FAILED (ZOO_ERROR_RANGE_NETWORK - 12) /**< DNS resolution failed */

    // ==============================================================================
    // THREADING ERROR CODES (-400 to -499)
    // ==============================================================================

#define ZOO_ERROR_THREAD_CREATE_FAILED (ZOO_ERROR_RANGE_THREAD - 1)  /**< Thread creation failed */
#define ZOO_ERROR_THREAD_JOIN_FAILED (ZOO_ERROR_RANGE_THREAD - 2)    /**< Thread join failed */
#define ZOO_ERROR_MUTEX_LOCK_FAILED (ZOO_ERROR_RANGE_THREAD - 3)     /**< Mutex lock failed */
#define ZOO_ERROR_MUTEX_UNLOCK_FAILED (ZOO_ERROR_RANGE_THREAD - 4)   /**< Mutex unlock failed */
#define ZOO_ERROR_SEMAPHORE_WAIT_FAILED (ZOO_ERROR_RANGE_THREAD - 5) /**< Semaphore wait failed */
#define ZOO_ERROR_SEMAPHORE_POST_FAILED (ZOO_ERROR_RANGE_THREAD - 6) /**< Semaphore post failed */
#define ZOO_ERROR_CONDITION_WAIT_FAILED (ZOO_ERROR_RANGE_THREAD - 7) /**< Condition wait failed */
#define ZOO_ERROR_DEADLOCK_DETECTED (ZOO_ERROR_RANGE_THREAD - 8)     /**< Deadlock detected */
#define ZOO_ERROR_RACE_CONDITION (ZOO_ERROR_RANGE_THREAD - 9)        /**< Race condition */
#define ZOO_ERROR_THREAD_CANCELLED (ZOO_ERROR_RANGE_THREAD - 10)     /**< Thread cancelled */

    // ==============================================================================
    // HARDWARE ERROR CODES (-500 to -599)
    // ==============================================================================

#define ZOO_ERROR_HARDWARE_FAULT (ZOO_ERROR_RANGE_HARDWARE - 1)       /**< Hardware fault */
#define ZOO_ERROR_WATCHDOG_TIMEOUT (ZOO_ERROR_RANGE_HARDWARE - 2)     /**< Watchdog timeout */
#define ZOO_ERROR_POWER_FAILURE (ZOO_ERROR_RANGE_HARDWARE - 3)        /**< Power failure */
#define ZOO_ERROR_TEMPERATURE_CRITICAL (ZOO_ERROR_RANGE_HARDWARE - 4) /**< Critical temperature */
#define ZOO_ERROR_VOLTAGE_OUT_OF_RANGE (ZOO_ERROR_RANGE_HARDWARE - 5) /**< Voltage out of range */
#define ZOO_ERROR_CLOCK_FAILURE (ZOO_ERROR_RANGE_HARDWARE - 6)        /**< Clock failure */
#define ZOO_ERROR_PERIPHERAL_ERROR (ZOO_ERROR_RANGE_HARDWARE - 7)     /**< Peripheral error */
#define ZOO_ERROR_DMA_ERROR (ZOO_ERROR_RANGE_HARDWARE - 8)            /**< DMA error */
#define ZOO_ERROR_INTERRUPT_ERROR (ZOO_ERROR_RANGE_HARDWARE - 9)      /**< Interrupt error */
#define ZOO_ERROR_CALIBRATION_ERROR (ZOO_ERROR_RANGE_HARDWARE - 10)   /**< Calibration error */
#define ZOO_ERROR_DEVICE_NOT_READY (ZOO_ERROR_RANGE_HARDWARE - 11)    /**< Device not ready */
#define ZOO_ERROR_DEVICE_ERROR (ZOO_ERROR_RANGE_HARDWARE - 12)        /**< Device error */

    // ==============================================================================
    // BUFFER ERROR CODES (-1600 to -1699)
    // ==============================================================================

#define ZOO_ERROR_BUFFER_FULL (ZOO_ERROR_RANGE_BUFFER - 1)               /**< Buffer is full */
#define ZOO_ERROR_BUFFER_EMPTY (ZOO_ERROR_RANGE_BUFFER - 2)              /**< Buffer is empty */
#define ZOO_ERROR_BUFFER_OUT_OF_RANGE (ZOO_ERROR_RANGE_BUFFER - 3)       /**< Index out of range */
#define ZOO_ERROR_BUFFER_INVALID_HANDLE (ZOO_ERROR_RANGE_BUFFER - 4)     /**< Invalid buffer handle */
#define ZOO_ERROR_BUFFER_LOCK_FAILED (ZOO_ERROR_RANGE_BUFFER - 5)        /**< Buffer lock failed */
#define ZOO_ERROR_BUFFER_UNLOCK_FAILED (ZOO_ERROR_RANGE_BUFFER - 6)      /**< Buffer unlock failed */
#define ZOO_ERROR_BUFFER_INIT_FAILED (ZOO_ERROR_RANGE_BUFFER - 7)        /**< Buffer initialization failed */
#define ZOO_ERROR_BUFFER_DESTROY_FAILED (ZOO_ERROR_RANGE_BUFFER - 8)     /**< Buffer destruction failed */
#define ZOO_ERROR_BUFFER_RESIZE_FAILED (ZOO_ERROR_RANGE_BUFFER - 9)      /**< Buffer resize failed */
#define ZOO_ERROR_BUFFER_ALLOCATION_FAILED (ZOO_ERROR_RANGE_BUFFER - 10) /**< Buffer allocation failed */

    // ==============================================================================
    // SYSTEM CALL ERROR CODES (Aliases for compatibility)
    // ==============================================================================

#define ZOO_ERROR_SYSTEM_CALL_FAILED ZOO_ERROR_OPERATION_FAILED /**< System call failed */
#define ZOO_ERROR_SYSTEM_CALL ZOO_ERROR_OPERATION_FAILED        /**< Alias for system call failed */

    // ==============================================================================
    // BACKWARD COMPATIBILITY ALIASES
    // ==============================================================================

    /** @brief Error code type definition */
    typedef int ZOO_ERROR_T;

    /** @brief Legacy compatibility alias for error code type */
    typedef ZOO_ERROR_T ZOO_ERROR_TYPE;

    /** @brief Error severity levels */
    typedef enum
    {
        ZOO_SEVERITY_INFO = 0,     /**< Informational message */
        ZOO_SEVERITY_WARNING = 1,  /**< Warning condition */
        ZOO_SEVERITY_ERROR = 2,    /**< Error condition */
        ZOO_SEVERITY_CRITICAL = 3, /**< Critical error condition */
        ZOO_SEVERITY_FATAL = 4     /**< Fatal error condition */
    } ZOO_ERROR_SEVERITY_ENUM;

    /** @brief Error context structure */
    typedef struct
    {
        ZOO_ERROR_T code;                 /**< Error code */
        ZOO_ERROR_SEVERITY_ENUM severity; /**< Error severity */
        const char *file;                 /**< Source file where error occurred */
        int line;                         /**< Line number where error occurred */
        const char *function;             /**< Function name where error occurred */
        const char *message;              /**< Error message */
    } ZOO_ERROR_CONTEXT_STRUCT;

    // ==============================================================================
    // ERROR HANDLING MACROS
    // ==============================================================================

    /** @brief Check if error code indicates success */
#define ZOO_IS_SUCCESS(code) ((code) == ZOO_OK)

    /** @brief Check if error code indicates failure */
#define ZOO_IS_ERROR(code) ((code) != ZOO_OK)

    /** @brief Return if condition is true with specific error code */
#define ZOO_RETURN_IF(condition, error_code) \
    do                                       \
    {                                        \
        if (condition)                       \
        {                                    \
            return (error_code);             \
        }                                    \
    } while (0)

    /** @brief Return if pointer is NULL */
#define ZOO_RETURN_IF_NULL(ptr) \
    ZOO_RETURN_IF((ptr) == NULL, ZOO_ERROR_NULL_POINTER)

    /** @brief Return if condition is true with specific error code and cleanup */
#define ZOO_RETURN_IF_WITH_CLEANUP(condition, error_code, cleanup) \
    do                                                             \
    {                                                              \
        if (condition)                                             \
        {                                                          \
            cleanup;                                               \
            return (error_code);                                   \
        }                                                          \
    } while (0)

    /** @brief Check error and return if failed */
#define ZOO_CHECK_ERROR(expression)        \
    do                                     \
    {                                      \
        ZOO_ERROR_T _error = (expression); \
        if (ZOO_IS_ERROR(_error))          \
        {                                  \
            return _error;                 \
        }                                  \
    } while (0)

    /** @brief Check error and goto cleanup if failed */
#define ZOO_CHECK_ERROR_GOTO(expression, label) \
    do                                          \
    {                                           \
        ZOO_ERROR_T _error = (expression);      \
        if (ZOO_IS_ERROR(_error))               \
        {                                       \
            goto label;                         \
        }                                       \
    } while (0)

    /** @brief Assert condition and return error if failed */
#ifdef ZOO_DEBUG
#define ZOO_ASSERT_RETURN(condition, error_code)            \
    do                                                      \
    {                                                       \
        if (!(condition))                                   \
        {                                                   \
            zoo_error_log(__FILE__, __LINE__, __FUNCTION__, \
                          "Assertion failed: " #condition); \
            return (error_code);                            \
        }                                                   \
    } while (0)
#else
#define ZOO_ASSERT_RETURN(condition, error_code) \
    ZOO_RETURN_IF(!(condition), error_code)
#endif

    // ==============================================================================
    // ERROR HANDLING FUNCTION DECLARATIONS
    // ==============================================================================

    /**
     * @brief Convert error code to human-readable string
     * @param error_code The error code to convert
     * @return String representation of the error code
     */
    const char *zoo_error_to_string(ZOO_ERROR_T error_code);

    /**
     * @brief Get error severity from error code
     * @param error_code The error code to analyze
     * @return Error severity level
     */
    ZOO_ERROR_SEVERITY_ENUM zoo_error_get_severity(ZOO_ERROR_T error_code);

    /**
     * @brief Get error category from error code
     * @param error_code The error code to categorize
     * @return String representation of error category
     */
    const char *zoo_error_get_category(ZOO_ERROR_T error_code);

    // ==============================================================================
    // LEGACY COMPATIBILITY ALIASES
    // ==============================================================================
    /** @brief Simple aliases for backward compatibility with test files */
    #define ZOO_ERROR ZOO_ERROR_GENERAL
    #define ZOO_TIMEOUT ZOO_ERROR_TIMEOUT  
    #define ZOO_INVALID_PARAM ZOO_ERROR_INVALID_PARAM

#ifdef __cplusplus
}
#endif

#endif /* ZOO_ERROR_H */