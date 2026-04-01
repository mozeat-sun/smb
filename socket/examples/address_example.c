/*******************************************************************************
 * Address utility example using ZOO Socket library
 ******************************************************************************/

#include "zoo_socket.h"
#include <stdio.h>

int main(void) {
    printf("ZOO Socket Address Utility Example\n");
    printf("===================================\n\n");
    
    ZOO_SOCKADDR_UNION addr1, addr2;
    
    // IPv4 address examples
    zoo_sockaddr_inet_init(&addr1, 0x7F000001, 8080);  // 127.0.0.1:8080
    printf("Created IPv4 address: 127.0.0.1:8080\n");
    
    zoo_sockaddr_loopback_init(&addr2, ZOO_AF_INET, 8080);
    printf("Created IPv4 loopback address\n");
    
    if (zoo_sockaddr_equal(&addr1, &addr2)) {
        printf("Addresses are equal\n");
    } else {
        printf("Addresses are different\n");
    }
    
    // Any address example
    zoo_sockaddr_any_init(&addr1, ZOO_AF_INET, 0);
    printf("Created 'any' address for binding\n");
    
    printf("\nAddress utility example completed\n");
    return 0;
}
