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

void test_zoo_memory_allocation_functions(void)
{
    // Test safe malloc with zero initialization
    void *ptr = zoo_safety_malloc_zero(1024);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Verify memory is zeroed
    unsigned char *byte_ptr = (unsigned char *)ptr;
    for (int i = 0; i < 1024; i++)
    {
        TEST_ASSERT_EQUAL(0, byte_ptr[i]);
    }
    
    // Test safe deletion
    zoo_safety_delete_pointer(&ptr);
    TEST_ASSERT_NULL(ptr);
}

void test_zoo_memory_calloc_functions(void)
{
    // Test safe calloc
    int *int_array = (int *)zoo_safety_calloc(10, sizeof(int));
    TEST_ASSERT_NOT_NULL(int_array);
    
    // Verify memory is zeroed
    for (int i = 0; i < 10; i++)
    {
        TEST_ASSERT_EQUAL(0, int_array[i]);
    }
    
    void *array_ptr = (void *)int_array;
    zoo_safety_delete_array(&array_ptr);
    TEST_ASSERT_NULL(array_ptr);
}

void test_zoo_memory_realloc_functions(void)
{
    // Test safe realloc
    void *ptr = zoo_safety_malloc_zero(100);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Fill with test data
    memset(ptr, 0xAA, 100);
    
    // Reallocate to larger size
    void *new_ptr = zoo_safety_realloc(ptr, 200);
    TEST_ASSERT_NOT_NULL(new_ptr);
    
    // Verify original data is preserved
    unsigned char *byte_ptr = (unsigned char *)new_ptr;
    for (int i = 0; i < 100; i++)
    {
        TEST_ASSERT_EQUAL(0xAA, byte_ptr[i]);
    }
    
    zoo_safety_delete_pointer(&new_ptr);
    TEST_ASSERT_NULL(new_ptr);
}

void test_zoo_memory_comparison_functions(void)
{
    unsigned char buffer1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    unsigned char buffer2[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    unsigned char buffer3[10] = {1, 2, 3, 4, 0, 6, 7, 8, 9, 10};
    
    // Test equal buffers
    int result = zoo_safety_memcmp(buffer1, buffer2, 10);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test different buffers
    result = zoo_safety_memcmp(buffer1, buffer3, 10);
    TEST_ASSERT_NOT_EQUAL(0, result);
    
    // Test with NULL pointers
    result = zoo_safety_memcmp(NULL, buffer2, 10);
    TEST_ASSERT_EQUAL(-1, result);
    
    result = zoo_safety_memcmp(buffer1, NULL, 10);
    TEST_ASSERT_EQUAL(1, result);
    
    result = zoo_safety_memcmp(NULL, NULL, 10);
    TEST_ASSERT_EQUAL(0, result);
}

void test_zoo_memory_copy_functions(void)
{
    unsigned char source[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    unsigned char dest[10] = {0};
    
    // Test safe memory copy
    int result = zoo_safety_memcpy(dest, source, 10);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    for (int i = 0; i < 10; i++)
    {
        TEST_ASSERT_EQUAL(source[i], dest[i]);
    }
    
    // Test with NULL pointers
    result = zoo_safety_memcpy(NULL, source, 10);
    TEST_ASSERT_EQUAL(-1, result);
    
    result = zoo_safety_memcpy(dest, NULL, 10);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_zoo_memory_set_functions(void)
{
    unsigned char buffer[10];
    
    // Test safe memory set
    int result = zoo_safety_memset(buffer, 0xFF, 10);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    for (int i = 0; i < 10; i++)
    {
        TEST_ASSERT_EQUAL(0xFF, buffer[i]);
    }
    
    // Test with NULL pointer
    result = zoo_safety_memset(NULL, 0xFF, 10);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_zoo_memory_aligned_allocation(void)
{
    // Test aligned memory allocation
    void *aligned_ptr = zoo_safety_malloc_aligned(1024, 16);
    TEST_ASSERT_NOT_NULL(aligned_ptr);
    
    // Check alignment
    ZOO_BOOL is_aligned = zoo_memory_is_aligned(aligned_ptr, 16);
    TEST_ASSERT_TRUE(is_aligned);
    
    // Free aligned memory
    zoo_safety_free_aligned(aligned_ptr);
}

void test_zoo_memory_secure_wipe(void)
{
    unsigned char buffer[10];
    memset(buffer, 0xAA, 10);
    
    // Test secure wipe
    zoo_memory_secure_wipe(buffer, 10);
    
    for (int i = 0; i < 10; i++)
    {
        TEST_ASSERT_EQUAL(0, buffer[i]);
    }
}

int main_memory_tests(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_zoo_memory_allocation_functions);
    RUN_TEST(test_zoo_memory_calloc_functions);
    RUN_TEST(test_zoo_memory_realloc_functions);
    RUN_TEST(test_zoo_memory_comparison_functions);
    RUN_TEST(test_zoo_memory_copy_functions);
    RUN_TEST(test_zoo_memory_set_functions);
    RUN_TEST(test_zoo_memory_aligned_allocation);
    RUN_TEST(test_zoo_memory_secure_wipe);
    
    return UNITY_END();
}

int main(void)
{
    return main_memory_tests();
}
