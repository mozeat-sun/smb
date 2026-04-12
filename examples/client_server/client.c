#if 0
/**
 * @file client.c
 * @brief Example SMB client for Zoo project (transport-agnostic).
 *
 * Demonstrates how to create a generic SMB client, send a request to a server,
 * and receive a reply. Includes retry logic and configurable log level.
 *
 * Usage:
 *   ./zoo_example_client -t <server_name> -p <payload> -l <log_level>
 *
 * Author: Zoo Project Contributors
 * Date: 2026-04-12
 */
#endif
#include "zoo_smb_client.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

/**
 * @file client.c
 * @brief Example SMB client with configurable log level.
 *
 * Usage:
 *   ./client [log_level]
 *   log_level: 0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=FATAL, 6=OFF (default: 2)
 */

// Configuration constants
#define TIMEOUT_MS 5000
#define REPLY_BUFFER_SIZE 1024
#define MAX_SEND_RETRIES 30
#define RETRY_SLEEP_US 200000

// Global variables for signal handling
static volatile ZOO_BOOL g_running = ZOO_TRUE;

static char client_name[128];

/**
 * @brief Main entry point for the SMB client example.
 *
 * Parses command-line arguments, creates an SMB client, sends a request,
 * and waits for a reply. Retries sending if necessary.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 on success, non-zero on failure.
 */
int main(int argc, char* argv[])
{
    COMMAND_OPTIONS_STRUCT options;
    memset(&options, 0x0, sizeof(COMMAND_OPTIONS_STRUCT));
    int parse_result = parse_arguments(argc, argv, &options);
    if (parse_result > 0)
    {
        return EXIT_SUCCESS;
    }
    if (parse_result < 0)
    {
        printf("Invalid arguments, please use --help for details\n");
        return EXIT_FAILURE;
    }

    ZOO_LOG_LEVEL_ENUM log_level = (ZOO_LOG_LEVEL_ENUM)options.log_level;
    zoo_log_set_level(log_level);
    // zoo_log_set_target(ZOO_LOG_TARGET_BOTH);

    memset(client_name, 0x0, sizeof(client_name));
    // Client identity must be unique from the server identity.
    snprintf(client_name, sizeof(client_name), "DemoClient_%d", getpid());
    ZOO_LOG_INFO("Starting SMB client: %s -> %s (topic: %s)", client_name, options.target, options.topic);

    // Client transport is resolved from discovered server service metadata.
    ZOO_SMB_CLIENT_HANDLE client = zoo_smb_create_client(client_name, options.target, options.topic, NULL);
    if (client == NULL)
    {
        ZOO_LOG_ERROR("[Client] Failed to create client");
        return EXIT_FAILURE;
    }

    uint32_t msg_id = time(NULL);
    int64_t request_id = 0;
    size_t payload_size = strlen(options.payload) + 1;
    size_t recv_payload_size = 0;
    void* reply_buffer = malloc(REPLY_BUFFER_SIZE);
    int sent_ok = 0;
    for (int retry = 0; retry < MAX_SEND_RETRIES; retry++)
    {
        if (ZOO_SMB_OK == zoo_smb_client_send_request(client, msg_id, options.payload, payload_size, &request_id))
        {
            sent_ok = 1;
            break;
        }
        ZOO_LOG_WARN("[Client] Send retry %d/%d", retry + 1, MAX_SEND_RETRIES);
        usleep(RETRY_SLEEP_US);
    }

    if (!sent_ok)
    {
        ZOO_LOG_ERROR("[Client] Failed to send message after retries: %s", options.payload);
        free(reply_buffer);
        zoo_smb_destroy_client(client);
        return EXIT_FAILURE;
    }

    memset(reply_buffer, 0, REPLY_BUFFER_SIZE);
    if (ZOO_SMB_OK != zoo_smb_client_recv_reply(client, msg_id, request_id, &reply_buffer, &recv_payload_size, TIMEOUT_MS))
    {
        ZOO_LOG_ERROR("[Client] Failed to receive reply");
        free(reply_buffer);
        zoo_smb_destroy_client(client);
        return EXIT_FAILURE;
    }

    ZOO_LOG_INFO("Received reply: %s", (char*)reply_buffer);

    zoo_smb_destroy_client(client);
    free(reply_buffer);
    return 0;
}