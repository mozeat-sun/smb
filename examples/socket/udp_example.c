/*******************************************************************************
 * Simple UDP example using ZOO Socket library
 ******************************************************************************/

#include "zoo_socket.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    printf("ZOO Socket UDP Example\n");
    printf("======================\n\n");
    
    zoo_socket_init();
    
    ZOO_SOCKET_INFO_STRUCT udp_socket;
    ZOO_SOCKADDR_UNION addr;
    const char* message = "UDP test message";
    size_t bytes_sent;
    
    // Create UDP socket
    if (zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_DGRAM, ZOO_IPPROTO_UDP, &udp_socket) == ZOO_OK) {
        printf("UDP socket created successfully\n");
        
        // Send to localhost:12345
        zoo_sockaddr_loopback_init(&addr, ZOO_AF_INET, 12345);
        
        if (zoo_socket_sendto(&udp_socket, message, strlen(message), &addr, &bytes_sent) == ZOO_OK) {
            printf("Sent %zu bytes via UDP\n", bytes_sent);
        }
        
        zoo_socket_close(&udp_socket);
    }
    
    zoo_socket_cleanup();
    printf("UDP example completed\n");
    return 0;
}
