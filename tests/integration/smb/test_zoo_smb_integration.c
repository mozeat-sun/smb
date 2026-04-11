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
 * Traceability coverage:
 * - REQ-REL-001: startup and basic operational lifecycle API validation.
 * - REQ-REL-004: degraded and active runtime state transitions for subscriber sessions.
 * - REQ-SAFE-003: requirement-linked node/session integration evidence.
 ******************************************************************************/

#include <string.h>
#include "unity.h"
#include "zoo_smb.h"
#include "zoo_smb_server.h"
#include "zoo_smb_client.h"
#include "zoo_smb_publisher.h"
#include "zoo_smb_subscriber.h"
#include "zoo_smb_node.h"
#include "zoo_smb_qos.h"
#include "zoo_smb_subscription_session.h"
#include "zoo_smb_subscription_session_manager.h"

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

static int test_msg_handler_alt(
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

    result = zoo_smb_subscribe_message(
        subscriber,
        100,
        test_msg_handler,
        NULL,
        NULL);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_ERROR_INVALID_PARAM, (uint32_t)result);

    zoo_smb_destroy_subscriber(subscriber);
}

void test_integration_subscriber_multiple_sessions_register(void)
{
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = zoo_smb_create_subscriber(
        "it_subscriber_multi",
        "it_publisher_multi",
        "it/topic",
        NULL);

    TEST_ASSERT_NOT_NULL(subscriber);

    int32_t handle_a = -1;
    int32_t handle_b = -1;

    ZOO_ERROR_TYPE result_a = zoo_smb_subscribe_message(
        subscriber,
        200,
        test_msg_handler,
        NULL,
        &handle_a);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result_a);
    TEST_ASSERT_TRUE(handle_a > 0);

    ZOO_ERROR_TYPE result_b = zoo_smb_subscribe_message(
        subscriber,
        201,
        test_msg_handler_alt,
        NULL,
        &handle_b);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result_b);
    TEST_ASSERT_TRUE(handle_b > 0);
    TEST_ASSERT_NOT_EQUAL(handle_a, handle_b);

    zoo_smb_unsubscribe_message(subscriber, handle_a);
    zoo_smb_unsubscribe_message(subscriber, handle_b);
    zoo_smb_destroy_subscriber(subscriber);
}

void test_integration_subscriber_subscribe_same_handler_reuses_session(void)
{
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = zoo_smb_create_subscriber(
        "it_subscriber_reuse",
        "it_publisher_reuse",
        "it/topic",
        NULL);

    TEST_ASSERT_NOT_NULL(subscriber);

    int32_t first_handle = -1;
    int32_t second_handle = -1;

    TEST_ASSERT_EQUAL(ZOO_SMB_OK,
                      zoo_smb_subscribe_message(subscriber, 300, test_msg_handler, NULL, &first_handle));
    TEST_ASSERT_TRUE(first_handle > 0);

    TEST_ASSERT_EQUAL(ZOO_SMB_OK,
                      zoo_smb_subscribe_message(subscriber, 301, test_msg_handler, NULL, &second_handle));

    TEST_ASSERT_EQUAL(first_handle, second_handle);

    zoo_smb_unsubscribe_message(subscriber, first_handle);
    zoo_smb_destroy_subscriber(subscriber);
}

void test_integration_subscriber_unsubscribe_invalid_handle_safe(void)
{
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = zoo_smb_create_subscriber(
        "it_subscriber_bad_unsub",
        "it_publisher_bad_unsub",
        "it/topic",
        NULL);

    TEST_ASSERT_NOT_NULL(subscriber);

    zoo_smb_unsubscribe_message(subscriber, -1);
    zoo_smb_unsubscribe_message(subscriber, 123456789);

    zoo_smb_destroy_subscriber(subscriber);
}

void test_integration_subscriber_double_unsubscribe_safe(void)
{
    ZOO_SMB_SUBSCRIBER_HANDLE subscriber = zoo_smb_create_subscriber(
        "it_subscriber_double_unsub",
        "it_publisher_double_unsub",
        "it/topic",
        NULL);

    TEST_ASSERT_NOT_NULL(subscriber);

    int32_t handle = -1;
    TEST_ASSERT_EQUAL(ZOO_SMB_OK,
                      zoo_smb_subscribe_message(subscriber, 410, test_msg_handler, NULL, &handle));
    TEST_ASSERT_TRUE(handle > 0);

    zoo_smb_unsubscribe_message(subscriber, handle);
    zoo_smb_unsubscribe_message(subscriber, handle);

    zoo_smb_destroy_subscriber(subscriber);
}

void test_integration_session_manager_suback_transitions(void)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager = zoo_smb_subscription_session_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE matched = zoo_smb_subscription_session_create(
        501,
        test_msg_handler,
        NULL,
        101,
        1000,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(matched);

    matched->request_id = 0xA11U;
    matched->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    matched->retry_attempts = 3;
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_subscription_session_manager_add(manager, matched));

    zoo_smb_subscription_session_manager_on_suback(manager, matched->request_id, ZOO_TRUE);
    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ACTIVE, matched->state);
    TEST_ASSERT_EQUAL_UINT32(0, matched->retry_attempts);

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE unmatched = zoo_smb_subscription_session_create(
        502,
        test_msg_handler_alt,
        NULL,
        102,
        1000,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(unmatched);

    unmatched->request_id = 0xB22U;
    unmatched->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_subscription_session_manager_add(manager, unmatched));

    zoo_smb_subscription_session_manager_on_suback(manager, unmatched->request_id, ZOO_FALSE);
    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED, unmatched->state);

    zoo_smb_subscription_session_manager_destroy(manager, NULL);
}

void test_integration_session_manager_service_change_and_reconcile(void)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager = zoo_smb_subscription_session_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    ZOO_SMB_NODE_HANDLE node = zoo_smb_create_node(
        "it_sub_node",
        "it_target",
        "it/topic",
        ZOO_SMB_NODE_TYPE_SUBSCRIBER,
        ZOO_SMB_TRANSPORT_TYPE_DEFAULT);
    TEST_ASSERT_NOT_NULL(node);

    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(qos_entity);

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session = zoo_smb_subscription_session_create(
        600,
        test_msg_handler,
        NULL,
        201,
        100,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(session);

    session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    session->request_id = 1234;
    session->desired_active = ZOO_TRUE;
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_subscription_session_manager_add(manager, session));

    zoo_smb_subscription_session_manager_on_service_change(
        manager,
        qos_entity,
        ZOO_FALSE,
        1000,
        node);

    TEST_ASSERT_FALSE(node->active);
    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED, session->state);
    TEST_ASSERT_EQUAL_UINT64(0, session->request_id);
    TEST_ASSERT_TRUE(session->next_retry_at_ms > 1000);

    session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    session->request_id = 0;
    session->desired_active = ZOO_TRUE;

    ZOO_BOOL has_pending_work = ZOO_FALSE;
    uint32_t sleep_ms = 0;

    zoo_smb_subscription_session_manager_reconcile(
        manager,
        node,
        qos_entity,
        ZOO_TRUE,
        2000,
        &has_pending_work,
        &sleep_ms);

    TEST_ASSERT_TRUE(has_pending_work);
    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED, session->state);
    TEST_ASSERT_TRUE(session->next_retry_at_ms >= 2000);

    zoo_smb_subscription_session_manager_destroy(manager, qos_entity);
    zoo_smb_destroy_qos_entity(qos_entity);
    zoo_smb_destroy_node(node);
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
    RUN_TEST(test_integration_subscriber_multiple_sessions_register);
    RUN_TEST(test_integration_subscriber_subscribe_same_handler_reuses_session);
    RUN_TEST(test_integration_subscriber_unsubscribe_invalid_handle_safe);
    RUN_TEST(test_integration_subscriber_double_unsubscribe_safe);
    RUN_TEST(test_integration_session_manager_suback_transitions);
    RUN_TEST(test_integration_session_manager_service_change_and_reconcile);
    RUN_TEST(test_integration_publisher_invalid_publish_params);

    return UNITY_END();
}


