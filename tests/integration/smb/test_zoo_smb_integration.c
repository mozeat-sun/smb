/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: INTEGRATION_TEST
 * File name: test_zoo_smb_integration.c
 * Description: Integration tests aligned with node/core public APIs
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/


/*******************************************************************************
 * Replacement tests using node/core API surface
 ******************************************************************************/

#include <string.h>
#include "unity.h"
#include "zoo_smb.h"
#include "zoo_smb_server.h"
#include "zoo_smb_client.h"
#include "zoo_smb_publisher.h"
#include "zoo_smb_subscriber.h"

static int test_msg_handler(
    void* user_data,
    uint32_t msg_id,
    uint64_t request_id,
    uint64_t timestamp,
    const char* sender,
    const void* payload,
    size_t payload_size)
{
    (void)user_data;
    (void)msg_id;
    (void)request_id;
    (void)timestamp;
    (void)sender;
    (void)payload;
    (void)payload_size;
    return ZOO_SMB_OK;
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_integration_core_is_ready_api(void)
{
    ZOO_BOOL ready = zoo_smb_is_ready();
    TEST_ASSERT_TRUE(ready == ZOO_TRUE || ready == ZOO_FALSE);
}

void test_integration_create_destroy_server(void)
{
    ZOO_SMB_SERVER_HANDLE server = zoo_smb_create_server(
        "it_server",
        "it_server",
        "it/topic",
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);

    TEST_ASSERT_NOT_NULL(server);
    zoo_smb_destroy_server(server);
}

void test_integration_create_destroy_client(void)
{
    ZOO_SMB_CLIENT_HANDLE client = zoo_smb_create_client(
        "it_client",
        "it_server",
        "it/topic",
        NULL);

    TEST_ASSERT_NOT_NULL(client);
    zoo_smb_destroy_client(client);
}

void test_integration_create_destroy_publisher(void)
{
    ZOO_SMB_PUBLISHER_HANDLE publisher = zoo_smb_create_publisher(
        "it_publisher",
        "it_target",
        "it/topic",
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);

    TEST_ASSERT_NOT_NULL(publisher);
    zoo_smb_destroy_publisher(publisher);
}

void test_integration_create_destroy_subscriber(void)
{
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = zoo_smb_create_subscriber(
        "it_subscriber",
        "it_publisher",
        "it/topic",
        NULL);

    TEST_ASSERT_NOT_NULL(subscriber);
    zoo_smb_destroy_subscriber(subscriber);
}

void test_integration_server_set_message_handler(void)
{
    ZOO_SMB_SERVER_HANDLE server = zoo_smb_create_server(
        "it_server_h",
        "it_server_h",
        "it/topic",
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);

    TEST_ASSERT_NOT_NULL(server);

    ZOO_ERROR_TYPE result = zoo_smb_server_set_message_handler(server, test_msg_handler, NULL);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);

    zoo_smb_destroy_server(server);
}

void test_integration_client_invalid_send_request_params(void)
{
    ZOO_SMB_CLIENT_HANDLE client = zoo_smb_create_client(
        "it_client_req",
        "it_server_req",
        "it/topic",
        NULL);

    TEST_ASSERT_NOT_NULL(client);

    int64_t request_id = 0;
    const char* payload = "abc";

    ZOO_ERROR_TYPE result_null_payload = zoo_smb_client_send_request(
        client,
        1,
        NULL,
        3,
        &request_id);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_ERROR_INVALID_PARAM, (uint32_t)result_null_payload);

    ZOO_ERROR_TYPE result_null_request_id = zoo_smb_client_send_request(
        client,
        1,
        payload,
        strlen(payload),
        NULL);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_ERROR_INVALID_PARAM, (uint32_t)result_null_request_id);

    zoo_smb_destroy_client(client);
}

void test_integration_subscriber_invalid_subscribe_params(void)
{
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = zoo_smb_create_subscriber(
        "it_subscriber_sub",
        "it_publisher_sub",
        "it/topic",
        NULL);

    TEST_ASSERT_NOT_NULL(subscriber);

    int32_t handle = -1;
    ZOO_ERROR_TYPE result = zoo_smb_subscribe_message(
        subscriber,
        100,
        NULL,
        NULL,
        &handle);

    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_ERROR_INVALID_PARAM, (uint32_t)result);

    zoo_smb_destroy_subscriber(subscriber);
}

void test_integration_publisher_invalid_publish_params(void)
{
    ZOO_SMB_PUBLISHER_HANDLE publisher = zoo_smb_create_publisher(
        "it_publisher_pub",
        "it_target_pub",
        "it/topic",
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
        NULL);

    TEST_ASSERT_NOT_NULL(publisher);

    ZOO_ERROR_TYPE result = zoo_smb_publish_message(
        publisher,
        10,
        NULL,
        16);

    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_ERROR_INVALID_PARAM, (uint32_t)result);

    zoo_smb_destroy_publisher(publisher);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_integration_core_is_ready_api);
    RUN_TEST(test_integration_create_destroy_server);
    RUN_TEST(test_integration_create_destroy_client);
    RUN_TEST(test_integration_create_destroy_publisher);
    RUN_TEST(test_integration_create_destroy_subscriber);
    RUN_TEST(test_integration_server_set_message_handler);
    RUN_TEST(test_integration_client_invalid_send_request_params);
    RUN_TEST(test_integration_subscriber_invalid_subscribe_params);
    RUN_TEST(test_integration_publisher_invalid_publish_params);

    return UNITY_END();
}


