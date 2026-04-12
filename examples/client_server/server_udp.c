#include "zoo_smb_server.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>

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

int udp_server_event_handler(void* user_data,
                             uint32_t msg_id,
                             uint64_t request_id,
                             uint64_t timestamp,
                             const char* sender,
                             const void* payload,
                             size_t payload_size)
{
    ZOO_SMB_SERVER_HANDLE node = (ZOO_SMB_SERVER_HANDLE)user_data;

    ZOO_LOG_INFO("[UDP Server] Received [%s] request on msg_id: %u, request_id: %ld, timestamp: %lu", sender, msg_id, request_id, timestamp);
    if (payload && payload_size > 0)
    {
        int printable_len = (payload_size > (size_t)INT_MAX) ? INT_MAX : (int)payload_size;
        ZOO_LOG_INFO("[UDP Server] payload_size: %zu, Payload: %.*s", payload_size, printable_len, (const char*)payload);
    }

    const char* reply_msg = "UDP server received your request";
    size_t reply_size = strlen(reply_msg) + 1;

    ZOO_ERROR_TYPE error = zoo_smb_server_send_reply(
        (ZOO_SMB_SERVER_HANDLE)node, sender, msg_id, reply_msg, reply_size, request_id);

    if (error == ZOO_SMB_OK)
    {
        ZOO_LOG_INFO("[UDP Server] Reply sent successfully");
    }
    else
    {
        ZOO_LOG_ERROR("[UDP Server] Failed to send reply, error code: %d", error);
    }
    return 0;
}

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

    if (setup_signals() != 0)
    {
        return EXIT_FAILURE;
    }

    memset(server_name, 0x0, sizeof(server_name));
    snprintf(server_name, sizeof(server_name), "%s", options.target);

    ZOO_SMB_SERVER_HANDLE server = zoo_smb_create_server(
        server_name,
        options.target,
        options.topic,
        ZOO_SMB_TRANSPORT_TYPE_UDP,
        NULL);
    if (server == NULL)
    {
        ZOO_LOG_ERROR("[UDP Server] Failed to create server");
        return EXIT_FAILURE;
    }

    ZOO_ERROR_TYPE error = zoo_smb_server_set_message_handler(server, udp_server_event_handler, server);
    if (error != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("[UDP Server] Failed to set event handler, error code: %d", error);
        zoo_smb_destroy_server(server);
        return EXIT_FAILURE;
    }

    ZOO_LOG_INFO("UDP server is running, waiting for requests...");
    ZOO_LOG_INFO("Press Ctrl+C to stop the server...\n");
    while (g_running)
    {
        usleep(100000);
    }

    ZOO_LOG_INFO("Shutting down UDP server...");
    zoo_smb_destroy_server(server);
    return 0;
}
