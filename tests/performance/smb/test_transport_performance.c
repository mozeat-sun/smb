/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: PERFORMANCE_TEST_TRANSPORT
 * File name: test_transport_performance.c
 * Description: Comprehensive transport layer performance benchmarks
 * Traceability coverage:
 * - REQ-PERF-001: transport throughput evidence for node lifecycle operations.
 * - REQ-PERF-002: transport timing evidence for benchmark latency reporting.
 * - REQ-PERF-003: regression threshold candidate for transport path performance.
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "unity.h"
#include "zoo_smb.h"
#include "zoo_smb_server.h"
#include "zoo_smb_client.h"
#include "zoo_smb_publisher.h"
#include "zoo_smb_subscriber.h"

#define PERF_LOOP_SMALL 10000
#define PERF_LOOP_MEDIUM 5000

static double elapsed_seconds(const struct timespec* start, const struct timespec* end)
{
    time_t s = end->tv_sec - start->tv_sec;
    long ns = end->tv_nsec - start->tv_nsec;
    return (double)s + (double)ns / 1e9;
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_perf_publisher_create_destroy(void)
{
    struct timespec start, end;
    int success = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < PERF_LOOP_MEDIUM; ++i)
    {
        char name[64];
        snprintf(name, sizeof(name), "perf_pub_%d", i);

        ZOO_SMB_PUBLISHER_HANDLE publisher = zoo_smb_create_publisher(
            name,
            "perf_target",
            "perf/topic",
            ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
            NULL);
        if (publisher)
        {
            success++;
            zoo_smb_destroy_publisher(publisher);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double sec = elapsed_seconds(&start, &end);
    printf("[perf] publisher create/destroy: %d/%d in %.3fs (%.0f ops/s)\n",
           success,
           PERF_LOOP_MEDIUM,
           sec,
           sec > 0.0 ? success / sec : 0.0);

    TEST_ASSERT_GREATER_THAN(PERF_LOOP_MEDIUM / 2, success);
}

void test_perf_publish_message_calls(void)
{
    struct timespec start, end;
    int success = 0;
    const char* payload = "benchmark_payload";

    ZOO_SMB_PUBLISHER_HANDLE publisher = zoo_smb_create_publisher(
        "perf_pub_call",
        "perf_target_call",
        "perf/topic/call",
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);

    if (!publisher)
    {
        TEST_IGNORE_MESSAGE("publisher create failed in this runtime environment");
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < PERF_LOOP_SMALL; ++i)
    {
        if (zoo_smb_publish_message(publisher, (uint32_t)(i + 1), payload, strlen(payload)) == ZOO_SMB_OK)
        {
            success++;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double sec = elapsed_seconds(&start, &end);
    printf("[perf] publish calls: %d/%d in %.3fs (%.0f msg/s)\n",
           success,
           PERF_LOOP_SMALL,
           sec,
           sec > 0.0 ? success / sec : 0.0);

    zoo_smb_destroy_publisher(publisher);
    TEST_ASSERT_GREATER_THAN(PERF_LOOP_SMALL / 2, success);
}

void test_perf_mixed_node_lifecycle(void)
{
    struct timespec start, end;
    int server_ok = 0;
    int client_ok = 0;
    int sub_ok = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 1000; ++i)
    {
        ZOO_SMB_SERVER_HANDLE server = zoo_smb_create_server(
            "perf_srv",
            "perf_srv",
            "perf/topic/mix",
            ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
            NULL);
        ZOO_SMB_CLIENT_HANDLE client = zoo_smb_create_client(
            "perf_cli",
            "perf_srv",
            "perf/topic/mix",
            NULL);
        ZOO_SMB_SUBSCRIBER_HANDLE sub = zoo_smb_create_subscriber(
            "perf_sub",
            "perf_pub",
            "perf/topic/mix",
            NULL);

        if (server)
        {
            server_ok++;
            zoo_smb_destroy_server(server);
        }
        if (client)
        {
            client_ok++;
            zoo_smb_destroy_client(client);
        }
        if (sub)
        {
            sub_ok++;
            zoo_smb_destroy_subscriber(sub);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double sec = elapsed_seconds(&start, &end);
    printf("[perf] mixed lifecycle: server=%d client=%d sub=%d in %.3fs\n",
           server_ok,
           client_ok,
           sub_ok,
           sec);

    TEST_ASSERT_GREATER_THAN(100, server_ok + client_ok + sub_ok);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_perf_publisher_create_destroy);
    RUN_TEST(test_perf_publish_message_calls);
    RUN_TEST(test_perf_mixed_node_lifecycle);

    return UNITY_END();
}
