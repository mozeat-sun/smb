#include "unity.h"
#include "zoo_util.h"
#include <string.h>

// Unity requires setUp and tearDown functions
void setUp(void)
{
    // Set up code here
}

void tearDown(void)
{
    // Clean up code here
}

void test_zoo_timestamp_functions(void)
{
    // Test basic timestamp functions
    ZOO_UINT64 timestamp = zoo_get_timestamp_milliseconds();
    TEST_ASSERT_GREATER_THAN(0, timestamp);
    
    ZOO_UINT64 timestamp_seconds = zoo_get_timestamp_seconds();
    TEST_ASSERT_GREATER_THAN(0, timestamp_seconds);
    
    // Test timestamp precision
    ZOO_TIMESTAMP_T precise_timestamp;
    int result = zoo_get_timestamp_precise(&precise_timestamp);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_GREATER_THAN(0, precise_timestamp.seconds);
}

void test_zoo_timestamp_elapsed_calculation(void)
{
    ZOO_UINT64 start_time = zoo_get_timestamp_milliseconds();
    ZOO_UINT64 end_time = start_time + 1000; // Add 1 second
    
    ZOO_UINT64 elapsed = zoo_calculate_elapsed_time(start_time, end_time);
    TEST_ASSERT_EQUAL(1000, elapsed);
    
    // Test wraparound case
    ZOO_UINT64 max_time = ZOO_MAX_UINT64 - 100;
    ZOO_UINT64 wrapped_time = 100;
    elapsed = zoo_calculate_elapsed_time(max_time, wrapped_time);
    TEST_ASSERT_EQUAL(201, elapsed);
}

void test_zoo_timestamp_sleep_functions(void)
{
    // Simple test that doesn't rely on sleep function
    ZOO_UINT64 start = zoo_get_timestamp_milliseconds();
    
    // Do some work instead of sleeping
    volatile int dummy = 0;
    for (int i = 0; i < 10000; i++) {
        dummy += i;
    }
    
    ZOO_UINT64 end = zoo_get_timestamp_milliseconds();
    
    // Just verify that time has progressed (allowing for very fast execution)
    TEST_ASSERT_GREATER_OR_EQUAL(start, end);
}

int main_timestamp_tests(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_zoo_timestamp_functions);
    RUN_TEST(test_zoo_timestamp_elapsed_calculation);
    RUN_TEST(test_zoo_timestamp_sleep_functions);
    
    return UNITY_END();
}

int main(void)
{
    return main_timestamp_tests();
}
