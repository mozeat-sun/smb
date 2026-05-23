/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: E2E_PERF_BENCHMARK
 * File name: e2e_performance_benchmark.c
 * Description: End-to-end performance benchmarks for complete SMB system
 * Traceability coverage:
 * - REQ-PERF-001: end-to-end throughput and latency evidence for SMB flows.
 * - REQ-PERF-002: benchmark timing captures support p50, p95, and p99 reporting.
 * - REQ-PERF-003: benchmark output supports baseline and regression review.
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
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#include "zoo_smb_publisher.h"
#include "zoo_smb_server.h"
#include "zoo_smb_client.h"
#include "zoo_smb_subscriber.h"
#include "zoo_smb_config.h"
#include "zoo_log.h"

#define BENCH_RPC_MSG_ID 1U
#define BENCH_PUBSUB_MSG_ID 1U
#define BENCH_RPC_ROUNDS 200
#define BENCH_PUBSUB_ROUNDS 200
#define BENCH_WAIT_TIMEOUT_MS 5000U
#define BENCH_SUBSCRIPTION_SETTLE_US 500000U
#define BENCH_THROUGHPUT_WARMUP_MESSAGES 16
#define BENCH_STARTUP_DELIVERY_THRESHOLD 4
#define BENCH_THROUGHPUT_SMALL_PAYLOAD_SIZE 64U

typedef struct
{
    uint64_t send_ns;
    uint32_t seq;
} BENCH_PUBSUB_PAYLOAD_STRUCT;

typedef enum
{
    BENCH_RUNTIME_MODE_GUARANTEED_DELIVERY = 0,
    BENCH_RUNTIME_MODE_MAX_THROUGHPUT = 1
} BENCH_RUNTIME_MODE_ENUM;

typedef struct
{
    BENCH_RUNTIME_MODE_ENUM mode;
    const char* mode_name;
    int bench1_message_count;
    int bench2_rounds;
    uint32_t bench1_measurement_ms;
    uint32_t bench2_measurement_ms;
    int publish_retry_attempts;
    useconds_t inter_send_delay_us;
} BENCH_RUNTIME_CONFIG_STRUCT;

static volatile sig_atomic_t g_child_running = 1;
static const char* g_program_path = NULL;
static BENCH_RUNTIME_CONFIG_STRUCT g_bench_runtime_config = {
    BENCH_RUNTIME_MODE_GUARANTEED_DELIVERY,
    "guaranteed-delivery",
    20000,
    5000,
    0U,
    0U,
    32,
    200U
};

/*
 * The SMB config defaults to 127.0.0.1 when libconfig is unavailable.
 * For cross-machine benchmarks we need each process to advertise its real
 * interface address so discovery and routing work across hosts.
 */
static void bench_apply_network_overrides(void)
{
    const char* local_addr = getenv("ZOO_BENCH_LOCAL_ADDRESS");
    const char* mcast_interface = getenv("ZOO_BENCH_MULTICAST_INTERFACE");
    ZOO_SMB_CONFIG_STRUCT* cfg;

    if ((!local_addr || local_addr[0] == '\0') &&
        (!mcast_interface || mcast_interface[0] == '\0'))
    {
        return;
    }

    cfg = (ZOO_SMB_CONFIG_STRUCT*)zoo_smb_config_init();
    if (!cfg)
    {
        return;
    }

    if (local_addr && local_addr[0] != '\0')
    {
        snprintf(cfg->local_machine.address,
                 sizeof(cfg->local_machine.address),
                 "%s",
                 local_addr);
    }

    if (mcast_interface && mcast_interface[0] != '\0')
    {
        snprintf(cfg->multicast.interface,
                 sizeof(cfg->multicast.interface),
                 "%s",
                 mcast_interface);
    }

    // printf removed
}

static void bench_runtime_set_mode(BENCH_RUNTIME_MODE_ENUM mode)
{
    if (mode == BENCH_RUNTIME_MODE_MAX_THROUGHPUT)
    {
        g_bench_runtime_config.mode = mode;
        g_bench_runtime_config.mode_name = "max-throughput";
        g_bench_runtime_config.bench1_message_count = 100000;
        g_bench_runtime_config.bench2_rounds = 20000;
        g_bench_runtime_config.bench1_measurement_ms = 10000U;
        g_bench_runtime_config.bench2_measurement_ms = 4000U;
        g_bench_runtime_config.publish_retry_attempts = 8;
        g_bench_runtime_config.inter_send_delay_us = 0U;
        return;
    }

    g_bench_runtime_config.mode = BENCH_RUNTIME_MODE_GUARANTEED_DELIVERY;
    g_bench_runtime_config.mode_name = "guaranteed-delivery";
    g_bench_runtime_config.bench1_message_count = 20000;
    g_bench_runtime_config.bench2_rounds = 5000;
    g_bench_runtime_config.bench1_measurement_ms = 0U;
    g_bench_runtime_config.bench2_measurement_ms = 0U;
    g_bench_runtime_config.publish_retry_attempts = 32;
    g_bench_runtime_config.inter_send_delay_us = 200U;
}

static BENCH_RUNTIME_MODE_ENUM bench_runtime_parse_mode(const char* arg)
{
    if (arg && (strcmp(arg, "max-throughput") == 0 || strcmp(arg, "throughput") == 0 || strcmp(arg, "fast") == 0))
    {
        return BENCH_RUNTIME_MODE_MAX_THROUGHPUT;
    }

    return BENCH_RUNTIME_MODE_GUARANTEED_DELIVERY;
}

static void print_benchmark_usage(const char* program)
{
    (void)program;

    // printf removed
}

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
    uint64_t first_receive_ns;
    uint64_t last_receive_ns;
} BENCH_PUBSUB_CONTEXT_STRUCT;

typedef struct
{
    uint64_t calls;
    uint64_t successes;
    uint64_t failed_calls;
    uint64_t total_attempts;
    uint64_t queue_full_failures;
    uint64_t other_failures;
    uint64_t non_retryable_failures;
    uint32_t first_non_retryable_error;
    uint64_t timeout_failures;
    uint64_t service_unavailable_failures;
    uint64_t service_not_found_failures;
    uint64_t not_initialized_failures;
    uint64_t operation_failed_failures;
    uint64_t shm_buffer_full_failures;
    uint64_t allocation_failed_failures;
    uint64_t busy_again_failures;
    uint64_t total_duration_ns;
    uint64_t max_duration_ns;
} BENCH_CALL_TIMING_STATS_STRUCT;

static int wait_for_server_ready(const char* target, uint32_t timeout_ms);
static int wait_for_pubsub_messages(BENCH_PUBSUB_CONTEXT_STRUCT* ctx, int expected, uint32_t timeout_ms);
static int publish_with_retry(
    ZOO_SMB_PUBLISHER_HANDLE publisher,
    uint32_t msg_id,
    const void* payload,
    size_t payload_size,
    int max_attempts,
    BENCH_CALL_TIMING_STATS_STRUCT* stats);

static void bench_call_timing_record_error(
    BENCH_CALL_TIMING_STATS_STRUCT* stats,
    ZOO_ERROR_TYPE err)
{
    if (!stats)
    {
        return;
    }

    if (err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_TIMEOUT)
    {
        stats->timeout_failures++;
    }
    else if (err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SERVICE_UNAVAILABLE)
    {
        stats->service_unavailable_failures++;
    }
    else if (err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SERVICE_NOT_FOUND)
    {
        stats->service_not_found_failures++;
    }
    else if (err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_NOT_INITIALIZED)
    {
        stats->not_initialized_failures++;
    }
    else if (err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_OPERATION_FAILED)
    {
        stats->operation_failed_failures++;
    }
    else if (err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SHM_BUFFER_FULL)
    {
        stats->shm_buffer_full_failures++;
    }
    else if (err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_OUT_OF_MEMORY ||
             err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_ALLOCATION_FAILED ||
             err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_POOL_EXHAUSTED)
    {
        stats->allocation_failed_failures++;
    }
    else if (err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_BUSY ||
             err == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_AGAIN)
    {
        stats->busy_again_failures++;
    }
}

static void bench_call_timing_record(
    BENCH_CALL_TIMING_STATS_STRUCT* stats,
    ZOO_BOOL success,
    uint64_t attempts,
    uint64_t queue_full_failures,
    uint64_t other_failures,
    uint64_t duration_ns)
{
    if (!stats)
    {
        return;
    }

    stats->calls++;
    stats->total_attempts += attempts;
    stats->queue_full_failures += queue_full_failures;
    stats->other_failures += other_failures;
    stats->total_duration_ns += duration_ns;
    if (duration_ns > stats->max_duration_ns)
    {
        stats->max_duration_ns = duration_ns;
    }

    if (success)
    {
        stats->successes++;
    }
    else
    {
        stats->failed_calls++;
    }
}

static void bench_call_timing_print(
    const char* label,
    const BENCH_CALL_TIMING_STATS_STRUCT* stats)
{
    double avg_attempts;
    double avg_call_us;
    double max_call_us;

    if (!label || !stats || stats->calls == 0)
    {
        return;
    }

    avg_attempts = (double)stats->total_attempts / (double)stats->calls;
    avg_call_us = ((double)stats->total_duration_ns / (double)stats->calls) / 1000.0;
    max_call_us = (double)stats->max_duration_ns / 1000.0;

    printf("[%s] calls=%llu success=%llu failed=%llu avg_attempts=%.2f avg_call=%.2f us max_call=%.2f us queue_full=%llu other_retryable=%llu\n",
           label,
           (unsigned long long)stats->calls,
           (unsigned long long)stats->successes,
           (unsigned long long)stats->failed_calls,
           avg_attempts,
           avg_call_us,
           max_call_us,
           (unsigned long long)stats->queue_full_failures,
           (unsigned long long)stats->other_failures);

    if (stats->non_retryable_failures > 0)
    {
        printf("[%s] non_retryable_failures=%llu first_error=%u\n",
               label,
               (unsigned long long)stats->non_retryable_failures,
               stats->first_non_retryable_error);
    }

    if (stats->timeout_failures > 0 ||
        stats->service_unavailable_failures > 0 ||
        stats->service_not_found_failures > 0 ||
        stats->not_initialized_failures > 0 ||
        stats->operation_failed_failures > 0 ||
        stats->shm_buffer_full_failures > 0 ||
        stats->allocation_failed_failures > 0 ||
        stats->busy_again_failures > 0)
    {
        printf("[%s] timeout=%llu service_unavailable=%llu service_not_found=%llu not_initialized=%llu operation_failed=%llu shm_buffer_full=%llu allocation_failed=%llu busy_again=%llu\n",
               label,
               (unsigned long long)stats->timeout_failures,
               (unsigned long long)stats->service_unavailable_failures,
               (unsigned long long)stats->service_not_found_failures,
               (unsigned long long)stats->not_initialized_failures,
               (unsigned long long)stats->operation_failed_failures,
               (unsigned long long)stats->shm_buffer_full_failures,
               (unsigned long long)stats->allocation_failed_failures,
               (unsigned long long)stats->busy_again_failures);
    }
}

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

    printf("%s success=%d throughput=%.0f msg/s avg=%.0f us p50=%lld us p95=%lld us p99=%lld us min=%lld us max=%lld us\n",
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
 * Reset the shared pub/sub benchmark context between warmup and measurement phases.
 */
static void reset_pubsub_context(BENCH_PUBSUB_CONTEXT_STRUCT* ctx)
{
    if (!ctx)
    {
        return;
    }

    pthread_mutex_lock(&ctx->mutex);
    ctx->received = 0;
    ctx->first_receive_ns = 0;
    ctx->last_receive_ns = 0;
    if (ctx->latency_us && ctx->capacity > 0)
    {
        memset(ctx->latency_us, 0, (size_t)ctx->capacity * sizeof(int64_t));
    }
    pthread_mutex_unlock(&ctx->mutex);
}

/*
 * Fill a benchmark payload buffer with the timestamp/sequence header expected by the subscriber.
 */
static void fill_pubsub_payload(void* payload, size_t payload_size, uint32_t seq)
{
    BENCH_PUBSUB_PAYLOAD_STRUCT* header = (BENCH_PUBSUB_PAYLOAD_STRUCT*)payload;

    if (!payload || payload_size < sizeof(BENCH_PUBSUB_PAYLOAD_STRUCT))
    {
        return;
    }

    memset(payload, 'A', payload_size);
    header->send_ns = monotonic_time_ns();
    header->seq = seq;
}

/*
 * Create a unique benchmark resource name so repeated runs do not discover stale prior instances.
 */
static void make_benchmark_name(char* buffer, size_t buffer_size, const char* prefix, uint32_t run_id)
{
    if (!buffer || buffer_size == 0U)
    {
        return;
    }

    snprintf(buffer, buffer_size, "%s_%u", prefix, run_id);
}

/*
 * Wait for the pub/sub delivery path to become live and observable before starting the measured phase.
 */
static int prepare_pubsub_measurement_window(BENCH_PUBSUB_CONTEXT_STRUCT* ctx, uint32_t timeout_ms)
{
    if (!ctx)
    {
        return 0;
    }

    if (wait_for_pubsub_messages(ctx, BENCH_STARTUP_DELIVERY_THRESHOLD, timeout_ms) < BENCH_STARTUP_DELIVERY_THRESHOLD)
    {
        return 0;
    }
    return 1;
}

/*
 * Snapshot receive counters and timestamps from the shared pub/sub context.
 */
static int snapshot_pubsub_context(
    BENCH_PUBSUB_CONTEXT_STRUCT* ctx,
    uint64_t* first_receive_ns,
    uint64_t* last_receive_ns)
{
    int received = 0;

    if (!ctx)
    {
        return 0;
    }

    pthread_mutex_lock(&ctx->mutex);
    received = ctx->received;
    if (first_receive_ns)
    {
        *first_receive_ns = ctx->first_receive_ns;
    }
    if (last_receive_ns)
    {
        *last_receive_ns = ctx->last_receive_ns;
    }
    pthread_mutex_unlock(&ctx->mutex);

    return received;
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
    int max_attempts,
    BENCH_CALL_TIMING_STATS_STRUCT* stats)
{
    uint64_t start_ns = monotonic_time_ns();
    uint64_t attempts = 0;
    uint64_t queue_full_failures = 0;
    uint64_t other_failures = 0;
    int attempt_limit = max_attempts;

    if (attempt_limit < 1)
    {
        attempt_limit = 1;
    }

    for (int attempt = 0; attempt < attempt_limit; ++attempt)
    {
        ZOO_ERROR_TYPE ret;
        ZOO_BOOL retryable;
        attempts++;

        ret = zoo_smb_publish_message(publisher, msg_id, payload, payload_size);
        if (ret == ZOO_SMB_OK)
        {
            bench_call_timing_record(stats,
                                     ZOO_TRUE,
                                     attempts,
                                     queue_full_failures,
                                     other_failures,
                                     monotonic_time_ns() - start_ns);
            return 1;
        }

        if (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_QUEUE_FULL)
        {
            queue_full_failures++;
        }
        else
        {
            other_failures++;
        }

        bench_call_timing_record_error(stats, ret);

        retryable = (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_QUEUE_FULL) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_TIMEOUT) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_NOT_INITIALIZED) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SERVICE_NOT_FOUND) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SERVICE_UNAVAILABLE) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_OPERATION_FAILED) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SHM_BUFFER_FULL) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_OUT_OF_MEMORY) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_ALLOCATION_FAILED) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_POOL_EXHAUSTED) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_BUSY) ||
                    (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_AGAIN);

        if (!retryable)
        {
            if (stats)
            {
                stats->non_retryable_failures++;
                if (stats->first_non_retryable_error == 0U)
                {
                    stats->first_non_retryable_error = (uint32_t)ret;
                }
            }
            break;
        }

        if (attempt + 1 < attempt_limit)
        {
            /* Exponential backoff on queue pressure avoids immediate re-rejection storms. */
            useconds_t backoff_us;
            if (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_QUEUE_FULL)
            {
                useconds_t base_us = (payload_size <= 64U) ? 100U :
                                     (payload_size <= 256U) ? 150U :
                                     (payload_size <= 1024U) ? 250U :
                                     400U;
                useconds_t max_us = (payload_size <= 64U) ? 20000U :
                                    (payload_size <= 256U) ? 25000U :
                                    (payload_size <= 1024U) ? 30000U :
                                    40000U;
                backoff_us = (useconds_t)(base_us << (attempt < 5 ? attempt : 5));
                if (backoff_us > max_us)
                {
                    backoff_us = max_us;
                }
            }
            else if (ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_OUT_OF_MEMORY ||
                     ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_ALLOCATION_FAILED ||
                     ret == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_POOL_EXHAUSTED)
            {
                /* Allocation pressure needs longer drain time than queue pressure. */
                useconds_t base_us = (payload_size <= 64U) ? 300U :
                                     (payload_size <= 256U) ? 500U :
                                     (payload_size <= 1024U) ? 800U :
                                     1000U;
                useconds_t max_us = (payload_size <= 64U) ? 30000U :
                                    (payload_size <= 256U) ? 35000U :
                                    (payload_size <= 1024U) ? 40000U :
                                    50000U;
                backoff_us = (useconds_t)(base_us * (unsigned int)(attempt + 1));
                if (backoff_us > max_us)
                {
                    backoff_us = max_us;
                }
            }
            else
            {
                backoff_us = (payload_size <= 64U) ? 50U : 200U;
            }
            usleep(backoff_us);
        }
    }

    bench_call_timing_record(stats,
                             ZOO_FALSE,
                             attempts,
                             queue_full_failures,
                             other_failures,
                             monotonic_time_ns() - start_ns);
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

    if (msg->seq <= BENCH_THROUGHPUT_WARMUP_MESSAGES)
    {
        return ZOO_SMB_OK;
    }

    pthread_mutex_lock(&ctx->mutex);
    index = ctx->received;
    if (index < ctx->capacity)
    {
        if (ctx->received == 0)
        {
            ctx->first_receive_ns = now_ns;
        }
        ctx->latency_us[index] = (int64_t)((now_ns - msg->send_ns) / 1000ULL);
        ctx->last_receive_ns = now_ns;
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
    const char* topic_name,
    size_t payload_size,
    useconds_t inter_send_delay_us)
{
    ZOO_SMB_PUBLISHER_HANDLE publisher;
    uint8_t* payload;
    int publish_ok = 0;
    int publish_fail = 0;
    BENCH_CALL_TIMING_STATS_STRUCT call_stats = {0};

    if (payload_size < sizeof(BENCH_PUBSUB_PAYLOAD_STRUCT))
    {
        payload_size = sizeof(BENCH_PUBSUB_PAYLOAD_STRUCT);
    }

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

    payload = (uint8_t*)malloc(payload_size);
    if (!payload)
    {
        zoo_smb_destroy_publisher(publisher);
        return 3;
    }

    // Force lazy publisher association/registration before measured traffic.
    fill_pubsub_payload(payload, payload_size, 0U);
    (void)zoo_smb_publish_message(publisher, BENCH_PUBSUB_MSG_ID, payload, payload_size);

    // Ensure publisher service is discoverable before measured traffic.
    (void)wait_for_server_ready(publisher_name, BENCH_WAIT_TIMEOUT_MS * 3U);

    // Drive subscription negotiation with an explicit warmup burst, then pause before measurement.
    for (int i = 0; i < BENCH_THROUGHPUT_WARMUP_MESSAGES && g_child_running; ++i)
    {
        fill_pubsub_payload(payload, payload_size, (uint32_t)(i + 1));
        (void)publish_with_retry(
            publisher,
            BENCH_PUBSUB_MSG_ID,
            payload,
            payload_size,
            g_bench_runtime_config.publish_retry_attempts,
            &call_stats);
        usleep(1000);
    }

    usleep(500000);

    // Publish timestamped payloads so subscriber can compute one-way latency.
    for (int i = 0; i < rounds && g_child_running; ++i)
    {
        fill_pubsub_payload(payload, payload_size, (uint32_t)(i + 1 + BENCH_THROUGHPUT_WARMUP_MESSAGES));
        if (publish_with_retry(
                publisher,
                BENCH_PUBSUB_MSG_ID,
                payload,
                payload_size,
                g_bench_runtime_config.publish_retry_attempts,
            &call_stats))
        {
            publish_ok++;
        }
        else
        {
            publish_fail++;
        }
        if (inter_send_delay_us > 0U)
        {
            usleep(inter_send_delay_us);
        }
    }

    printf("[bench6-helper] publish_ok=%d publish_fail=%d\n", publish_ok, publish_fail);
    bench_call_timing_print("bench6-helper", &call_stats);
    usleep(1500000);

    free(payload);
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
    const char* arg4,
    const char* arg5,
    const char* arg6)
{
    pid_t pid;
    char* argv_exec[10];
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
    if (arg5)
        argv_exec[index++] = (char*)arg5;
    if (arg6)
        argv_exec[index++] = (char*)arg6;
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
    uint32_t run_id = (uint32_t)(monotonic_time_ns() & 0xffffffffu);
    struct timespec start, end;
    const int message_count = g_bench_runtime_config.bench1_message_count;
    const ZOO_BOOL time_window_mode = g_bench_runtime_config.bench1_measurement_ms > 0U;
    int received = 0;
    int status = 0;
    int sub_handle = -1;
    pid_t publisher_pid = -1;
    int helper_rounds;
    char publisher_name[64];
    char subscriber_name[64];
    char topic_name[64];
    char rounds_arg[32];
    char payload_arg[32];
    char delay_arg[32];
    double startup_ms;
    double sec;
    BENCH_PUBSUB_CONTEXT_STRUCT ctx;
    ZOO_ERROR_TYPE subscribe_result = ZOO_SMB_ERROR_INVALID_PARAM;
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = NULL;

    memset(&ctx, 0, sizeof(ctx));
    make_benchmark_name(publisher_name, sizeof(publisher_name), "bench1_pub", run_id);
    make_benchmark_name(subscriber_name, sizeof(subscriber_name), "bench1_sub", run_id);
    make_benchmark_name(topic_name, sizeof(topic_name), "bench1/topic", run_id);

    ctx.capacity = message_count;
    ctx.latency_us = (int64_t*)calloc((size_t)message_count, sizeof(int64_t));
    if (!ctx.latency_us || pthread_mutex_init(&ctx.mutex, NULL) != 0)
    {
        free(ctx.latency_us);
        // printf removed
        return;
    }

    for (int attempt = 0; attempt < 3 && !subscriber; ++attempt)
    {
        subscriber = zoo_smb_create_subscriber(subscriber_name, publisher_name, topic_name, NULL);
        if (!subscriber)
        {
            usleep(200000);
        }
    }

    if (!subscriber)
    {
        // printf removed
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
        // printf removed
        zoo_smb_destroy_subscriber(subscriber);
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return;
    }

    helper_rounds = time_window_mode ? INT_MAX : message_count;

    usleep(BENCH_SUBSCRIPTION_SETTLE_US);

    snprintf(rounds_arg, sizeof(rounds_arg), "%d", helper_rounds);
    snprintf(payload_arg, sizeof(payload_arg), "%u", (unsigned int)BENCH_THROUGHPUT_SMALL_PAYLOAD_SIZE);
    snprintf(delay_arg, sizeof(delay_arg), "%u", (unsigned int)g_bench_runtime_config.inter_send_delay_us);

    uint64_t startup_begin_ns = monotonic_time_ns();
    publisher_pid = spawn_helper_process_args(
        "bench6-publisher",
        rounds_arg,
        publisher_name,
        publisher_name,
        topic_name,
        payload_arg,
        delay_arg);
    if (publisher_pid <= 0)
    {
        printf("benchmark1: create publisher helper failed\n");
        if (sub_handle >= 0)
        {
            zoo_smb_unsubscribe_message(subscriber, sub_handle);
        }
        zoo_smb_destroy_subscriber(subscriber);
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return;
    }

    if (!prepare_pubsub_measurement_window(&ctx, BENCH_WAIT_TIMEOUT_MS * 4U))
    {
        // printf removed
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

    startup_ms = (double)(monotonic_time_ns() - startup_begin_ns) / 1000000.0;
        printf("[bench1] startup ready=1 settle_ms=%.2f warmup=%u\n",
            startup_ms,
            (unsigned int)BENCH_THROUGHPUT_WARMUP_MESSAGES);

    clock_gettime(CLOCK_MONOTONIC, &start);
    if (time_window_mode)
    {
        usleep(g_bench_runtime_config.bench1_measurement_ms * 1000U);
        received = snapshot_pubsub_context(&ctx, NULL, NULL);
        kill(publisher_pid, SIGTERM);
        (void)waitpid(publisher_pid, &status, 0);
    }
    else
    {
        received = wait_for_pubsub_messages(&ctx, message_count, BENCH_WAIT_TIMEOUT_MS * 12U);
        (void)waitpid(publisher_pid, &status, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    sec = elapsed_seconds(&start, &end);
    if (!time_window_mode && ctx.first_receive_ns > 0 && ctx.last_receive_ns > ctx.first_receive_ns)
    {
        sec = (double)(ctx.last_receive_ns - ctx.first_receive_ns) / 1000000000.0;
    }

    if (time_window_mode)
    {
        printf("[bench1] publish throughput: received=%d window_ms=%u startup_ms=%.2f, %.0f msg/s\n",
               received,
               g_bench_runtime_config.bench1_measurement_ms,
               startup_ms,
               sec > 0.0 ? (double)received / sec : 0.0);
    }
    else
    {
        printf("[bench1] publish throughput: received=%d/%d startup_ms=%.2f, %.0f msg/s\n",
               received,
               message_count,
               startup_ms,
               sec > 0.0 ? (double)received / sec : 0.0);
    }
    print_latency_stats("[bench1-latency]", ctx.latency_us, received, sec);

    if (sub_handle >= 0)
    {
        zoo_smb_unsubscribe_message(subscriber, sub_handle);
    }
    zoo_smb_destroy_subscriber(subscriber);
    pthread_mutex_destroy(&ctx.mutex);
    free(ctx.latency_us);
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
    uint32_t run_id = (uint32_t)(monotonic_time_ns() & 0xffffffffu);
    struct timespec start, end;
    const int rounds = g_bench_runtime_config.bench2_rounds;
    const ZOO_BOOL time_window_mode = g_bench_runtime_config.bench2_measurement_ms > 0U;
    const size_t sizes[] = {64, 256, 1024, 4096};
    char publisher_name[96];
    char subscriber_name[96];
    char topic_name[96];
    char prefix[48];
    char rounds_arg[32];
    char payload_arg[32];
    char delay_arg[32];
    int status = 0;
    double startup_ms;
    BENCH_PUBSUB_CONTEXT_STRUCT ctx;

    memset(&ctx, 0, sizeof(ctx));

    ctx.capacity = rounds;
    ctx.latency_us = (int64_t*)calloc((size_t)rounds, sizeof(int64_t));
    if (!ctx.latency_us || pthread_mutex_init(&ctx.mutex, NULL) != 0)
    {
        free(ctx.latency_us);
        // printf removed
        return;
    }

    snprintf(delay_arg, sizeof(delay_arg), "%u", (unsigned int)g_bench_runtime_config.inter_send_delay_us);

    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s)
    {
        ZOO_ERROR_TYPE subscribe_result = ZOO_SMB_ERROR_INVALID_PARAM;
        ZOO_SMB_SUBSCRIBER_HANDLE subscriber = NULL;
        int sub_handle = -1;
        pid_t publisher_pid = -1;
        ZOO_SMB_PUBLISHER_HANDLE local_publisher = NULL;
        uint8_t* local_payload = NULL;
        ZOO_BOOL use_local_publisher = ZOO_FALSE;
        uint32_t local_seq = (uint32_t)(BENCH_THROUGHPUT_WARMUP_MESSAGES + 1);
        int helper_rounds = time_window_mode ? INT_MAX : rounds;
        int received = 0;
        double sec;
        ZOO_BOOL ready = ZOO_FALSE;
        const uint32_t readiness_timeout_ms = 2500U;

        snprintf(prefix, sizeof(prefix), "bench2_pub_%zu", s);
        make_benchmark_name(publisher_name, sizeof(publisher_name), prefix, run_id);
        snprintf(prefix, sizeof(prefix), "bench2_sub_%zu", s);
        make_benchmark_name(subscriber_name, sizeof(subscriber_name), prefix, run_id);
        snprintf(prefix, sizeof(prefix), "bench2/topic/%zu", s);
        make_benchmark_name(topic_name, sizeof(topic_name), prefix, run_id);


        for (int attempt = 0; attempt < 3 && !subscriber; ++attempt)
        {
            subscriber = zoo_smb_create_subscriber(subscriber_name, publisher_name, topic_name, NULL);
            printf("[bench2-debug] payload=%zuB attempt=%d create_subscriber=%p\n", sizes[s], attempt, (void*)subscriber);
            if (!subscriber)
            {
                usleep(200000);
            }
        }

        if (!subscriber)
        {
            // printf removed
            continue;
        }

        for (int attempt = 0; attempt < 3; ++attempt)
        {
            subscribe_result = zoo_smb_subscribe_message(subscriber, BENCH_PUBSUB_MSG_ID, benchmark_pubsub_handler, &ctx, &sub_handle);
            printf("[bench2-debug] payload=%zuB attempt=%d subscribe_result=%d sub_handle=%d\n", sizes[s], attempt, (int)subscribe_result, sub_handle);
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
            // printf removed
            zoo_smb_destroy_subscriber(subscriber);
            continue;
        }


        printf("[bench2-debug] payload=%zuB subscriber created and subscribed, sleeping to settle...\n", sizes[s]);
        usleep(BENCH_SUBSCRIPTION_SETTLE_US);

        snprintf(rounds_arg, sizeof(rounds_arg), "%d", helper_rounds);
        snprintf(payload_arg, sizeof(payload_arg), "%u", (unsigned int)sizes[s]);
        reset_pubsub_context(&ctx);


        for (int startup_attempt = 0; startup_attempt < 1 && !ready; ++startup_attempt)
        {
            uint64_t startup_begin_ns = monotonic_time_ns();

            for (int spawn_attempt = 0; spawn_attempt < 3; ++spawn_attempt)
            {
                publisher_pid = spawn_helper_process_args(
                    "bench6-publisher",
                    rounds_arg,
                    publisher_name,
                    publisher_name,
                    topic_name,
                    payload_arg,
                    delay_arg);
                printf("[bench2-debug] payload=%zuB spawn_attempt=%d publisher_pid=%d\n", sizes[s], spawn_attempt, (int)publisher_pid);
                if (publisher_pid > 0)
                {
                    break;
                }

                if (spawn_attempt + 1 < 3)
                {
                    usleep(200000);
                }
            }

            if (publisher_pid <= 0)
            {
                printf("[bench2-debug] payload=%zuB failed to spawn publisher process\n", sizes[s]);
                break;
            }

            ready = prepare_pubsub_measurement_window(&ctx, readiness_timeout_ms) ? ZOO_TRUE : ZOO_FALSE;
            printf("[bench2-debug] payload=%zuB prepare_pubsub_measurement_window ready=%d\n", sizes[s], (int)ready);
            if (!ready)
            {
                kill(publisher_pid, SIGTERM);
                (void)waitpid(publisher_pid, &status, 0);
                publisher_pid = -1;
                reset_pubsub_context(&ctx);
                usleep(250000);
                continue;
            }

            startup_ms = (double)(monotonic_time_ns() - startup_begin_ns) / 1000000.0;
            printf("[bench2] payload=%zuB startup ready=1 settle_ms=%.2f warmup=%u\n",
                   sizes[s],
                   startup_ms,
                   (unsigned int)BENCH_THROUGHPUT_WARMUP_MESSAGES);
        }


        if (publisher_pid <= 0)
        {
            local_publisher = zoo_smb_create_publisher(
                publisher_name,
                publisher_name,
                topic_name,
                ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
                NULL);
            printf("[bench2-debug] payload=%zuB create_publisher=%p\n", sizes[s], (void*)local_publisher);
            if (!local_publisher)
            {
                printf("[bench2] payload=%zuB create publisher helper failed\n", sizes[s]);
                if (sub_handle >= 0)
                {
                    zoo_smb_unsubscribe_message(subscriber, sub_handle);
                }
                zoo_smb_destroy_subscriber(subscriber);
                continue;
            }

            local_payload = (uint8_t*)malloc(sizes[s]);
            printf("[bench2-debug] payload=%zuB malloc local_payload=%p\n", sizes[s], (void*)local_payload);
            if (!local_payload)
            {
                printf("[bench2] payload=%zuB allocate local payload failed\n", sizes[s]);
                zoo_smb_destroy_publisher(local_publisher);
                if (sub_handle >= 0)
                {
                    zoo_smb_unsubscribe_message(subscriber, sub_handle);
                }
                zoo_smb_destroy_subscriber(subscriber);
                continue;
            }

            for (int i = 0; i < BENCH_THROUGHPUT_WARMUP_MESSAGES; ++i)
            {
                fill_pubsub_payload(local_payload, sizes[s], (uint32_t)(i + 1));
                int pub_result = publish_with_retry(
                    local_publisher,
                    BENCH_PUBSUB_MSG_ID,
                    local_payload,
                    sizes[s],
                    g_bench_runtime_config.publish_retry_attempts,
                    NULL);
                printf("[bench2-debug] payload=%zuB warmup publish i=%d pub_result=%d\n", sizes[s], i, pub_result);
                usleep(1000);
            }

            ready = prepare_pubsub_measurement_window(&ctx, readiness_timeout_ms) ? ZOO_TRUE : ZOO_FALSE;
            printf("[bench2-debug] payload=%zuB after warmup, ready=%d\n", sizes[s], (int)ready);
            use_local_publisher = ZOO_TRUE;
        }

        if (!ready)
        {
            // printf removed
        }

        clock_gettime(CLOCK_MONOTONIC, &start);
        if (time_window_mode)
        {
            if (use_local_publisher)
            {
                uint64_t deadline_ns = monotonic_time_ns() + (uint64_t)g_bench_runtime_config.bench2_measurement_ms * 1000000ULL;
                while (monotonic_time_ns() < deadline_ns)
                {
                    fill_pubsub_payload(local_payload, sizes[s], local_seq++);
                    (void)publish_with_retry(
                        local_publisher,
                        BENCH_PUBSUB_MSG_ID,
                        local_payload,
                        sizes[s],
                        g_bench_runtime_config.publish_retry_attempts,
                        NULL);
                    if (g_bench_runtime_config.inter_send_delay_us > 0U)
                    {
                        usleep(g_bench_runtime_config.inter_send_delay_us);
                    }
                }
            }
            else
            {
                usleep(g_bench_runtime_config.bench2_measurement_ms * 1000U);
            }
            received = snapshot_pubsub_context(&ctx, NULL, NULL);
            if (!use_local_publisher)
            {
                kill(publisher_pid, SIGTERM);
                (void)waitpid(publisher_pid, &status, 0);
            }
        }
        else
        {
            if (use_local_publisher)
            {
                for (int i = 0; i < rounds; ++i)
                {
                    fill_pubsub_payload(local_payload, sizes[s], local_seq++);
                    (void)publish_with_retry(
                        local_publisher,
                        BENCH_PUBSUB_MSG_ID,
                        local_payload,
                        sizes[s],
                        g_bench_runtime_config.publish_retry_attempts,
                        NULL);
                }
            }
            received = wait_for_pubsub_messages(&ctx, rounds, BENCH_WAIT_TIMEOUT_MS * 12U);
            if (!use_local_publisher)
            {
                (void)waitpid(publisher_pid, &status, 0);
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        sec = elapsed_seconds(&start, &end);
        if (!time_window_mode && ctx.first_receive_ns > 0 && ctx.last_receive_ns > ctx.first_receive_ns)
        {
            sec = (double)(ctx.last_receive_ns - ctx.first_receive_ns) / 1000000000.0;
        }

        if (time_window_mode)
        {
            printf("[bench2] payload=%zuB received=%d window_ms=%u throughput=%.0f msg/s\n",
                   sizes[s],
                   received,
                   g_bench_runtime_config.bench2_measurement_ms,
                   sec > 0.0 ? (double)received / sec : 0.0);
        }
        else
        {
            printf("[bench2] payload=%zuB success=%d/%d throughput=%.0f msg/s\n",
                   sizes[s],
                   received,
                   rounds,
                   sec > 0.0 ? (double)received / sec : 0.0);
        }
        print_latency_stats("[bench2-latency]", ctx.latency_us, received, sec);

        if (sub_handle >= 0)
        {
            zoo_smb_unsubscribe_message(subscriber, sub_handle);
        }
        if (local_payload)
        {
            free(local_payload);
        }
        if (local_publisher)
        {
            zoo_smb_destroy_publisher(local_publisher);
        }
        zoo_smb_destroy_subscriber(subscriber);
        usleep(200000);
    }

    pthread_mutex_destroy(&ctx.mutex);
    free(ctx.latency_us);
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
            sec > 0.0 ? (double)ok / sec : 0.0);
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
    server_pid = spawn_helper_process_args("bench5-server", server_name, server_target, topic_name, NULL, NULL, NULL);
    if (server_pid <= 0)
    {
        // printf removed
        return;
    }

    usleep(1500000);

    client = zoo_smb_create_client(client_name, server_target, topic_name, NULL);
    if (!client)
    {
        // printf removed
        kill(server_pid, SIGTERM);
        (void)waitpid(server_pid, &status, 0);
        return;
    }

    if (!wait_for_server_ready(server_target, BENCH_WAIT_TIMEOUT_MS))
    {
        // printf removed
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
    char rounds_arg[16];
    char label[160];
    uint32_t run_id = (uint32_t)(time(NULL) ^ (uint32_t)getpid());
    struct timespec start;
    struct timespec end;
    BENCH_PUBSUB_CONTEXT_STRUCT ctx;
    int32_t sub_handle = -1;
    int published = 0;
    int received;
    int status = 0;
    uint64_t first_receive_ns = 0;
    uint64_t last_receive_ns = 0;
    pid_t publisher_pid = -1;
    ZOO_ERROR_TYPE subscribe_result = ZOO_SMB_ERROR_INVALID_PARAM;
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = NULL;

    memset(&ctx, 0, sizeof(ctx));
    snprintf(publisher_name, sizeof(publisher_name), "bench6_pub_%u", run_id);
    snprintf(subscriber_name, sizeof(subscriber_name), "bench6_sub_%u", run_id);
    snprintf(topic_name, sizeof(topic_name), "bench6/topic/%u", run_id);
    snprintf(rounds_arg, sizeof(rounds_arg), "%d", BENCH_PUBSUB_ROUNDS);

    ctx.capacity = BENCH_PUBSUB_ROUNDS;
    ctx.latency_us = (int64_t*)calloc((size_t)BENCH_PUBSUB_ROUNDS, sizeof(int64_t));
    if (!ctx.latency_us || pthread_mutex_init(&ctx.mutex, NULL) != 0)
    {
        free(ctx.latency_us);
        // printf removed
        return;
    }

    for (int attempt = 0; attempt < 3 && !subscriber; ++attempt)
    {
        subscriber = zoo_smb_create_subscriber(subscriber_name, publisher_name, topic_name, NULL);
        if (!subscriber)
        {
            usleep(200000);
        }
    }

    if (!subscriber)
    {
        // printf removed
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
        // printf removed
        kill(publisher_pid, SIGTERM);
        (void)waitpid(publisher_pid, &status, 0);
        zoo_smb_destroy_subscriber(subscriber);
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return;
    }

    usleep(BENCH_SUBSCRIPTION_SETTLE_US);

    publisher_pid = spawn_helper_process_args(
        "bench6-publisher",
        rounds_arg,
        publisher_name,
        publisher_name,
        topic_name,
        NULL,
        NULL);
    if (publisher_pid <= 0)
    {
        // printf removed
        if (sub_handle >= 0)
        {
            zoo_smb_unsubscribe_message(subscriber, sub_handle);
        }
        zoo_smb_destroy_subscriber(subscriber);
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return;
    }

    if (!prepare_pubsub_measurement_window(&ctx, BENCH_WAIT_TIMEOUT_MS * 4U))
    {
        // printf removed
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    published = BENCH_PUBSUB_ROUNDS;
    received = wait_for_pubsub_messages(&ctx, published, BENCH_WAIT_TIMEOUT_MS * 6U);
    (void)waitpid(publisher_pid, &status, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    (void)snapshot_pubsub_context(&ctx, &first_receive_ns, &last_receive_ns);

    double sec = elapsed_seconds(&start, &end);
    if (received > 1 && first_receive_ns > 0 && last_receive_ns > first_receive_ns)
    {
        sec = (double)(last_receive_ns - first_receive_ns) / 1000000000.0;
    }

    snprintf(label,
             sizeof(label),
             "[bench6] pub/sub e2e published=%d/%d received=%d loss=%d",
             published,
             BENCH_PUBSUB_ROUNDS,
             received,
             published - received);
    print_latency_stats(label, ctx.latency_us, received, sec);

    if (sub_handle >= 0)
    {
        zoo_smb_unsubscribe_message(subscriber, sub_handle);
    }

    zoo_smb_destroy_subscriber(subscriber);
    pthread_mutex_destroy(&ctx.mutex);
    free(ctx.latency_us);
}

/*
 * Run benchmark 5 in client-only mode for cross-machine LAN execution.
 *
 * This mode assumes the RPC server is already running on another machine.
 * It measures round-trip latency from this client process only.
 */
static int run_benchmark5_rpc_client_only(
    const char* client_name,
    const char* server_target,
    const char* topic_name,
    int rounds)
{
    struct timespec start;
    struct timespec end;
    int64_t* latencies_us;
    int success = 0;
    int attempted = 0;
    int consecutive_failures = 0;
    int total_rounds = rounds;
    char label[192];
    ZOO_SMB_CLIENT_HANDLE client;

    if (total_rounds <= 0)
    {
        total_rounds = BENCH_RPC_ROUNDS;
    }

    latencies_us = (int64_t*)calloc((size_t)total_rounds, sizeof(int64_t));
    if (!latencies_us)
    {
        // printf removed
        return 2;
    }

    client = zoo_smb_create_client(client_name, server_target, topic_name, NULL);
    if (!client)
    {
        // printf removed
        free(latencies_us);
        return 3;
    }

    if (!wait_for_server_ready(server_target, BENCH_WAIT_TIMEOUT_MS * 2U))
    {
        // printf removed
        zoo_smb_destroy_client(client);
        free(latencies_us);
        return 4;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < total_rounds; ++i)
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
        usleep(200);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    snprintf(label,
             sizeof(label),
             "[bench5-client] rpc round-trip target=%s attempts=%d/%d",
             server_target,
             attempted,
             total_rounds);
    print_latency_stats(label, latencies_us, success, elapsed_seconds(&start, &end));

    zoo_smb_destroy_client(client);
    free(latencies_us);
    return 0;
}

/*
 * Run benchmark 6 in subscriber-only mode for cross-machine LAN execution.
 *
 * This mode assumes a publisher is already active on another machine.
 */
static int run_benchmark6_subscriber_only(
    const char* subscriber_name,
    const char* publisher_name,
    const char* topic_name,
    int rounds)
{
    struct timespec start;
    struct timespec end;
    BENCH_PUBSUB_CONTEXT_STRUCT ctx;
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = NULL;
    ZOO_ERROR_TYPE subscribe_result = ZOO_SMB_ERROR_INVALID_PARAM;
    int32_t sub_handle = -1;
    int expected = rounds;
    int received;
    uint64_t first_receive_ns = 0;
    uint64_t last_receive_ns = 0;
    char label[192];

    if (expected <= 0)
    {
        expected = BENCH_PUBSUB_ROUNDS;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.capacity = expected;
    ctx.latency_us = (int64_t*)calloc((size_t)expected, sizeof(int64_t));
    if (!ctx.latency_us || pthread_mutex_init(&ctx.mutex, NULL) != 0)
    {
        free(ctx.latency_us);
        // printf removed
        return 2;
    }

    for (int attempt = 0; attempt < 3 && !subscriber; ++attempt)
    {
        subscriber = zoo_smb_create_subscriber(subscriber_name, publisher_name, topic_name, NULL);
        if (!subscriber)
        {
            usleep(200000);
        }
    }

    if (!subscriber)
    {
        // printf removed
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return 3;
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
        // printf removed
        zoo_smb_destroy_subscriber(subscriber);
        pthread_mutex_destroy(&ctx.mutex);
        free(ctx.latency_us);
        return 4;
    }

    usleep(BENCH_SUBSCRIPTION_SETTLE_US);

    clock_gettime(CLOCK_MONOTONIC, &start);
    received = wait_for_pubsub_messages(&ctx, expected, BENCH_WAIT_TIMEOUT_MS * 12U);
    clock_gettime(CLOCK_MONOTONIC, &end);
    (void)snapshot_pubsub_context(&ctx, &first_receive_ns, &last_receive_ns);

    double sec = elapsed_seconds(&start, &end);
    if (received > 1 && first_receive_ns > 0 && last_receive_ns > first_receive_ns)
    {
        sec = (double)(last_receive_ns - first_receive_ns) / 1000000000.0;
    }

    snprintf(label,
             sizeof(label),
             "[bench6-subscriber] pub/sub e2e publisher=%s expected=%d received=%d loss=%d",
             publisher_name,
             expected,
             received,
             expected - received);
    print_latency_stats(label, ctx.latency_us, received, sec);

    if (sub_handle >= 0)
    {
        zoo_smb_unsubscribe_message(subscriber, sub_handle);
    }

    zoo_smb_destroy_subscriber(subscriber);
    pthread_mutex_destroy(&ctx.mutex);
    free(ctx.latency_us);
    return 0;
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
    BENCH_RUNTIME_MODE_ENUM mode;

    g_program_path = argv[0];

    if (argc >= 2 &&
        (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "help") == 0))
    {
        print_benchmark_usage(argv[0]);
        return 0;
    }

    zoo_log_set_level(ZOO_LOG_LEVEL_WARN);
    bench_apply_network_overrides();

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
        size_t payload_size = sizeof(BENCH_PUBSUB_PAYLOAD_STRUCT);
        useconds_t inter_send_delay_us = 1000U;
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
        if (argc >= 7)
        {
            payload_size = (size_t)strtoul(argv[6], NULL, 10);
        }
        if (argc >= 8)
        {
            inter_send_delay_us = (useconds_t)strtoul(argv[7], NULL, 10);
        }
        return run_pubsub_publisher_child(rounds, publisher_name, target_name, topic_name, payload_size, inter_send_delay_us);
    }

    if (argc >= 2 && strcmp(argv[1], "bench5-client") == 0)
    {
        int rounds = argc >= 6 ? atoi(argv[5]) : BENCH_RPC_ROUNDS;
        const char* client_name = argc >= 3 ? argv[2] : "bench_rpc_client";
        const char* server_target = argc >= 4 ? argv[3] : "127.0.0.1:8080";
        const char* topic_name = argc >= 5 ? argv[4] : "default_topic";
        return run_benchmark5_rpc_client_only(client_name, server_target, topic_name, rounds);
    }

    if (argc >= 2 && strcmp(argv[1], "bench6-subscriber") == 0)
    {
        int rounds = argc >= 6 ? atoi(argv[5]) : BENCH_PUBSUB_ROUNDS;
        const char* subscriber_name = argc >= 3 ? argv[2] : "bench_subscriber";
        const char* publisher_name = argc >= 4 ? argv[3] : "bench_publisher";
        const char* topic_name = argc >= 5 ? argv[4] : "default_topic";
        return run_benchmark6_subscriber_only(subscriber_name, publisher_name, topic_name, rounds);
    }

    if (argc < 2)
    {
        print_benchmark_usage(argv[0]);
        return 1;
    }

    mode = bench_runtime_parse_mode(argc >= 3 ? argv[2] : NULL);
    bench_runtime_set_mode(mode);

    // printf removed

    if (strcmp(argv[1], "all") == 0)
    {
        char cmd[2048];
        unsigned int settle_seconds = BENCH_WAIT_TIMEOUT_MS / 1000U;

        snprintf(
            cmd,
            sizeof(cmd),
            "\"%s\" 1 %s && sleep %u && "
            "\"%s\" 2 %s && sleep %u && "
            "\"%s\" 3 %s && sleep %u && "
            "\"%s\" 4 %s && sleep %u && "
            "\"%s\" 5 %s && sleep %u && "
            "\"%s\" 6 %s",
            g_program_path, g_bench_runtime_config.mode_name, settle_seconds,
            g_program_path, g_bench_runtime_config.mode_name, settle_seconds,
            g_program_path, g_bench_runtime_config.mode_name, settle_seconds,
            g_program_path, g_bench_runtime_config.mode_name, settle_seconds,
            g_program_path, g_bench_runtime_config.mode_name, settle_seconds,
            g_program_path, g_bench_runtime_config.mode_name);

        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        // printf removed
        return 1;
    }

    switch (atoi(argv[1]))
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
            // printf removed
            return 1;
    }

    // printf removed
    return 0;
}
