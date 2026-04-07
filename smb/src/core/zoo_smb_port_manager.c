/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_PORT_MANAGER
 * File name: zoo_smb_port_manager.c
 * Description: Implementation of port manager for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-30     weiwang.sun       created
 * 1.1       2025-06-19     weiwang.sun       refactored large functions
 ******************************************************************************/

#include "zoo_smb_port_manager.h"
#include "../../log/inc/zoo_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT_TABLE_SIZE (MAX_TRANSPORT_PORT - MIN_TRANSPORT_PORT + 1)
#define LOCK_FILE_DIR "/var/lock"
#define SHM_NAME "/zoo_smb_port_table"
#define PORT_TABLE_FILE "/tmp/zoo_smb_port_table"

/**
 * @brief Structure for port usage table.
 *
 * This structure keeps track of which process is using which port,
 * protected by a mutex for multi-process safety.
 */
typedef struct
{
    pid_t pids[PORT_TABLE_SIZE]; /**< Array of PIDs occupying each port */
    pthread_mutex_t mutex;       /**< Mutex to protect shared resources */
    int initialized;             /**< Initialization flag */
} ZOO_SMB_PORT_TABLE_STRUCT;

/* Internal function declarations */
static ZOO_SMB_PORT_TABLE_STRUCT* init_port_table(void);
static int lock_port(int port);
static void unlock_port(int lock_fd, int port);
static int acquire_port(ZOO_SMB_PORT_TABLE_STRUCT* table, int port);
static void release_port(ZOO_SMB_PORT_TABLE_STRUCT* table, int port);
static int is_port_in_use(int port);

/**
 * @brief Creates and opens the port table file
 *
 * This function creates the port table file with appropriate permissions
 * and sets its size to accommodate the port table structure.
 *
 * @return File descriptor on success, -1 on failure
 *
 * @note Creates file with 0666 permissions
 * @note Sets file size using ftruncate
 */
static int create_port_table_file(void)
{
    int fd = open(PORT_TABLE_FILE, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
    {
        ZOO_LOG_ERROR("Failed to create port table file: %s", strerror(errno));
        return -1;
    }

    if (ftruncate(fd, sizeof(ZOO_SMB_PORT_TABLE_STRUCT)) == -1)
    {
        ZOO_LOG_ERROR("Failed to set port table file size: %s", strerror(errno));
        close(fd);
        return -1;
    }

    ZOO_LOG_DEBUG("Port table file created successfully: %s", PORT_TABLE_FILE);
    return fd;
}

/**
 * @brief Maps port table file to memory
 *
 * This function creates a shared memory mapping of the port table file
 * for inter-process communication.
 *
 * @param fd File descriptor of the port table file
 * @return Pointer to mapped memory on success, NULL on failure
 *
 * @note Uses MAP_SHARED for inter-process visibility
 * @note Closes file descriptor after mapping
 */
static ZOO_SMB_PORT_TABLE_STRUCT* map_port_table_file(int fd)
{
    ZOO_SMB_PORT_TABLE_STRUCT* table = mmap(NULL, sizeof(ZOO_SMB_PORT_TABLE_STRUCT), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (table == MAP_FAILED)
    {
        ZOO_LOG_ERROR("Failed to map port table file to memory: %s", strerror(errno));
        return NULL;
    }

    ZOO_LOG_DEBUG("Port table file mapped to memory successfully");
    return table;
}

/**
 * @brief Initializes mutex attributes for inter-process sharing
 *
 * This function configures mutex attributes to enable sharing between
 * multiple processes.
 *
 * @param attr Pointer to mutex attributes structure
 * @return ZOO_TRUE on success, ZOO_FALSE on failure
 *
 * @note Sets PTHREAD_PROCESS_SHARED attribute
 * @note Handles attribute cleanup on failure
 */
static ZOO_BOOL init_mutex_attributes(pthread_mutexattr_t* attr)
{
    if (pthread_mutexattr_init(attr) != 0)
    {
        ZOO_LOG_ERROR("Failed to initialize mutex attributes");
        return ZOO_FALSE;
    }

    if (pthread_mutexattr_setpshared(attr, PTHREAD_PROCESS_SHARED) != 0)
    {
        ZOO_LOG_ERROR("Failed to set mutex process-shared attribute");
        pthread_mutexattr_destroy(attr);
        return ZOO_FALSE;
    }

    return ZOO_TRUE;
}

/**
 * @brief Initializes port table structure for first use
 *
 * This function performs first-time initialization of the port table
 * including mutex setup and PID array clearing.
 *
 * @param table Pointer to the port table structure
 * @return ZOO_TRUE on success, ZOO_FALSE on failure
 *
 * @note Only initializes if table->initialized is ZOO_FALSE
 * @note Sets up inter-process mutex
 */
static ZOO_BOOL initialize_port_table_structure(ZOO_SMB_PORT_TABLE_STRUCT* table)
{
    if (table->initialized)
    {
        ZOO_LOG_DEBUG("Port table already initialized");
        return ZOO_TRUE;
    }

    pthread_mutexattr_t attr;
    if (!init_mutex_attributes(&attr))
    {
        return ZOO_FALSE;
    }

    if (pthread_mutex_init(&table->mutex, &attr) != 0)
    {
        ZOO_LOG_ERROR("Failed to initialize port table mutex");
        pthread_mutexattr_destroy(&attr);
        return ZOO_FALSE;
    }

    pthread_mutexattr_destroy(&attr);
    memset(table->pids, 0, sizeof(table->pids));
    table->initialized = 1;

    ZOO_LOG_DEBUG("Port table structure initialized successfully");
    return ZOO_TRUE;
}

/**
 * @brief Initialize the port usage table in shared memory.
 *
 * This function creates or opens the shared port table file and maps it
 * to memory for inter-process port management.
 *
 * @return Pointer to the ZOO_SMB_PORT_TABLE_STRUCT structure, or NULL on failure.
 *
 * @note Creates shared memory mapping for inter-process communication
 * @note Initializes mutex and data structures on first creation
 */
static ZOO_SMB_PORT_TABLE_STRUCT* init_port_table(void)
{
    int fd = create_port_table_file();
    if (fd < 0)
    {
        return NULL;
    }

    ZOO_SMB_PORT_TABLE_STRUCT* table = map_port_table_file(fd);
    if (!table)
    {
        return NULL;
    }

    if (!initialize_port_table_structure(table))
    {
        munmap(table, sizeof(ZOO_SMB_PORT_TABLE_STRUCT));
        return NULL;
    }
    return table;
}

/**
 * @brief Creates lock file path for the specified port
 *
 * This function generates the full path for the port lock file
 * and ensures the lock directory exists.
 *
 * @param port Port number
 * @param lockfile Buffer to store the lock file path
 * @param buffer_size Size of the lockfile buffer
 * @return ZOO_TRUE on success, ZOO_FALSE on failure
 *
 * @note Creates lock directory if it doesn't exist
 * @note Uses LOCK_FILE_DIR as base directory
 */
static ZOO_BOOL create_lock_file_path(int port, char* lockfile, size_t buffer_size)
{
    if (mkdir(LOCK_FILE_DIR, 0755) != 0 && errno != EEXIST)
    {
        ZOO_LOG_ERROR("Failed to create lock directory %s: %s",
                          LOCK_FILE_DIR,
                          strerror(errno));
        return ZOO_FALSE;
    }

    int ret = snprintf(lockfile, buffer_size, "%s/zoo_smb_port_%d.lock", LOCK_FILE_DIR, port);
    if (ret >= (int)buffer_size)
    {
        ZOO_LOG_ERROR("Lock file path too long for port %d", port);
        return ZOO_FALSE;
    }

    return ZOO_TRUE;
}

/**
 * @brief Acquires file lock on the port lock file
 *
 * This function attempts to acquire an exclusive write lock on the
 * port lock file using fcntl.
 *
 * @param fd File descriptor of the lock file
 * @return ZOO_TRUE if lock acquired, ZOO_FALSE otherwise
 *
 * @note Uses non-blocking lock acquisition
 * @note Handles EAGAIN/EACCES as expected failures
 */
static ZOO_BOOL acquire_file_lock(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
        if (errno == EAGAIN || errno == EACCES)
        {
            ZOO_LOG_DEBUG("Port lock already held by another process");
            return ZOO_FALSE;
        }
        ZOO_LOG_ERROR("Failed to acquire file lock: %s", strerror(errno));
        return ZOO_FALSE;
    }

    return ZOO_TRUE;
}

/**
 * @brief Writes process ID to lock file
 *
 * This function writes the current process ID to the lock file
 * for debugging and monitoring purposes.
 *
 * @param fd File descriptor of the lock file
 *
 * @note Writes PID as text string with newline
 * @note Logs warning if write fails but doesn't treat as error
 */
static void write_pid_to_lock_file(int fd)
{
    char pid_str[16];
    int len = snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());

    if (write(fd, pid_str, len) != len)
    {
        ZOO_LOG_WARN("Failed to write PID to lock file: %s", strerror(errno));
    }
}

/**
 * @brief Lock a port using a file lock.
 *
 * This function creates a lock file for the specified port and acquires
 * an exclusive lock on it to prevent other processes from using the port.
 *
 * @param port The port number to lock.
 * @return File descriptor of the lock file on success, ZOO_SMB_INVALID_PORT on failure.
 *
 * @note Lock is released when file descriptor is closed
 * @note Creates lock directory if it doesn't exist
 * @note Writes PID to lock file for debugging
 */
static int lock_port(int port)
{
    ZOO_LOG_DEBUG("Attempting to lock port %d", port);

    char lockfile[128];
    if (!create_lock_file_path(port, lockfile, sizeof(lockfile)))
    {
        return ZOO_SMB_INVALID_PORT;
    }

    int fd = open(lockfile, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
    {
        ZOO_LOG_ERROR("Failed to create lock file %s: %s",
                          lockfile,
                          strerror(errno));
        return ZOO_SMB_INVALID_PORT;
    }

    if (!acquire_file_lock(fd))
    {
        close(fd);
        return ZOO_SMB_INVALID_PORT;
    }

    write_pid_to_lock_file(fd);
    ZOO_LOG_DEBUG("Successfully locked port %d", port);
    return fd;
}

/**
 * @brief Unlock a port by closing and removing the lock file.
 *
 * @param lock_fd File descriptor of the lock file.
 * @param port    The port number to unlock.
 */
static void unlock_port(int lock_fd, int port)
{
    if (lock_fd >= 0)
    {
        close(lock_fd);

        /* Remove lock file */
        char lockfile[128];
        snprintf(lockfile, sizeof(lockfile), "%s/zoo_smb_port_%d.lock", LOCK_FILE_DIR, port);
        unlink(lockfile);
    }
}

/**
 * @brief Checks if a process is still running
 *
 * This function uses the kill system call with signal 0 to check
 * if a process with the given PID is still running.
 *
 * @param pid Process ID to check
 * @return ZOO_TRUE if process exists, ZOO_FALSE otherwise
 *
 * @note Uses kill(pid, 0) which doesn't send signal but checks existence
 * @note Handles ESRCH errno to detect non-existent processes
 */
static ZOO_BOOL is_process_alive(pid_t pid)
{
    if (pid <= 0)
    {
        return ZOO_FALSE;
    }

    if (kill(pid, 0) == 0)
    {
        return ZOO_TRUE;
    }

    if (errno == ESRCH)
    {
        ZOO_LOG_DEBUG("Process %d no longer exists", pid);
        return ZOO_FALSE;
    }

    ZOO_LOG_WARN("Unable to check process %d status: %s", pid, strerror(errno));
    return ZOO_TRUE;  // Assume alive if we can't determine
}

/**
 * @brief Try to acquire a port in the port table.
 *
 * @param table Pointer to the ZOO_SMB_PORT_TABLE_STRUCT.
 * @param port  The port number to acquire.
 * @return 1 if acquired successfully, 0 otherwise.
 */
static int acquire_port(ZOO_SMB_PORT_TABLE_STRUCT* table, int port)
{
    if (port < MIN_TRANSPORT_PORT || port > MAX_TRANSPORT_PORT)
        return 0;

    pthread_mutex_lock(&table->mutex);

    int index = port - MIN_TRANSPORT_PORT;
    if (table->pids[index] == 0)
    {
        /* Port is not occupied */
        table->pids[index] = getpid();
        pthread_mutex_unlock(&table->mutex);
        return 1;
    }

    /* Check if the occupying process still exists */
    if (!is_process_alive(table->pids[index]))
    {
        /* Process does not exist, take over the port */
        table->pids[index] = getpid();
        pthread_mutex_unlock(&table->mutex);
        return 1;
    }

    pthread_mutex_unlock(&table->mutex);
    return 0;
}

/**
 * @brief Release a port in the port table.
 *
 * @param table Pointer to the ZOO_SMB_PORT_TABLE_STRUCT.
 * @param port  The port number to release.
 */
static void release_port(ZOO_SMB_PORT_TABLE_STRUCT* table, int port)
{
    if (port < MIN_TRANSPORT_PORT || port > MAX_TRANSPORT_PORT)
        return;

    pthread_mutex_lock(&table->mutex);

    int index = port - MIN_TRANSPORT_PORT;
    if (table->pids[index] == getpid())
    {
        table->pids[index] = 0;
    }

    pthread_mutex_unlock(&table->mutex);
}

/**
 * @brief Creates and configures a socket for port checking
 *
 * This function creates a socket with the specified type and configures
 * it with SO_REUSEADDR option to handle TIME_WAIT states.
 *
 * @param socket_type Socket type (SOCK_STREAM or SOCK_DGRAM)
 * @param protocol_name Protocol name for logging ("TCP" or "UDP")
 * @return Socket file descriptor on success, -1 on failure
 *
 * @note Sets SO_REUSEADDR to handle TIME_WAIT state
 * @note Logs errors with protocol-specific information
 */
static int create_test_socket(int socket_type, const char* protocol_name)
{
    int sockfd = socket(AF_INET, socket_type, 0);
    if (sockfd < 0)
    {
        ZOO_LOG_ERROR("%s socket creation failed: %s",
                          protocol_name,
                          strerror(errno));
        return -1;
    }

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        ZOO_LOG_WARN("Failed to set SO_REUSEADDR for %s socket: %s",
                         protocol_name,
                         strerror(errno));
    }

    return sockfd;
}

/**
 * @brief Tests if a port is available for the specified protocol
 *
 * This function attempts to bind to the specified port using the given
 * socket to determine if the port is available.
 *
 * @param sockfd Socket file descriptor
 * @param port Port number to test
 * @param protocol_name Protocol name for logging
 * @return ZOO_TRUE if port is available, ZOO_FALSE if in use
 *
 * @note Closes socket after testing
 * @note Logs debug information about port availability
 */
static ZOO_BOOL test_port_availability(int sockfd, int port, const char* protocol_name)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    ZOO_BOOL available = (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0);

    if (!available)
    {
        ZOO_LOG_DEBUG("%s port %d is in use: %s",
                          protocol_name,
                          port,
                          strerror(errno));
    }

    close(sockfd);
    return available;
}

/**
 * @brief Check if a port is actually in use by the system.
 *
 * This function tests both TCP and UDP protocols to determine if a port
 * is available for use by attempting to bind to it.
 *
 * @param port The port number to check.
 * @return 1 if the port is in use, 0 if available.
 *
 * @note Tests both TCP and UDP protocols
 * @note Uses SO_REUSEADDR to handle TIME_WAIT states
 * @note Returns 1 (in use) if either TCP or UDP binding fails
 */
static int is_port_in_use(int port)
{
    ZOO_LOG_TRACE("Checking if port %d is in use", port);

    /* Check TCP port */
    int tcp_sock = create_test_socket(SOCK_STREAM, "TCP");
    if (tcp_sock < 0)
    {
        return 1;  // Assume in use if we can't create socket
    }

    if (!test_port_availability(tcp_sock, port, "TCP"))
    {
        return 1;
    }

    /* Check UDP port */
    int udp_sock = create_test_socket(SOCK_DGRAM, "UDP");
    if (udp_sock < 0)
    {
        return 1;  // Assume in use if we can't create socket
    }

    if (!test_port_availability(udp_sock, port, "UDP"))
    {
        return 1;
    }

    ZOO_LOG_DEBUG("Port %d is available", port);
    return 0;
}

/**
 * @brief Validates port range parameters
 *
 * This function checks if the provided port range is valid and within
 * acceptable bounds.
 *
 * @param MIN_TRANSPORT_PORT Minimum port number
 * @param MAX_TRANSPORT_PORT Maximum port number
 * @return ZOO_TRUE if range is valid, ZOO_FALSE otherwise
 *
 * @note Checks for proper range order and system limits
 * @note Requires ports to be >= 1024 (non-privileged range)
 */
static ZOO_BOOL validate_port_range(int min, int max)
{
    if (min > max)
    {
        ZOO_LOG_ERROR("Invalid port range: min (%d) > max (%d)",
                          min,
                          max);
        return ZOO_FALSE;
    }

    if (min < 1024 || max > 65535)
    {
        ZOO_LOG_ERROR("Port range [%d, %d] outside valid bounds [1024, 65535]",
                          min,
                          max);
        return ZOO_FALSE;
    }

    return ZOO_TRUE;
}

/**
 * @brief Attempts to allocate a specific port
 *
 * This function tries to allocate a specific port by checking availability,
 * acquiring it in the port table, and locking it with a file lock.
 *
 * @param table Pointer to the port table structure
 * @param port Port number to allocate
 * @return Port number on success, ZOO_SMB_INVALID_PORT on failure
 *
 * @note Performs double-checking of port availability
 * @note Handles cleanup on allocation failure
 */
static int attempt_port_allocation(ZOO_SMB_PORT_TABLE_STRUCT* table, int port)
{
    ZOO_LOG_DEBUG("Attempting to allocate port %d", port);

    if (is_port_in_use(port))
    {
        ZOO_LOG_DEBUG("Port %d is in use by system", port);
        return ZOO_SMB_INVALID_PORT;
    }

    if (!acquire_port(table, port))
    {
        ZOO_LOG_DEBUG("Failed to acquire port %d in table", port);
        return ZOO_SMB_INVALID_PORT;
    }

    int lock_fd = lock_port(port);
    if (lock_fd < 0)
    {
        ZOO_LOG_DEBUG("Failed to lock port %d", port);
        release_port(table, port);
        return ZOO_SMB_INVALID_PORT;
    }

    // Double-check port availability after locking
    if (is_port_in_use(port))
    {
        ZOO_LOG_DEBUG("Port %d became unavailable after locking", port);
        unlock_port(lock_fd, port);
        release_port(table, port);
        return ZOO_SMB_INVALID_PORT;
    }

    ZOO_LOG_INFO("Successfully allocated port %d", port);
    return port;
}

/**
 * @brief Find an available port within the specified range.
 *
 * This function searches for an available port between MIN_TRANSPORT_PORT and MAX_TRANSPORT_PORT
 * by testing each port for availability and attempting to allocate it.
 *
 * @param MIN_TRANSPORT_PORT Minimum port number in the search range
 * @param MAX_TRANSPORT_PORT Maximum port number in the search range
 * @return The available port number on success, or ZOO_SMB_INVALID_PORT on failure.
 *
 * @note Validates port range before searching
 * @note Tests both system availability and internal allocation
 * @note Returns first available port found in the range
 */
int zoo_smb_find_available_port(int min, int max)
{
    ZOO_LOG_DEBUG("Searching for available port in range [%d, %d]",
                      min,
                      max);

    if (!validate_port_range(min, max))
    {
        return ZOO_SMB_INVALID_PORT;
    }

    ZOO_SMB_PORT_TABLE_STRUCT* table = init_port_table();
    if (!table)
    {
        ZOO_LOG_ERROR("Failed to initialize port table");
        return ZOO_SMB_INVALID_PORT;
    }

    for (int port = min; port <= max; port++)
    {
        int allocated_port = attempt_port_allocation(table, port);
        if (allocated_port != ZOO_SMB_INVALID_PORT)
        {
            return allocated_port;
        }
    }

    ZOO_LOG_ERROR("No available ports found in range [%d, %d]",
                      MIN_TRANSPORT_PORT,
                      MAX_TRANSPORT_PORT);
    return ZOO_SMB_INVALID_PORT;
}

/**
 * @brief Release port resources.
 *
 * This function releases the resources associated with the specified port.
 *
 * @param port The port number to release.
 */
void zoo_smb_release_port_resources(int port)
{
    ZOO_SMB_PORT_TABLE_STRUCT* table = init_port_table();
    if (table)
    {
        release_port(table, port);

        /* Try to release file lock */
        int lock_fd = lock_port(port);
        if (lock_fd >= 0)
        {
            unlock_port(lock_fd, port);
        }
    }
}
