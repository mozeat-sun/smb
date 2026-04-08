/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 * Product: ZOO
 * Module: Timer Tests
 * Component ID: TIMER_TEST
 * File name: test_runner.c
 * Description: Main test runner for ZOO timer unit tests
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01     AI Assistant      Created Unity test runner
 ******************************************************************************/

#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

// ==============================================================================
// External Test Group Declarations
// ==============================================================================

// From test_zoo_timer.c
extern void test_zoo_timer_create_group(void);
extern void test_zoo_timer_start_group(void);
extern void test_zoo_timer_stop_group(void);
extern void test_zoo_timer_destroy_group(void);
extern void test_zoo_timer_edge_cases_group(void);

// From test_zoo_timer_platform.c
extern void test_zoo_timer_platform_sleep_group(void);
extern void test_zoo_timer_platform_threading_group(void);
extern void test_zoo_timer_platform_memory_group(void);
extern void test_zoo_timer_platform_error_group(void);
extern void test_zoo_timer_platform_stress_group(void);

// ==============================================================================
// Test Statistics
// ==============================================================================

static void print_test_header(void)
{
    printf("\n");
    printf("================================================================================\n");
    printf("                        ZOO Timer Module Unit Tests\n");
    printf("================================================================================\n");
    printf("Test Framework: Unity v2.6.2\n");
    printf("Mock Framework: Custom mocks for platform abstraction\n");
    printf("Target Module: zoo_timer.c\n");
    printf("================================================================================\n");
    printf("\n");
}

static void print_test_summary(void)
{
    printf("\n");
    printf("================================================================================\n");
    printf("                           Test Execution Summary\n");
    printf("================================================================================\n");
    printf("Total Tests: %lu\n", (unsigned long)Unity.NumberOfTests);
    printf("Passed:      %lu\n", (unsigned long)(Unity.NumberOfTests - Unity.TestFailures));
    printf("Failed:      %lu\n", (unsigned long)Unity.TestFailures);
    printf("Ignored:     %lu\n", (unsigned long)Unity.TestIgnores);
    
    if (Unity.TestFailures == 0) {
        printf("\n✅ ALL TESTS PASSED! 🎉\n");
    } else {
        printf("\n❌ %lu TEST(S) FAILED\n", (unsigned long)Unity.TestFailures);
    }
    printf("================================================================================\n");
}

// ==============================================================================
// Main Test Runner
// ==============================================================================

int main(void)
{
    print_test_header();
    
    // Initialize Unity
    UNITY_BEGIN();
    
    // ==============================================================================
    // Core Timer Functionality Tests
    // ==============================================================================
    
    printf("Running Core Timer Functionality Tests...\n");
    printf("------------------------------------------\n");
    
    printf("🔧 Timer Creation Tests:\n");
    test_zoo_timer_create_group();
    
    printf("▶️  Timer Start Tests:\n");
    test_zoo_timer_start_group();
    
    printf("⏹️  Timer Stop Tests:\n");
    test_zoo_timer_stop_group();
    
    printf("🗑️  Timer Destroy Tests:\n");
    test_zoo_timer_destroy_group();
    
    printf("🎯 Edge Cases Tests:\n");
    test_zoo_timer_edge_cases_group();
    
    // ==============================================================================
    // Platform-Specific Tests
    // ==============================================================================
    
    printf("\nRunning Platform-Specific Tests...\n");
    printf("-----------------------------------\n");
    
    printf("😴 Platform Sleep Tests:\n");
    test_zoo_timer_platform_sleep_group();
    
    printf("🧵 Platform Threading Tests:\n");
    test_zoo_timer_platform_threading_group();
    
    printf("💾 Platform Memory Tests:\n");
    test_zoo_timer_platform_memory_group();
    
    printf("⚠️  Platform Error Handling Tests:\n");
    test_zoo_timer_platform_error_group();
    
    printf("💪 Platform Stress Tests:\n");
    test_zoo_timer_platform_stress_group();
    
    // ==============================================================================
    // Test Completion
    // ==============================================================================
    
    print_test_summary();
    
    // End Unity and return result
    int result = UNITY_END();
    
    return result;
}
