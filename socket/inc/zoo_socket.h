/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: Socket
 * Component ID: ZOO_SOCKET
 * File name: zoo_socket.h
 * Description: Cross-platform socket abstraction supporting POSIX sockets and lwIP
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-05     assistant       Initial creation
 ******************************************************************************/

#ifndef ZOO_SOCKET_H
#define ZOO_SOCKET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_platform.h"
#include "zoo_types.h"
#include <stdint.h>
#include <stdbool.h>

#define ZOO_SOCKET_VERSION "1.0.0"

// ==============================================================================
// ERROR CODES
// ==============================================================================

// Include platform headers for error codes
#include "zoo_platform.h"
#include "../../platform/inc/zoo_error.h"

// Use platform error codes instead of redefining them
// ZOO_OK, ZOO_ERROR_INVALID_PARAM, etc. are already defined in zoo_error.h

    // ==============================================================================
    // SOCKET ABSTRACTION LAYER
    // ==============================================================================

    /**
     * @brief Socket domain/family types
     */
    typedef enum
    {
        ZOO_AF_INET = 0, /**< IPv4 Internet protocols */
        ZOO_AF_INET6,    /**< IPv6 Internet protocols */
        ZOO_AF_UNIX,     /**< Unix domain sockets (local) */
        ZOO_AF_UNSPEC    /**< Unspecified */
    } ZOO_SOCKET_FAMILY_ENUM;

    /**
     * @brief Socket type definitions
     */
    typedef enum
    {
        ZOO_SOCK_STREAM = 0, /**< TCP stream socket */
        ZOO_SOCK_DGRAM,      /**< UDP datagram socket */
        ZOO_SOCK_RAW         /**< Raw socket */
    } ZOO_SOCKET_TYPE_ENUM;

    /**
     * @brief Socket protocol types
     */
    typedef enum
    {
        ZOO_IPPROTO_TCP = 0, /**< TCP protocol */
        ZOO_IPPROTO_UDP,     /**< UDP protocol */
        ZOO_IPPROTO_RAW      /**< Raw protocol */
    } ZOO_SOCKET_PROTOCOL_ENUM;

    /**
     * @brief Socket shutdown options
     */
    typedef enum
    {
        ZOO_SHUT_RD = 0, /**< Shutdown read operations */
        ZOO_SHUT_WR,     /**< Shutdown write operations */
        ZOO_SHUT_RDWR    /**< Shutdown both read and write */
    } ZOO_SOCKET_SHUTDOWN_ENUM;

    /**
     * @brief Socket option levels
     */
    typedef enum
    {
        ZOO_SOL_SOCKET = 0,  /**< Socket level options */
        ZOO_IPPROTO_IP,      /**< IP level options */
        ZOO_IPPROTO_TCP_OPT, /**< TCP level options */
        ZOO_IPPROTO_UDP_OPT  /**< UDP level options */
    } ZOO_SOCKET_LEVEL_ENUM;

    /**
     * @brief Socket options
     */
    typedef enum
    {
        ZOO_SO_REUSEADDR = 0, /**< Reuse address */
        ZOO_SO_REUSEPORT,     /**< Reuse port */
        ZOO_SO_BROADCAST,     /**< Enable broadcast */
        ZOO_SO_KEEPALIVE,     /**< Keep connections alive */
        ZOO_SO_LINGER,        /**< Linger on close */
        ZOO_SO_RCVBUF,        /**< Receive buffer size */
        ZOO_SO_SNDBUF,        /**< Send buffer size */
        ZOO_SO_RCVTIMEO,      /**< Receive timeout */
        ZOO_SO_SNDTIMEO,      /**< Send timeout */
        ZOO_SO_NONBLOCK,      /**< Non-blocking mode */
        ZOO_TCP_NODELAY       /**< Disable Nagle algorithm */
    } ZOO_SOCKET_OPTION_ENUM;

    /**
     * @brief Socket select types for monitoring
     */
    typedef enum
    {
        ZOO_SELECT_READ = 0, /**< Monitor for read operations */
        ZOO_SELECT_WRITE,    /**< Monitor for write operations */
        ZOO_SELECT_EXCEPT,   /**< Monitor for exceptional conditions */
        ZOO_SELECT_ALL       /**< Monitor for all conditions */
    } ZOO_SOCKET_SELECT_TYPE_ENUM;

// ==============================================================================
// SOCKET ADDRESS CONSTANTS
// ==============================================================================

/**
 * @brief IPv4 address constants
 */
#define ZOO_INADDR_ANY 0x00000000U       /**< Bind to any address (0.0.0.0) */
#define ZOO_INADDR_LOOPBACK 0x7F000001U  /**< Loopback address (127.0.0.1) */
#define ZOO_INADDR_BROADCAST 0xFFFFFFFFU /**< Broadcast address (255.255.255.255) */
#define ZOO_INADDR_NONE 0xFFFFFFFFU      /**< Invalid address */

    /**
     * @brief Socket address structure (IPv4)
     */
    typedef struct
    {
        ZOO_SOCKET_FAMILY_ENUM family; /**< Address family */
        uint16_t port;                 /**< Port number in host byte order */
        uint32_t addr;                 /**< IP address in host byte order */
        uint8_t zero[8];               /**< Padding for compatibility */
    } ZOO_SOCKADDR_IN_STRUCT;

    /**
     * @brief Socket address structure (IPv6)
     */
    typedef struct
    {
        ZOO_SOCKET_FAMILY_ENUM family; /**< Address family */
        uint16_t port;                 /**< Port number in host byte order */
        uint32_t flowinfo;             /**< IPv6 flow information */
        uint8_t addr[16];              /**< IPv6 address */
        uint32_t scope_id;             /**< IPv6 scope ID */
    } ZOO_SOCKADDR_IN6_STRUCT;

    /**
     * @brief Generic socket address structure
     */
    typedef union
    {
        ZOO_SOCKET_FAMILY_ENUM family; /**< Address family */
        ZOO_SOCKADDR_IN_STRUCT in;     /**< IPv4 address */
        ZOO_SOCKADDR_IN6_STRUCT in6;   /**< IPv6 address */
        uint8_t storage[128];          /**< Storage for any socket address */
    } ZOO_SOCKADDR_UNION;

/**
 * @brief Socket handle - platform dependent implementation
 */
#if ZOO_HAS_POSIX
    // Linux/POSIX: use native file descriptor
    typedef int ZOO_SOCKET_TYPE;
#define ZOO_INVALID_SOCKET (-1)
#elif defined(ZOO_USE_LWIP)
// lwIP: use lwIP socket structures
typedef int ZOO_SOCKET_TYPE; // lwIP also uses int for socket descriptors
#define ZOO_INVALID_SOCKET (-1)
#else
// Bare metal or other platforms: custom implementation
typedef struct zoo_socket_impl *ZOO_SOCKET_TYPE;
#define ZOO_INVALID_SOCKET NULL
#endif

    /**
     * @brief Socket information structure
     */
    typedef struct
    {
        ZOO_SOCKET_TYPE fd;                /**< Socket file descriptor/handle */
        ZOO_SOCKET_FAMILY_ENUM family;     /**< Socket family */
        ZOO_SOCKET_TYPE_ENUM type;         /**< Socket type */
        ZOO_SOCKET_PROTOCOL_ENUM protocol; /**< Socket protocol */
        ZOO_SOCKADDR_UNION local_addr;     /**< Local address */
        ZOO_SOCKADDR_UNION remote_addr;    /**< Remote address */
        bool is_connected;                 /**< Connection status */
        bool is_bound;                     /**< Bind status */
        bool is_listening;                 /**< Listen status */
        bool is_blocking;                  /**< Blocking mode */
    } ZOO_SOCKET_INFO_STRUCT;

    // ==============================================================================
    // CORE SOCKET FUNCTIONS
    // ==============================================================================

    /**
     * @brief Initialize the socket subsystem
     *
     * Must be called before using any socket functions.
     *
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_init(void);

    /**
     * @brief Cleanup the socket subsystem
     *
     * Should be called when done using socket functions.
     */
    void zoo_socket_cleanup(void);

    /**
     * @brief Create a socket
     *
     * @param family Socket family (ZOO_AF_INET, ZOO_AF_INET6, etc.)
     * @param type Socket type (ZOO_SOCK_STREAM, ZOO_SOCK_DGRAM, etc.)
     * @param protocol Socket protocol (ZOO_IPPROTO_TCP, ZOO_IPPROTO_UDP, etc.)
     * @param socket_info Pointer to socket info structure to fill
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_create(ZOO_SOCKET_FAMILY_ENUM family,
                                     ZOO_SOCKET_TYPE_ENUM type,
                                     ZOO_SOCKET_PROTOCOL_ENUM protocol,
                                     ZOO_SOCKET_INFO_STRUCT *socket_info);

    /**
     * @brief Close a socket
     *
     * @param socket_info Pointer to socket info structure
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_close(ZOO_SOCKET_INFO_STRUCT *socket_info);

    /**
     * @brief Bind a socket to an address
     *
     * @param socket_info Pointer to socket info structure
     * @param addr Address to bind to
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_bind(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                   const ZOO_SOCKADDR_UNION *addr);

    /**
     * @brief Listen for connections on a socket
     *
     * @param socket_info Pointer to socket info structure
     * @param backlog Maximum number of pending connections
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_listen(ZOO_SOCKET_INFO_STRUCT *socket_info, int backlog);

    /**
     * @brief Accept a connection on a listening socket
     *
     * @param server_info Pointer to server socket info structure
     * @param client_info Pointer to client socket info structure to fill
     * @param client_addr Pointer to store client address (can be NULL)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_accept(ZOO_SOCKET_INFO_STRUCT *server_info,
                                     ZOO_SOCKET_INFO_STRUCT *client_info,
                                     ZOO_SOCKADDR_UNION *client_addr);

    /**
     * @brief Connect a socket to a remote address
     *
     * @param socket_info Pointer to socket info structure
     * @param addr Remote address to connect to
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_connect(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                      const ZOO_SOCKADDR_UNION *addr);

    /**
     * @brief Send data through a connected socket
     *
     * @param socket_info Pointer to socket info structure
     * @param data Pointer to data to send
     * @param len Length of data to send
     * @param bytes_sent Pointer to store actual bytes sent (can be NULL)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_send(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                   const void *data,
                                   size_t len,
                                   size_t *bytes_sent);

    /**
     * @brief Receive data from a connected socket
     *
     * @param socket_info Pointer to socket info structure
     * @param buffer Buffer to store received data
     * @param len Maximum length to receive
     * @param bytes_received Pointer to store actual bytes received (can be NULL)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_recv(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                   void *buffer,
                                   size_t len,
                                   size_t *bytes_received);

    /**
     * @brief Send data to a specific address (UDP)
     *
     * @param socket_info Pointer to socket info structure
     * @param data Pointer to data to send
     * @param len Length of data to send
     * @param addr Destination address
     * @param bytes_sent Pointer to store actual bytes sent (can be NULL)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_sendto(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                     const void *data,
                                     size_t len,
                                     const ZOO_SOCKADDR_UNION *addr,
                                     size_t *bytes_sent);

    /**
     * @brief Receive data from any address (UDP)
     *
     * @param socket_info Pointer to socket info structure
     * @param buffer Buffer to store received data
     * @param len Maximum length to receive
     * @param addr Pointer to store sender address (can be NULL)
     * @param bytes_received Pointer to store actual bytes received (can be NULL)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_recvfrom(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                       void *buffer,
                                       size_t len,
                                       ZOO_SOCKADDR_UNION *addr,
                                       size_t *bytes_received);

    /**
     * @brief Shutdown part of a socket connection
     *
     * @param socket_info Pointer to socket info structure
     * @param how How to shutdown (ZOO_SHUT_RD, ZOO_SHUT_WR, ZOO_SHUT_RDWR)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_shutdown(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                       ZOO_SOCKET_SHUTDOWN_ENUM how);

    // ==============================================================================
    // SOCKET OPTIONS AND CONTROL
    // ==============================================================================

    /**
     * @brief Set socket option
     *
     * @param socket_info Pointer to socket info structure
     * @param level Option level
     * @param option Option name
     * @param value Pointer to option value
     * @param value_len Length of option value
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_setsockopt(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                         ZOO_SOCKET_LEVEL_ENUM level,
                                         ZOO_SOCKET_OPTION_ENUM option,
                                         const void *value,
                                         size_t value_len);

    /**
     * @brief Get socket option
     *
     * @param socket_info Pointer to socket info structure
     * @param level Option level
     * @param option Option name
     * @param value Pointer to store option value
     * @param value_len Pointer to option value length (in/out)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_getsockopt(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                         ZOO_SOCKET_LEVEL_ENUM level,
                                         ZOO_SOCKET_OPTION_ENUM option,
                                         void *value,
                                         size_t *value_len);

    /**
     * @brief Set socket to blocking or non-blocking mode
     *
     * @param socket_info Pointer to socket info structure
     * @param blocking true for blocking, false for non-blocking
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_set_blocking(ZOO_SOCKET_INFO_STRUCT *socket_info, bool blocking);

    /**
     * @brief Check if socket is ready for I/O operations
     *
     * @param socket_info Pointer to socket info structure
     * @param check_read Check for read readiness
     * @param check_write Check for write readiness
     * @param check_error Check for error conditions
     * @param timeout_ms Timeout in milliseconds (0 for immediate return, -1 for infinite)
     * @param ready_read Set to true if ready for reading (can be NULL)
     * @param ready_write Set to true if ready for writing (can be NULL)
     * @param has_error Set to true if error condition (can be NULL)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_select(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                     bool check_read,
                                     bool check_write,
                                     bool check_error,
                                     int timeout_ms,
                                     bool *ready_read,
                                     bool *ready_write,
                                     bool *has_error);

    // ==============================================================================
    // ADDRESS MANIPULATION FUNCTIONS
    // ==============================================================================

    /**
     * @brief Create IPv4 address structure
     *
     * @param addr Pointer to address structure to fill
     * @param ip_str IP address string (e.g., "192.168.1.1")
     * @param port Port number in host byte order
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_sockaddr_ipv4(ZOO_SOCKADDR_UNION *addr, const char *ip_str, uint16_t port);

    /**
     * @brief Create IPv6 address structure
     *
     * @param addr Pointer to address structure to fill
     * @param ip_str IPv6 address string (e.g., "::1")
     * @param port Port number in host byte order
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_sockaddr_ipv6(ZOO_SOCKADDR_UNION *addr, const char *ip_str, uint16_t port);

    /**
     * @brief Convert address structure to string representation
     *
     * @param addr Pointer to address structure
     * @param buffer Buffer to store string representation
     * @param buffer_size Size of buffer
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_sockaddr_to_string(const ZOO_SOCKADDR_UNION *addr, char *buffer, size_t buffer_size);

    /**
     * @brief Get port number from address structure
     *
     * @param addr Pointer to address structure
     * @return Port number in host byte order, 0 on error
     */
    uint16_t zoo_sockaddr_get_port(const ZOO_SOCKADDR_UNION *addr);

    /**
     * @brief Set port number in address structure
     *
     * @param addr Pointer to address structure
     * @param port Port number in host byte order
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_sockaddr_set_port(ZOO_SOCKADDR_UNION *addr, uint16_t port);

    // ==============================================================================
    // ADDRESS INITIALIZATION UTILITIES
    // ==============================================================================

    /**
     * @brief Initialize IPv4 socket address with raw address and port
     *
     * @param addr Pointer to socket address union to initialize
     * @param ip_address IPv4 address in host byte order
     * @param port Port number in host byte order
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_sockaddr_inet_init(ZOO_SOCKADDR_UNION *addr,
                                          uint32_t ip_address,
                                          uint16_t port);

    /**
     * @brief Initialize IPv6 socket address with raw address and port
     *
     * @param addr Pointer to socket address union to initialize
     * @param ip6_address Pointer to 16-byte IPv6 address array
     * @param port Port number in host byte order
     * @param flowinfo IPv6 flow information
     * @param scope_id IPv6 scope identifier
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_sockaddr_inet6_init(ZOO_SOCKADDR_UNION *addr,
                                           const uint8_t *ip6_address,
                                           uint16_t port,
                                           uint32_t flowinfo,
                                           uint32_t scope_id);

    /**
     * @brief Initialize socket address for any/wildcard address
     *
     * @param addr Pointer to socket address union to initialize
     * @param family Socket family (IPv4 or IPv6)
     * @param port Port number in host byte order
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_sockaddr_any_init(ZOO_SOCKADDR_UNION *addr,
                                         ZOO_SOCKET_FAMILY_ENUM family,
                                         uint16_t port);

    /**
     * @brief Initialize socket address for loopback address
     *
     * @param addr Pointer to socket address union to initialize
     * @param family Socket family (IPv4 or IPv6)
     * @param port Port number in host byte order
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_sockaddr_loopback_init(ZOO_SOCKADDR_UNION *addr,
                                              ZOO_SOCKET_FAMILY_ENUM family,
                                              uint16_t port);

    /**
     * @brief Compare two socket addresses for equality
     *
     * @param addr1 First socket address
     * @param addr2 Second socket address
     * @return true if addresses are equal, false otherwise
     */
    bool zoo_sockaddr_equal(const ZOO_SOCKADDR_UNION *addr1, const ZOO_SOCKADDR_UNION *addr2);

    // ==============================================================================
    // UTILITY FUNCTIONS
    // ==============================================================================

    /**
     * @brief Get socket error description
     *
     * @param error_code Error code from socket operations
     * @return String description of error
     */
    const char *zoo_socket_error_string(ZOO_ERROR_TYPE error_code);

    /**
     * @brief Get last socket error
     *
     * @return Last socket error code
     */
    ZOO_ERROR_TYPE zoo_socket_get_last_error(void);

    /**
     * @brief Convert hostname to IP address
     *
     * @param hostname Hostname to resolve
     * @param addr Pointer to address structure to fill
     * @param family Address family (ZOO_AF_INET or ZOO_AF_INET6)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_resolve_hostname(const char *hostname,
                                               ZOO_SOCKADDR_UNION *addr,
                                               ZOO_SOCKET_FAMILY_ENUM family);

    /**
     * @brief Check if socket subsystem is initialized
     *
     * @return true if initialized, false otherwise
     */
    bool zoo_socket_is_initialized(void);

    /**
     * @brief Get socket information
     *
     * @param socket_info Pointer to socket info structure
     * @param local_addr Pointer to store local address (can be NULL)
     * @param remote_addr Pointer to store remote address (can be NULL)
     * @return ZOO_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_socket_get_addresses(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                            ZOO_SOCKADDR_UNION *local_addr,
                                            ZOO_SOCKADDR_UNION *remote_addr);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SOCKET_H */
