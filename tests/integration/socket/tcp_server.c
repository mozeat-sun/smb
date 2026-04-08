/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: Socket Examples
 * Component ID: ZOO_SOCKET_EXAMPLES
 * File name: tcp_server.c
 * Description: Simple TCP server example using ZOO Socket library
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-05     assistant       Initial creation
 ******************************************************************************/

#include "zoo_socket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    ZOO_ERROR_TYPE result;
    ZOO_SOCKET_INFO_STRUCT server_socket, client_socket;
    ZOO_SOCKADDR_UNION server_addr, client_addr;
    const char* response = "Hello from ZOO Socket server!";
    char buffer[256];
    size_t bytes_received, bytes_sent;
    int reuse = 1;
    
    printf("ZOO Socket TCP Server Example\n");
    printf("=============================\n\n");
    
    // Initialize socket subsystem
    result = zoo_socket_init();
    if (result != ZOO_OK) {
        printf("Failed to initialize socket subsystem: %s\n", 
               zoo_socket_error_string(result));
        return 1;
    }
    
    printf("Socket subsystem initialized successfully\n");
    
    // Create TCP socket
    result = zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &server_socket);
    if (result != ZOO_OK) {
        printf("Failed to create socket: %s\n", zoo_socket_error_string(result));
        zoo_socket_cleanup();
        return 1;
    }
    
    printf("TCP socket created successfully\n");
    
    // Set socket option to reuse address
    result = zoo_socket_setsockopt(&server_socket, ZOO_SOL_SOCKET, ZOO_SO_REUSEADDR, 
                                   &reuse, sizeof(reuse));
    if (result != ZOO_OK) {
        printf("Warning: Failed to set SO_REUSEADDR: %s\n", zoo_socket_error_string(result));
    }
    
    // Set up server address (bind to any address on port 8080)
    result = zoo_sockaddr_any_init(&server_addr, ZOO_AF_INET, 8080);
    if (result != ZOO_OK) {
        printf("Failed to initialize server address: %s\n", zoo_socket_error_string(result));
        zoo_socket_close(&server_socket);
        zoo_socket_cleanup();
        return 1;
    }
    
    // Bind socket
    result = zoo_socket_bind(&server_socket, &server_addr);
    if (result != ZOO_OK) {
        printf("Failed to bind socket: %s\n", zoo_socket_error_string(result));
        zoo_socket_close(&server_socket);
        zoo_socket_cleanup();
        return 1;
    }
    
    printf("Socket bound to port 8080\n");
    
    // Start listening
    result = zoo_socket_listen(&server_socket, 5);
    if (result != ZOO_OK) {
        printf("Failed to listen on socket: %s\n", zoo_socket_error_string(result));
        zoo_socket_close(&server_socket);
        zoo_socket_cleanup();
        return 1;
    }
    
    printf("Server listening on port 8080...\n");
    printf("Waiting for client connections (press Ctrl+C to stop)\n\n");
    
    // Accept connections (simple single-client server)
    while (1) {
        result = zoo_socket_accept(&server_socket, &client_socket, &client_addr);
        if (result != ZOO_OK) {
            printf("Failed to accept connection: %s\n", zoo_socket_error_string(result));
            continue;
        }
        
        printf("Client connected!\n");
        
        // Receive data from client
        result = zoo_socket_recv(&client_socket, buffer, sizeof(buffer) - 1, &bytes_received);
        if (result == ZOO_OK) {
            buffer[bytes_received] = '\0';
            printf("Received %zu bytes from client: '%s'\n", bytes_received, buffer);
            
            // Send response
            result = zoo_socket_send(&client_socket, response, strlen(response), &bytes_sent);
            if (result == ZOO_OK) {
                printf("Sent %zu bytes response to client\n", bytes_sent);
            } else {
                printf("Failed to send response: %s\n", zoo_socket_error_string(result));
            }
        } else {
            printf("Failed to receive data: %s\n", zoo_socket_error_string(result));
        }
        
        // Close client connection
        zoo_socket_close(&client_socket);
        printf("Client disconnected\n\n");
    }
    
    // Clean up (this code won't be reached in this simple example)
    zoo_socket_close(&server_socket);
    zoo_socket_cleanup();
    
    return 0;
}
