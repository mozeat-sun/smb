/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: dispatcher
 * Component id: ZOO_DISPATCHER
 * File name: test_zoo_dispatcher_performance.c
 * Description: Specialized performance test suite for ZOO dispatcher module
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-04     AI Assistant      created
 ******************************************************************************/

#include "zoo_dispatcher.h"
#include "zoo_memory_pool.h"
#include "zoo_queue.h"
#include "zoo_math.h"
#include "zoo_string.h"
#include "zoo.h"
#include "zoo_platform.h"
#include "zoo_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <math.h>
#include <errno.h>

// Performance test configuration
#define PERF_MEMORY_POOL_SIZE (8 * 1024 * 1024) // 8MB
#define PERF_QUEUE_SIZE 10000
#define PERF_MESSAGE_COUNT 10000
#define PERF_BURST_COUNT 50000
#define PERF_THREAD_COUNT 8
#define PERF_ITERATIONS 10
#define PERF_WARMUP_ITERATIONS 3

// Performance metrics structure
typedef struct
{
    double min_latency;
    double max_latency;
    double avg_latency;
    double total_time;
    double throughput;
    int messages_processed;
    int errors_encountered;
} PerformanceMetrics;

// Performance test message
typedef struct
{
    ZOO_INT64 id;
    struct timeval enqueue_time;
    struct timeval process_time;
    char payload[512]; // Larger payload for realistic testing
    int priority;
    int thread_id;
} PerfTestMessage;

// Performance test context
typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int messages_processed;
    int target_count;
    ZOO_BOOL test_complete;
    double *latencies;
    int latency_count;
    struct timeval start_time;
    struct timeval end_time;
} PerfTestContext;

// Global test control
static volatile ZOO_BOOL g_test_running = ZOO_TRUE;

static void *dispatcher_thread_entry(void *arg)
{
    zoo_start_dispatcher((ZOO_DISPATCHER_HANDLE)arg);
    return NULL;
}

static void stop_and_join_dispatcher(pthread_t dispatcher_thread, ZOO_DISPATCHER_HANDLE dispatcher)
{
    zoo_stop_dispatcher(dispatcher);
#ifdef __linux__
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;
    if (pthread_timedjoin_np(dispatcher_thread, NULL, &ts) != 0)
    {
        (void)pthread_cancel(dispatcher_thread);
        (void)pthread_join(dispatcher_thread, NULL);
    }
#else
    (void)pthread_cancel(dispatcher_thread);
    (void)pthread_join(dispatcher_thread, NULL);
#endif
}

static void wait_for_processing_with_timeout(PerfTestContext *ctx, int timeout_sec)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_sec;

    pthread_mutex_lock(&ctx->mutex);
    while (!ctx->test_complete && g_test_running)
    {
        int rc = pthread_cond_timedwait(&ctx->cond, &ctx->mutex, &ts);
        if (rc == ETIMEDOUT)
        {
            break;
        }
    }
    pthread_mutex_unlock(&ctx->mutex);
}

// Signal handler for graceful shutdown
static void signal_handler(int signal)
{
    g_test_running = ZOO_FALSE;
    printf("\nReceived signal %d, shutting down gracefully...\n", signal);
}

// Calculate time difference in microseconds
static double time_diff_us(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) * 1000000.0 + (end->tv_usec - start->tv_usec);
}

// Performance message handler
static void perf_message_handler(void *user_data, void *msg, void *context)
{
    PerfTestContext *perf_ctx = (PerfTestContext *)user_data;
    PerfTestMessage *perf_msg = (PerfTestMessage *)msg;

    if (!perf_ctx || !perf_msg)
    {
        return;
    }

    // Record processing time
    gettimeofday(&perf_msg->process_time, NULL);

    pthread_mutex_lock(&perf_ctx->mutex);

    // Calculate latency
    if (perf_ctx->latency_count < perf_ctx->target_count)
    {
        perf_ctx->latencies[perf_ctx->latency_count] =
            time_diff_us(&perf_msg->enqueue_time, &perf_msg->process_time);
        perf_ctx->latency_count++;
    }

    perf_ctx->messages_processed++;

    // Check if test is complete
    if (perf_ctx->messages_processed >= perf_ctx->target_count)
    {
        perf_ctx->test_complete = ZOO_TRUE;
        gettimeofday(&perf_ctx->end_time, NULL);
        pthread_cond_signal(&perf_ctx->cond);
    }

    pthread_mutex_unlock(&perf_ctx->mutex);
}

// Initialize performance test environment
static ZOO_BOOL setup_perf_environment(void)
{
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize memory pool
    if (zoo_create_memory_pool(PERF_MEMORY_POOL_SIZE) != ZOO_OK)
    {
        printf("Failed to create memory pool for performance tests\n");
        return ZOO_FALSE;
    }

    printf("Performance test environment initialized\n");
    return ZOO_TRUE;
}

// Cleanup performance test environment
static void cleanup_perf_environment(void)
{
    zoo_destroy_memory_pool();
    printf("Performance test environment cleaned up\n");
}

// Calculate performance metrics
static void calculate_metrics(PerfTestContext *ctx, PerformanceMetrics *metrics)
{
    metrics->messages_processed = ctx->messages_processed;
    metrics->total_time = time_diff_us(&ctx->start_time, &ctx->end_time) / 1000000.0;
    metrics->throughput = (double)ctx->messages_processed / metrics->total_time;

    if (ctx->latency_count > 0)
    {
        double sum = 0.0;
        metrics->min_latency = ctx->latencies[0];
        metrics->max_latency = ctx->latencies[0];

        for (int i = 0; i < ctx->latency_count; i++)
        {
            sum += ctx->latencies[i];
            if (ctx->latencies[i] < metrics->min_latency)
            {
                metrics->min_latency = ctx->latencies[i];
            }
            if (ctx->latencies[i] > metrics->max_latency)
            {
                metrics->max_latency = ctx->latencies[i];
            }
        }

        metrics->avg_latency = sum / ctx->latency_count;
    }
    else
    {
        metrics->min_latency = 0.0;
        metrics->max_latency = 0.0;
        metrics->avg_latency = 0.0;
    }
}

// Print performance metrics
static void print_metrics(const char *test_name, PerformanceMetrics *metrics)
{
    printf("\n%s Performance Results:\n", test_name);
    printf("=====================================\n");
    printf("Messages Processed:   %d\n", metrics->messages_processed);
    printf("Total Time:          %.3f seconds\n", metrics->total_time);
    printf("Throughput:          %.2f messages/second\n", metrics->throughput);
    printf("Average Latency:     %.2f microseconds\n", metrics->avg_latency);
    printf("Min Latency:         %.2f microseconds\n", metrics->min_latency);
    printf("Max Latency:         %.2f microseconds\n", metrics->max_latency);
    printf("Errors:              %d\n", metrics->errors_encountered);
}

// Test 1: Basic throughput test
static void test_basic_throughput(void)
{
    printf("\n=== Basic Throughput Test ===\n");

    ZOO_QUEUE_HANDLE queue = zoo_create_queue(PERF_QUEUE_SIZE);
    ZOO_DISPATCHER_HANDLE dispatcher = zoo_create_dispatcher(queue);
    if (!queue || !dispatcher)
    {
        printf("Failed to initialize queue/dispatcher for basic throughput test\n");
        if (dispatcher)
            zoo_destroy_dispatcher(dispatcher);
        if (queue)
            zoo_destroy_queue(queue);
        return;
    }

    PerfTestContext ctx = {0};
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);
    ctx.target_count = PERF_MESSAGE_COUNT;
    ctx.latencies = malloc(PERF_MESSAGE_COUNT * sizeof(double));
    if (!ctx.latencies)
    {
        printf("Failed to allocate latency buffer\n");
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    // Start dispatcher
    pthread_t dispatcher_thread;
    if (pthread_create(&dispatcher_thread, NULL, dispatcher_thread_entry, dispatcher) != 0)
    {
        printf("Failed to create dispatcher thread\n");
        free(ctx.latencies);
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    gettimeofday(&ctx.start_time, NULL);

    // Enqueue messages
    int enqueued_count = 0;
    for (int i = 0; i < PERF_MESSAGE_COUNT && g_test_running; i++)
    {
        PerfTestMessage *msg = zoo_allocate_from_pool(sizeof(PerfTestMessage));
        if (!msg)
        {
            printf("Allocation failed at message %d\n", i);
            break;
        }
        msg->id = zoo_generate_uuid64();
        gettimeofday(&msg->enqueue_time, NULL);
        snprintf(msg->payload, sizeof(msg->payload),
                 "Performance test message %d with some payload data", i);
        msg->priority = i % 10;
        msg->thread_id = 0;

        if (!zoo_queue_enqueue(queue, msg, &ctx, perf_message_handler, &ctx))
        {
            zoo_free_to_pool(msg);
            break;
        }
        enqueued_count++;
    }

    ctx.target_count = enqueued_count;
    if (ctx.target_count == 0)
    {
        gettimeofday(&ctx.end_time, NULL);
        stop_and_join_dispatcher(dispatcher_thread, dispatcher);
        free(ctx.latencies);
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    // Wait for completion
    wait_for_processing_with_timeout(&ctx, 20);

    stop_and_join_dispatcher(dispatcher_thread, dispatcher);

    PerformanceMetrics metrics;
    calculate_metrics(&ctx, &metrics);
    print_metrics("Basic Throughput", &metrics);

    free(ctx.latencies);
    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);
    zoo_destroy_dispatcher(dispatcher);
    zoo_destroy_queue(queue);
}

// Test 2: Burst load test
static void test_burst_load(void)
{
    printf("\n=== Burst Load Test ===\n");

    ZOO_QUEUE_HANDLE queue = zoo_create_queue(PERF_BURST_COUNT + 1000);
    ZOO_DISPATCHER_HANDLE dispatcher = zoo_create_dispatcher(queue);
    if (!queue || !dispatcher)
    {
        printf("Failed to initialize queue/dispatcher for burst test\n");
        if (dispatcher)
            zoo_destroy_dispatcher(dispatcher);
        if (queue)
            zoo_destroy_queue(queue);
        return;
    }

    PerfTestContext ctx = {0};
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);
    ctx.target_count = PERF_BURST_COUNT;
    ctx.latencies = malloc(PERF_BURST_COUNT * sizeof(double));
    if (!ctx.latencies)
    {
        printf("Failed to allocate latency buffer\n");
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    // Start dispatcher
    pthread_t dispatcher_thread;
    if (pthread_create(&dispatcher_thread, NULL, dispatcher_thread_entry, dispatcher) != 0)
    {
        printf("Failed to create dispatcher thread\n");
        free(ctx.latencies);
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    gettimeofday(&ctx.start_time, NULL);

    // Burst enqueue all messages at once
    int enqueued_count = 0;
    for (int i = 0; i < PERF_BURST_COUNT && g_test_running; i++)
    {
        PerfTestMessage *msg = zoo_allocate_from_pool(sizeof(PerfTestMessage));
        if (!msg)
        {
            printf("Allocation failed at burst message %d\n", i);
            break;
        }
        msg->id = zoo_generate_uuid64();
        gettimeofday(&msg->enqueue_time, NULL);
        snprintf(msg->payload, sizeof(msg->payload),
                 "Burst test message %d", i);
        msg->priority = rand() % 10;
        msg->thread_id = 0;

        if (!zoo_queue_enqueue(queue, msg, &ctx, perf_message_handler, &ctx))
        {
            zoo_free_to_pool(msg);
            break;
        }
        enqueued_count++;
    }

    ctx.target_count = enqueued_count;

    printf("Enqueued %d messages in burst, waiting for processing...\n", PERF_BURST_COUNT);

    // Wait for completion
    wait_for_processing_with_timeout(&ctx, 30);

    stop_and_join_dispatcher(dispatcher_thread, dispatcher);

    PerformanceMetrics metrics;
    calculate_metrics(&ctx, &metrics);
    print_metrics("Burst Load", &metrics);

    free(ctx.latencies);
    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);
    zoo_destroy_dispatcher(dispatcher);
    zoo_destroy_queue(queue);
}

// Producer thread function for multi-threaded test
typedef struct
{
    ZOO_QUEUE_HANDLE queue;
    PerfTestContext *ctx;
    int thread_id;
    int messages_to_send;
} ProducerThreadData;

static void *producer_thread(void *arg)
{
    ProducerThreadData *data = (ProducerThreadData *)arg;

    for (int i = 0; i < data->messages_to_send && g_test_running; i++)
    {
        PerfTestMessage *msg = zoo_allocate_from_pool(sizeof(PerfTestMessage));
        if (!msg)
        {
            usleep(1000);
            continue;
        }
        msg->id = zoo_generate_uuid64();
        gettimeofday(&msg->enqueue_time, NULL);
        snprintf(msg->payload, sizeof(msg->payload),
                 "Multi-thread message T%d-M%d", data->thread_id, i);
        msg->priority = (data->thread_id + i) % 10;
        msg->thread_id = data->thread_id;

        if (!zoo_queue_enqueue(data->queue, msg, data->ctx, perf_message_handler, data->ctx))
        {
            zoo_free_to_pool(msg);
            usleep(100);
        }

        // Small delay to avoid overwhelming the queue
        if (i % 100 == 0)
        {
            usleep(100);
        }
    }

    return NULL;
}

// Test 3: Multi-threaded producer test
static void test_multi_threaded_producers(void)
{
    printf("\n=== Multi-threaded Producers Test ===\n");

    ZOO_QUEUE_HANDLE queue = zoo_create_queue(PERF_MESSAGE_COUNT + 1000);
    ZOO_DISPATCHER_HANDLE dispatcher = zoo_create_dispatcher(queue);
    if (!queue || !dispatcher)
    {
        printf("Failed to initialize queue/dispatcher for multi-threaded test\n");
        if (dispatcher)
            zoo_destroy_dispatcher(dispatcher);
        if (queue)
            zoo_destroy_queue(queue);
        return;
    }

    PerfTestContext ctx = {0};
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);
    ctx.target_count = PERF_MESSAGE_COUNT;
    ctx.latencies = malloc(PERF_MESSAGE_COUNT * sizeof(double));
    if (!ctx.latencies)
    {
        printf("Failed to allocate latency buffer\n");
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    // Start dispatcher
    pthread_t dispatcher_thread;
    if (pthread_create(&dispatcher_thread, NULL, dispatcher_thread_entry, dispatcher) != 0)
    {
        printf("Failed to create dispatcher thread\n");
        free(ctx.latencies);
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    gettimeofday(&ctx.start_time, NULL);

    // Create producer threads
    pthread_t producer_threads[PERF_THREAD_COUNT];
    ProducerThreadData thread_data[PERF_THREAD_COUNT];
    int messages_per_thread = PERF_MESSAGE_COUNT / PERF_THREAD_COUNT;

    for (int i = 0; i < PERF_THREAD_COUNT; i++)
    {
        thread_data[i].queue = queue;
        thread_data[i].ctx = &ctx;
        thread_data[i].thread_id = i;
        thread_data[i].messages_to_send = messages_per_thread;

        pthread_create(&producer_threads[i], NULL, producer_thread, &thread_data[i]);
    }

    // Wait for all producers to finish
    for (int i = 0; i < PERF_THREAD_COUNT; i++)
    {
        pthread_join(producer_threads[i], NULL);
    }

    printf("All producer threads completed, waiting for processing...\n");

    // Wait for processing completion
    wait_for_processing_with_timeout(&ctx, 30);

    stop_and_join_dispatcher(dispatcher_thread, dispatcher);

    PerformanceMetrics metrics;
    calculate_metrics(&ctx, &metrics);
    print_metrics("Multi-threaded Producers", &metrics);

    free(ctx.latencies);
    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);
    zoo_destroy_dispatcher(dispatcher);
    zoo_destroy_queue(queue);
}

// Test 4: Memory usage stress test
static void test_memory_stress(void)
{
    printf("\n=== Memory Stress Test ===\n");

    // Use a smaller queue to force more memory cycling
    ZOO_QUEUE_HANDLE queue = zoo_create_queue(1000);
    ZOO_DISPATCHER_HANDLE dispatcher = zoo_create_dispatcher(queue);
    if (!queue || !dispatcher)
    {
        printf("Failed to initialize queue/dispatcher for memory stress test\n");
        if (dispatcher)
            zoo_destroy_dispatcher(dispatcher);
        if (queue)
            zoo_destroy_queue(queue);
        return;
    }

    PerfTestContext ctx = {0};
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);
    ctx.target_count = PERF_MESSAGE_COUNT * 2; // More messages than queue size
    ctx.latencies = malloc(ctx.target_count * sizeof(double));
    if (!ctx.latencies)
    {
        printf("Failed to allocate latency buffer\n");
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    // Start dispatcher
    pthread_t dispatcher_thread;
    if (pthread_create(&dispatcher_thread, NULL, dispatcher_thread_entry, dispatcher) != 0)
    {
        printf("Failed to create dispatcher thread\n");
        free(ctx.latencies);
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    gettimeofday(&ctx.start_time, NULL);

    // Continuously enqueue messages to stress memory allocation
    int enqueued_count = 0;
    for (int i = 0; i < ctx.target_count && g_test_running; i++)
    {
        PerfTestMessage *msg = zoo_allocate_from_pool(sizeof(PerfTestMessage));
        if (!msg)
        {
            usleep(1000);
            i--;
            continue;
        }
        msg->id = zoo_generate_uuid64();
        gettimeofday(&msg->enqueue_time, NULL);

        // Create larger payloads to stress memory
        snprintf(msg->payload, sizeof(msg->payload),
                 "Memory stress test message %d with large payload data "
                 "that consumes more memory and tests allocation patterns",
                 i);
        msg->priority = i % 10;
        msg->thread_id = 0;

        if (!zoo_queue_enqueue(queue, msg, &ctx, perf_message_handler, &ctx))
        {
            // If enqueue fails, wait a bit and retry
            usleep(1000);
            i--; // Retry this message
        }
        else
        {
            enqueued_count++;
        }

        // Periodic progress update
        if (i % 1000 == 0)
        {
            printf("Enqueued %d messages...\n", i);
        }
    }

    ctx.target_count = enqueued_count;

    // Wait for completion
    wait_for_processing_with_timeout(&ctx, 40);

    stop_and_join_dispatcher(dispatcher_thread, dispatcher);

    PerformanceMetrics metrics;
    calculate_metrics(&ctx, &metrics);
    print_metrics("Memory Stress", &metrics);

    free(ctx.latencies);
    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);
    zoo_destroy_dispatcher(dispatcher);
    zoo_destroy_queue(queue);
}

// Test 5: Sustained load test
static void test_sustained_load(void)
{
    printf("\n=== Sustained Load Test (30 seconds) ===\n");

    ZOO_QUEUE_HANDLE queue = zoo_create_queue(PERF_QUEUE_SIZE);
    ZOO_DISPATCHER_HANDLE dispatcher = zoo_create_dispatcher(queue);
    if (!queue || !dispatcher)
    {
        printf("Failed to initialize queue/dispatcher for sustained load test\n");
        if (dispatcher)
            zoo_destroy_dispatcher(dispatcher);
        if (queue)
            zoo_destroy_queue(queue);
        return;
    }

    PerfTestContext ctx = {0};
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);
    ctx.target_count = 0; // Will be set dynamically

    // Start dispatcher
    pthread_t dispatcher_thread;
    if (pthread_create(&dispatcher_thread, NULL, dispatcher_thread_entry, dispatcher) != 0)
    {
        printf("Failed to create dispatcher thread\n");
        pthread_mutex_destroy(&ctx.mutex);
        pthread_cond_destroy(&ctx.cond);
        zoo_destroy_dispatcher(dispatcher);
        zoo_destroy_queue(queue);
        return;
    }

    gettimeofday(&ctx.start_time, NULL);
    struct timeval current_time;
    int message_id = 0;
    int enqueued_count = 0;

    // Run for 30 seconds
    do
    {
        PerfTestMessage *msg = zoo_allocate_from_pool(sizeof(PerfTestMessage));
        if (!msg)
        {
            usleep(1000);
            continue;
        }
        msg->id = zoo_generate_uuid64();
        gettimeofday(&msg->enqueue_time, NULL);
        snprintf(msg->payload, sizeof(msg->payload),
                 "Sustained load message %d", message_id);
        msg->priority = message_id % 10;
        msg->thread_id = 0;

        if (zoo_queue_enqueue(queue, msg, &ctx, perf_message_handler, &ctx))
        {
            enqueued_count++;
            message_id++;
        }
        else
        {
            zoo_free_to_pool(msg);
        }

        gettimeofday(&current_time, NULL);

        // Maintain constant load (approximately 1000 msg/sec)
        usleep(1000);

    } while (time_diff_us(&ctx.start_time, &current_time) < 30000000 && g_test_running);

    ctx.target_count = enqueued_count;
    ctx.latencies = malloc(ctx.target_count * sizeof(double));

    printf("Sustained load phase completed, waiting for processing...\n");

    // Wait a bit more for processing to complete
    sleep(5);
    gettimeofday(&ctx.end_time, NULL);

    stop_and_join_dispatcher(dispatcher_thread, dispatcher);

    PerformanceMetrics metrics;
    calculate_metrics(&ctx, &metrics);
    print_metrics("Sustained Load", &metrics);

    free(ctx.latencies);
    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);
    zoo_destroy_dispatcher(dispatcher);
    zoo_destroy_queue(queue);
}

// Main performance test runner
int main(int argc, char *argv[])
{
    printf("ZOO Dispatcher Performance Test Suite\n");
    printf("====================================\n");

    ZOO_BOOL run_basic = ZOO_FALSE;
    ZOO_BOOL run_burst = ZOO_FALSE;
    ZOO_BOOL run_multithread = ZOO_FALSE;
    ZOO_BOOL run_memory = ZOO_FALSE;
    ZOO_BOOL run_sustained = ZOO_FALSE;
    ZOO_BOOL run_all = ZOO_TRUE;

    // Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        if (zoo_string_is_the_same(argv[i], "--basic"))
        {
            run_basic = ZOO_TRUE;
            run_all = ZOO_FALSE;
        }
        else if (zoo_string_is_the_same(argv[i], "--burst"))
        {
            run_burst = ZOO_TRUE;
            run_all = ZOO_FALSE;
        }
        else if (zoo_string_is_the_same(argv[i], "--multithread"))
        {
            run_multithread = ZOO_TRUE;
            run_all = ZOO_FALSE;
        }
        else if (zoo_string_is_the_same(argv[i], "--memory"))
        {
            run_memory = ZOO_TRUE;
            run_all = ZOO_FALSE;
        }
        else if (zoo_string_is_the_same(argv[i], "--sustained"))
        {
            run_sustained = ZOO_TRUE;
            run_all = ZOO_FALSE;
        }
        else if (zoo_string_is_the_same(argv[i], "--help"))
        {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --basic       Run basic throughput test\n");
            printf("  --burst       Run burst load test\n");
            printf("  --multithread Run multi-threaded producers test\n");
            printf("  --memory      Run memory stress test\n");
            printf("  --sustained   Run sustained load test\n");
            printf("  --help        Show this help\n");
            printf("\nRunning without options will execute all tests.\n");
            return 0;
        }
    }

    // Setup performance test environment
    if (!setup_perf_environment())
    {
        printf("Failed to setup performance test environment\n");
        return 1;
    }

    printf("Starting performance tests...\n");
    printf("Press Ctrl+C to stop tests gracefully.\n");

    // Run tests based on arguments
    if (run_all || run_basic)
    {
        test_basic_throughput();
    }

    if (run_all || run_burst)
    {
        test_burst_load();
    }

    if (run_all || run_multithread)
    {
        test_multi_threaded_producers();
    }

    if (run_all || run_memory)
    {
        test_memory_stress();
    }

    if (run_all || run_sustained)
    {
        test_sustained_load();
    }

    // Cleanup
    cleanup_perf_environment();

    printf("\nPerformance testing completed.\n");

    return 0;
}
