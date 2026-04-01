/*******************************************************************************
 * Test program for newly implemented APIs
 ******************************************************************************/

#include "zoo_socket.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main()
{
    printf("Testing newly implemented ZOO Socket APIs...\n");
    
    // Initialize socket subsystem
    ZOO_ERROR_TYPE result = zoo_socket_init();
    assert(result == ZOO_OK);
    
    // Test 1: zoo_sockaddr_ipv4
    printf("1. Testing zoo_sockaddr_ipv4...\n");
    ZOO_SOCKADDR_UNION addr1;
    result = zoo_sockaddr_ipv4(&addr1, "127.0.0.1", 8080);
    if (result == ZOO_OK) {
        printf("   ✓ IPv4 address parsing successful\n");
        
        // Test zoo_sockaddr_get_port
        uint16_t port = zoo_sockaddr_get_port(&addr1);
        printf("   ✓ Port retrieved: %u\n", port);
        assert(port == 8080);
        
        // Test zoo_sockaddr_to_string
        char buffer[64];
        result = zoo_sockaddr_to_string(&addr1, buffer, sizeof(buffer));
        if (result == ZOO_OK) {
            printf("   ✓ Address to string: %s\n", buffer);
        }
    } else {
        printf("   ✗ IPv4 address parsing failed\n");
    }
    
    // Test 2: zoo_sockaddr_ipv6
    printf("2. Testing zoo_sockaddr_ipv6...\n");
    ZOO_SOCKADDR_UNION addr2;
    result = zoo_sockaddr_ipv6(&addr2, "::1", 9090);
    if (result == ZOO_OK) {
        printf("   ✓ IPv6 address parsing successful\n");
        
        char buffer[128];
        result = zoo_sockaddr_to_string(&addr2, buffer, sizeof(buffer));
        if (result == ZOO_OK) {
            printf("   ✓ IPv6 address to string: %s\n", buffer);
        }
    } else {
        printf("   ✗ IPv6 address parsing failed\n");
    }
    
    // Test 3: zoo_sockaddr_set_port
    printf("3. Testing zoo_sockaddr_set_port...\n");
    result = zoo_sockaddr_set_port(&addr1, 9999);
    if (result == ZOO_OK) {
        uint16_t new_port = zoo_sockaddr_get_port(&addr1);
        printf("   ✓ Port updated to: %u\n", new_port);
        assert(new_port == 9999);
    }
    
    // Test 4: zoo_socket_shutdown (requires a socket)
    printf("4. Testing zoo_socket_shutdown...\n");
    ZOO_SOCKET_INFO_STRUCT socket_info;
    result = zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &socket_info);
    if (result == ZOO_OK) {
        result = zoo_socket_shutdown(&socket_info, ZOO_SHUT_RDWR);
        if (result == ZOO_OK) {
            printf("   ✓ Socket shutdown successful\n");
        } else {
            printf("   ⚠ Socket shutdown failed (expected for unconnected socket)\n");
        }
        zoo_socket_close(&socket_info);
    }
    
    // Test 5: zoo_socket_get_addresses
    printf("5. Testing zoo_socket_get_addresses...\n");
    result = zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &socket_info);
    if (result == ZOO_OK) {
        ZOO_SOCKADDR_UNION local_addr, remote_addr;
        result = zoo_socket_get_addresses(&socket_info, &local_addr, &remote_addr);
        if (result == ZOO_OK) {
            printf("   ✓ Socket addresses retrieved\n");
        } else {
            printf("   ⚠ Socket addresses retrieval failed (normal for unbound socket)\n");
        }
        zoo_socket_close(&socket_info);
    }
    
    // Test 6: zoo_socket_resolve_hostname
    printf("6. Testing zoo_socket_resolve_hostname...\n");
    ZOO_SOCKADDR_UNION resolved_addr;
    result = zoo_socket_resolve_hostname("localhost", &resolved_addr, ZOO_AF_INET);
    if (result == ZOO_OK) {
        printf("   ✓ Hostname resolution successful\n");
        char buffer[64];
        result = zoo_sockaddr_to_string(&resolved_addr, buffer, sizeof(buffer));
        if (result == ZOO_OK) {
            printf("   ✓ Resolved address: %s\n", buffer);
        }
    } else {
        printf("   ⚠ Hostname resolution failed\n");
    }
    
    zoo_socket_cleanup();
    printf("\nNew API testing completed!\n");
    
    return 0;
}
