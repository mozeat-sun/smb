/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_zoo_smb_qos_
 * File name: test_zoo_smb_qos.c
 * Description: Unity-based QoS tests (converted from GTest)
 * Traceability coverage:
 * - REQ-SAFE-001: deterministic QoS policy validation behavior.
 * - REQ-SAFE-003: requirement-linked unit evidence for QoS policy primitives.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                converted from GTest
 ******************************************************************************/

#include "test_utils.h"
#include "zoo_smb_qos.h"
#include "zoo_smb_qos_policy.h"

void test_create_and_destroy_qos_context(void)
{
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_create_context();
    TEST_ASSERT_NOT_NULL(qos_ctx);
    
    TEST_ASSERT_ERROR_OK(zoo_smb_qos_destroy_context(qos_ctx));
}

void test_qos_policy_creation(void)
{
    ZOO_SMB_QOS_POLICY_STRUCT policy;
    memset(&policy, 0, sizeof(policy));
    
    // Set some basic policy values
    policy.reliability = ZOO_SMB_QOS_RELIABILITY_RELIABLE;
    policy.durability = ZOO_SMB_QOS_DURABILITY_TRANSIENT;
    policy.priority = ZOO_SMB_QOS_PRIORITY_NORMAL;
    
    // Test policy validation
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
}

void test_qos_policy_invalid_params(void)
{
    // Test with NULL policy
    TEST_ASSERT_FALSE(zoo_smb_qos_is_policy_valid(NULL));
    
    ZOO_SMB_QOS_POLICY_STRUCT invalid_policy;
    memset(&invalid_policy, 0xFF, sizeof(invalid_policy)); // Fill with invalid values
    
    // Test with invalid policy values
    TEST_ASSERT_FALSE(zoo_smb_qos_is_policy_valid(&invalid_policy));
}

void test_qos_reliability_settings(void)
{
    ZOO_SMB_QOS_POLICY_STRUCT policy;
    memset(&policy, 0, sizeof(policy));
    
    // Test different reliability settings
    policy.reliability = ZOO_SMB_QOS_RELIABILITY_BEST_EFFORT;
    policy.durability = ZOO_SMB_QOS_DURABILITY_TRANSIENT;
    policy.priority = ZOO_SMB_QOS_PRIORITY_NORMAL;
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
    
    policy.reliability = ZOO_SMB_QOS_RELIABILITY_RELIABLE;
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
}

void test_qos_durability_settings(void)
{
    ZOO_SMB_QOS_POLICY_STRUCT policy;
    memset(&policy, 0, sizeof(policy));
    
    // Test different durability settings
    policy.reliability = ZOO_SMB_QOS_RELIABILITY_RELIABLE;
    policy.priority = ZOO_SMB_QOS_PRIORITY_NORMAL;
    
    policy.durability = ZOO_SMB_QOS_DURABILITY_VOLATILE;
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
    
    policy.durability = ZOO_SMB_QOS_DURABILITY_TRANSIENT;
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
    
    policy.durability = ZOO_SMB_QOS_DURABILITY_PERSISTENT;
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
}

void test_qos_priority_settings(void)
{
    ZOO_SMB_QOS_POLICY_STRUCT policy;
    memset(&policy, 0, sizeof(policy));
    
    // Test different priority settings
    policy.reliability = ZOO_SMB_QOS_RELIABILITY_RELIABLE;
    policy.durability = ZOO_SMB_QOS_DURABILITY_TRANSIENT;
    
    policy.priority = ZOO_SMB_QOS_PRIORITY_LOW;
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
    
    policy.priority = ZOO_SMB_QOS_PRIORITY_NORMAL;
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
    
    policy.priority = ZOO_SMB_QOS_PRIORITY_HIGH;
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
    
    policy.priority = ZOO_SMB_QOS_PRIORITY_CRITICAL;
    TEST_ASSERT_TRUE(zoo_smb_qos_is_policy_valid(&policy));
}

void test_qos_context_operations(void)
{
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_create_context();
    TEST_ASSERT_NOT_NULL(qos_ctx);
    
    ZOO_SMB_QOS_POLICY_STRUCT policy;
    memset(&policy, 0, sizeof(policy));
    policy.reliability = ZOO_SMB_QOS_RELIABILITY_RELIABLE;
    policy.durability = ZOO_SMB_QOS_DURABILITY_TRANSIENT;
    policy.priority = ZOO_SMB_QOS_PRIORITY_HIGH;
    
    // Set policy on context
    TEST_ASSERT_ERROR_OK(zoo_smb_qos_set_policy(qos_ctx, &policy));
    
    // Get policy from context
    ZOO_SMB_QOS_POLICY_STRUCT retrieved_policy;
    TEST_ASSERT_ERROR_OK(zoo_smb_qos_get_policy(qos_ctx, &retrieved_policy));
    
    // Compare policies
    TEST_ASSERT_EQUAL_INT(policy.reliability, retrieved_policy.reliability);
    TEST_ASSERT_EQUAL_INT(policy.durability, retrieved_policy.durability);
    TEST_ASSERT_EQUAL_INT(policy.priority, retrieved_policy.priority);
    
    TEST_ASSERT_ERROR_OK(zoo_smb_qos_destroy_context(qos_ctx));
}

void run_qos_tests(void)
{
    RUN_TEST(test_create_and_destroy_qos_context);
    RUN_TEST(test_qos_policy_creation);
    RUN_TEST(test_qos_policy_invalid_params);
    RUN_TEST(test_qos_reliability_settings);
    RUN_TEST(test_qos_durability_settings);
    RUN_TEST(test_qos_priority_settings);
    RUN_TEST(test_qos_context_operations);
}

TEST_SETUP()
TEST_TEARDOWN()

RUN_TEST_SUITE(run_qos_tests)
