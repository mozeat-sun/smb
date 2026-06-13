/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_zoo_smb_consumer_
 * File name: test_zoo_smb_consumer.c
 * Description: Unity-based consumer tests (converted from GTest)
 * Traceability coverage:
 * - REQ-REL-001: consumer create/destroy and registration lifecycle validation.
 * - REQ-SAFE-003: requirement-linked unit evidence for consumer management.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                converted from GTest
 ******************************************************************************/

#include "test_smb_helpers.h"
#include "zoo_smb_consumer.h"
#include <string.h>
#include <netinet/in.h>

// Global test state
static ZOO_LIST_HANDLE test_list = NULL;
static struct sockaddr_in test_addr;

void setUp(void)
{
    // Initialize memory pool and list
    test_setup_memory_pool();
    test_list = zoo_smb_create_list(1024);
    TEST_ASSERT_NOT_NULL(test_list);
    
    // Setup test address
    memset(&test_addr, 0, sizeof(test_addr));
    test_addr.sin_family = AF_INET;
    test_addr.sin_port = htons(12345);
    test_addr.sin_addr.s_addr = htonl(0x7F000001); // 127.0.0.1
}

void tearDown(void)
{
    if (test_list) {
        zoo_smb_destroy_list(test_list);
        test_list = NULL;
    }
    test_teardown_memory_pool();
}

void test_create_and_destroy_consumer(void)
{
    ZOO_SMB_CONSUMER_HANDLE consumer = zoo_smb_create_consumer("test", 42, &test_addr);
    TEST_ASSERT_NOT_NULL(consumer);
    TEST_ASSERT_STREQ("test", consumer->name);
    TEST_ASSERT_EQUAL_INT(42, consumer->fd);
    TEST_ASSERT_EQUAL_MEMORY(&consumer->addr, &test_addr, sizeof(test_addr));
    
    ZOO_ERROR_TYPE result = zoo_smb_destroy_consumer(consumer);
    TEST_ASSERT_ERROR_OK(result);
}

void test_create_consumer_null_name(void)
{
    ZOO_SMB_CONSUMER_HANDLE consumer = zoo_smb_create_consumer(NULL, 1, &test_addr);
    TEST_ASSERT_NULL(consumer);
}

void test_create_consumer_and_insert(void)
{
    ZOO_SMB_CONSUMER_HANDLE consumer = zoo_smb_create_consumer_and_insert(test_list, "inserted", 55, &test_addr);
    TEST_ASSERT_NOT_NULL(consumer);
    TEST_ASSERT_STREQ("inserted", consumer->name);
    TEST_ASSERT_EQUAL_INT(55, consumer->fd);
    
    size_t list_size = zoo_list_size(test_list);
    TEST_ASSERT_EQUAL_UINT32(1, list_size);
    
    ZOO_ERROR_TYPE result = zoo_smb_destroy_consumer(consumer);
    TEST_ASSERT_ERROR_OK(result);
}

void test_find_by_name(void)
{
    ZOO_SMB_CONSUMER_HANDLE consumer = zoo_smb_create_consumer_and_insert(test_list, "findme", 99, &test_addr);
    TEST_ASSERT_NOT_NULL(consumer);
    
    ZOO_SMB_CONSUMER_HANDLE found = zoo_smb_find_consumer_by_name(test_list, "findme");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(consumer, found);
    
    ZOO_SMB_CONSUMER_HANDLE not_found = zoo_smb_find_consumer_by_name(test_list, "notexist");
    TEST_ASSERT_NULL(not_found);
    
    ZOO_ERROR_TYPE result = zoo_smb_destroy_consumer(consumer);
    TEST_ASSERT_ERROR_OK(result);
}

void test_find_by_fd(void)
{
    ZOO_SMB_CONSUMER_HANDLE consumer = zoo_smb_create_consumer_and_insert(test_list, "findfd", 77, &test_addr);
    TEST_ASSERT_NOT_NULL(consumer);
    
    ZOO_SMB_CONSUMER_HANDLE found = zoo_smb_find_consumer_by_fd(test_list, 77);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(consumer, found);
    
    ZOO_SMB_CONSUMER_HANDLE not_found = zoo_smb_find_consumer_by_fd(test_list, 999);
    TEST_ASSERT_NULL(not_found);
    
    ZOO_ERROR_TYPE result = zoo_smb_destroy_consumer(consumer);
    TEST_ASSERT_ERROR_OK(result);
}

void test_consumer_validation(void)
{
    // Test with invalid parameters
    ZOO_SMB_CONSUMER_HANDLE consumer1 = zoo_smb_create_consumer("", 1, &test_addr);
    TEST_ASSERT_NULL(consumer1);
    
    ZOO_SMB_CONSUMER_HANDLE consumer2 = zoo_smb_create_consumer("test", -1, &test_addr);
    TEST_ASSERT_NULL(consumer2);
    
    ZOO_SMB_CONSUMER_HANDLE consumer3 = zoo_smb_create_consumer("test", 1, NULL);
    TEST_ASSERT_NULL(consumer3);
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_create_and_destroy_consumer);
    RUN_TEST(test_create_consumer_null_name);
    RUN_TEST(test_create_consumer_and_insert);
    RUN_TEST(test_find_by_name);
    RUN_TEST(test_find_by_fd);
    RUN_TEST(test_consumer_validation);
    
    return UNITY_END();
}
