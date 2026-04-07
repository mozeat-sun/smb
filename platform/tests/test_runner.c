/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Platform Tests
 * Component ID: ZOO_PLATFORM_TEST_RUNNER
 * File name: test_runner.c
 * Description: Unity test runner for ZOO platform tests
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     GitHub Copilot  Initial creation
 ******************************************************************************/

#include "unity.h"
#include "zoo.h"
#include <stdio.h>

/**
 * Global setUp function called before each test
 */
void setUp(void) {
    // Global setup code for each test
    // This function is required by Unity framework
}

/**
 * Global tearDown function called after each test
 */
void tearDown(void) {
    // Global cleanup code for each test
    // This function is required by Unity framework
}

// External test function declarations

// Basic platform tests
extern void test_platform_detection(void);
extern void test_feature_flags(void);
extern void test_boolean_definitions(void);
extern void test_error_codes(void);
extern void test_memory_alignment(void);
extern void test_compiler_and_architecture(void);
extern void test_type_sizes(void);
extern void test_utility_macros(void);

// Threading tests
extern void test_mutex_init_destroy(void);
extern void test_mutex_lock_unlock(void);
extern void test_condition_variable_init_destroy(void);
extern void test_condition_variable_signal(void);
extern void test_sleep_functions(void);
#if ZOO_HAS_THREADING
extern void test_thread_create_join(void);
extern void test_multiple_threads(void);
extern void test_condition_variable_wait_signal(void);
#endif

// Atomic operation tests
extern void test_atomic_boolean_operations(void);
extern void test_atomic_size_t_operations(void);
extern void test_atomic_integer_operations(void);
extern void test_memory_ordering(void);
#if ZOO_HAS_THREADING
extern void test_concurrent_atomic_operations(void);
extern void test_concurrent_compare_and_swap(void);
#endif

// Time operation tests
extern void test_basic_time_functions(void);
extern void test_high_resolution_clock(void);
extern void test_time_differences(void);
extern void test_nanosecond_precision(void);
extern void test_sleep_functionality(void);
extern void test_timeout_calculations(void);
extern void test_time_conversion_consistency(void);
extern void test_clock_consistency(void);

// RT-Thread specific tests
#ifdef TEST_RTTHREAD_MODE
extern void test_rtthread_detection(void);
extern void test_rtthread_time_functions(void);
extern void test_rtthread_thread_operations(void);
extern void test_rtthread_mutex_operations(void);
extern void test_rtthread_condition_operations(void);
extern void test_rtthread_sleep_functions(void);
extern void test_rtthread_error_handling(void);
#else
extern void test_rtthread_mode_not_enabled(void);
#endif

/**
 * Run all basic platform tests
 */
void run_basic_tests(void) {
    printf("\n=== Running Basic Platform Tests ===\n");
    
    RUN_TEST(test_platform_detection);
    RUN_TEST(test_feature_flags);
    RUN_TEST(test_boolean_definitions);
    RUN_TEST(test_error_codes);
    RUN_TEST(test_memory_alignment);
    RUN_TEST(test_compiler_and_architecture);
    RUN_TEST(test_type_sizes);
    RUN_TEST(test_utility_macros);
}

/**
 * Run all threading tests
 */
void run_threading_tests(void) {
    printf("\n=== Running Threading Tests ===\n");
    
    RUN_TEST(test_mutex_init_destroy);
    RUN_TEST(test_mutex_lock_unlock);
    RUN_TEST(test_condition_variable_init_destroy);
    RUN_TEST(test_condition_variable_signal);
    RUN_TEST(test_sleep_functions);
    
#if ZOO_HAS_THREADING
    printf("Threading support available - running advanced tests\n");
    RUN_TEST(test_thread_create_join);
    RUN_TEST(test_multiple_threads);
    RUN_TEST(test_condition_variable_wait_signal);
#else
    printf("Threading support not available - skipping advanced tests\n");
#endif
}

/**
 * Run all atomic operation tests
 */
void run_atomic_tests(void) {
    printf("\n=== Running Atomic Operation Tests ===\n");
    
    RUN_TEST(test_atomic_boolean_operations);
    RUN_TEST(test_atomic_size_t_operations);
    RUN_TEST(test_atomic_integer_operations);
    RUN_TEST(test_memory_ordering);
    
#if ZOO_HAS_THREADING
    printf("Threading support available - running concurrent atomic tests\n");
    RUN_TEST(test_concurrent_atomic_operations);
    RUN_TEST(test_concurrent_compare_and_swap);
#else
    printf("Threading support not available - skipping concurrent tests\n");
#endif
}

/**
 * Run all time operation tests
 */
void run_time_tests(void) {
    printf("\n=== Running Time Operation Tests ===\n");
    
    RUN_TEST(test_basic_time_functions);
    RUN_TEST(test_high_resolution_clock);
    RUN_TEST(test_time_differences);
    RUN_TEST(test_nanosecond_precision);
    RUN_TEST(test_sleep_functionality);
    RUN_TEST(test_timeout_calculations);
    RUN_TEST(test_time_conversion_consistency);
    RUN_TEST(test_clock_consistency);
}

/**
 * Run RT-Thread specific tests
 */
void run_rtthread_tests(void) {
    printf("\n=== Running RT-Thread Specific Tests ===\n");
    
#ifdef TEST_RTTHREAD_MODE
    printf("RT-Thread test mode enabled\n");
    RUN_TEST(test_rtthread_detection);
    RUN_TEST(test_rtthread_time_functions);
    RUN_TEST(test_rtthread_thread_operations);
    RUN_TEST(test_rtthread_mutex_operations);
    RUN_TEST(test_rtthread_condition_operations);
    RUN_TEST(test_rtthread_sleep_functions);
    RUN_TEST(test_rtthread_error_handling);
#else
    printf("RT-Thread test mode not enabled\n");
    RUN_TEST(test_rtthread_mode_not_enabled);
#endif
}

/**
 * Print platform information
 */
void print_platform_info(void) {
    printf("\n=== ZOO Platform Test Suite ===\n");
    printf("Platform: %s\n", ZOO_GET_PLATFORM_INFO());
    printf("OS: %s\n", ZOO_OS_NAME);
    printf("Version: %s\n", ZOO_VERSION_STRING);
    printf("Threading Support: %s\n", ZOO_HAS_THREADING ? "Yes" : "No");
    printf("Embedded Mode: %s\n", ZOO_IS_EMBEDDED ? "Yes" : "No");
    printf("64-bit Platform: %s\n", ZOO_IS_64BIT() ? "Yes" : "No");
    printf("ARM Architecture: %s\n", ZOO_IS_ARM() ? "Yes" : "No");
    printf("x86 Architecture: %s\n", ZOO_IS_X86() ? "Yes" : "No");
    
#ifdef TEST_RTTHREAD_MODE
    printf("RT-Thread Test Mode: Enabled\n");
#else
    printf("RT-Thread Test Mode: Disabled\n");
#endif
    
    printf("Cache Line Size: %d bytes\n", ZOO_CACHE_LINE_SIZE);
    printf("====================================\n");
}

/**
 * Main test runner
 */
int main(void) {
    // Initialize Unity
    UNITY_BEGIN();
    
    // Print platform information
    print_platform_info();
    
    // Run all test suites
    run_basic_tests();
    run_threading_tests();
    run_atomic_tests();
    run_time_tests();
    run_rtthread_tests();
    
    // Finish and return results
    printf("\n=== Test Summary ===\n");
    return UNITY_END();
}
