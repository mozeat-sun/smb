#include "zoo_smb_server.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static char server_name[128];
static volatile int g_running = 1;

static void stop_running(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        g_running = 0;
    }
}

static int setup_signals(void)
{
    struct sigaction sa = {.sa_handler = stop_running, .sa_flags = 0};
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1)
    {
        ZOO_LOG_ERROR("Failed to setup signal handlers");
        return -1;
    }
    return 0;
}

/**
 * @brief Event handler callback function for server events
 *
 * This function is called when server events occur and needs to be implemented
 * to handle various server-side events such as client connections, disconnections,
 * data reception, or other server state changes.
 *
 * @param user_data Pointer to user-defined data that was passed during handler registration.
 *                  This allows the handler to access context-specific information.
 */
int server_event_handler(void* user_data,
                         uint32_t msg_id,
                         uint64_t request_id,
                         uint64_t timestamp,
                         const char* sender,
                         const void* payload,
                         size_t payload_size)
{
    ZOO_SMB_SERVER_HANDLE node = (ZOO_SMB_SERVER_HANDLE)user_data;

    ZOO_LOG_INFO("Received [%s] request on msg_id: %u, request_id: %ld, timestamp: %lu", sender, msg_id, request_id, timestamp);
    if (payload && payload_size > 0)
    {
        ZOO_LOG_INFO("payload_size: %zu, Payload: %.*s", payload_size, (const char*)payload);
    }

    // Memory pool usage printing removed - not available in parent implementation
    const char* reply_msg = "Server received your request, Congratulations ---------------------------------------------------------!";
    size_t reply_size = strlen(reply_msg) + 1;

    ZOO_ERROR_TYPE error = zoo_smb_server_send_reply(
        (ZOO_SMB_SERVER_HANDLE)node, sender, msg_id, reply_msg, reply_size, request_id);

    if (error == ZOO_SMB_OK)
    {
        ZOO_LOG_INFO("Reply sent successfully");
    }
    else
    {
        ZOO_LOG_ERROR("Failed to send reply, error code: %d", error);
    }
    return 0;
}

/**
 * @brief Entry point for the server application.
 *
 * Initializes and starts the server. Handles command-line arguments.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 * @return Exit status code (0 for success, non-zero for failure).
 */
int main(int argc, char* argv[])
{
    COMMAND_OPTIONS_STRUCT options;
    memset(&options, 0x0, sizeof(COMMAND_OPTIONS_STRUCT));
    if (0 != parse_arguments(argc, argv, &options))
    {
        printf("Invalid argumenst,please use --help for details");
        return EXIT_FAILURE;
    }

    ZOO_LOG_LEVEL_ENUM log_level = (ZOO_LOG_LEVEL_ENUM)options.log_level;
    zoo_log_set_level(log_level);
    // zoo_log_set_target(ZOO_LOG_TARGET_BOTH);

    if (setup_signals() != 0)
    {
        return EXIT_FAILURE;
    }

    memset(server_name, 0x0, sizeof(server_name));
    sprintf(server_name, "DemoServer_%d", getpid());
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type = options.transport_type == 0 ? ZOO_SMB_TRANSPORT_TYPE_UDP : (ZOO_SMB_TRANSPORT_TYPE_ENUM)options.transport_type;
    ZOO_SMB_SERVER_HANDLE server = zoo_smb_create_server(server_name, options.target, options.topic, transport_type, NULL);
    if (server == NULL)
    {
        ZOO_LOG_ERROR("Failed to create server");
        return EXIT_FAILURE;
    }

    ZOO_ERROR_TYPE error = zoo_smb_server_set_message_handler(server, server_event_handler, server);
    if (error != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("Failed to set event handler, error code: %d", error);
        zoo_smb_destroy_server(server);
        return EXIT_FAILURE;
    }

    ZOO_LOG_INFO("Server is running, waiting for requests...");
    ZOO_LOG_INFO("Press Ctrl+C to stop the server...\n");
    while (g_running)
    {
        usleep(100000);
    }

    ZOO_LOG_INFO("Shutting down server...");
    zoo_smb_destroy_server(server);
    return 0;
}