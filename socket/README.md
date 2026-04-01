# ZOO Socket Library

A cross-platform socket abstraction library supporting POSIX sockets (Linux/Unix) and lwIP (embedded systems).

## Features

- **Cross-platform**: Supports POSIX sockets and lwIP
- **Comprehensive API**: TCP, UDP, IPv4, IPv6 support
- **Thread-safe**: Built-in mutex protection for shared state
- **Error handling**: Standardized error codes and messages
- **Address utilities**: Helper functions for socket address management
- **Select support**: Non-blocking I/O with timeout support
- **Socket options**: Standard socket option configuration
- **Unity tests**: Complete test suite with Unity framework

## Supported Platforms

- **Linux**: Full POSIX socket support
- **Unix-like systems**: Full POSIX socket support  
- **Embedded systems**: lwIP support
- **Bare metal**: Stub implementation for testing

## Platform Detection

The library automatically detects platform capabilities:

```c
#if ZOO_HAS_POSIX
    // POSIX socket implementation
#elif defined(ZOO_USE_LWIP)
    // lwIP implementation
#else
    // Bare metal stub implementation
#endif
```

## Quick Start

### Initialization

```c
#include "zoo_socket.h"

// Initialize socket subsystem
ZOO_ERROR_TYPE result = zoo_socket_init();
if (result != ZOO_OK) {
    printf("Failed to initialize: %s\n", zoo_socket_error_string(result));
    return -1;
}

// ... use socket functions ...

// Cleanup when done
zoo_socket_cleanup();
```

### TCP Client Example

```c
zoo_socket_info_t client;
zoo_sockaddr_t server_addr;

// Create TCP socket
zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &client);

// Connect to server
zoo_sockaddr_loopback_init(&server_addr, ZOO_AF_INET, 8080);
zoo_socket_connect(&client, &server_addr);

// Send data
const char* message = "Hello Server!";
size_t bytes_sent;
zoo_socket_send(&client, message, strlen(message), &bytes_sent);

// Receive response
char buffer[256];
size_t bytes_received;
zoo_socket_recv(&client, buffer, sizeof(buffer), &bytes_received);

// Close socket
zoo_socket_close(&client);
```

### TCP Server Example

```c
zoo_socket_info_t server, client;
zoo_sockaddr_t server_addr, client_addr;

// Create and configure server socket
zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &server);

int reuse = 1;
zoo_socket_setsockopt(&server, ZOO_SOL_SOCKET, ZOO_SO_REUSEADDR, &reuse, sizeof(reuse));

// Bind and listen
zoo_sockaddr_any_init(&server_addr, ZOO_AF_INET, 8080);
zoo_socket_bind(&server, &server_addr);
zoo_socket_listen(&server, 5);

// Accept client
zoo_socket_accept(&server, &client, &client_addr);

// Handle client communication...

zoo_socket_close(&client);
zoo_socket_close(&server);
```

### UDP Example

```c
zoo_socket_info_t udp_socket;
zoo_sockaddr_t dest_addr;

// Create UDP socket
zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_DGRAM, ZOO_IPPROTO_UDP, &udp_socket);

// Send to destination
zoo_sockaddr_inet_init(&dest_addr, 0xC0A80101, 12345); // 192.168.1.1:12345
const char* data = "UDP message";
size_t bytes_sent;
zoo_socket_sendto(&udp_socket, data, strlen(data), &dest_addr, &bytes_sent);

// Receive from any source
char buffer[512];
zoo_sockaddr_t from_addr;
size_t bytes_received;
zoo_socket_recvfrom(&udp_socket, buffer, sizeof(buffer), &from_addr, &bytes_received);

zoo_socket_close(&udp_socket);
```

## API Reference

### Initialization Functions

- `zoo_socket_init()` - Initialize socket subsystem
- `zoo_socket_cleanup()` - Cleanup socket subsystem
- `zoo_socket_is_initialized()` - Check initialization status

### Socket Management

- `zoo_socket_create()` - Create a new socket
- `zoo_socket_close()` - Close a socket
- `zoo_socket_bind()` - Bind socket to address
- `zoo_socket_listen()` - Listen for connections
- `zoo_socket_accept()` - Accept incoming connection
- `zoo_socket_connect()` - Connect to remote address
- `zoo_socket_shutdown()` - Shutdown socket operations

### Data Transfer

- `zoo_socket_send()` - Send data (TCP)
- `zoo_socket_recv()` - Receive data (TCP)
- `zoo_socket_sendto()` - Send data (UDP)
- `zoo_socket_recvfrom()` - Receive data (UDP)

### Socket Options

- `zoo_socket_setsockopt()` - Set socket option
- `zoo_socket_getsockopt()` - Get socket option
- `zoo_socket_set_blocking()` - Set blocking/non-blocking mode

### Address Utilities

- `zoo_sockaddr_inet_init()` - Initialize IPv4 address
- `zoo_sockaddr_inet6_init()` - Initialize IPv6 address  
- `zoo_sockaddr_any_init()` - Initialize "any" address
- `zoo_sockaddr_loopback_init()` - Initialize loopback address
- `zoo_sockaddr_equal()` - Compare addresses

### Advanced Functions

- `zoo_socket_select()` - Monitor socket for I/O readiness
- `zoo_socket_resolve_hostname()` - Resolve hostname to addresses
- `zoo_socket_get_addresses()` - Get local/remote addresses

### Error Handling

- `zoo_socket_get_last_error()` - Get last error code
- `zoo_socket_error_string()` - Get error description

## Building

### CMake Build

```bash
cd zoo/socket
mkdir build && cd build
cmake ..
make
```

### Build Options

- `ZOO_SOCKET_BUILD_TESTS=ON/OFF` - Build unit tests (default: ON)
- `ZOO_SOCKET_BUILD_EXAMPLES=ON/OFF` - Build examples (default: ON)

### Dependencies

- ZOO platform headers (`zoo_platform.h`)
- ZOO utility headers (`zoo_util.h`)
- Unity test framework (for tests)

## Testing

Run the test suite:

```bash
cd build
make test
# or
ctest --verbose
```

Run specific test categories:

```bash
./tests/zoo_socket_tests
```

## Examples

Build and run examples:

```bash
cd build
make

# Run TCP server (in one terminal)
./examples/tcp_server_example

# Run TCP client (in another terminal)  
./examples/tcp_client_example

# Run other examples
./examples/udp_example
./examples/address_example
```

## Error Codes

The library uses standardized error codes from the ZOO framework:

- `ZOO_OK` - Success
- `ZOO_ERROR_INVALID_PARAM` - Invalid parameter
- `ZOO_ERROR_NOT_INITIALIZED` - Not initialized
- `ZOO_ERROR_CONNECTION_REFUSED` - Connection refused
- `ZOO_ERROR_CONNECTION_RESET` - Connection reset
- `ZOO_ERROR_TIMEOUT` - Operation timeout
- `ZOO_ERROR_AGAIN` - Try again (non-blocking)

## Thread Safety

The socket library is thread-safe for:

- Initialization/cleanup functions
- Error state management
- Individual socket operations on different sockets

**Note**: Multiple threads accessing the same socket concurrently is not supported.

## Platform-Specific Notes

### Linux/POSIX
- Full socket API support
- IPv6 support available
- Unix domain socket support
- Complete error mapping

### lwIP (Embedded)
- TCP/UDP support
- Limited socket options
- No Unix domain sockets
- Simplified error handling

### Bare Metal
- Stub implementation
- Suitable for unit testing
- No actual network functionality

## License

Copyright (C) 2025, ZOO Ltd. All rights reserved.

## Contributing

Please follow the ZOO framework coding standards and ensure all tests pass before submitting changes.
