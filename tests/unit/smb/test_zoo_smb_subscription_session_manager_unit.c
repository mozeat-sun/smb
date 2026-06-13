/*******************************************************************************
 * Copyright (C) 2026, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: UNIT_TEST
 * File name: test_zoo_smb_subscription_session_manager_unit.c
 * Description: Unit-style branch coverage tests for subscriber subscription
 *              session manager lifecycle and reconciliation behavior.
 * Traceability coverage:
 * - REQ-REL-004: degraded, active, failed, and pending runtime state handling.
 * - REQ-SAFE-003: requirement-linked unit evidence for session lifecycle logic.
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-11     github.copilot    created
 ******************************************************************************/

#include "unity.h"
#include "test_smb_helpers.h"
#include "zoo_smb_subscription_session_manager.h"
#include "zoo_smb_subscription_session.h"
#include "zoo_smb_qos_ctx.h"
#include <string.h>

static int test_handler(
    IN void* user_data,
    IN uint32_t msg_id,
    IN uint64_t request_id,
    IN uint64_t timestamp,
    IN const char* sender,
    IN const void* payload,
    IN size_t payload_size)
{
    (void)user_data;
    (void)msg_id;
    (void)request_id;
    (void)timestamp;
    (void)sender;
    (void)payload;
    (void)payload_size;
    return 0;
}

static int test_handler_alt(
    IN void* user_data,
    IN uint32_t msg_id,
    IN uint64_t request_id,
    IN uint64_t timestamp,
    IN const char* sender,
    IN const void* payload,
    IN size_t payload_size)
{
    (void)user_data;
    (void)msg_id;
    (void)request_id;
    (void)timestamp;
    (void)sender;
    (void)payload;
    (void)payload_size;
    return 0;
}

void setUp(void)
{
    test_setup_memory_pool();
}

void tearDown(void)
{
    test_teardown_memory_pool();
}

void test_manager_add_find_remove_and_invalid_params(void)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager = zoo_smb_subscription_session_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session = zoo_smb_subscription_session_create(
        100,
        test_handler,
        NULL,
        1,
        0,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(session);

    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_ERROR_INVALID_PARAM,
                            (uint32_t)zoo_smb_subscription_session_manager_add(NULL, session));
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_ERROR_INVALID_PARAM,
                            (uint32_t)zoo_smb_subscription_session_manager_add(manager, NULL));

    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_subscription_session_manager_add(manager, session));

    TEST_ASSERT_EQUAL_PTR(session,
                          zoo_smb_subscription_session_manager_find_by_id(manager, 1));
    TEST_ASSERT_EQUAL_PTR(session,
                          zoo_smb_subscription_session_manager_find_by_handler(manager, test_handler));
    TEST_ASSERT_NULL(zoo_smb_subscription_session_manager_find_by_handler(manager, test_handler_alt));

    zoo_smb_subscription_session_manager_remove(manager, NULL, session);
    TEST_ASSERT_NULL(zoo_smb_subscription_session_manager_find_by_id(manager, 1));

    zoo_smb_subscription_session_manager_destroy(manager, NULL);
}

void test_manager_on_suback_transitions(void)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager = zoo_smb_subscription_session_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE success_session = zoo_smb_subscription_session_create(
        110,
        test_handler,
        NULL,
        10,
        0,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(success_session);
    success_session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    success_session->request_id = 0x111U;
    success_session->retry_attempts = 3;
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_subscription_session_manager_add(manager, success_session));

    zoo_smb_subscription_session_manager_on_suback(manager, success_session->request_id, ZOO_TRUE);
    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ACTIVE, success_session->state);
    TEST_ASSERT_EQUAL_UINT32(0U, success_session->retry_attempts);

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE fail_session = zoo_smb_subscription_session_create(
        111,
        test_handler_alt,
        NULL,
        11,
        0,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(fail_session);
    fail_session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    fail_session->request_id = 0x222U;
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_subscription_session_manager_add(manager, fail_session));

    zoo_smb_subscription_session_manager_on_suback(manager, fail_session->request_id, ZOO_FALSE);
    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED, fail_session->state);

    zoo_smb_subscription_session_manager_destroy(manager, NULL);
}

void test_manager_on_service_change_marks_degraded_and_clears_request(void)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager = zoo_smb_subscription_session_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(qos_entity);
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(qos_entity);
    TEST_ASSERT_NOT_NULL(qos_ctx);

    ZOO_SMB_NODE_STRUCT node;
    memset(&node, 0, sizeof(node));
    node.active = ZOO_TRUE;

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session = zoo_smb_subscription_session_create(
        120,
        test_handler,
        NULL,
        12,
        50,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(session);
    session->desired_active = ZOO_TRUE;
    session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    session->request_id = 777U;

    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_subscription_session_manager_add(manager, session));
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_qos_ctx_set_state(qos_ctx, 777U, ZOO_SMB_MSG_ST_WAITING_ACK, NULL));

    zoo_smb_subscription_session_manager_on_service_change(
        manager,
        qos_entity,
        ZOO_FALSE,
        100,
        &node);

    TEST_ASSERT_FALSE(node.active);
    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED, session->state);
    TEST_ASSERT_EQUAL_UINT64(0U, session->request_id);
    TEST_ASSERT_TRUE(session->next_retry_at_ms > 100U);
    TEST_ASSERT_NULL(zoo_smb_qos_ctx_get_state(qos_ctx, 777U));

    zoo_smb_subscription_session_manager_destroy(manager, qos_entity);
    zoo_smb_destroy_qos_entity(qos_entity);
}

void test_manager_reconcile_waiting_suback_with_missing_request_id_degrades(void)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager = zoo_smb_subscription_session_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(qos_entity);

    ZOO_SMB_NODE_STRUCT node;
    memset(&node, 0, sizeof(node));
    node.active = ZOO_TRUE;

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session = zoo_smb_subscription_session_create(
        130,
        test_handler,
        NULL,
        13,
        200,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(session);
    session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    session->request_id = 0U;
    session->desired_active = ZOO_TRUE;

    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_subscription_session_manager_add(manager, session));

    ZOO_BOOL has_pending_work = ZOO_FALSE;
    uint32_t sleep_ms = 0U;

    zoo_smb_subscription_session_manager_reconcile(
        manager,
        &node,
        qos_entity,
        ZOO_TRUE,
        200,
        &has_pending_work,
        &sleep_ms);

    TEST_ASSERT_TRUE(has_pending_work);
    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED, session->state);
    TEST_ASSERT_TRUE(session->next_retry_at_ms > 200U);
    TEST_ASSERT_TRUE(sleep_ms <= 1000U);

    zoo_smb_subscription_session_manager_destroy(manager, qos_entity);
    zoo_smb_destroy_qos_entity(qos_entity);
}

void test_manager_reconcile_failed_qos_result_branches(void)
{
    ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager = zoo_smb_subscription_session_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(qos_entity);
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(qos_entity);
    TEST_ASSERT_NOT_NULL(qos_ctx);

    ZOO_SMB_NODE_STRUCT node;
    memset(&node, 0, sizeof(node));
    node.active = ZOO_TRUE;

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE incompatible_session = zoo_smb_subscription_session_create(
        140,
        test_handler,
        NULL,
        14,
        300,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(incompatible_session);
    incompatible_session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    incompatible_session->request_id = 901U;

    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_subscription_session_manager_add(manager, incompatible_session));
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_qos_ctx_set_state(qos_ctx, 901U, ZOO_SMB_MSG_ST_WAITING_ACK, NULL));
    zoo_smb_qos_ctx_set_msg_status(qos_ctx, 901U, ZOO_SMB_MSG_ST_FAILED);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_qos_ctx_set_match_result(qos_ctx, 901U, ZOO_FALSE, 0x1U));

    ZOO_BOOL has_pending_work = ZOO_FALSE;
    uint32_t sleep_ms = 0U;
    zoo_smb_subscription_session_manager_reconcile(
        manager,
        &node,
        qos_entity,
        ZOO_TRUE,
        300,
        &has_pending_work,
        &sleep_ms);

    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED, incompatible_session->state);
    TEST_ASSERT_EQUAL_UINT64(0U, incompatible_session->request_id);

    ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE retryable_session = zoo_smb_subscription_session_create(
        141,
        test_handler_alt,
        NULL,
        15,
        400,
        ZOO_TRUE);
    TEST_ASSERT_NOT_NULL(retryable_session);
    retryable_session->state = ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK;
    retryable_session->request_id = 902U;

    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_subscription_session_manager_add(manager, retryable_session));
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_qos_ctx_set_state(qos_ctx, 902U, ZOO_SMB_MSG_ST_WAITING_ACK, NULL));
    zoo_smb_qos_ctx_set_msg_status(qos_ctx, 902U, ZOO_SMB_MSG_ST_FAILED);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK,
                            (uint32_t)zoo_smb_qos_ctx_set_match_result(qos_ctx, 902U, ZOO_FALSE, 0U));

    has_pending_work = ZOO_FALSE;
    sleep_ms = 0U;
    zoo_smb_subscription_session_manager_reconcile(
        manager,
        &node,
        qos_entity,
        ZOO_TRUE,
        400,
        &has_pending_work,
        &sleep_ms);

    TEST_ASSERT_EQUAL(ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED, retryable_session->state);
    TEST_ASSERT_EQUAL_UINT64(0U, retryable_session->request_id);
    TEST_ASSERT_TRUE(retryable_session->next_retry_at_ms > 400U);
    TEST_ASSERT_TRUE(has_pending_work);

    zoo_smb_subscription_session_manager_destroy(manager, qos_entity);
    zoo_smb_destroy_qos_entity(qos_entity);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_manager_add_find_remove_and_invalid_params);
    RUN_TEST(test_manager_on_suback_transitions);
    RUN_TEST(test_manager_on_service_change_marks_degraded_and_clears_request);
    RUN_TEST(test_manager_reconcile_waiting_suback_with_missing_request_id_degrades);
    RUN_TEST(test_manager_reconcile_failed_qos_result_branches);

    return UNITY_END();
}