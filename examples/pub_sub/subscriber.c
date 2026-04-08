/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: zoo_smb_
 * File name: subscriber.c
 * Description: A simple subscriber demo for ZOO Soft Message Bus (SMB)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-06     AI                created
 ******************************************************************************/

#include "zoo_smb.h"
#include "zoo_smb_subscriber.h"
#include "zoo_smb_node.h"
#include "zoo_smb_qos.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define NODE_NAME "fisherman"
#define TARGET_NAME "sea"
#define TOPIC_NAME "water"

// Global flag for graceful shutdown
static volatile int g_running = 1;

/**
 * @brief Signal handler for graceful shutdown
 */
static void stop_running(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
        g_running = 0;
}

/**
 * @brief Initialize signal handlers
 * @return 0 on success, -1 on error
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
 * @brief Parse log level from command line arguments
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param log_level Pointer to log level variable
 * @return 0 on success, -1 on error
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
 * @brief Message handler callback
 *
 * This function is called when a message is published to the subscribed topic.
 *
 * @param node Subscriber node handle
 * @param topic Message topic
 * @param message Pointer to received message
 * @param size Message size in bytes
 * @param user_data User-defined data pointer
 */
static int message_handler(void* data, uint32_t msg_id, uint64_t request_id, uint64_t timestamp, const char* sender, const void* payload, size_t payload_size)
{
    (void)data;
    if (!payload || payload_size == 0)
        return -1;
    ZOO_LOG_INFO("msg_id: %u, request_id: %lu, sender: %s, timestamp: %lu",
                     msg_id,
                     request_id,
                     sender,
                     timestamp);
    return 0;
}

/**
 * @brief Entry point for the subscriber application.
 *
 * This function initializes the subscriber, processes command-line arguments,
 * and starts listening for published messages.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 * @return int Returns 0 on successful execution, or a non-zero error code on failure.
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
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = NULL;
    int time = 5;
    while (subscriber == NULL && time--)
    {
        subscriber = zoo_smb_create_subscriber(NODE_NAME, TARGET_NAME, TOPIC_NAME, &qos_policy);
        ZOO_LOG_INFO("Subscriber node %p", subscriber);
        sleep(2);
    }
    if (!subscriber)
    {
        ZOO_LOG_ERROR("Failed to create subscriber node");
        return EXIT_FAILURE;
    }
    ZOO_LOG_INFO("Subscriber node %s created successfully", NODE_NAME);

    const uint32_t subscribed_msg_id = 1;
    int32_t subscription_handle;
    ZOO_ERROR_TYPE result = zoo_smb_subscribe_message(
        subscriber, subscribed_msg_id, message_handler, NULL, &subscription_handle);
    if (result != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("Failed to subscribe to topic %s, error code: %d", TOPIC_NAME, result);
        return EXIT_FAILURE;
    }
    ZOO_LOG_INFO("Subscribed to topic %s successfully", TOPIC_NAME);

    // Main loop - wait for messages until interrupted
    printf("Subscriber running (Press Ctrl+C to exit)...\n");
    while (g_running)
        usleep(100000);

    zoo_smb_unsubscribe_message(subscriber, subscription_handle);
    zoo_smb_destroy_subscriber(subscriber);
    ZOO_LOG_INFO("Unsubscribed from topic %s", TOPIC_NAME);
    ZOO_LOG_INFO("Subscriber node destroyed");
    return EXIT_SUCCESS;
}