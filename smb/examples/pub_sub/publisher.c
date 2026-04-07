/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: zoo_smb_
 * File name: publisher.c
 * Description: A simple publisher demo for ZOO Soft Message Bus (SMB)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-06     AI                created
 ******************************************************************************/

#include "zoo_smb.h"
#include "zoo_smb_publisher.h"
#include "zoo_smb_qos_policy.h"
#include "zoo_smb_node.h"
#include "zoo_smb_qos.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#define NODE_NAME "fish"
#define TARGET_NAME "sea"
#define TOPIC_NAME "water"
#define MESSAGE_CONTENT "Hello, I am a fish!"
#define PUBLISH_INTERVAL_MS 1000

static volatile int g_running = 1;

/**
 * @brief Signal handler to stop the running process gracefully.
 * @param signo The signal number that triggered the handler.
 */
static void stop_running(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        ZOO_LOG_INFO("Received signal %d, shutting down gracefully...", signo);
        g_running = 0;
    }
}

/**
 * @brief Sets up signal handlers for the application.
 *
 * This function configures the necessary signal handlers to ensure
 * proper handling of system signals (such as SIGINT, SIGTERM, etc.)
 * during the application's execution. It is typically called during
 * initialization to allow for graceful shutdown or cleanup when
 * signals are received.
 *
 * @return int Returns 0 on success, or a negative error code on failure.
 */
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
 * @brief Logs the result of publishing a message.
 *
 * This function logs the outcome of a publish operation, including the message ID,
 * the message content, and the result of the operation.
 *
 * @param msg_id   The unique identifier of the published message.
 * @param message  The content of the message that was published.
 * @param result   The result of the publish operation, represented by a ZOO_ERROR_TYPE value.
 */
static void log_publish_result(uint64_t msg_id, const char* message, ZOO_ERROR_TYPE result)
{
    if (result == ZOO_SMB_OK)
        ZOO_LOG_INFO("Published message %lu: %s", (unsigned long)msg_id, message);
    else
        ZOO_LOG_ERROR("Failed to publish message %lu, error code: %s", (unsigned long)msg_id, zoo_smb_get_error_string(result));
}

/**
 * @brief Publishes a message using the specified publisher handle and message ID.
 *
 * This function sends a message identified by the given msg_id through the provided
 * publisher handle. It returns a value of type ZOO_ERROR_TYPE indicating the
 * success or failure of the publish operation.
 *
 * @param publisher The handle to the publisher instance used to send the message.
 * @param msg_id The unique identifier of the message to be published.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the publish operation.
 */
static ZOO_ERROR_TYPE publish_message(ZOO_SMB_PUBLISHER_HANDLE publisher, uint64_t msg_id)
{
    const char* message = MESSAGE_CONTENT;
    size_t message_size = strlen(message) + 1;
    ZOO_ERROR_TYPE result = zoo_smb_publish_message(publisher, msg_id, message, message_size);
    log_publish_result(msg_id, message, result);
    return result;
}

/**
 * @brief Parses the log level from the command-line arguments.
 *
 * This function examines the provided command-line arguments to determine
 * the desired log level for the application. If a valid log level is found,
 * it is stored in the variable pointed to by @p log_level.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 * @param log_level Pointer to a ZOO_LOG_LEVEL_ENUM variable where the parsed log level will be stored.
 * @return 0 on success, or a non-zero error code if parsing fails.
 */
static int parse_log_level(int argc, char* argv[], ZOO_LOG_LEVEL_ENUM* log_level)
{
    if (argc > 1)
    {
        int level = atoi(argv[1]);
        if (level >= ZOO_LOG_LEVEL_TRACE && level <= ZOO_LOG_LEVEL_OFF)
        {
            *log_level = (ZOO_LOG_LEVEL_ENUM)level;
            return 0;
        }
        printf("Usage: %s [log_level]\n", argv[0]);
        printf("  log_level: 0-TRACE, 1-DEBUG, 2-INFO, 3-WARN, 4-ERROR, 5-OFF\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Interruptible sleep function that respects the running flag
 * @param milliseconds Time to sleep in milliseconds
 * @return 0 if sleep completed, -1 if interrupted
 */
static int interruptible_sleep_ms(int milliseconds)
{
    const int check_interval_ms = 100;  // Check running flag every 100ms
    int remaining_ms = milliseconds;

    while (remaining_ms > 0 && g_running)
    {
        int sleep_time = (remaining_ms < check_interval_ms) ? remaining_ms : check_interval_ms;
        usleep(sleep_time * 1000);
        remaining_ms -= sleep_time;
    }

    return g_running ? 0 : -1;
}

/**
 * @brief Entry point for the publisher application.
 */
int main(int argc, char* argv[])
{
    ZOO_LOG_LEVEL_ENUM log_level = ZOO_LOG_LEVEL_TRACE;
    if (parse_log_level(argc, argv, &log_level) != 0)
        return EXIT_FAILURE;

    zoo_log_set_level(log_level);
    zoo_log_set_target(ZOO_LOG_TARGET_BOTH);

    if (setup_signals() != 0)
        return EXIT_FAILURE;

    ZOO_SMB_QOS_POLICY_STRUCT qos_policy;
    zoo_smb_init_qos_policy(&qos_policy);

    ZOO_SMB_PUBLISHER_HANDLE publisher = zoo_smb_create_publisher(
        NODE_NAME, TARGET_NAME, TOPIC_NAME, ZOO_SMB_TRANSPORT_TYPE_UDP, &qos_policy);

    if (!publisher)
    {
        ZOO_LOG_ERROR("Failed to create publisher node");
        return EXIT_FAILURE;
    }

    ZOO_LOG_INFO("Publisher node %s created successfully", NODE_NAME);

    const uint64_t msg_id = 1;
    printf("Publisher running (Press Ctrl+C to exit)...\n");

    while (g_running)
    {
        publish_message(publisher, msg_id);

        // Use interruptible sleep to respond faster to signals
        if (interruptible_sleep_ms(PUBLISH_INTERVAL_MS) != 0)
        {
            ZOO_LOG_INFO("Sleep interrupted, exiting...");
            break;
        }
    }

    ZOO_LOG_INFO("Shutting down publisher...");
    zoo_smb_destroy_publisher(publisher);
    ZOO_LOG_INFO("Publisher node destroyed");

    return EXIT_SUCCESS;
}