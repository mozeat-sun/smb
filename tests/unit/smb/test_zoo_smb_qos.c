/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_zoo_smb_qos_
 * File name: test_zoo_smb_qos.c
 * Description: Unity-based QoS tests — updated for current sub-struct policy API.
 * Traceability coverage:
 * - REQ-SAFE-001: deterministic QoS policy validation behavior.
 * - REQ-SAFE-003: requirement-linked unit evidence for QoS policy primitives.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                converted from GTest
 * 2.0       2026-06-13     claude            updated for current API
 ******************************************************************************/

#include "test_smb_helpers.h"
#include "zoo_smb_qos.h"
#include "zoo_smb_qos_policy.h"

void test_create_and_destroy_qos_entity(void)
{
    ZOO_SMB_QOS_ENTITY_HANDLE entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(entity);

    ZOO_SMB_QOS_POLICY_HANDLE policy = zoo_smb_qos_get_policy(entity);
    TEST_ASSERT_NOT_NULL(policy);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(entity);
    TEST_ASSERT_NOT_NULL(qos_ctx);

    zoo_smb_destroy_qos_entity(entity);
}

void test_qos_policy_is_valid_with_default(void)
{
    ZOO_SMB_QOS_ENTITY_HANDLE entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(entity);

    ZOO_SMB_QOS_POLICY_HANDLE policy = zoo_smb_qos_get_policy(entity);
    TEST_ASSERT_NOT_NULL(policy);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_destroy_qos_entity(entity);
}

void test_qos_policy_null_rejected(void)
{
    TEST_ASSERT_FALSE(zoo_smb_qos_policy_is_valid(NULL));
}

void test_qos_policy_priority_settings(void)
{
    ZOO_SMB_QOS_ENTITY_HANDLE entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(entity);

    ZOO_SMB_QOS_POLICY_HANDLE policy = zoo_smb_qos_get_policy(entity);
    TEST_ASSERT_NOT_NULL(policy);

    zoo_smb_qos_policy_set_priority(policy, ZOO_SMB_QOS_PRIORITY_LOW);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_qos_policy_set_priority(policy, ZOO_SMB_QOS_PRIORITY_MEDIUM);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_qos_policy_set_priority(policy, ZOO_SMB_QOS_PRIORITY_HIGH);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_qos_policy_set_priority(policy, ZOO_SMB_QOS_PRIORITY_REALTIME);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_destroy_qos_entity(entity);
}

void test_qos_policy_reliability_settings(void)
{
    ZOO_SMB_QOS_ENTITY_HANDLE entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(entity);

    ZOO_SMB_QOS_POLICY_HANDLE policy = zoo_smb_qos_get_policy(entity);
    TEST_ASSERT_NOT_NULL(policy);

    zoo_smb_qos_policy_set_reliability(policy, ZOO_SMB_QOS_RELIABILITY_BEST_EFFORT, 1000, 3, 500);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_qos_policy_set_reliability(policy, ZOO_SMB_QOS_RELIABILITY_RELIABLE, 2000, 5, 1000);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_destroy_qos_entity(entity);
}

void test_qos_policy_durability_settings(void)
{
    ZOO_SMB_QOS_ENTITY_HANDLE entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(entity);

    ZOO_SMB_QOS_POLICY_HANDLE policy = zoo_smb_qos_get_policy(entity);
    TEST_ASSERT_NOT_NULL(policy);

    /* Test each valid durability value */
    zoo_smb_qos_policy_set_durability(policy, ZOO_SMB_QOS_DURABILITY_VOLATILE);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_qos_policy_set_durability(policy, ZOO_SMB_QOS_DURABILITY_TRANSIENT_LOCAL);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_qos_policy_set_durability(policy, ZOO_SMB_QOS_DURABILITY_PERSISTENT);
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_valid(policy));

    zoo_smb_destroy_qos_entity(entity);
}

void test_qos_policy_match_compatible(void)
{
    ZOO_SMB_QOS_ENTITY_HANDLE pub_entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(pub_entity);

    ZOO_SMB_QOS_ENTITY_HANDLE sub_entity = zoo_smb_create_qos_default_entity();
    TEST_ASSERT_NOT_NULL(sub_entity);

    ZOO_SMB_QOS_POLICY_HANDLE pub_policy = zoo_smb_qos_get_policy(pub_entity);
    ZOO_SMB_QOS_POLICY_HANDLE sub_policy = zoo_smb_qos_get_policy(sub_entity);

    /* Default policies should be compatible with each other */
    TEST_ASSERT_TRUE(zoo_smb_qos_policy_is_compatible(pub_policy, sub_policy));

    zoo_smb_destroy_qos_entity(pub_entity);
    zoo_smb_destroy_qos_entity(sub_entity);
}

void run_qos_tests(void)
{
    RUN_TEST(test_create_and_destroy_qos_entity);
    RUN_TEST(test_qos_policy_is_valid_with_default);
    RUN_TEST(test_qos_policy_null_rejected);
    RUN_TEST(test_qos_policy_priority_settings);
    RUN_TEST(test_qos_policy_reliability_settings);
    RUN_TEST(test_qos_policy_durability_settings);
    RUN_TEST(test_qos_policy_match_compatible);
}

TEST_SETUP()
TEST_TEARDOWN()

RUN_TEST_SUITE(run_qos_tests)
