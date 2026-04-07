/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ADVANCED_INTEGRATION_EXAMPLE
 * File name: advanced_integration_example.c
 * Description: Advanced integration examples using node public APIs
 * History recorder:
 * Version   date           author            context
 * 1.1       2026-04-01     AI Assistant      node-API cleanup
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "zoo_smb_server.h"
#include "zoo_smb_client.h"
#include "zoo_smb_publisher.h"
#include "zoo_smb_subscriber.h"

/*
 * Process a demo RPC request on the server side.
 *
 * This callback prints the inbound request metadata so the example shows what a
 * typical handler receives from the SMB runtime. It then sends a fixed reply to
 * confirm that the request/reply path is wired correctly end to end.
 */
static int demo_server_handler(
    void* user_data,
    uint32_t msg_id,
    uint64_t request_id,
    uint64_t timestamp,
    const char* sender,
    const void* payload,
    size_t payload_size)
{
    ZOO_SMB_SERVER_HANDLE server = (ZOO_SMB_SERVER_HANDLE)user_data;
    (void)timestamp;

    printf("[server] recv msg_id=%u req=%llu from=%s payload_size=%zu\n",
           msg_id,
           (unsigned long long)request_id,
           sender ? sender : "(null)",
           payload_size);

    if (payload && payload_size > 0)
    {
        printf("[server] payload: %.*s\n", (int)payload_size, (const char*)payload);
    }

    // Echo-style request/reply flow to validate basic RPC wiring.
    const char* reply = "ack-from-server";
    return zoo_smb_server_send_reply(server, sender, msg_id, reply, strlen(reply), (int64_t)request_id);
}

static int demo_sub_handler(
    void* user_data,
    uint32_t msg_id,
    uint64_t request_id,
    uint64_t timestamp,
    const char* sender,
    const void* payload,
    size_t payload_size)
{
    int* counter = (int*)user_data;
    (*counter)++;
    printf("[sub] #%d msg_id=%u req=%llu sender=%s ts=%llu payload=%.*s\n",
           *counter,
           msg_id,
           (unsigned long long)request_id,
           sender ? sender : "(null)",
           (unsigned long long)timestamp,
           (int)payload_size,
           payload ? (const char*)payload : "");
    return ZOO_SMB_OK;
}

/*
 * Run a minimal client/server request-reply scenario.
 *
 * The example creates one server and one client, installs the server callback,
 * submits a single request, and reports the result. The goal is to demonstrate
 * the basic lifecycle and call sequence for the public RPC-style node APIs.
 */
static void example1_client_server(void)
{
    printf("\n=== Example 1: Client/Server APIs ===\n");

    ZOO_SMB_SERVER_HANDLE server = zoo_smb_create_server(
        "demo_server",
        "demo_server",
        "demo/rpc",
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);

    if (!server)
    {
        printf("create server failed\n");
        return;
    }

    ZOO_ERROR_TYPE r = zoo_smb_server_set_message_handler(server, demo_server_handler, server);
    printf("set server handler: %d\n", r);

    ZOO_SMB_CLIENT_HANDLE client = zoo_smb_create_client(
        "demo_client",
        "demo_server",
        "demo/rpc",
        NULL);

    if (!client)
    {
        printf("create client failed\n");
        zoo_smb_destroy_server(server);
        return;
    }

    // Submit one request to show API shape for request/reply usage.
    int64_t request_id = 0;
    const char* req = "hello-server";
    r = zoo_smb_client_send_request(client, 1001, req, strlen(req), &request_id);
    printf("client send request result=%d request_id=%lld\n", r, (long long)request_id);

    zoo_smb_destroy_client(client);
    zoo_smb_destroy_server(server);
}

/*
 * Run a short publish/subscribe interaction.
 *
 * This example creates one publisher and one subscriber, registers a callback
 * for a single message ID, publishes a small burst of messages, and then tears
 * everything down. It is intended as a compact walkthrough of the pub/sub API.
 */
static void example2_pub_sub(void)
{
    printf("\n=== Example 2: Publisher/Subscriber APIs ===\n");

    ZOO_SMB_PUBLISHER_HANDLE pub = zoo_smb_create_publisher(
        "demo_pub",
        "demo_pub",
        "demo/topic",
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);

    ZOO_SMB_SUBSCRIBER_HANDLE sub = zoo_smb_create_subscriber(
        "demo_sub",
        "demo_pub",
        "demo/topic",
        NULL);

    if (!pub || !sub)
    {
        printf("create pub/sub failed\n");
        if (pub) zoo_smb_destroy_publisher(pub);
        if (sub) zoo_smb_destroy_subscriber(sub);
        return;
    }

    int recv_counter = 0;
    int32_t sub_handle = -1;
    ZOO_ERROR_TYPE r = zoo_smb_subscribe_message(sub, 2001, demo_sub_handler, &recv_counter, &sub_handle);
    printf("subscribe result=%d handle=%d\n", r, sub_handle);

    // Publish a short burst so subscriber callback can be observed.
    for (int i = 0; i < 5; ++i)
    {
        char payload[64];
        snprintf(payload, sizeof(payload), "pub-message-%d", i + 1);
        r = zoo_smb_publish_message(pub, 2001, payload, strlen(payload));
        printf("publish #%d result=%d\n", i + 1, r);
    }

    if (sub_handle >= 0)
    {
        zoo_smb_unsubscribe_message(sub, sub_handle);
    }

    zoo_smb_destroy_subscriber(sub);
    zoo_smb_destroy_publisher(pub);
}

/*
 * Exercise repeated publisher creation and destruction.
 *
 * The loop intentionally focuses on node lifecycle cost rather than message
 * delivery. It gives a quick indication of how expensive repeated setup and
 * teardown are when the application creates many short-lived nodes.
 */
static void example3_node_lifecycle_batch(void)
{
    printf("\n=== Example 3: Batch node lifecycle ===\n");

    int created = 0;
    const int rounds = 500;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Stress create/destroy path for a single node type.
    for (int i = 0; i < rounds; ++i)
    {
        char name[64];
        snprintf(name, sizeof(name), "batch_pub_%d", i);

        ZOO_SMB_PUBLISHER_HANDLE pub = zoo_smb_create_publisher(
            name,
            "batch_target",
            "batch/topic",
            ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
            NULL);
        if (pub)
        {
            created++;
            zoo_smb_destroy_publisher(pub);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double sec = (double)(end.tv_sec - start.tv_sec) +
                 (double)(end.tv_nsec - start.tv_nsec) / 1e9;
    printf("batch created/destroyed=%d/%d in %.3fs (%.0f ops/s)\n",
           created,
           rounds,
           sec,
           sec > 0.0 ? created / sec : 0.0);
}

/*
 * Dispatch the requested example scenario.
 *
 * The entry point accepts either a specific example number or the special
 * "all" selector. Running all scenarios provides a lightweight smoke test for
 * the public node APIs exposed by this module.
 */
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <example> [1|2|3|all]\n", argv[0]);
        return 1;
    }

    // Run all scenarios for quick smoke validation of public node APIs.
    if (strcmp(argv[1], "all") == 0)
    {
        example1_client_server();
        example2_pub_sub();
        example3_node_lifecycle_batch();
        return 0;
    }

    switch (atoi(argv[1]))
    {
        case 1:
            example1_client_server();
            break;
        case 2:
            example2_pub_sub();
            break;
        case 3:
            example3_node_lifecycle_batch();
            break;
        default:
            printf("Invalid example\n");
            return 1;
    }

    return 0;
}
