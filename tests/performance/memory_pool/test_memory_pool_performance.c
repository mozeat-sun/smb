/*
 * Traceability coverage:
 * - REQ-PERF-001: memory-pool allocation/free throughput evidence.
 * - REQ-PERF-002: mixed-size churn timing for allocator hot path.
 * - REQ-PERF-003: baseline metrics to detect memory pool regressions.
 */

#include "unity.h"
#include "zoo_memory_pool.h"

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PERF_POOL_SIZE_BYTES (8U * 1024U * 1024U)
#define PERF_THREAD_COUNT 4U
#define PERF_THREAD_ITERATIONS 60000U

static double elapsed_ms(const struct timespec* start, const struct timespec* end)
{
    time_t sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;

    return (double)sec * 1000.0 + (double)nsec / 1000000.0;
}

static void capture_time(struct timespec* ts)
{
    TEST_ASSERT_NOT_NULL(ts);
    TEST_ASSERT_EQUAL_INT(TIME_UTC, timespec_get(ts, TIME_UTC));
}

typedef struct
{
    uint32_t thread_index;
    uint32_t iterations;
    uint32_t allocation_failures;
} MEMORY_POOL_THREAD_WORK_STRUCT;

static void* memory_pool_thread_worker(void* arg)
{
    static const size_t sizes[] = {24U, 48U, 96U, 192U, 384U, 768U};
    MEMORY_POOL_THREAD_WORK_STRUCT* work = (MEMORY_POOL_THREAD_WORK_STRUCT*)arg;

    if (!work)
    {
        return NULL;
    }

    for (uint32_t i = 0; i < work->iterations; ++i)
    {
        size_t size = sizes[(i + work->thread_index) % (sizeof(sizes) / sizeof(sizes[0]))];
        void* ptr = zoo_allocate_from_pool(size);

        if (!ptr)
        {
            work->allocation_failures++;
            continue;
        }

        zoo_free_to_pool(ptr);
    }

    return NULL;
}

void setUp(void)
{
    TEST_ASSERT_EQUAL_INT(ZOO_OK, zoo_create_memory_pool(PERF_POOL_SIZE_BYTES));
}

void tearDown(void)
{
    zoo_destroy_memory_pool();
}

void test_memory_pool_fixed_size_alloc_free_throughput(void)
{
    const size_t iterations = 200000;
    struct timespec start;
    struct timespec end;

    capture_time(&start);

    for (size_t i = 0; i < iterations; ++i)
    {
        void* ptr = zoo_allocate_from_pool(64U);
        TEST_ASSERT_NOT_NULL(ptr);
        zoo_free_to_pool(ptr);
    }

    capture_time(&end);

    {
        double ms = elapsed_ms(&start, &end);
        double ops_per_sec = (ms > 0.0) ? ((double)iterations / (ms / 1000.0)) : 0.0;

        printf("[memory_pool_perf] fixed-size iterations=%zu elapsed_ms=%.2f ops_per_sec=%.0f\n",
               iterations,
               ms,
               ops_per_sec);

        TEST_ASSERT_TRUE(ms < 15000.0);
    }
}

void test_memory_pool_mixed_size_churn_performance(void)
{
    static const size_t sizes[] = {16U, 32U, 64U, 128U, 256U, 512U, 1024U, 2048U};
    const size_t rounds = 1500;
    const size_t slots_per_round = sizeof(sizes) / sizeof(sizes[0]);
    void* slots[sizeof(sizes) / sizeof(sizes[0])] = {0};
    struct timespec start;
    struct timespec end;

    capture_time(&start);

    for (size_t r = 0; r < rounds; ++r)
    {
        for (size_t i = 0; i < slots_per_round; ++i)
        {
            slots[i] = zoo_allocate_from_pool(sizes[i]);
            TEST_ASSERT_NOT_NULL(slots[i]);
        }

        for (size_t i = slots_per_round; i-- > 0;)
        {
            zoo_free_to_pool(slots[i]);
            slots[i] = NULL;
        }
    }

    capture_time(&end);

    {
        double ms = elapsed_ms(&start, &end);
        size_t operations = rounds * slots_per_round * 2U;
        double ops_per_sec = (ms > 0.0) ? ((double)operations / (ms / 1000.0)) : 0.0;

        printf("[memory_pool_perf] mixed-size rounds=%zu ops=%zu elapsed_ms=%.2f ops_per_sec=%.0f\n",
               rounds,
               operations,
               ms,
               ops_per_sec);

        TEST_ASSERT_TRUE(ms < 12000.0);
    }
}

void test_memory_pool_usage_statistics_after_churn(void)
{
    ZOO_MEMORY_USAGE_T usage;
    void* ptr = zoo_allocate_from_pool(4096U);

    TEST_ASSERT_NOT_NULL(ptr);
    zoo_memory_pool_get_usage(&usage);
    TEST_ASSERT_TRUE(usage.used_size > 0U);

    zoo_free_to_pool(ptr);
    zoo_memory_pool_get_usage(&usage);

    printf("[memory_pool_perf] usage pool=%zu used=%zu free=%zu used_pct=%zu\n",
           (size_t)usage.pool_size,
           (size_t)usage.used_size,
           (size_t)usage.free_size,
           (size_t)usage.used_pct);

    TEST_ASSERT_TRUE(usage.pool_size > 0U);
}

void test_memory_pool_multithread_contention_performance(void)
{
    pthread_t threads[PERF_THREAD_COUNT];
    MEMORY_POOL_THREAD_WORK_STRUCT work[PERF_THREAD_COUNT];
    struct timespec start;
    struct timespec end;
    uint32_t total_failures = 0U;

    capture_time(&start);

    for (uint32_t i = 0; i < PERF_THREAD_COUNT; ++i)
    {
        work[i].thread_index = i;
        work[i].iterations = PERF_THREAD_ITERATIONS;
        work[i].allocation_failures = 0U;

        TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, memory_pool_thread_worker, &work[i]));
    }

    for (uint32_t i = 0; i < PERF_THREAD_COUNT; ++i)
    {
        TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
        total_failures += work[i].allocation_failures;
    }

    capture_time(&end);

    {
        double ms = elapsed_ms(&start, &end);
        uint64_t operations = (uint64_t)PERF_THREAD_COUNT * (uint64_t)PERF_THREAD_ITERATIONS;
        double ops_per_sec = (ms > 0.0) ? ((double)operations / (ms / 1000.0)) : 0.0;

        printf("[memory_pool_perf] multithread threads=%u iterations_per_thread=%u elapsed_ms=%.2f ops_per_sec=%.0f failures=%u\n",
               PERF_THREAD_COUNT,
               PERF_THREAD_ITERATIONS,
               ms,
               ops_per_sec,
               total_failures);

        TEST_ASSERT_TRUE(ms < 25000.0);
    }

    TEST_ASSERT_EQUAL_UINT32(0U, total_failures);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_memory_pool_fixed_size_alloc_free_throughput);
    RUN_TEST(test_memory_pool_mixed_size_churn_performance);
    RUN_TEST(test_memory_pool_usage_statistics_after_churn);
    RUN_TEST(test_memory_pool_multithread_contention_performance);

    return UNITY_END();
}
