/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd
 *
 * Product: ZOO
 * Module: Socket Examples
 * Component ID: ZOO_SOCKET_EXAMPLES
 * File name: tcp_client.c
 * Description: Simple TCP client example using ZOO Socket library
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
    ZOO_ERROR_TYPE result;
    ZOO_SOCKET_INFO_STRUCT client_socket;
    ZOO_SOCKADDR_UNION server_addr;
    const char* message = "Hello from ZOO Socket client!";
    char response[256];
    size_t bytes_sent, bytes_received;
    
    printf("ZOO Socket TCP Client Example\n");
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
    result = zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &client_socket);
    if (result != ZOO_OK) {
        printf("Failed to create socket: %s\n", zoo_socket_error_string(result));
        zoo_socket_cleanup();
        return 1;
    }
    
    printf("TCP socket created successfully\n");
    
    // Set up server address (localhost:8080)
    result = zoo_sockaddr_loopback_init(&server_addr, ZOO_AF_INET, 8080);
    if (result != ZOO_OK) {
        printf("Failed to initialize server address: %s\n", zoo_socket_error_string(result));
        zoo_socket_close(&client_socket);
        zoo_socket_cleanup();
        return 1;
    }
    
    printf("Attempting to connect to localhost:8080...\n");
    
    // Connect to server
    result = zoo_socket_connect(&client_socket, &server_addr);
    if (result != ZOO_OK) {
        printf("Failed to connect to server: %s\n", zoo_socket_error_string(result));
        printf("Note: Make sure a server is running on localhost:8080\n");
        zoo_socket_close(&client_socket);
        zoo_socket_cleanup();
        return 1;
    }
    
    printf("Connected to server successfully\n");
    
    // Send message
    result = zoo_socket_send(&client_socket, message, strlen(message), &bytes_sent);
    if (result != ZOO_OK) {
        printf("Failed to send message: %s\n", zoo_socket_error_string(result));
    } else {
        printf("Sent %zu bytes: '%s'\n", bytes_sent, message);
    }
    
    // Receive response
    result = zoo_socket_recv(&client_socket, response, sizeof(response) - 1, &bytes_received);
    if (result == ZOO_OK) {
        response[bytes_received] = '\0';
        printf("Received %zu bytes: '%s'\n", bytes_received, response);
    } else {
        printf("Failed to receive response: %s\n", zoo_socket_error_string(result));
    }
    
    // Clean up
    printf("\nClosing connection...\n");
    zoo_socket_close(&client_socket);
    zoo_socket_cleanup();
    
    printf("TCP client example completed\n");
    return 0;
}
