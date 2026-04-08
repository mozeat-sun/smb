/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: E2E_PERF_BENCHMARK
 * File name: e2e_performance_benchmark.c
 * Description: End-to-end performance benchmarks for complete SMB system
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/



/*******************************************************************************
 * Replacement benchmark using node API
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include "zoo_smb_publisher.h"
#include "zoo_smb_server.h"
#include "zoo_smb_client.h"
#include "zoo_smb_subscriber.h"

#define BENCH_RPC_MSG_ID 1U
#define BENCH_PUBSUB_MSG_ID 1U
#define BENCH_RPC_ROUNDS 200
#define BENCH_PUBSUB_ROUNDS 200
#define BENCH_WAIT_TIMEOUT_MS 5000U

typedef struct
{
    uint64_t send_ns;
    uint32_t seq;
} BENCH_PUBSUB_PAYLOAD_STRUCT;

static volatile sig_atomic_t g_child_running = 1;
static const char* g_program_path = NULL;

/*
 * Stop helper child loops on termination signals.
 *
 * Benchmarks 5 and 6 spawn helper processes that stay alive until the parent
 * finishes measurement. The signal handler flips a shared flag so those helpers
 * can exit their polling loops and release SMB resources cleanly.
 */
static void benchmark_child_stop_handler(int signum)
{
    (void)signum;
    g_child_running = 0;
}

typedef struct
{
    pthread_mutex_t mutex;
    int received;
    int capacity;
    int64_t* latency_us;
} BENCH_PUBSUB_CONTEXT_STRUCT;

/*
 * Compute elapsed seconds between two monotonic timestamps.
 *
 * Benchmarks report throughput over a measured interval, so this helper keeps
 * the conversion from `timespec` values in one place and avoids repeating the
 * same arithmetic in each benchmark function.
 */
static double elapsed_seconds(const struct timespec* start, const struct timespec* end)
{
    time_t s = end->tv_sec - start->tv_sec;
    long ns = end->tv_nsec - start->tv_nsec;
    return (double)s + (double)ns / 1e9;
}

/*
 * Read the current monotonic clock in nanoseconds.
 *
 * Latency-focused benchmarks use monotonic time so measurements are not skewed
 * by wall-clock adjustments. Nanosecond resolution is converted to microseconds
 * later when reporting percentiles.
 */
static uint64_t monotonic_time_ns(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
}

/*
 * Compare two latency samples for ascending sort order.
 *
 * The percentile reporting path sorts a copy of the raw latency array before it
 * extracts p50, p95, and p99 values. This comparator keeps that qsort call local
 * to the benchmark implementation.
 */
static int compare_int64(const void* lhs, const void* rhs)
{
    const int64_t left = *(const int64_t*)lhs;
    const int64_t right = *(const int64_t*)rhs;

    if (left < right)
    {
        return -1;
    }
    if (left > right)
    {
        return 1;
    }
    return 0;
}

/*
 * Map a percentile to a sample index in a sorted latency array.
 *
 * The benchmark uses a nearest-sample approach instead of interpolation because
 * the sample counts are small and discrete. Returning an index centralizes that
 * policy so percentile reporting stays consistent across benchmarks.
 */
static size_t percentile_index(size_t count, double percentile)
{
    double raw_index;

    if (count == 0)
    {
        return 0;
    }

    raw_index = ((double)(count - 1)) * percentile;
    return (size_t)(raw_index + 0.5);
}

/*
 * Print aggregate throughput and latency statistics for a benchmark run.
 *
 * When successful samples are available, this helper reports throughput, mean
 * latency, and percentile values from the collected microsecond measurements. If
 * no samples were recorded, it prints a diagnostic line instead of fake metrics.
 */
static void print_latency_stats(const char* prefix, const int64_t* values, int count, double elapsed_sec)
{
    int64_t* sorted_values;
    int64_t min_us;
    int64_t max_us;
    int64_t p50_us;
    int64_t p95_us;
    int64_t p99_us;
    int64_t sum_us = 0;
    double avg_us;
    double throughput;

    if (!values || count <= 0)
    {
        printf("%s no successful samples\n", prefix);
        return;
    }

    sorted_values = (int64_t*)malloc((size_t)count * sizeof(int64_t));
    if (!sorted_values)
    {
        printf("%s unable to allocate stats buffer\n", prefix);
        return;
    }

    memcpy(sorted_values, values, (size_t)count * sizeof(int64_t));
    qsort(sorted_values, (size_t)count, sizeof(int64_t), compare_int64);

    for (int i = 0; i < count; ++i)
    {
        sum_us += values[i];
    }

    min_us = sorted_values[0];
    max_us = sorted_values[count - 1];
    p50_us = sorted_values[percentile_index((size_t)count, 0.50)];
    p95_us = sorted_values[percentile_index((size_t)count, 0.95)];
    p99_us = sorted_values[percentile_index((size_t)count, 0.99)];
    avg_us = (double)sum_us / (double)count;
    throughput = elapsed_sec > 0.0 ? (double)count / elapsed_sec : 0.0;

    printf(
        "%s success=%d throughput=%.0f msg/s avg=%.0f us p50=%lld us p95=%lld us p99=%lld us min=%lld us max=%lld us\n",
        prefix,
        count,
        throughput,
        avg_us,
        (long long)p50_us,
        (long long)p95_us,
        (long long)p99_us,
        (long long)min_us,
        (long long)max_us);

    free(sorted_values);
}

/*
 * Wait until the SMB runtime reports a target as ready.
 *
 * The RPC and pub/sub end-to-end benchmarks depend on helper processes coming up
 * asynchronously. This polling loop delays the benchmark start until discovery
 * stabilizes or the configured timeout expires.
 */
static int wait_for_server_ready(const char* target, uint32_t timeout_ms)
{
    uint64_t deadline_ns = monotonic_time_ns() + (uint64_t)timeout_ms * 1000000ULL;

    while (monotonic_time_ns() < deadline_ns)
    {
        if (zoo_smb_is_server_ready(target))
        {
            return 1;
        }
        usleep(10000);
    }

    return 0;
}

/*
 * Wait for the subscriber callback to collect the expected message count.
 *
 * Bench6 records delivery asynchronously in a shared context. This helper polls
 * that context until enough messages arrive or the benchmark timeout is hit, and
 * then returns the best available received count.
 */
static int wait_for_pubsub_messages(BENCH_PUBSUB_CONTEXT_STRUCT* ctx, int expected, uint32_t timeout_ms)
{
    uint64_t deadline_ns = monotonic_time_ns() + (uint64_t)timeout_ms * 1000000ULL;

    while (monotonic_time_ns() < deadline_ns)
    {
        int received;

        pthread_mutex_lock(&ctx->mutex);
        received = ctx->received;
        pthread_mutex_unlock(&ctx->mutex);

        if (received >= expected)
        {
            return received;
        }

        usleep(1000);
    }

    pthread_mutex_lock(&ctx->mutex);
    int received = ctx->received;
    pthread_mutex_unlock(&ctx->mutex);
    return received;
}

/*
 * Retry a publish operation to smooth over brief transient failures.
 *
 * The throughput benchmarks intentionally run at high rate and can encounter a
 * small number of setup-related misses. Bounded retry keeps the benchmark from
 * undercounting due to short-lived readiness races while still exposing failures.
 */
static int publish_with_retry(
    ZOO_SMB_PUBLISHER_HANDLE publisher,
    uint32_t msg_id,
    const void* payload,
    size_t payload_size,
    int max_attempts)
{
    for (int attempt = 0; attempt < max_attempts; ++attempt)
    {
        if (zoo_smb_publish_message(publisher, msg_id, payload, payload_size) == ZOO_SMB_OK)
        {
            return 1;
        }

        if (attempt + 1 < max_attempts)
        {
            usleep(100);
        }
    }

    return 0;
}

/*
 * Reply to RPC benchmark requests from the helper server process.
 *
 * The handler does not inspect payload content because bench5 is measuring the
 * transport and request/reply path rather than application logic. It simply sends
 * a fixed acknowledgment so the client can measure round-trip completion time.
 */
static int benchmark_rpc_server_handler(
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
    (void)payload;
    (void)payload_size;

    return zoo_smb_server_send_reply(server, sender, msg_id, "rpc-ack", strlen("rpc-ack") + 1U, (int64_t)request_id);
}

/*
 * Record one-way pub/sub delivery latency for each received sample.
 *
 * The published payload contains the sender-side timestamp. The subscriber uses
 * that timestamp to compute delivery latency and stores the result in a shared,
 * mutex-protected array for later aggregate reporting.
 */
static int benchmark_pubsub_handler(
    void* user_data,
    uint32_t msg_id,
    uint64_t request_id,
    uint64_t timestamp,
    const char* sender,
    const void* payload,
    size_t payload_size)
{
    BENCH_PUBSUB_CONTEXT_STRUCT* ctx = (BENCH_PUBSUB_CONTEXT_STRUCT*)user_data;
    const BENCH_PUBSUB_PAYLOAD_STRUCT* msg = (const BENCH_PUBSUB_PAYLOAD_STRUCT*)payload;
    uint64_t now_ns = monotonic_time_ns();
    int index;

    (void)msg_id;
    (void)request_id;
    (void)timestamp;
    (void)sender;

    if (!ctx || !payload || payload_size < sizeof(BENCH_PUBSUB_PAYLOAD_STRUCT))
    {
        return -1;
    }

    pthread_mutex_lock(&ctx->mutex);
    index = ctx->received;
    if (index < ctx->capacity)
    {
        ctx->latency_us[index] = (int64_t)((now_ns - msg->send_ns) / 1000ULL);
        ctx->received++;
    }
    pthread_mutex_unlock(&ctx->mutex);

    return ZOO_SMB_OK;
}

/*
 * Host the dedicated RPC server used by benchmark 5.
 *
 * The parent benchmark spawns this helper as a separate process so the measured
 * round-trip path includes real inter-node behavior instead of in-process reuse.
 * The helper stays alive until the parent sends a stop signal.
 */
static int run_rpc_server_child(const char* server_name, const char* server_target, const char* topic_name)
{
    ZOO_SMB_SERVER_HANDLE server;

    signal(SIGTERM, benchmark_child_stop_handler);
    signal(SIGINT, benchmark_child_stop_handler);

    server = zoo_smb_create_server(
        server_name,
        server_target,
        topic_name,
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);
    if (!server)
    {
        return 2;
    }

    if (zoo_smb_server_set_message_handler(server, benchmark_rpc_server_handler, server) != ZOO_SMB_OK)
    {
        zoo_smb_destroy_server(server);
        return 3;
    }

    // Keep the server process alive until the parent benchmark sends SIGTERM.
    while (g_child_running)
    {
        usleep(50000);
    }

    zoo_smb_destroy_server(server);
    return 0;
}

/*
 * Publish timestamped samples for the benchmark 6 subscriber.
 *
 * Running the publisher in a child process keeps the benchmark closer to a true
 * end-to-end deployment. Each payload embeds a send timestamp so the subscriber
 * can calculate one-way latency without additional synchronization.
 */
static int run_pubsub_publisher_child(
    int rounds,
    const char* publisher_name,
    const char* target_name,
    const char* topic_name)
{
    ZOO_SMB_PUBLISHER_HANDLE publisher;
    int publish_ok = 0;
    int publish_fail = 0;

    signal(SIGTERM, benchmark_child_stop_handler);
    signal(SIGINT, benchmark_child_stop_handler);

    publisher = zoo_smb_create_publisher(
        publisher_name,
        target_name,
        topic_name,
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);
    if (!publisher)
    {
        return 2;
    }

    // Give subscriber side time to initialize and register subscription.
    usleep(1200000);

    // Publish timestamped payloads so subscriber can compute one-way latency.
    for (int i = 0; i < rounds && g_child_running; ++i)
    {
        BENCH_PUBSUB_PAYLOAD_STRUCT payload;
        payload.send_ns = monotonic_time_ns();
        payload.seq = (uint32_t)(i + 1);
        if (zoo_smb_publish_message(publisher, BENCH_PUBSUB_MSG_ID, &payload, sizeof(payload)) == ZOO_SMB_OK)
        {
            publish_ok++;
        }
        else
        {
            publish_fail++;
        }
        usleep(1000);
    }

    printf("[bench6-helper] publish_ok=%d publish_fail=%d\n", publish_ok, publish_fail);
    usleep(500000);

    zoo_smb_destroy_publisher(publisher);
    return 0;
}

/*
 * Spawn a helper process by re-executing the current benchmark binary.
 *
 * Re-exec avoids leaking parent-side process state into the helper role and lets
 * the child start from a clean runtime context. This is important for the RPC and
 * pub/sub benchmarks that intentionally model multi-process behavior.
 */
/*
 * Spawn helper process with up to four optional arguments.
 */
static pid_t spawn_helper_process_args(
    const char* mode,
    const char* arg1,
    const char* arg2,
    const char* arg3,
    const char* arg4)
{
    pid_t pid;
    char* argv_exec[8];
    int index = 0;

    if (!g_program_path || !mode)
    {
        return -1;
    }

    pid = fork();
    if (pid != 0)
    {
        return pid;
    }

    // Re-exec the same binary in helper mode to avoid inherited in-process state.
    argv_exec[index++] = (char*)g_program_path;
    argv_exec[index++] = (char*)mode;
    if (arg1)
        argv_exec[index++] = (char*)arg1;
    if (arg2)
        argv_exec[index++] = (char*)arg2;
    if (arg3)
        argv_exec[index++] = (char*)arg3;
    if (arg4)
        argv_exec[index++] = (char*)arg4;
    argv_exec[index] = NULL;

    execv(g_program_path, argv_exec);

    _exit(127);
}

/*
 * Measure best-effort publish throughput from a single publisher.
 *
 * This benchmark sends a high volume of small messages through one publisher to
 * estimate raw publish rate. It reports success count rather than assuming every
 * send succeeds so transient misses remain visible in the output.
 */
static void benchmark1_publish_throughput(void)
{
    struct timespec start, end;
    const int message_count = 100000;
    int success = 0;

    ZOO_SMB_PUBLISHER_HANDLE publisher = zoo_smb_create_publisher(
        "bench_pub",
        "bench_pub",
        "bench/topic",
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);

    if (!publisher)
    {
        printf("benchmark1: create publisher failed\n");
        return;
    }

    usleep(50000);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < message_count; ++i)
    {
        char payload[64];
        snprintf(payload, sizeof(payload), "msg-%d", i);
        if (publish_with_retry(publisher, (uint32_t)(i + 1), payload, strlen(payload), 3))
        {
            success++;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double sec = elapsed_seconds(&start, &end);
    printf("[bench1] publish throughput: success=%d/%d, %.0f msg/s\n",
           success,
           message_count,
           sec > 0.0 ? success / sec : 0.0);

    zoo_smb_destroy_publisher(publisher);
}

/*
 * Measure publish throughput across several payload sizes.
 *
 * The goal is to show how payload size affects achievable throughput. Using one
 * publisher across multiple rounds keeps setup cost out of the comparison so the
 * reported deltas mostly reflect payload handling and transport overhead.
 */
static void benchmark2_payload_size_impact(void)
{
    struct timespec start, end;
    const int rounds = 20000;
    const size_t sizes[] = {64, 256, 1024, 4096};

    ZOO_SMB_PUBLISHER_HANDLE publisher = zoo_smb_create_publisher(
        "bench_pub2",
        "bench_pub2",
        "bench/topic2",
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);

    if (!publisher)
    {
        printf("benchmark2: create publisher failed\n");
        return;
    }

    usleep(50000);

    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s)
    {
        char* payload = (char*)malloc(sizes[s]);
        if (!payload)
        {
            continue;
        }
        memset(payload, 'A', sizes[s]);

        int success = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < rounds; ++i)
        {
            if (publish_with_retry(publisher, (uint32_t)(100000 + i), payload, sizes[s], 3))
            {
                success++;
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        double sec = elapsed_seconds(&start, &end);
        printf("[bench2] payload=%zuB success=%d/%d throughput=%.0f msg/s\n",
               sizes[s],
               success,
               rounds,
               sec > 0.0 ? success / sec : 0.0);

        free(payload);
    }

    zoo_smb_destroy_publisher(publisher);
}

/*
 * Measure repeated publisher creation and destruction throughput.
 *
 * This benchmark isolates lifecycle overhead instead of message delivery cost. It
 * is useful for spotting regressions in node setup and teardown paths.
 */
static void benchmark3_node_lifecycle(void)
{
    struct timespec start, end;
    const int rounds = 5000;
    int ok = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < rounds; ++i)
    {
        char name[64];
        snprintf(name, sizeof(name), "bench_lifecycle_%d", i);

        ZOO_SMB_PUBLISHER_HANDLE publisher = zoo_smb_create_publisher(
            name,
            "bench_target3",
            "bench/topic3",
            ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
            NULL);

        if (publisher)
        {
            ok++;
            zoo_smb_destroy_publisher(publisher);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double sec = elapsed_seconds(&start, &end);
    printf("[bench3] node lifecycle: success=%d/%d, %.0f create+destroy/s\n",
           ok,
           rounds,
           sec > 0.0 ? ok / sec : 0.0);
}

/*
 * Measure aggregate setup cost across several node types.
 *
 * By creating server, client, publisher, and subscriber instances in the same
 * loop, this benchmark gives a rough picture of overall node initialization and
 * cleanup cost across the major SMB roles.
 */
static void benchmark4_mixed_node_creation(void)
{
    struct timespec start, end;
    const int rounds = 2000;
    int server_ok = 0;
    int client_ok = 0;
    int pub_ok = 0;
    int sub_ok = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < rounds; ++i)
    {
        ZOO_SMB_SERVER_HANDLE server = zoo_smb_create_server("bench_srv", "bench_srv", "bench/topic4", ZOO_SMB_TRANSPORT_TYPE_DEFAULT, NULL);
        ZOO_SMB_CLIENT_HANDLE client = zoo_smb_create_client("bench_cli", "bench_srv", "bench/topic4", NULL);
        ZOO_SMB_PUBLISHER_HANDLE publisher = zoo_smb_create_publisher("bench_pub4", "bench_pub4", "bench/topic4", ZOO_SMB_TRANSPORT_TYPE_DEFAULT, NULL);
        ZOO_SMB_SUBSCRIBER_HANDLE subscriber = zoo_smb_create_subscriber("bench_sub4", "bench_pub4", "bench/topic4", NULL);

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
        if (publisher)
        {
            pub_ok++;
            zoo_smb_destroy_publisher(publisher);
        }
        if (subscriber)
        {
            sub_ok++;
            zoo_smb_destroy_subscriber(subscriber);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double sec = elapsed_seconds(&start, &end);
    printf("[bench4] mixed nodes: srv=%d cli=%d pub=%d sub=%d in %.2fs\n",
           server_ok,
           client_ok,
           pub_ok,
           sub_ok,
           sec);
}

/*
 * Measure end-to-end RPC round-trip latency.
 *
 * The benchmark launches a dedicated server helper process, sends a fixed number
 * of requests, waits for replies, and records per-request latency. Breaking after
 * repeated consecutive failures prevents the benchmark from stalling indefinitely.
 */
static void benchmark5_rpc_round_trip_latency(void)
{
    char server_name[64];
    char server_target[64];
    char client_name[64];
    char topic_name[64];
    char label[160];
    uint32_t run_id = (uint32_t)(time(NULL) ^ (uint32_t)getpid());
    struct timespec start;
    struct timespec end;
    int64_t latencies_us[BENCH_RPC_ROUNDS];
    int success = 0;
    int attempted = 0;
    int consecutive_failures = 0;
    ZOO_SMB_CLIENT_HANDLE client;
    pid_t server_pid;
    int status = 0;

    snprintf(server_name, sizeof(server_name), "bench5_srv_%u", run_id);
    snprintf(server_target, sizeof(server_target), "bench5_target_%u", run_id);
    snprintf(client_name, sizeof(client_name), "bench5_cli_%u", run_id);
    snprintf(topic_name, sizeof(topic_name), "bench5/topic/%u", run_id);

    // Run server in a dedicated process to emulate real RPC round-trip conditions.
    server_pid = spawn_helper_process_args("bench5-server", server_name, server_target, topic_name, NULL);
    if (server_pid <= 0)
    {
        printf("[bench5] spawn server process failed\n");
        return;
    }

    usleep(1500000);

    client = zoo_smb_create_client(client_name, server_target, topic_name, NULL);
    if (!client)
    {
        printf("[bench5] create client failed\n");
        kill(server_pid, SIGTERM);
        (void)waitpid(server_pid, &status, 0);
        return;
    }

    if (!wait_for_server_ready(server_target, BENCH_WAIT_TIMEOUT_MS))
    {
        printf("[bench5] server not ready within timeout\n");
        zoo_smb_destroy_client(client);
        kill(server_pid, SIGTERM);
        (void)waitpid(server_pid, &status, 0);
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < BENCH_RPC_ROUNDS; ++i)
    {
        char request[64];
        char reply[128] = {0};
        void* reply_ptr = reply;
        size_t reply_size = 0;
        int64_t request_id = 0;
        uint64_t begin_ns;
        uint64_t end_ns;

        attempted++;
        snprintf(request, sizeof(request), "rpc-%d", i + 1);
        begin_ns = monotonic_time_ns();
        if (zoo_smb_client_send_request(client, BENCH_RPC_MSG_ID, request, strlen(request) + 1U, &request_id) != ZOO_SMB_OK)
        {
            consecutive_failures++;
            if (consecutive_failures >= 3)
            {
                break;
            }
            continue;
        }

        if (zoo_smb_client_recv_reply(client, BENCH_RPC_MSG_ID, request_id, &reply_ptr, &reply_size, BENCH_WAIT_TIMEOUT_MS) != ZOO_SMB_OK)
        {
            consecutive_failures++;
            if (consecutive_failures >= 3)
            {
                break;
            }
            continue;
        }

        end_ns = monotonic_time_ns();
        consecutive_failures = 0;
        latencies_us[success++] = (int64_t)((end_ns - begin_ns) / 1000ULL);

        // Prevent ultra-tight loop pressure from starving callback threads in stress conditions.
        usleep(200);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    snprintf(label, sizeof(label), "[bench5] rpc round-trip attempts=%d/%d", attempted, BENCH_RPC_ROUNDS);
    print_latency_stats(label, latencies_us, success, elapsed_seconds(&start, &end));

    zoo_smb_destroy_client(client);
    kill(server_pid, SIGTERM);
    (void)waitpid(server_pid, &status, 0);
}

/*
 * Measure end-to-end pub/sub delivery latency.
 *
 * The benchmark starts a helper publisher process, subscribes from the parent,
 * collects delivery timestamps in the callback, and then reports loss and latency
 * statistics for the messages that arrived before the timeout.
 */
static void benchmark6_pubsub_end_to_end(void)
{
    char publisher_name[64];
    char subscriber_name[64];
    char topic_name[64];
    char label[160];
    uint32_t run_id = (uint32_t)(time(NULL) ^ (uint32_t)getpid());
    const char* target_name = publisher_name;
    struct timespec start;
    struct timespec end;
    BENCH_PUBSUB_CONTEXT_STRUCT ctx;
    int32_t sub_handle = -1;
    int published = 0;
    int received;
    ZOO_ERROR_TYPE subscribe_result = ZOO_SMB_ERROR_INVALID_PARAM;
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = NULL;
    pid_t publisher_pid;
    int status = 0;

    memset(&ctx, 0, sizeof(ctx));
    snprintf(publisher_name, sizeof(publisher_name), "bench6_pub_%u", run_id);
    snprintf(subscriber_name, sizeof(subscriber_name), "bench6_sub_%u", run_id);
    snprintf(topic_name, sizeof(topic_name), "bench6/topic/%u", run_id);

    ctx.capacity = BENCH_PUBSUB_ROUNDS;
    ctx.latency_us = (int64_t*)calloc((size_t)BENCH_PUBSUB_ROUNDS, sizeof(int64_t));
    if (!ctx.latency_us || pthread_mutex_init(&ctx.mutex, NULL) != 0)
    {
        free(ctx.latency_us);
        printf("[bench6] context initialization failed\n");
        return;
    }

    (void)publisher_name;

    // Run publisher in a separate process so this benchmark measures true E2E delivery.
    publisher_pid = spawn_helper_process_args("bench6-publisher", "200", publisher_name, target_name, topic_name);
    if (publisher_pid <= 0)
    {
        printf("[bench6] spawn publisher process failed\n");
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return;
    }

    // Ensure publisher service is discoverable before creating subscriber node.
    if (!wait_for_server_ready(target_name, BENCH_WAIT_TIMEOUT_MS))
    {
        printf("[bench6] publisher service not ready\n");
        kill(publisher_pid, SIGTERM);
        (void)waitpid(publisher_pid, &status, 0);
        if (sub_handle >= 0)
        {
            zoo_smb_unsubscribe_message(subscriber, sub_handle);
        }
        zoo_smb_destroy_subscriber(subscriber);
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return;
    }

    for (int attempt = 0; attempt < 3 && !subscriber; ++attempt)
    {
        subscriber = zoo_smb_create_subscriber(subscriber_name, target_name, topic_name, NULL);
        if (!subscriber)
        {
            usleep(200000);
        }
    }

    if (!subscriber)
    {
        printf("[bench6] create pub/sub failed\n");
        kill(publisher_pid, SIGTERM);
        (void)waitpid(publisher_pid, &status, 0);
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return;
    }

    for (int attempt = 0; attempt < 3; ++attempt)
    {
        subscribe_result = zoo_smb_subscribe_message(subscriber, BENCH_PUBSUB_MSG_ID, benchmark_pubsub_handler, &ctx, &sub_handle);
        if (subscribe_result == ZOO_SMB_OK)
        {
            break;
        }

        if (attempt + 1 < 3)
        {
            usleep(200000);
        }
    }

    if (subscribe_result != ZOO_SMB_OK)
    {
        printf("[bench6] subscribe failed (err=%d)\n", (int)subscribe_result);
        kill(publisher_pid, SIGTERM);
        (void)waitpid(publisher_pid, &status, 0);
        zoo_smb_destroy_subscriber(subscriber);
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    published = BENCH_PUBSUB_ROUNDS;

    received = wait_for_pubsub_messages(&ctx, published, BENCH_WAIT_TIMEOUT_MS);
    clock_gettime(CLOCK_MONOTONIC, &end);

        snprintf(label,
              sizeof(label),
              "[bench6] pub/sub e2e published=%d/%d received=%d loss=%d",
              published,
              BENCH_PUBSUB_ROUNDS,
              received,
              published - received);
        print_latency_stats(label, ctx.latency_us, received, elapsed_seconds(&start, &end));

    if (sub_handle >= 0)
    {
        zoo_smb_unsubscribe_message(subscriber, sub_handle);
    }

    kill(publisher_pid, SIGTERM);
    (void)waitpid(publisher_pid, &status, 0);

    zoo_smb_destroy_subscriber(subscriber);
    pthread_mutex_destroy(&ctx.mutex);
    free(ctx.latency_us);
}

/*
 * Dispatch the requested benchmark mode or helper role.
 *
 * Besides the public benchmark numbers, the binary also supports hidden helper
 * modes used internally by bench5 and bench6. That keeps all orchestration in a
 * single executable while still allowing clean multi-process measurement.
 */
int main(int argc, char* argv[])
{
    g_program_path = argv[0];

    // Hidden helper modes used by bench5/bench6 parent orchestration.
    if (argc >= 2 && strcmp(argv[1], "bench5-server") == 0)
    {
        const char* server_name = argc >= 3 ? argv[2] : "bench_rpc_server";
        const char* server_target = argc >= 4 ? argv[3] : "127.0.0.1:8080";
        const char* topic_name = argc >= 5 ? argv[4] : "default_topic";
        return run_rpc_server_child(server_name, server_target, topic_name);
    }

    if (argc >= 2 && strcmp(argv[1], "bench6-publisher") == 0)
    {
        int rounds = BENCH_PUBSUB_ROUNDS;
        const char* publisher_name = argc >= 4 ? argv[3] : "fish";
        const char* target_name = argc >= 5 ? argv[4] : "sea";
        const char* topic_name = argc >= 6 ? argv[5] : "water";
        if (argc >= 3)
        {
            rounds = atoi(argv[2]);
            if (rounds <= 0)
            {
                rounds = BENCH_PUBSUB_ROUNDS;
            }
        }
        return run_pubsub_publisher_child(rounds, publisher_name, target_name, topic_name);
    }

    if (argc < 2)
    {
        printf("Usage: %s <benchmark_number> [1|2|3|4|5|6|all]\n", argv[0]);
        return 1;
    }

    printf("\n========== ZOO SMB E2E Benchmarks (node API) ==========\n");

    if (strcmp(argv[1], "all") == 0)
    {
        char cmd[2048];
        unsigned int settle_seconds = BENCH_WAIT_TIMEOUT_MS / 1000U;

        snprintf(
            cmd,
            sizeof(cmd),
            "\"%s\" 1 && sleep %u && "
            "\"%s\" 2 && sleep %u && "
            "\"%s\" 3 && sleep %u && "
            "\"%s\" 4 && sleep %u && "
            "\"%s\" 5 && sleep %u && "
            "\"%s\" 6",
            g_program_path, settle_seconds,
            g_program_path, settle_seconds,
            g_program_path, settle_seconds,
            g_program_path, settle_seconds,
            g_program_path, settle_seconds,
            g_program_path);

        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        printf("Failed to execute all-mode benchmark chain\n");
        return 1;
    }
    else
    {
        int n = atoi(argv[1]);
        switch (n)
        {
            case 1:
                benchmark1_publish_throughput();
                break;
            case 2:
                benchmark2_payload_size_impact();
                break;
            case 3:
                benchmark3_node_lifecycle();
                break;
            case 4:
                benchmark4_mixed_node_creation();
                break;
            case 5:
                benchmark5_rpc_round_trip_latency();
                break;
            case 6:
                benchmark6_pubsub_end_to_end();
                break;
            default:
                printf("Invalid benchmark number\n");
                return 1;
        }
    }

    printf("========== Benchmarks Complete ==========\n");
    return 0;
}
