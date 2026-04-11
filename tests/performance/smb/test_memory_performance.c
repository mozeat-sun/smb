/*
 * Traceability coverage:
 * - REQ-PERF-001: memory-pool throughput benchmark for SMB runtime allocation path.
 * - REQ-PERF-002: allocation/free timing evidence for latency reporting.
 * - REQ-PERF-003: regression threshold candidate for deterministic memory path.
 */

#include "unity.h"
#include "../common/test_utils.h"
#include <time.h>

static double elapsed_ms(struct timespec start, struct timespec end)
{
	double sec = (double)(end.tv_sec - start.tv_sec) * 1000.0;
	double nsec = (double)(end.tv_nsec - start.tv_nsec) / 1000000.0;
	return sec + nsec;
}

void setUp(void)
{
	zoo_create_memory_pool(4 * 1024 * 1024);
}

void tearDown(void)
{
	zoo_destroy_memory_pool();
}

void test_memory_pool_basic_performance(void)
{
	const size_t iterations = 50000;
	struct timespec start;
	struct timespec end;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &start));
	for (size_t i = 0; i < iterations; ++i)
	{
		void* ptr = zoo_allocate_from_pool(64);
		TEST_ASSERT_NOT_NULL(ptr);
		zoo_free_to_pool(ptr);
	}
	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &end));

	/* Sanity threshold: this should complete quickly on CI and dev machines. */
	double ms = elapsed_ms(start, end);
	TEST_ASSERT_TRUE(ms < 5000.0);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_memory_pool_basic_performance);
	return UNITY_END();
}
