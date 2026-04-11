/*
 * Traceability coverage:
 * - REQ-PERF-001: queue throughput benchmark for SMB buffering path.
 * - REQ-PERF-002: latency-sensitive push/pop timing evidence.
 * - REQ-PERF-003: regression threshold candidate for queue path performance.
 */

#include "unity.h"
#include "../common/test_utils.h"
#include "zoo_smb_ring_buffer.h"
#include <time.h>
#include <stdlib.h>

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

void test_message_queue_push_pop_performance(void)
{
	const size_t iterations = 20000;
	uint8_t payload[128] = {0};
	uint8_t out[128] = {0};
	struct timespec start;
	struct timespec end;
	size_t memory_size = zoo_smb_ring_buffer_get_required_size(256 * 1024);
	void* memory = malloc(memory_size);
	TEST_ASSERT_NOT_NULL(memory);

	ZOO_SMB_RING_BUFFER_HANDLE rb = zoo_smb_ring_buffer_create_in_memory(memory, memory_size, ZOO_FALSE);
	TEST_ASSERT_NOT_NULL(rb);

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &start));
	for (size_t i = 0; i < iterations; ++i)
	{
		ZOO_ERROR_TYPE written = zoo_smb_ring_buffer_write(rb, payload, sizeof(payload));
		TEST_ASSERT_EQUAL_INT(ZOO_SMB_OK, written);

		ZOO_ERROR_TYPE read = zoo_smb_ring_buffer_read(rb, out, sizeof(out));
		TEST_ASSERT_EQUAL_INT(ZOO_SMB_OK, read);
	}
	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &end));

	double ms = elapsed_ms(start, end);
	TEST_ASSERT_TRUE(ms < 5000.0);

	zoo_smb_ring_buffer_destroy(rb);
	free(memory);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_message_queue_push_pop_performance);
	return UNITY_END();
}
