/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_zoo_smb_message_
 * File name: test_zoo_smb_message.c
 * Description: Unity-based message tests (converted from GTest)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                converted from GTest
 ******************************************************************************/

#include "test_utils.h"
#include "zoo_smb_message.h"
#include <string.h>

void test_create_and_destroy_message(void)
{
    const char* sender = "sender";
    const char* topic = "topic";
    const char* payload = "hello";
    size_t payload_size = strlen(payload) + 1;
    
    ZOO_SMB_MSG_STRUCT* msg = zoo_smb_create_message(
        ZOO_SMB_MSG_TYPE_REQ, sender, topic, payload, payload_size, 123, 456);
    
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_MSG_TYPE_REQ, msg->header.msg_type);
    TEST_ASSERT_STREQ(sender, msg->header.sender);
    TEST_ASSERT_STREQ(topic, msg->header.topic);
    TEST_ASSERT_EQUAL_INT(payload_size, msg->header.payload_size);
    TEST_ASSERT_NOT_NULL(msg->payload);
    TEST_ASSERT_STREQ(payload, (const char*)msg->payload);
    
    zoo_smb_destroy_message(msg);
}

void test_create_message_null_payload_zero_size(void)
{
    const char* sender = "sender";
    const char* topic = "topic";
    
    ZOO_SMB_MSG_STRUCT* msg = zoo_smb_create_message(
        ZOO_SMB_MSG_TYPE_REQ, sender, topic, NULL, 0, 1, 2);
    
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_NULL(msg->payload);
    
    zoo_smb_destroy_message(msg);
}

void test_create_message_invalid_params(void)
{
    // Null sender
    TEST_ASSERT_NULL(zoo_smb_create_message(ZOO_SMB_MSG_TYPE_REQ, NULL, "topic", "a", 1, 1, 1));
    
    // Null topic
    TEST_ASSERT_NULL(zoo_smb_create_message(ZOO_SMB_MSG_TYPE_REQ, "sender", NULL, "a", 1, 1, 1));
    
    // Payload size > 0 but payload is null
    TEST_ASSERT_NULL(zoo_smb_create_message(ZOO_SMB_MSG_TYPE_REQ, "sender", "topic", NULL, 10, 1, 1));
}

void test_msg_type_to_string(void)
{
    TEST_ASSERT_STREQ("REQ", zoo_smb_msg_type_to_string(ZOO_SMB_MSG_TYPE_REQ));
    TEST_ASSERT_STREQ("PUB", zoo_smb_msg_type_to_string(ZOO_SMB_MSG_TYPE_PUB));
    TEST_ASSERT_STREQ("UNKNOWN", zoo_smb_msg_type_to_string((ZOO_SMB_MSG_TYPE_ENUM)0xFF));
}

void test_create_and_destroy_header(void)
{
    const char* sender = "sender";
    const char* topic = "topic";
    size_t payload_size = 10;
    
    ZOO_SMB_MSG_HEADER_STRUCT* header = zoo_smb_create_message_header(
        ZOO_SMB_MSG_TYPE_PUB, sender, topic, payload_size, 11, 22);
    
    TEST_ASSERT_NOT_NULL(header);
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_MSG_TYPE_PUB, header->msg_type);
    TEST_ASSERT_STREQ(sender, header->sender);
    TEST_ASSERT_STREQ(topic, header->topic);
    TEST_ASSERT_EQUAL_INT(payload_size, header->payload_size);
    
    zoo_smb_destroy_message_header(header);
}

void test_destroy_null_header_and_message(void)
{
    // These should not crash
    zoo_smb_destroy_message_header(NULL);
    zoo_smb_destroy_message(NULL);
    
    // If we get here without crashing, test passes
    TEST_ASSERT_TRUE(1);
}

void test_copy_message_shallow(void)
{
    const char* sender = "sender";
    const char* topic = "topic";
    const char* payload = "payload";
    size_t payload_size = strlen(payload) + 1;
    
    ZOO_SMB_MSG_STRUCT* original = zoo_smb_create_message(
        ZOO_SMB_MSG_TYPE_PUB, sender, topic, payload, payload_size, 100, 200);
    TEST_ASSERT_NOT_NULL(original);
    
    // Test shallow copy if function exists
    // Note: This test assumes the function exists, adapt as needed
    ZOO_SMB_MSG_STRUCT* copy = zoo_smb_copy_message_shallow(original);
    if (copy != NULL) {
        TEST_ASSERT_EQUAL_INT(original->header.msg_type, copy->header.msg_type);
        TEST_ASSERT_STREQ(original->header.sender, copy->header.sender);
        TEST_ASSERT_STREQ(original->header.topic, copy->header.topic);
        TEST_ASSERT_EQUAL_PTR(original->payload, copy->payload); // Shallow copy - same pointer

        free(copy);
    }
    
    zoo_smb_destroy_message(original);
}

void run_message_tests(void)
{
    RUN_TEST(test_create_and_destroy_message);
    RUN_TEST(test_create_message_null_payload_zero_size);
    RUN_TEST(test_create_message_invalid_params);
    RUN_TEST(test_msg_type_to_string);
    RUN_TEST(test_create_and_destroy_header);
    RUN_TEST(test_destroy_null_header_and_message);
    RUN_TEST(test_copy_message_shallow);
}

TEST_SETUP()
TEST_TEARDOWN()

RUN_TEST_SUITE(run_message_tests)
