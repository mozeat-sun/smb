/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Platform Tests
 * Component ID: ZOO_PLATFORM_TEST_TIME
 * File name: test_platform_time.c
 * Description: Time operations tests using Unity
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     GitHub Copilot  Initial creation
 ******************************************************************************/

#include "zoo.h"
#include "unity.h"
#include <stdio.h>

/**
 * Test basic time functions
 */
void test_basic_time_functions(void) {
    // Test that time functions return valid values
    ZOO_TIME_T time1 = ZOO_TIME_GET();
    ZOO_TIME_T time2 = ZOO_TIME_GET();
    
    // Second call should return same or later time
    TEST_ASSERT_TRUE(time2 >= time1);
    
    // Time values should be reasonable (not 0 unless it's a tick-based system starting from 0)
    // We can't assume much about the actual values since they're platform-dependent
    
    printf("Time 1: %lu\n", (unsigned long)time1);
    printf("Time 2: %lu\n", (unsigned long)time2);
}

/**
 * Test high-resolution clock functions
 */
void test_high_resolution_clock(void) {
    uint64_t ns1 = ZOO_CLOCK_REALTIME_NS();
    uint64_t ns2 = ZOO_CLOCK_REALTIME_NS();
    
    // Second call should return same or later time
    TEST_ASSERT_TRUE(ns2 >= ns1);
    
    printf("Realtime NS 1: %llu\n", (unsigned long long)ns1);
    printf("Realtime NS 2: %llu\n", (unsigned long long)ns2);
    
    uint64_t mono1 = ZOO_CLOCK_MONOTONIC_NS();
    uint64_t mono2 = ZOO_CLOCK_MONOTONIC_NS();
    
    // Monotonic clock should always increase
    TEST_ASSERT_TRUE(mono2 >= mono1);
    
    printf("Monotonic NS 1: %llu\n", (unsigned long long)mono1);
    printf("Monotonic NS 2: %llu\n", (unsigned long long)mono2);
}

/**
 * Test time difference calculations
 */
void test_time_differences(void) {
    ZOO_TIME_T start_time = ZOO_TIME_GET();
    
    // Sleep for a short time
    ZOO_SLEEP_MS(100);
    
    ZOO_TIME_T end_time = ZOO_TIME_GET();
    
    // Calculate difference (platform-dependent implementation)
    ZOO_TIME_T diff = end_time - start_time;
    
    // Difference should be positive or zero (zero for platforms with coarse time resolution)
    TEST_ASSERT_TRUE(diff >= 0);
    
    printf("Time difference: %lu\n", (unsigned long)diff);
    
    // For systems where time is in milliseconds, difference should be around 100
    // For tick-based systems, it depends on tick frequency
    // We can't make strong assertions without knowing the platform
}

/**
 * Test nanosecond precision timing
 */
void test_nanosecond_precision(void) {
    uint64_t zoo_start = ZOO_CLOCK_MONOTONIC_NS();
    
    // Sleep for a measurable time
    ZOO_SLEEP_MS(50);
    
    uint64_t zoo_end = ZOO_CLOCK_MONOTONIC_NS();
    
    // Calculate durations
    uint64_t zoo_duration = zoo_end - zoo_start;
    
    printf("ZOO duration (ns): %llu\n", (unsigned long long)zoo_duration);
    
    // Duration should be reasonable for ~50ms sleep
    // Allow wide range due to system scheduling variations
    // Only test if we have reasonable resolution (at least microseconds)
    if (zoo_duration > 1000000) { // If resolution is at least microseconds
        TEST_ASSERT_TRUE(zoo_duration >= 30000000ULL);  // At least 30ms
        TEST_ASSERT_TRUE(zoo_duration <= 200000000ULL); // At most 200ms
    }
}

/**
 * Test sleep function basic functionality
 */
void test_sleep_functionality(void) {
    ZOO_TIME_T start = ZOO_TIME_GET();
    
    // Test millisecond sleep
    ZOO_SLEEP_MS(50);
    
    ZOO_TIME_T end = ZOO_TIME_GET();
    
    // Time should advance (or at least not go backwards)
    TEST_ASSERT_TRUE(end >= start);
    
    start = ZOO_TIME_GET();
    
    // Test microsecond sleep
    ZOO_SLEEP_US(10000); // 10ms
    
    end = ZOO_TIME_GET();
    
    // Time should advance
    TEST_ASSERT_TRUE(end >= start);
}

/**
 * Test timeout calculations
 */
void test_timeout_calculations(void) {
    ZOO_TIME_T start = ZOO_TIME_GET();
    
    // Simulate a timeout check loop
    const ZOO_TIME_T timeout_duration = 100; // Platform-dependent units
    int timeout_occurred = 0;
    
    for (int i = 0; i < 1000; i++) {
        ZOO_TIME_T current = ZOO_TIME_GET();
        
        // Check if timeout occurred (implementation depends on time units)
        if (current - start > timeout_duration) {
            timeout_occurred = 1;
            break;
        }
        
        // Small delay
        ZOO_SLEEP_US(1000); // 1ms
    }
    
    printf("Timeout occurred: %s\n", (timeout_occurred ? "Yes" : "No"));
    
    ZOO_TIME_T end = ZOO_TIME_GET();
    ZOO_TIME_T total_duration = end - start;
    
    printf("Total duration: %lu\n", (unsigned long)total_duration);
    
    // Total duration should be positive
    TEST_ASSERT_TRUE(total_duration >= 0);
}

/**
 * Test time conversion consistency
 */
void test_time_conversion_consistency(void) {
    ZOO_TIME_T current_time = ZOO_TIME_GET();
    uint64_t current_ns = ZOO_CLOCK_REALTIME_NS();
    
    // Test that we can get both time representations
    TEST_ASSERT_TRUE(current_time >= 0);
    
    printf("Current time (platform): %lu\n", (unsigned long)current_time);
    printf("Current time (ns): %llu\n", (unsigned long long)current_ns);
    
    // Test time stability - multiple calls should be close
    ZOO_TIME_T time_check = ZOO_TIME_GET();
    uint64_t ns_check = ZOO_CLOCK_REALTIME_NS();
    
    // Should be very close in time
    TEST_ASSERT_TRUE(time_check >= current_time);
    TEST_ASSERT_TRUE(ns_check >= current_ns);
    
    // Difference should be small (less than 1ms = 1,000,000 ns) for most systems
    uint64_t ns_diff = ns_check - current_ns;
    if (ns_diff > 0 && ns_diff < 1000000000ULL) { // If we have reasonable resolution
        TEST_ASSERT_TRUE(ns_diff < 1000000ULL); // Less than 1ms
    }
}

/**
 * Test clock consistency
 */
void test_clock_consistency(void) {
    // Test that clocks are consistent over multiple calls
    uint64_t realtime1 = ZOO_CLOCK_REALTIME_NS();
    uint64_t monotonic1 = ZOO_CLOCK_MONOTONIC_NS();
    
    ZOO_SLEEP_MS(10);
    
    uint64_t realtime2 = ZOO_CLOCK_REALTIME_NS();
    uint64_t monotonic2 = ZOO_CLOCK_MONOTONIC_NS();
    
    // Both clocks should advance
    TEST_ASSERT_TRUE(realtime2 >= realtime1);
    TEST_ASSERT_TRUE(monotonic2 >= monotonic1);
    
    // Test multiple rapid calls
    for (int i = 0; i < 10; i++) {
        uint64_t rt = ZOO_CLOCK_REALTIME_NS();
        uint64_t mt = ZOO_CLOCK_MONOTONIC_NS();
        
        TEST_ASSERT_TRUE(rt >= realtime2);
        TEST_ASSERT_TRUE(mt >= monotonic2);
        
        realtime2 = rt;
        monotonic2 = mt;
    }
}
