/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Platform Tests
 * Component ID: ZOO_PLATFORM_TEST_BASIC
 * File name: test_platform_basic.c
 * Description: Basic platform abstraction tests using Unity
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     GitHub Copilot  Initial creation
 ******************************************************************************/

#include "zoo.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

/**
 * Test platform detection and basic information
 */
void test_platform_detection(void) {
    // Test that platform info is available
    const char* platform_info = ZOO_GET_PLATFORM_INFO();
    TEST_ASSERT_NOT_NULL(platform_info);
    TEST_ASSERT_TRUE(strlen(platform_info) > 0);
    
    // Test OS name is defined
    TEST_ASSERT_NOT_NULL(ZOO_OS_NAME);
    TEST_ASSERT_TRUE(strlen(ZOO_OS_NAME) > 0);
    
    printf("Platform: %s\n", platform_info);
    printf("OS: %s\n", ZOO_OS_NAME);
}

/**
 * Test platform feature flags
 */
void test_feature_flags(void) {
    // Test threading support flag (should be boolean)
    int has_threading = ZOO_HAS_THREADING;
    TEST_ASSERT_TRUE(has_threading == 1 || has_threading == 0);
    
    // Test embedded flag (should be boolean)  
    int is_embedded = ZOO_IS_EMBEDDED;
    TEST_ASSERT_TRUE(is_embedded == 1 || is_embedded == 0);
    
    printf("Threading support: %s\n", (has_threading ? "Yes" : "No"));
    printf("Embedded: %s\n", (is_embedded ? "Yes" : "No"));
}

/**
 * Test boolean definitions
 */
void test_boolean_definitions(void) {
    // Test ZOO boolean values
    TEST_ASSERT_EQUAL(0, ZOO_FALSE);
    TEST_ASSERT_EQUAL(1, ZOO_TRUE);
    TEST_ASSERT_NOT_EQUAL(ZOO_TRUE, ZOO_FALSE);
}

/**
 * Test error code definitions
 */
void test_error_codes(void) {
    // Test basic error codes
    TEST_ASSERT_EQUAL(0, ZOO_OK);
    TEST_ASSERT_NOT_EQUAL(ZOO_ERROR, ZOO_OK);
    TEST_ASSERT_NOT_EQUAL(ZOO_TIMEOUT, ZOO_OK);
    TEST_ASSERT_NOT_EQUAL(ZOO_INVALID_PARAM, ZOO_OK);
}

/**
 * Test memory alignment macros
 */
void test_memory_alignment(void) {
    // Test that alignment macros work
    size_t test_size = 13;
    size_t aligned_8 = ZOO_ALIGN_UP(test_size, 8);
    size_t aligned_16 = ZOO_ALIGN_UP(test_size, 16);
    
    TEST_ASSERT_EQUAL(16, aligned_8);  // 13 aligned to 8 bytes = 16
    TEST_ASSERT_EQUAL(16, aligned_16); // 13 aligned to 16 bytes = 16
    
    // Test power of 2 alignment
    TEST_ASSERT_EQUAL(4, ZOO_ALIGN_UP(1, 4));
    TEST_ASSERT_EQUAL(4, ZOO_ALIGN_UP(4, 4));
    TEST_ASSERT_EQUAL(8, ZOO_ALIGN_UP(5, 4));
}

/**
 * Test compiler and architecture detection
 */
void test_compiler_and_architecture(void) {
    // Test that we can get architecture information
    int is_arm = ZOO_IS_ARM();
    int is_x86 = ZOO_IS_X86();
    int is_64bit = ZOO_IS_64BIT();
    int is_32bit = ZOO_IS_32BIT();
    
    // These should be boolean values
    TEST_ASSERT_TRUE(is_arm == 0 || is_arm == 1);
    TEST_ASSERT_TRUE(is_x86 == 0 || is_x86 == 1);
    TEST_ASSERT_TRUE(is_64bit == 0 || is_64bit == 1);
    TEST_ASSERT_TRUE(is_32bit == 0 || is_32bit == 1);
    
    // Should be exactly one of each pair
    TEST_ASSERT_TRUE(is_64bit != is_32bit);
    
    printf("Architecture - ARM: %s, x86: %s\n", 
           (is_arm ? "Yes" : "No"), (is_x86 ? "Yes" : "No"));
    printf("Platform - 64-bit: %s, 32-bit: %s\n", 
           (is_64bit ? "Yes" : "No"), (is_32bit ? "Yes" : "No"));
}

/**
 * Test type sizes
 */
void test_type_sizes(void) {
    // Test that types have correct sizes
    TEST_ASSERT_EQUAL(1, sizeof(ZOO_INT8));
    TEST_ASSERT_EQUAL(1, sizeof(ZOO_UINT8));
    TEST_ASSERT_EQUAL(2, sizeof(ZOO_INT16));
    TEST_ASSERT_EQUAL(2, sizeof(ZOO_UINT16));
    TEST_ASSERT_EQUAL(4, sizeof(ZOO_INT32));
    TEST_ASSERT_EQUAL(4, sizeof(ZOO_UINT32));
    TEST_ASSERT_EQUAL(8, sizeof(ZOO_INT64));
    TEST_ASSERT_EQUAL(8, sizeof(ZOO_UINT64));
    
    // Test platform-specific sizes
#ifdef ZOO_PLATFORM_64BIT
    TEST_ASSERT_EQUAL(8, sizeof(ZOO_SIZE_T));
    TEST_ASSERT_EQUAL(8, sizeof(ZOO_UINTPTR_T));
#else
    TEST_ASSERT_EQUAL(4, sizeof(ZOO_SIZE_T));
    TEST_ASSERT_EQUAL(4, sizeof(ZOO_UINTPTR_T));
#endif
}

/**
 * Test utility macros
 */
void test_utility_macros(void) {
    // Test array size macro
    int test_array[10];
    TEST_ASSERT_EQUAL(10, ZOO_ARRAY_SIZE(test_array));
    
    // Test min/max macros
    TEST_ASSERT_EQUAL(5, ZOO_MIN(5, 10));
    TEST_ASSERT_EQUAL(10, ZOO_MAX(5, 10));
    TEST_ASSERT_EQUAL(7, ZOO_CLAMP(7, 5, 10));
    TEST_ASSERT_EQUAL(5, ZOO_CLAMP(3, 5, 10));
    TEST_ASSERT_EQUAL(10, ZOO_CLAMP(15, 5, 10));
}
