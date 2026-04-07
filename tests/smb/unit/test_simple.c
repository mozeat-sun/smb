/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_simple_
 * File name: test_simple.c
 * Description: Simple Unity test to verify framework works
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                created
 ******************************************************************************/

#include "unity.h"
#include <stdio.h>

void setUp(void)
{
    // Setup before each test
}

void tearDown(void)
{
    // Cleanup after each test
}

void test_unity_framework_works(void)
{
    TEST_ASSERT_EQUAL_INT(42, 42);
    TEST_ASSERT_TRUE(1);
    TEST_ASSERT_FALSE(0);
    TEST_ASSERT_NOT_NULL("hello");
}

void test_basic_math(void)
{
    TEST_ASSERT_EQUAL_INT(4, 2 + 2);
    TEST_ASSERT_EQUAL_INT(10, 5 * 2);
    TEST_ASSERT_GREATER_THAN(5, 10);
}

void test_string_operations(void)
{
    const char* str = "Hello Unity";
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Hello Unity", str);
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_unity_framework_works);
    RUN_TEST(test_basic_math);
    RUN_TEST(test_string_operations);
    
    return UNITY_END();
}
