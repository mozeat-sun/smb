/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_zoo_smb_ring_buffer_
 * File name: test_zoo_smb_ring_buffer.c
 * Description: Unity-based ring buffer tests (converted from GTest)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                converted from GTest
 ******************************************************************************/

#include "test_utils.h"
#include "zoo_smb_ring_buffer.h"
#include "zoo_smb_protocol.h"
#include <string.h>

// Test fixture data
static void* memory = NULL;
static size_t data_size = 0;
static size_t total_size = 0;
static ZOO_SMB_RING_BUFFER_HANDLE ring = NULL;

void setUp(void)
{
    test_setup_memory_pool();
    test_memory_tracker_init();
    
    data_size = 4096;
    total_size = zoo_smb_ring_buffer_get_required_size(data_size);
    memory = test_tracked_malloc(total_size);
    TEST_ASSERT_NOT_NULL(memory);
    
    ring = zoo_smb_ring_buffer_create_in_memory(memory, total_size, ZOO_FALSE);
    TEST_ASSERT_NOT_NULL(ring);
}

void tearDown(void)
{
    if (ring) {
        zoo_smb_ring_buffer_destroy(ring);
        ring = NULL;
    }
    
    test_memory_tracker_cleanup();
    test_teardown_memory_pool();
}

void test_create_and_destroy(void)
{
    TEST_ASSERT_TRUE(zoo_smb_ring_buffer_is_valid(ring));
    
    zoo_smb_ring_buffer_destroy(ring);
    ring = NULL;
}

void test_write_and_read(void)
{
    const char* test_data = "hello ring buffer";
    size_t len = strlen(test_data) + 1;
    
    TEST_ASSERT_ERROR_OK(zoo_smb_ring_buffer_write(ring, test_data, len));
    
    char read_buffer[256] = {0};

    TEST_ASSERT_ERROR_OK(zoo_smb_ring_buffer_read(ring, read_buffer, len));
    TEST_ASSERT_STREQ(test_data, read_buffer);
}

void test_write_read_multiple(void)
{
    const char* data1 = "first message";
    const char* data2 = "second message";
    const char* data3 = "third message";
    
    size_t len1 = strlen(data1) + 1;
    size_t len2 = strlen(data2) + 1;
    size_t len3 = strlen(data3) + 1;
    
    // Write multiple messages
    TEST_ASSERT_ERROR_OK(zoo_smb_ring_buffer_write(ring, data1, len1));
    TEST_ASSERT_ERROR_OK(zoo_smb_ring_buffer_write(ring, data2, len2));
    TEST_ASSERT_ERROR_OK(zoo_smb_ring_buffer_write(ring, data3, len3));
    
    // Read them back in order
    char read_buffer[256] = {0};

    TEST_ASSERT_ERROR_OK(zoo_smb_ring_buffer_read(ring, read_buffer, len1));
    TEST_ASSERT_STREQ(data1, read_buffer);

    memset(read_buffer, 0, sizeof(read_buffer));
    TEST_ASSERT_ERROR_OK(zoo_smb_ring_buffer_read(ring, read_buffer, len2));
    TEST_ASSERT_STREQ(data2, read_buffer);

    memset(read_buffer, 0, sizeof(read_buffer));
    TEST_ASSERT_ERROR_OK(zoo_smb_ring_buffer_read(ring, read_buffer, len3));
    TEST_ASSERT_STREQ(data3, read_buffer);
}

void test_read_empty_buffer(void)
{
    char read_buffer[256];
    
    // Reading from empty buffer should return appropriate error
    ZOO_ERROR_TYPE result = zoo_smb_ring_buffer_read(ring, read_buffer, sizeof(read_buffer));
    TEST_ASSERT_ERROR_NOT_OK(result);
}

void test_write_oversized_data(void)
{
    // Try to write data larger than buffer capacity
    size_t large_size = data_size + 1000;
    char* large_data = test_tracked_malloc(large_size);
    TEST_ASSERT_NOT_NULL(large_data);
    
    memset(large_data, 'A', large_size - 1);
    large_data[large_size - 1] = '\0';
    
    ZOO_ERROR_TYPE result = zoo_smb_ring_buffer_write(ring, large_data, large_size);
    TEST_ASSERT_ERROR_NOT_OK(result);
}

void test_buffer_capacity_info(void)
{
    size_t free_space = zoo_smb_ring_buffer_get_free_space(ring);
    size_t used_space = zoo_smb_ring_buffer_get_used_space(ring);
    
    // Initially buffer should be empty
    TEST_ASSERT_GREATER_THAN(0, free_space);
    TEST_ASSERT_EQUAL_INT(0, used_space);
    
    // Write some data
    const char* test_data = "test data";
    size_t len = strlen(test_data) + 1;
    TEST_ASSERT_ERROR_OK(zoo_smb_ring_buffer_write(ring, test_data, len));
    
    // Check space changed
    size_t new_free_space = zoo_smb_ring_buffer_get_free_space(ring);
    size_t new_used_space = zoo_smb_ring_buffer_get_used_space(ring);
    
    TEST_ASSERT_TRUE(new_free_space < free_space);
    TEST_ASSERT_TRUE(new_used_space > used_space);
}

void run_ring_buffer_tests(void)
{
    RUN_TEST(test_create_and_destroy);
    RUN_TEST(test_write_and_read);
    RUN_TEST(test_write_read_multiple);
    RUN_TEST(test_read_empty_buffer);
    RUN_TEST(test_write_oversized_data);
    RUN_TEST(test_buffer_capacity_info);
}

RUN_TEST_SUITE(run_ring_buffer_tests)
