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

void test_zoo_string_copy_functions(void)
{
    char buffer[256];
    
    // Test safe string copy
    int result = zoo_safety_copy_string(buffer, "Hello, World!", sizeof(buffer));
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", buffer);
    
    // Test string copy with truncation
    char small_buffer[5];
    result = zoo_safety_copy_string(small_buffer, "Hello, World!", sizeof(small_buffer));
    TEST_ASSERT_EQUAL(ZOO_OK, result); // API returns success with safe truncation
    TEST_ASSERT_EQUAL_STRING("Hell", small_buffer);
    TEST_ASSERT_EQUAL('\0', small_buffer[4]); // Should be null-terminated
}

void test_zoo_string_concat_functions(void)
{
    char buffer[256] = "Hello";
    
    // Test safe string concatenation
    int result = zoo_safety_concat_string(buffer, ", World!", sizeof(buffer));
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", buffer);
    
    // Test concatenation with truncation
    char small_buffer[10] = "Hi";
    result = zoo_safety_concat_string(small_buffer, " there friend!", sizeof(small_buffer));
    TEST_ASSERT_EQUAL(ZOO_OK, result); // API returns success with safe truncation
    TEST_ASSERT_EQUAL_STRING("Hi there ", small_buffer);
    TEST_ASSERT_EQUAL('\0', small_buffer[9]); // Should be null-terminated
}

void test_zoo_string_comparison_functions(void)
{
    // Test safe string comparison with length limit
    int result = zoo_safety_compare_string("Hello", "Hello", 5);
    TEST_ASSERT_EQUAL(0, result);
    
    result = zoo_safety_compare_string("Hello", "World", 5);
    TEST_ASSERT_LESS_THAN(0, result);
    
    // Test substring comparison
    result = zoo_safety_compare_string("Hello", "Hell", 4);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test with NULL pointers
    result = zoo_safety_compare_string(NULL, "Hello", 5);
    TEST_ASSERT_EQUAL(-1, result);
    
    result = zoo_safety_compare_string("Hello", NULL, 5);
    TEST_ASSERT_EQUAL(1, result);
}

void test_zoo_string_utility_functions(void)
{
    // Test string trimming
    char test_string[] = "  Hello World  ";
    char *trimmed = zoo_string_trim(test_string);
    TEST_ASSERT_EQUAL_STRING("Hello World", trimmed);
    
    // Test case conversion
    char upper_test[] = "hello world";
    zoo_string_to_upper(upper_test);
    TEST_ASSERT_EQUAL_STRING("HELLO WORLD", upper_test);
    
    char lower_test[] = "HELLO WORLD";
    zoo_string_to_lower(lower_test);
    TEST_ASSERT_EQUAL_STRING("hello world", lower_test);
}

void test_zoo_string_search_functions(void)
{
    const char *haystack = "Hello World";
    
    // Test prefix checking
    ZOO_BOOL result = zoo_string_starts_with(haystack, "Hello");
    TEST_ASSERT_TRUE(result);
    
    result = zoo_string_starts_with(haystack, "World");
    TEST_ASSERT_FALSE(result);
    
    // Test suffix checking
    result = zoo_string_ends_with(haystack, "World");
    TEST_ASSERT_TRUE(result);
    
    result = zoo_string_ends_with(haystack, "Hello");
    TEST_ASSERT_FALSE(result);
}

void test_zoo_string_character_replacement(void)
{
    char test_string[] = "Hello World";
    ZOO_SIZE_T count = zoo_string_replace_char(test_string, 'l', 'x');
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL_STRING("Hexxo Worxd", test_string);
}

int main_string_tests(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_zoo_string_copy_functions);
    RUN_TEST(test_zoo_string_concat_functions);
    RUN_TEST(test_zoo_string_comparison_functions);
    RUN_TEST(test_zoo_string_utility_functions);
    RUN_TEST(test_zoo_string_search_functions);
    RUN_TEST(test_zoo_string_character_replacement);
    
    return UNITY_END();
}

int main(void)
{
    return main_string_tests();
}
