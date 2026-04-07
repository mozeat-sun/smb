/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: Socket
 * Component ID: ZOO_SOCKET
 * File name: zoo_socket.c
 * Description: Cross-platform socket abstraction implementation
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-05     assistant       Initial creation
 ******************************************************************************/

#include "zoo_socket.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

#ifdef ZOO_HAS_POSIX
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#endif

#ifdef ZOO_USE_LWIP
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#endif

// ==============================================================================
// GLOBAL STATE
// ==============================================================================

static bool g_socket_initialized = false;
static ZOO_MUTEX_T g_socket_mutex;

// ==============================================================================
// PLATFORM CONVERSION FUNCTIONS
// ==============================================================================

/**
 * @brief Convert ZOO socket family to platform family
 * @param family ZOO socket family enum value
 * @return Platform-specific socket family value (AF_INET, AF_INET6, etc.)
static pthread_mutex_t g_socket_mutex = PTHREAD_MUTEX_INITIALIZER;
 */
static int zoo_to_platform_family(ZOO_SOCKET_FAMILY_ENUM family)
{
    switch (family)
    {
    case ZOO_AF_INET:
        return AF_INET;
    case ZOO_AF_INET6:
        return AF_INET6;
#ifdef AF_UNIX
    case ZOO_AF_UNIX:
        return AF_UNIX;
#endif
    default:
        return AF_UNSPEC;
    }
}

/**
 * @brief Convert ZOO socket type to platform type
 * @param type ZOO socket type enum value
 * @return Platform-specific socket type value (SOCK_STREAM, SOCK_DGRAM, etc.)
 * @retval SOCK_STREAM as default if type is not recognized
 */
static int zoo_to_platform_type(ZOO_SOCKET_TYPE_ENUM type)
{
    switch (type)
    {
    /* Thread safety: Global mutex for socket operations */
    #if defined(ZOO_HAS_THREADING)
        #include <pthread.h>
        #define ZOO_SOCKET_LOCK()   pthread_mutex_lock(&g_socket_mutex)
        #define ZOO_SOCKET_UNLOCK() pthread_mutex_unlock(&g_socket_mutex)
    #else
        #define ZOO_SOCKET_LOCK()   do {} while(0)
        #define ZOO_SOCKET_UNLOCK() do {} while(0)
    #endif
    case ZOO_SOCK_STREAM:
        return SOCK_STREAM;
    case ZOO_SOCK_DGRAM:
        return SOCK_DGRAM;
    case ZOO_SOCK_RAW:
        return SOCK_RAW;
    default:
        return SOCK_STREAM;
    }
#define ZOO_SOCKET_VALIDATE_SOCKET(sock) \
    if (sock < 0) return ZOO_ERROR_INVALID_PARAM;
#define ZOO_SOCKET_VALIDATE_BUFFER(buf, len) \
    if (!buf || len == 0) return ZOO_ERROR_INVALID_PARAM;
#define ZOO_SOCKET_VALIDATE_ADDR(addr) \
    if (!addr) return ZOO_ERROR_INVALID_PARAM;
}

/**
 * @brief Convert ZOO protocol to platform protocol
 * @param protocol ZOO socket protocol enum value
 * @return Platform-specific protocol value (IPPROTO_TCP, IPPROTO_UDP, etc.)
 * @retval 0 as default if protocol is not recognized
 */
static int zoo_to_platform_protocol(ZOO_SOCKET_PROTOCOL_ENUM protocol)
{
    switch (protocol)
    {
    case ZOO_IPPROTO_TCP:
        return IPPROTO_TCP;
    case ZOO_IPPROTO_UDP:
        return IPPROTO_UDP;
    case ZOO_IPPROTO_RAW:
        return IPPROTO_RAW;
    default:
        return 0;
    }
}

/**
 * @brief Convert platform error to ZOO error
 * @param platform_error Platform-specific error code (errno value)
 * @return ZOO error type corresponding to the platform error
 * @retval ZOO_ERROR_GENERIC if error is not recognized
 */
static ZOO_ERROR_TYPE platform_to_zoo_error(int platform_error)
{
    switch (platform_error)
    {
    case 0:
        return ZOO_OK;
    case EINVAL:
        return ZOO_ERROR_INVALID_PARAM;
    case ENOMEM:
        return ZOO_ERROR_NO_MEMORY;
    case ENOTCONN:
        return ZOO_ERROR_NOT_CONNECTED;
    case ECONNREFUSED:
        return ZOO_ERROR_CONNECTION_REFUSED;
    case ENETUNREACH:
        return ZOO_ERROR_NETWORK_UNREACHABLE;
    case EADDRINUSE:
        return ZOO_ERROR_ADDRESS_IN_USE;
    case EWOULDBLOCK:
        return ZOO_ERROR_WOULD_BLOCK;
    case EINPROGRESS:
        return ZOO_ERROR_IN_PROGRESS;
    case ETIMEDOUT:
        return ZOO_ERROR_TIMEOUT;
    default:
        return ZOO_ERROR_GENERIC;
    }
}

// ==============================================================================
// SOCKET INITIALIZATION
// ==============================================================================

/**
 * @brief Initialize the socket subsystem
 * @details Performs platform-specific socket initialization. Must be called
 *          before any other socket operations. Thread-safe and idempotent.
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_OK if initialization successful or already initialized
 */
ZOO_ERROR_TYPE zoo_socket_init(void)
{
    if (g_socket_initialized)
    {
        return ZOO_OK;
    }

#ifdef ZOO_HAS_POSIX
    // Ignore SIGPIPE signal to prevent crashes on broken connections
    signal(SIGPIPE, SIG_IGN);
#endif

#ifdef ZOO_USE_LWIP
    // lwIP initialization would go here if needed
#endif

    // Initialize mutex
    ZOO_MUTEX_INIT(&g_socket_mutex);

    g_socket_initialized = true;
    return ZOO_OK;
}

/**
 * @brief Cleanup the socket subsystem
 * @details Releases resources allocated during socket initialization.
 *          Safe to call multiple times. Should be called before program exit.
 */
void zoo_socket_cleanup(void)
{
    if (!g_socket_initialized)
    {
        return;
    }

    ZOO_MUTEX_DESTROY(&g_socket_mutex);
    g_socket_initialized = false;
}

/**
 * @brief Check if socket subsystem is initialized
 * @return true if initialized, false otherwise
 */
bool zoo_socket_is_initialized(void)
{
    return g_socket_initialized;
}

// ==============================================================================
// SOCKET MANAGEMENT
// ==============================================================================

/**
 * @brief Create a new socket
 * @param family Socket family (IPv4, IPv6, Unix domain)
 * @param type Socket type (stream, datagram, raw)
 * @param protocol Socket protocol (TCP, UDP, raw)
 * @param socket_info Pointer to socket info structure to initialize
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_NOT_INIT if socket subsystem not initialized
 * @retval ZOO_ERROR_INVALID_PARAM if socket_info is NULL
 * @retval ZOO_ERROR_GENERIC if socket creation fails
 */
ZOO_ERROR_TYPE zoo_socket_create(ZOO_SOCKET_FAMILY_ENUM family,
                                 ZOO_SOCKET_TYPE_ENUM type,
                                 ZOO_SOCKET_PROTOCOL_ENUM protocol,
                                 ZOO_SOCKET_INFO_STRUCT *socket_info)
{
    if (!g_socket_initialized)
    {
        return ZOO_ERROR_NOT_INIT;
    }

    if (!socket_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Initialize socket_info to invalid state first
    socket_info->fd = ZOO_INVALID_SOCKET;
    socket_info->family = family;
    socket_info->type = type;
    socket_info->protocol = protocol;
    socket_info->is_connected = false;
    socket_info->is_bound = false;
    socket_info->is_listening = false;
    socket_info->is_blocking = true;

    int platform_family = zoo_to_platform_family(family);
    int platform_type = zoo_to_platform_type(type);
    int platform_protocol = zoo_to_platform_protocol(protocol);

#ifdef ZOO_HAS_POSIX
    int sock = socket(platform_family, platform_type, platform_protocol);
    if (sock < 0)
    {
        return platform_to_zoo_error(errno);
    }
    socket_info->fd = sock;
#endif

#ifdef ZOO_USE_LWIP
    int sock = lwip_socket(platform_family, platform_type, platform_protocol);
    if (sock < 0)
    {
        return ZOO_ERROR_GENERIC;
    }
    socket_info->fd = sock;
#endif

    return ZOO_OK;
}

/**
 * @brief Close a socket and release associated resources
 * @param socket_info Pointer to socket info structure
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if socket_info is NULL
 */
ZOO_ERROR_TYPE zoo_socket_close(ZOO_SOCKET_INFO_STRUCT *socket_info)
{
    if (!socket_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Check if socket is already closed
    if (socket_info->fd < 0 || socket_info->fd == ZOO_INVALID_SOCKET)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

#ifdef ZOO_HAS_POSIX
    close(socket_info->fd);
    socket_info->fd = ZOO_INVALID_SOCKET;
#endif

#ifdef ZOO_USE_LWIP
    lwip_close(socket_info->fd);
    socket_info->fd = ZOO_INVALID_SOCKET;
#endif

    socket_info->is_connected = false;
    socket_info->is_listening = false;

    return ZOO_OK;
}

// ==============================================================================
// SOCKET OPERATIONS
// ==============================================================================

/**
 * @brief Bind a socket to a local address
 * @param socket_info Pointer to socket info structure
 * @param addr Pointer to socket address union containing bind address
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 * @retval ZOO_ERROR_ADDRESS_IN_USE if address is already in use
 */
ZOO_ERROR_TYPE zoo_socket_bind(ZOO_SOCKET_INFO_STRUCT *socket_info,
                               const ZOO_SOCKADDR_UNION *addr)
{
    if (!socket_info || !addr)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    int result;
#ifdef ZOO_HAS_POSIX
    result = bind(socket_info->fd,
                  (const struct sockaddr *)&addr->storage,
                  sizeof(struct sockaddr));
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_bind(socket_info->fd,
                       (const struct sockaddr *)&addr->storage,
                       sizeof(struct sockaddr));
#endif

    if (result == 0)
    {
        socket_info->is_bound = true;
        return ZOO_OK;
    }
    return platform_to_zoo_error(errno);
}

/**
 * @brief Listen for incoming connections on a socket
 * @param socket_info Pointer to socket info structure
 * @param backlog Maximum number of pending connections
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if socket_info is NULL
 */
ZOO_ERROR_TYPE zoo_socket_listen(ZOO_SOCKET_INFO_STRUCT *socket_info, int backlog)
{
    if (!socket_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    int result;
#ifdef ZOO_HAS_POSIX
    result = listen(socket_info->fd, backlog);
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_listen(socket_info->fd, backlog);
#endif

    if (result == 0)
    {
        socket_info->is_listening = true;
    }

    return (result == 0) ? ZOO_OK : platform_to_zoo_error(errno);
}

/**
 * @brief Accept an incoming connection on a listening socket
 * @param server_info Pointer to listening server socket info
 * @param client_info Pointer to client socket info structure to initialize
 * @param client_addr Optional pointer to store client address (can be NULL)
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if server_info or client_info is NULL
 * @retval ZOO_ERROR_WOULD_BLOCK if socket is non-blocking and no connection available
 */
ZOO_ERROR_TYPE zoo_socket_accept(ZOO_SOCKET_INFO_STRUCT *server_info,
                                 ZOO_SOCKET_INFO_STRUCT *client_info,
                                 ZOO_SOCKADDR_UNION *client_addr)
{
    if (!server_info || !client_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    socklen_t addr_len = sizeof(client_addr->storage);
    int client_sock;

#ifdef ZOO_HAS_POSIX
    client_sock = accept(server_info->fd,
                         client_addr ? (struct sockaddr *)&client_addr->storage : NULL,
                         client_addr ? &addr_len : NULL);
#endif

#ifdef ZOO_USE_LWIP
    client_sock = lwip_accept(server_info->fd,
                              client_addr ? (struct sockaddr *)&client_addr->storage : NULL,
                              client_addr ? &addr_len : NULL);
#endif

    if (client_sock < 0)
    {
        return platform_to_zoo_error(errno);
    }

    // Initialize client socket info
    client_info->fd = client_sock;
    client_info->family = server_info->family;
    client_info->type = server_info->type;
    client_info->protocol = server_info->protocol;
    client_info->is_connected = true;
    client_info->is_listening = false;
    client_info->is_blocking = true;

    return ZOO_OK;
}

/**
 * @brief Connect a socket to a remote address
 * @param socket_info Pointer to socket info structure
 * @param addr Pointer to socket address union containing remote address
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 * @retval ZOO_ERROR_CONNECTION_REFUSED if connection is refused
 * @retval ZOO_ERROR_NETWORK_UNREACHABLE if network is unreachable
 * @retval ZOO_ERROR_IN_PROGRESS if connection is in progress (non-blocking)
 */
ZOO_ERROR_TYPE zoo_socket_connect(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                  const ZOO_SOCKADDR_UNION *addr)
{
    if (!socket_info || !addr)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Check for valid file descriptor
    if (socket_info->fd < 0 || socket_info->fd == ZOO_INVALID_SOCKET)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    int result;
#ifdef ZOO_HAS_POSIX
    result = connect(socket_info->fd,
                     (const struct sockaddr *)&addr->storage,
                     sizeof(struct sockaddr));
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_connect(socket_info->fd,
                          (const struct sockaddr *)&addr->storage,
                          sizeof(struct sockaddr));
#endif

    if (result == 0)
    {
        socket_info->is_connected = true;
    }

    return (result == 0) ? ZOO_OK : platform_to_zoo_error(errno);
}

// ==============================================================================
// DATA TRANSFER
// ==============================================================================

/**
 * @brief Send data on a connected socket
 * @param socket_info Pointer to socket info structure
 * @param data Pointer to data buffer to send
 * @param data_len Length of data to send in bytes
 * @param bytes_sent Pointer to store actual number of bytes sent
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if any parameter is NULL
 * @retval ZOO_ERROR_NOT_CONNECTED if socket is not connected
 * @retval ZOO_ERROR_WOULD_BLOCK if operation would block (non-blocking socket)
 */
ZOO_ERROR_TYPE zoo_socket_send(ZOO_SOCKET_INFO_STRUCT *socket_info,
                               const void *data,
                               size_t data_len,
                               size_t *bytes_sent)
{
    if (!socket_info || !data || !bytes_sent)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }
    ZOO_SOCKET_VALIDATE_SOCKET(socket_info->fd);
    ZOO_SOCKET_VALIDATE_BUFFER(data, data_len);

    ZOO_SOCKET_LOCK();

    if (!socket_info->is_connected)
    {
        return ZOO_ERROR_NOT_CONNECTED;
    }

    ssize_t result;
#ifdef ZOO_HAS_POSIX
    result = send(socket_info->fd, data, data_len, 0);
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_send(socket_info->fd, data, data_len, 0);
#endif

    if (result < 0)
    {
        *bytes_sent = 0;
        return platform_to_zoo_error(errno);
    }

    *bytes_sent = (size_t)result;
    return ZOO_OK;
}

/**
 * @brief Receive data from a connected socket
 * @param socket_info Pointer to socket info structure
 * @param buffer Pointer to buffer to store received data
 * @param buffer_len Size of the receive buffer in bytes
 * @param bytes_received Pointer to store actual number of bytes received
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if any parameter is NULL
 * @retval ZOO_ERROR_WOULD_BLOCK if operation would block (non-blocking socket)
 */
ZOO_ERROR_TYPE zoo_socket_recv(ZOO_SOCKET_INFO_STRUCT *socket_info,
                               void *buffer,
                               size_t buffer_len,
                               size_t *bytes_received)
{
    if (!socket_info || !buffer || !bytes_received)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }
    ZOO_SOCKET_VALIDATE_SOCKET(socket_info->fd);
    ZOO_SOCKET_VALIDATE_BUFFER(buffer, buffer_len);

    ZOO_SOCKET_LOCK();

    ssize_t result;
#ifdef ZOO_HAS_POSIX
    result = recv(socket_info->fd, buffer, buffer_len, 0);
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_recv(socket_info->fd, buffer, buffer_len, 0);
#endif

    if (result < 0)
    {
        *bytes_received = 0;
        return platform_to_zoo_error(errno);
    }

    *bytes_received = (size_t)result;
    return ZOO_OK;
}

/**
 * @brief Send data to a specific destination address (UDP)
 * @param socket_info Pointer to socket info structure
 * @param data Pointer to data buffer to send
 * @param data_len Length of data to send in bytes
 * @param dest_addr Pointer to destination socket address
 * @param bytes_sent Pointer to store actual number of bytes sent
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if any parameter is NULL
 * @retval ZOO_ERROR_WOULD_BLOCK if operation would block (non-blocking socket)
 */
ZOO_ERROR_TYPE zoo_socket_sendto(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                 const void *data,
                                 size_t data_len,
                                 const ZOO_SOCKADDR_UNION *dest_addr,
                                 size_t *bytes_sent)
{
    if (!socket_info || !data || !dest_addr || !bytes_sent)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ssize_t result;
#ifdef ZOO_HAS_POSIX
    result = sendto(socket_info->fd, data, data_len, 0,
                    (const struct sockaddr *)&dest_addr->storage,
                    sizeof(struct sockaddr));
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_sendto(socket_info->fd, data, data_len, 0,
                         (const struct sockaddr *)&dest_addr->storage,
                         sizeof(struct sockaddr));
#endif

    if (result < 0)
    {
        *bytes_sent = 0;
        return platform_to_zoo_error(errno);
    }

    *bytes_sent = (size_t)result;
    return ZOO_OK;
}

/**
 * @brief Receive data from any source address (UDP)
 * @param socket_info Pointer to socket info structure
 * @param buffer Pointer to buffer to store received data
 * @param buffer_len Size of the receive buffer in bytes
 * @param src_addr Optional pointer to store source address (can be NULL)
 * @param bytes_received Pointer to store actual number of bytes received
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if socket_info, buffer, or bytes_received is NULL
 * @retval ZOO_ERROR_WOULD_BLOCK if operation would block (non-blocking socket)
 */
ZOO_ERROR_TYPE zoo_socket_recvfrom(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                   void *buffer,
                                   size_t buffer_len,
                                   ZOO_SOCKADDR_UNION *src_addr,
                                   size_t *bytes_received)
{
    if (!socket_info || !buffer || !bytes_received)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    socklen_t addr_len = sizeof(src_addr->storage);
    ssize_t result;

#ifdef ZOO_HAS_POSIX
    result = recvfrom(socket_info->fd, buffer, buffer_len, 0,
                      src_addr ? (struct sockaddr *)&src_addr->storage : NULL,
                      src_addr ? &addr_len : NULL);
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_recvfrom(socket_info->fd, buffer, buffer_len, 0,
                           src_addr ? (struct sockaddr *)&src_addr->storage : NULL,
                           src_addr ? &addr_len : NULL);
#endif

    if (result < 0)
    {
        *bytes_received = 0;
        return platform_to_zoo_error(errno);
    }

    *bytes_received = (size_t)result;
    return ZOO_OK;
}

// ==============================================================================
// ADDRESS UTILITIES
// ==============================================================================

/**
 * @brief Initialize a socket address structure for IPv4
 * @param addr Pointer to socket address union to initialize
 * @param ip_address IPv4 address in host byte order
 * @param port Port number in host byte order
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if addr is NULL
 */
ZOO_ERROR_TYPE zoo_sockaddr_inet_init(ZOO_SOCKADDR_UNION *addr,
                                      uint32_t ip_address,
                                      uint16_t port)
{
    if (!addr)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Clear the entire union
    memset(addr, 0, sizeof(*addr));

    // Fill the platform sockaddr_in structure for socket operations
    struct sockaddr_in *sin = (struct sockaddr_in *)&addr->storage;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = htonl(ip_address);
    sin->sin_port = htons(port);

    // Fill the ZOO custom structure for application use
    addr->family = ZOO_AF_INET;
    addr->in.family = ZOO_AF_INET;
    addr->in.port = port;
    addr->in.addr = ip_address;

    return ZOO_OK;
}

/**
 * @brief Initialize a socket address structure for IPv6
 * @param addr Pointer to socket address union to initialize
 * @param ip6_address Pointer to 16-byte IPv6 address array
 * @param port Port number in host byte order
 * @param flowinfo IPv6 flow information
 * @param scope_id IPv6 scope identifier
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if addr or ip6_address is NULL
 */
ZOO_ERROR_TYPE zoo_sockaddr_inet6_init(ZOO_SOCKADDR_UNION *addr,
                                       const uint8_t *ip6_address,
                                       uint16_t port,
                                       uint32_t flowinfo,
                                       uint32_t scope_id)
{
    if (!addr || !ip6_address)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Clear the entire union first
    memset(addr, 0, sizeof(*addr));

    // Initialize platform-specific sockaddr_in6 structure
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addr->storage;
    sin6->sin6_family = AF_INET6;
    memcpy(&sin6->sin6_addr, ip6_address, 16);
    sin6->sin6_port = htons(port);
    sin6->sin6_flowinfo = flowinfo;
    sin6->sin6_scope_id = scope_id;

    // Initialize ZOO custom structure for application use
    addr->family = ZOO_AF_INET6;
    memcpy(addr->in6.addr, ip6_address, 16);
    addr->in6.port = port;
    addr->in6.flowinfo = flowinfo;
    addr->in6.scope_id = scope_id;

    return ZOO_OK;
}

/**
 * @brief Initialize a socket address structure for any address (INADDR_ANY)
 * @param addr Pointer to socket address union to initialize
 * @param family Socket family (IPv4 or IPv6)
 * @param port Port number in host byte order
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if addr is NULL
 * @retval ZOO_ERROR_NOT_SUPPORTED if family is not supported
 */
ZOO_ERROR_TYPE zoo_sockaddr_any_init(ZOO_SOCKADDR_UNION *addr,
                                     ZOO_SOCKET_FAMILY_ENUM family,
                                     uint16_t port)
{
    if (!addr)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    switch (family)
    {
    case ZOO_AF_INET:
        return zoo_sockaddr_inet_init(addr, ZOO_INADDR_ANY, port);
    case ZOO_AF_INET6:
    {
        uint8_t any_addr[16] = {0};
        return zoo_sockaddr_inet6_init(addr, any_addr, port, 0, 0);
    }
    default:
        return ZOO_ERROR_INVALID_PARAM;
    }
}

/**
 * @brief Initialize a socket address structure for loopback address
 * @param addr Pointer to socket address union to initialize
 * @param family Socket family (IPv4 or IPv6)
 * @param port Port number in host byte order
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if addr is NULL
 * @retval ZOO_ERROR_NOT_SUPPORTED if family is not supported
 */
ZOO_ERROR_TYPE zoo_sockaddr_loopback_init(ZOO_SOCKADDR_UNION *addr,
                                          ZOO_SOCKET_FAMILY_ENUM family,
                                          uint16_t port)
{
    if (!addr)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    switch (family)
    {
    case ZOO_AF_INET:
        return zoo_sockaddr_inet_init(addr, ZOO_INADDR_LOOPBACK, port);
    case ZOO_AF_INET6:
    {
        uint8_t loopback_addr[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        return zoo_sockaddr_inet6_init(addr, loopback_addr, port, 0, 0);
    }
    default:
        return ZOO_ERROR_INVALID_PARAM;
    }
}

// ==============================================================================
// ERROR HANDLING
// ==============================================================================

/**
 * @brief Get the last socket error that occurred
 * @return ZOO error code representing the last error
 * @retval ZOO_ERROR_GENERIC if no specific error mapping available
 */
ZOO_ERROR_TYPE zoo_socket_get_last_error(void)
{
#ifdef ZOO_HAS_POSIX
    return platform_to_zoo_error(errno);
#endif
#ifdef ZOO_USE_LWIP
    return ZOO_ERROR_GENERIC; // lwIP doesn't have errno equivalent
#endif
    return ZOO_ERROR_GENERIC;
}

/**
 * @brief Get human-readable string description of a ZOO error code
 * @param error ZOO error code to convert to string
 * @return Pointer to static string describing the error
 * @retval "Unknown error" if error code is not recognized
 */
const char *zoo_socket_error_string(ZOO_ERROR_TYPE error)
{
    switch (error)
    {
    case ZOO_OK:
        return "Success";
    case ZOO_ERROR_GENERIC:
        return "Generic error";
    case ZOO_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case ZOO_ERROR_NO_MEMORY:
        return "Out of memory";
    case ZOO_ERROR_NOT_SUPPORTED:
        return "Operation not supported";
    case ZOO_ERROR_TIMEOUT:
        return "Timeout";
    case ZOO_ERROR_NETWORK:
        return "Network error";
    case ZOO_ERROR_ALREADY_INIT:
        return "Already initialized";
    case ZOO_ERROR_NOT_INIT:
        return "Not initialized";
    case ZOO_ERROR_NOT_CONNECTED:
        return "Not connected";
    case ZOO_ERROR_CONNECTION_REFUSED:
        return "Connection refused";
    case ZOO_ERROR_NETWORK_UNREACHABLE:
        return "Network unreachable";
    case ZOO_ERROR_ADDRESS_IN_USE:
        return "Address in use";
    case ZOO_ERROR_WOULD_BLOCK:
        return "Operation would block";
    case ZOO_ERROR_IN_PROGRESS:
        return "Operation in progress";
    default:
        return "Unknown error";
    }
}

// ==============================================================================
// SOCKET OPTIONS
// ==============================================================================

/**
 * @brief Set socket option
 * @param socket_info Pointer to socket info structure
 * @param level Protocol level (SOL_SOCKET, IPPROTO_TCP, etc.)
 * @param option Option to set
 * @param value Pointer to option value
 * @param value_len Length of option value
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 * @retval ZOO_ERROR_NOT_SUPPORTED if option not supported
 */
ZOO_ERROR_TYPE zoo_socket_setsockopt(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                     ZOO_SOCKET_LEVEL_ENUM level,
                                     ZOO_SOCKET_OPTION_ENUM option,
                                     const void *value,
                                     size_t value_len)
{
    if (!socket_info || !value)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Convert ZOO enums to platform values
    int platform_level;
    int platform_option;

    // Convert level
    switch (level)
    {
    case ZOO_SOL_SOCKET:
        platform_level = SOL_SOCKET;
        break;
    default:
        return ZOO_ERROR_NOT_SUPPORTED;
    }

    // Convert option
    switch (option)
    {
    case ZOO_SO_REUSEADDR:
        platform_option = SO_REUSEADDR;
        break;
    case ZOO_SO_KEEPALIVE:
        platform_option = SO_KEEPALIVE;
        break;
    case ZOO_SO_RCVBUF:
        platform_option = SO_RCVBUF;
        break;
    case ZOO_SO_SNDBUF:
        platform_option = SO_SNDBUF;
        break;
    default:
        return ZOO_ERROR_NOT_SUPPORTED;
    }

    int result;
#ifdef ZOO_HAS_POSIX
    result = setsockopt(socket_info->fd, platform_level, platform_option, value, (socklen_t)value_len);
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_setsockopt(socket_info->fd, platform_level, platform_option, value, (socklen_t)value_len);
#endif

    return (result == 0) ? ZOO_OK : platform_to_zoo_error(errno);
}

/**
 * @brief Get socket option
 * @param socket_info Pointer to socket info structure
 * @param level Protocol level (SOL_SOCKET, IPPROTO_TCP, etc.)
 * @param option Option to get
 * @param value Pointer to buffer for option value
 * @param value_len Pointer to length of option value buffer
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 * @retval ZOO_ERROR_NOT_SUPPORTED if option not supported
 */
ZOO_ERROR_TYPE zoo_socket_getsockopt(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                     ZOO_SOCKET_LEVEL_ENUM level,
                                     ZOO_SOCKET_OPTION_ENUM option,
                                     void *value,
                                     size_t *value_len)
{
    if (!socket_info || !value || !value_len)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Convert ZOO enums to platform values
    int platform_level;
    int platform_option;

    // Convert level
    switch (level)
    {
    case ZOO_SOL_SOCKET:
        platform_level = SOL_SOCKET;
        break;
    default:
        return ZOO_ERROR_NOT_SUPPORTED;
    }

    // Convert option
    switch (option)
    {
    case ZOO_SO_REUSEADDR:
        platform_option = SO_REUSEADDR;
        break;
    case ZOO_SO_KEEPALIVE:
        platform_option = SO_KEEPALIVE;
        break;
    case ZOO_SO_RCVBUF:
        platform_option = SO_RCVBUF;
        break;
    case ZOO_SO_SNDBUF:
        platform_option = SO_SNDBUF;
        break;
    default:
        return ZOO_ERROR_NOT_SUPPORTED;
    }

    socklen_t len = (socklen_t)*value_len;
    int result;
#ifdef ZOO_HAS_POSIX
    result = getsockopt(socket_info->fd, platform_level, platform_option, value, &len);
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_getsockopt(socket_info->fd, platform_level, platform_option, value, &len);
#endif

    *value_len = (size_t)len;
    return (result == 0) ? ZOO_OK : platform_to_zoo_error(errno);
}

/**
 * @brief Set socket blocking mode
 * @param socket_info Pointer to socket info structure
 * @param blocking true for blocking, false for non-blocking
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if socket_info is NULL
 */
ZOO_ERROR_TYPE zoo_socket_set_blocking(ZOO_SOCKET_INFO_STRUCT *socket_info, bool blocking)
{
    if (!socket_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

#ifdef ZOO_HAS_POSIX
    int flags = fcntl(socket_info->fd, F_GETFL, 0);
    if (flags < 0)
    {
        return platform_to_zoo_error(errno);
    }

    if (blocking)
    {
        flags &= ~O_NONBLOCK;
    }
    else
    {
        flags |= O_NONBLOCK;
    }

    if (fcntl(socket_info->fd, F_SETFL, flags) < 0)
    {
        return platform_to_zoo_error(errno);
    }
#endif

#ifdef ZOO_USE_LWIP
    int opt = blocking ? 0 : 1;
    if (lwip_ioctl(socket_info->fd, FIONBIO, &opt) < 0)
    {
        return ZOO_ERROR_GENERIC;
    }
#endif

    socket_info->is_blocking = blocking;
    return ZOO_OK;
}

/**
 * @brief Monitor socket for I/O events
 * @param socket_info Pointer to socket info structure
 * @param check_read Check for read readiness
 * @param check_write Check for write readiness
 * @param check_error Check for error conditions
 * @param timeout_ms Timeout in milliseconds (0 for non-blocking, -1 for infinite)
 * @param ready_read Output: true if ready for read
 * @param ready_write Output: true if ready for write
 * @param has_error Output: true if has error
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 * @retval ZOO_ERROR_TIMEOUT if timeout occurred
 */
ZOO_ERROR_TYPE zoo_socket_select(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                 bool check_read,
                                 bool check_write,
                                 bool check_error,
                                 int timeout_ms,
                                 bool *ready_read,
                                 bool *ready_write,
                                 bool *has_error)
{
    if (!socket_info || !ready_read || !ready_write || !has_error)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Check for valid file descriptor to prevent buffer overflow
    if (socket_info->fd < 0 || socket_info->fd == ZOO_INVALID_SOCKET)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    *ready_read = false;
    *ready_write = false;
    *has_error = false;

#ifdef ZOO_HAS_POSIX
    fd_set read_fds, write_fds, error_fds;
    struct timeval tv;
    struct timeval *tv_ptr = NULL;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);

    if (check_read)
        FD_SET(socket_info->fd, &read_fds);
    if (check_write)
        FD_SET(socket_info->fd, &write_fds);
    if (check_error)
        FD_SET(socket_info->fd, &error_fds);

    if (timeout_ms >= 0)
    {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tv_ptr = &tv;
    }

    int result = select(socket_info->fd + 1,
                        check_read ? &read_fds : NULL,
                        check_write ? &write_fds : NULL,
                        check_error ? &error_fds : NULL,
                        tv_ptr);

    if (result < 0)
    {
        return platform_to_zoo_error(errno);
    }
    else if (result == 0)
    {
        return ZOO_ERROR_TIMEOUT;
    }

    *ready_read = check_read && FD_ISSET(socket_info->fd, &read_fds);
    *ready_write = check_write && FD_ISSET(socket_info->fd, &write_fds);
    *has_error = check_error && FD_ISSET(socket_info->fd, &error_fds);
#endif

#ifdef ZOO_USE_LWIP
    // lwIP select implementation would go here
    return ZOO_ERROR_NOT_SUPPORTED;
#endif

    return ZOO_OK;
}

/**
 * @brief Compare two socket addresses for equality
 * @param addr1 First socket address
 * @param addr2 Second socket address
 * @return true if addresses are equal, false otherwise
 */
bool zoo_sockaddr_equal(const ZOO_SOCKADDR_UNION *addr1, const ZOO_SOCKADDR_UNION *addr2)
{
    if (!addr1 || !addr2)
    {
        return false;
    }

    if (addr1->family != addr2->family)
    {
        return false;
    }

    switch (addr1->family)
    {
    case ZOO_AF_INET:
    {
        const struct sockaddr_in *sin1 = (const struct sockaddr_in *)&addr1->storage;
        const struct sockaddr_in *sin2 = (const struct sockaddr_in *)&addr2->storage;
        return (sin1->sin_addr.s_addr == sin2->sin_addr.s_addr &&
                sin1->sin_port == sin2->sin_port);
    }
    case ZOO_AF_INET6:
    {
        const struct sockaddr_in6 *sin6_1 = (const struct sockaddr_in6 *)&addr1->storage;
        const struct sockaddr_in6 *sin6_2 = (const struct sockaddr_in6 *)&addr2->storage;
        return (memcmp(&sin6_1->sin6_addr, &sin6_2->sin6_addr, 16) == 0 &&
                sin6_1->sin6_port == sin6_2->sin6_port &&
                sin6_1->sin6_flowinfo == sin6_2->sin6_flowinfo &&
                sin6_1->sin6_scope_id == sin6_2->sin6_scope_id);
    }
    default:
        return false;
    }
}

// ==============================================================================
// ADDITIONAL SOCKET FUNCTIONS
// ==============================================================================

/**
 * @brief Shutdown part of a socket connection
 * @param socket_info Pointer to socket info structure
 * @param how How to shutdown (ZOO_SHUT_RD, ZOO_SHUT_WR, ZOO_SHUT_RDWR)
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if socket_info is NULL or invalid
 */
ZOO_ERROR_TYPE zoo_socket_shutdown(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                   ZOO_SOCKET_SHUTDOWN_ENUM how)
{
    if (!socket_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Check for valid file descriptor
    if (socket_info->fd < 0 || socket_info->fd == ZOO_INVALID_SOCKET)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    int platform_how;
    switch (how)
    {
    case ZOO_SHUT_RD:
        platform_how = SHUT_RD;
        break;
    case ZOO_SHUT_WR:
        platform_how = SHUT_WR;
        break;
    case ZOO_SHUT_RDWR:
        platform_how = SHUT_RDWR;
        break;
    default:
        return ZOO_ERROR_INVALID_PARAM;
    }

    int result;
#ifdef ZOO_HAS_POSIX
    result = shutdown(socket_info->fd, platform_how);
#endif

#ifdef ZOO_USE_LWIP
    result = lwip_shutdown(socket_info->fd, platform_how);
#endif

    return (result == 0) ? ZOO_OK : platform_to_zoo_error(errno);
}

// ==============================================================================
// ADDRESS MANIPULATION FUNCTIONS
// ==============================================================================

/**
 * @brief Create IPv4 address structure from string
 * @param addr Pointer to address structure to fill
 * @param ip_str IP address string (e.g., "192.168.1.1")
 * @param port Port number in host byte order
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 */
ZOO_ERROR_TYPE zoo_sockaddr_ipv4(ZOO_SOCKADDR_UNION *addr, const char *ip_str, uint16_t port)
{
    if (!addr || !ip_str)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

#ifdef ZOO_HAS_POSIX
    struct in_addr inaddr;
    if (inet_aton(ip_str, &inaddr) == 0)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }
    return zoo_sockaddr_inet_init(addr, ntohl(inaddr.s_addr), port);
#endif

#ifdef ZOO_USE_LWIP
    ip4_addr_t ip4_addr;
    if (!ip4addr_aton(ip_str, &ip4_addr))
    {
        return ZOO_ERROR_INVALID_PARAM;
    }
    return zoo_sockaddr_inet_init(addr, ntohl(ip4_addr.addr), port);
#endif

    return ZOO_ERROR_NOT_SUPPORTED;
}

/**
 * @brief Create IPv6 address structure from string
 * @param addr Pointer to address structure to fill
 * @param ip_str IPv6 address string (e.g., "::1")
 * @param port Port number in host byte order
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 */
ZOO_ERROR_TYPE zoo_sockaddr_ipv6(ZOO_SOCKADDR_UNION *addr, const char *ip_str, uint16_t port)
{
    if (!addr || !ip_str)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

#ifdef ZOO_HAS_POSIX
    struct in6_addr in6addr;
    if (inet_pton(AF_INET6, ip_str, &in6addr) != 1)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }
    return zoo_sockaddr_inet6_init(addr, (const uint8_t *)&in6addr, port, 0, 0);
#endif

#ifdef ZOO_USE_LWIP
    ip6_addr_t ip6_addr;
    if (!ip6addr_aton(ip_str, &ip6_addr))
    {
        return ZOO_ERROR_INVALID_PARAM;
    }
    return zoo_sockaddr_inet6_init(addr, (const uint8_t *)&ip6_addr.addr, port, 0, 0);
#endif

    return ZOO_ERROR_NOT_SUPPORTED;
}

/**
 * @brief Convert address structure to string representation
 * @param addr Pointer to address structure
 * @param buffer Buffer to store string representation
 * @param buffer_size Size of buffer
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL or buffer too small
 */
ZOO_ERROR_TYPE zoo_sockaddr_to_string(const ZOO_SOCKADDR_UNION *addr, char *buffer, size_t buffer_size)
{
    if (!addr || !buffer || buffer_size == 0)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    switch (addr->family)
    {
    case ZOO_AF_INET:
    {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)&addr->storage;
        if (buffer_size < INET_ADDRSTRLEN + 10) // IP + ":" + port + null
        {
            return ZOO_ERROR_INVALID_PARAM;
        }
#ifdef ZOO_HAS_POSIX
        char ip_str[INET_ADDRSTRLEN];
        if (!inet_ntop(AF_INET, &sin->sin_addr, ip_str, INET_ADDRSTRLEN))
        {
            return platform_to_zoo_error(errno);
        }
        snprintf(buffer, buffer_size, "%s:%u", ip_str, ntohs(sin->sin_port));
#endif
        return ZOO_OK;
    }
    case ZOO_AF_INET6:
    {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)&addr->storage;
        if (buffer_size < INET6_ADDRSTRLEN + 15) // "[" + IP + "]:" + port + null
        {
            return ZOO_ERROR_INVALID_PARAM;
        }
#ifdef ZOO_HAS_POSIX
        char ip_str[INET6_ADDRSTRLEN];
        if (!inet_ntop(AF_INET6, &sin6->sin6_addr, ip_str, INET6_ADDRSTRLEN))
        {
            return platform_to_zoo_error(errno);
        }
        snprintf(buffer, buffer_size, "[%s]:%u", ip_str, ntohs(sin6->sin6_port));
#endif
        return ZOO_OK;
    }
    default:
        return ZOO_ERROR_NOT_SUPPORTED;
    }
}

/**
 * @brief Get port number from address structure
 * @param addr Pointer to address structure
 * @return Port number in host byte order, 0 on error
 */
uint16_t zoo_sockaddr_get_port(const ZOO_SOCKADDR_UNION *addr)
{
    if (!addr)
    {
        return 0;
    }

    switch (addr->family)
    {
        case ZOO_AF_INET:
        {
            const struct sockaddr_in *sin = (const struct sockaddr_in *)&addr->storage;
            return ntohs(sin->sin_port);
        }
        case ZOO_AF_INET6:
        {
            const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)&addr->storage;
            return ntohs(sin6->sin6_port);
        }
        default:
            return 0;
    }
}

/**
 * @brief Set port number in address structure
 * @param addr Pointer to address structure
 * @param port Port number in host byte order
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if addr is NULL
 * @retval ZOO_ERROR_NOT_SUPPORTED if address family not supported
 */
ZOO_ERROR_TYPE zoo_sockaddr_set_port(ZOO_SOCKADDR_UNION *addr, uint16_t port)
{
    if (!addr)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    switch (addr->family)
    {
        case ZOO_AF_INET:
        {
            struct sockaddr_in *sin = (struct sockaddr_in *)&addr->storage;
            sin->sin_port = htons(port);
            addr->in.port = port;
            return ZOO_OK;
        }
        case ZOO_AF_INET6:
        {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addr->storage;
            sin6->sin6_port = htons(port);
            addr->in6.port = port;
            return ZOO_OK;
        }
        default:
            return ZOO_ERROR_NOT_SUPPORTED;
    }
}

/**
 * @brief Convert hostname to IP address
 * @param hostname Hostname to resolve
 * @param addr Pointer to address structure to fill
 * @param family Address family (ZOO_AF_INET or ZOO_AF_INET6)
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if parameters are NULL
 * @retval ZOO_ERROR_NOT_SUPPORTED if family not supported
 */
ZOO_ERROR_TYPE zoo_socket_resolve_hostname(const char *hostname,
                                           ZOO_SOCKADDR_UNION *addr,
                                           ZOO_SOCKET_FAMILY_ENUM family)
{
    if (!hostname || !addr)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

#ifdef ZOO_HAS_POSIX
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));

    switch (family)
    {
    case ZOO_AF_INET:
        hints.ai_family = AF_INET;
        break;
    case ZOO_AF_INET6:
        hints.ai_family = AF_INET6;
        break;
    default:
        return ZOO_ERROR_NOT_SUPPORTED;
    }

    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(hostname, NULL, &hints, &result);
    if (ret != 0)
    {
        return ZOO_ERROR_NETWORK;
    }

    // Copy the first result
    memcpy(&addr->storage, result->ai_addr, result->ai_addrlen);
    addr->family = family;

    freeaddrinfo(result);
    return ZOO_OK;
#endif

#ifdef ZOO_USE_LWIP
    // lwIP hostname resolution would go here
    return ZOO_ERROR_NOT_SUPPORTED;
#endif

    return ZOO_ERROR_NOT_SUPPORTED;
}

/**
 * @brief Get socket local and remote addresses
 * @param socket_info Pointer to socket info structure
 * @param local_addr Pointer to store local address (can be NULL)
 * @param remote_addr Pointer to store remote address (can be NULL)
 * @return ZOO_OK on success, error code on failure
 * @retval ZOO_ERROR_INVALID_PARAM if socket_info is NULL or invalid
 */
ZOO_ERROR_TYPE zoo_socket_get_addresses(ZOO_SOCKET_INFO_STRUCT *socket_info,
                                        ZOO_SOCKADDR_UNION *local_addr,
                                        ZOO_SOCKADDR_UNION *remote_addr)
{
    if (!socket_info)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    // Check for valid file descriptor
    if (socket_info->fd < 0 || socket_info->fd == ZOO_INVALID_SOCKET)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

#ifdef ZOO_HAS_POSIX
    if (local_addr)
    {
        socklen_t len = sizeof(local_addr->storage);
        if (getsockname(socket_info->fd, (struct sockaddr *)&local_addr->storage, &len) != 0)
        {
            return platform_to_zoo_error(errno);
        }
        // Set family based on socket address family
        struct sockaddr *sa = (struct sockaddr *)&local_addr->storage;
        local_addr->family = (sa->sa_family == AF_INET) ? ZOO_AF_INET : (sa->sa_family == AF_INET6) ? ZOO_AF_INET6
                                                                                                    : ZOO_AF_UNSPEC;
    }

    if (remote_addr)
    {
        socklen_t len = sizeof(remote_addr->storage);
        if (getpeername(socket_info->fd, (struct sockaddr *)&remote_addr->storage, &len) != 0)
        {
            return platform_to_zoo_error(errno);
        }
        // Set family based on socket address family
        struct sockaddr *sa = (struct sockaddr *)&remote_addr->storage;
        remote_addr->family = (sa->sa_family == AF_INET) ? ZOO_AF_INET : (sa->sa_family == AF_INET6) ? ZOO_AF_INET6
                                                                                                     : ZOO_AF_UNSPEC;
    }
#endif

#ifdef ZOO_USE_LWIP
    // lwIP address retrieval would go here
    return ZOO_ERROR_NOT_SUPPORTED;
#endif

    return ZOO_OK;
}
