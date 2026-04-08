#include "unity.h"
#include "test_utils.h"
#include "zoo_smb_qos_ctx.h"
#include <string.h>
#include <stdint.h>

/**
 * @brief Test or example function setUp.
 */
void setUp(void) {
    // Setup for each test
    zoo_create_memory_pool(1024*1024);
}

/**
 * @brief Test or example function tearDown.
 */
void tearDown(void) {
    // Cleanup after each test
}

/**
 * @brief Test or example function test_create_and_destroy_qos_ctx.
 */
void test_create_and_destroy_qos_ctx(void) {
    ZOO_SMB_QOS_CTX_HANDLE ctx = zoo_smb_create_qos_ctx(NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    zoo_smb_destroy_qos_ctx(ctx);
}

/**
 * @brief Test or example function test_get_qos_policy.
 */
void test_get_qos_policy(void) {
    ZOO_SMB_QOS_POLICY_STRUCT policy = ZOO_SMB_DEFAULT_QOS_POLICY();
    ZOO_SMB_QOS_POLICY_HANDLE dummy_policy = &policy;
    ZOO_SMB_QOS_CTX_HANDLE ctx = zoo_smb_create_qos_ctx(dummy_policy);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL_PTR(dummy_policy, zoo_smb_qos_ctx_get_qos_policy(ctx));
    zoo_smb_destroy_qos_ctx(ctx);
}

/**
 * @brief Test or example function test_set_and_get_state.
 */
void test_set_and_get_state(void) {
    ZOO_SMB_QOS_CTX_HANDLE ctx = zoo_smb_create_qos_ctx(NULL);
    TEST_ASSERT_NOT_NULL(ctx);

    uint64_t req_id = 42;
    void* dummy_msg = (void*)0x5678;
    zoo_smb_qos_ctx_set_state(ctx, req_id, ZOO_SMB_MSG_ST_SENDING, dummy_msg);

    ZOO_SMB_QOS_STATE_STRUCT* state = zoo_smb_qos_ctx_get_state(ctx, req_id);
    TEST_ASSERT_NOT_NULL(state);
    TEST_ASSERT_EQUAL_UINT64(req_id, state->request_id);
    TEST_ASSERT_EQUAL(ZOO_SMB_MSG_ST_SENDING, state->status);
    TEST_ASSERT_EQUAL_PTR(dummy_msg, state->msg);

    zoo_smb_destroy_qos_ctx(ctx);
}

/**
 * @brief Test or example function test_set_msg_status_and_get_msg_status.
 */
void test_set_msg_status_and_get_msg_status(void) {
    ZOO_SMB_QOS_CTX_HANDLE ctx = zoo_smb_create_qos_ctx(NULL);
    TEST_ASSERT_NOT_NULL(ctx);

    uint64_t req_id = 100;
    zoo_smb_qos_ctx_set_state(ctx, req_id, ZOO_SMB_MSG_ST_SENDING, NULL);
    zoo_smb_qos_ctx_set_msg_status(ctx, req_id, ZOO_SMB_MSG_ST_ACKED);

    TEST_ASSERT_EQUAL(ZOO_SMB_MSG_ST_ACKED, zoo_smb_qos_ctx_get_msg_status(ctx, req_id));

    zoo_smb_destroy_qos_ctx(ctx);
}

/**
 * @brief Test or example function test_remove_state.
 */
void test_remove_state(void) {
    ZOO_SMB_QOS_CTX_HANDLE ctx = zoo_smb_create_qos_ctx(NULL);
    TEST_ASSERT_NOT_NULL(ctx);

    uint64_t req_id = 200;
    zoo_smb_qos_ctx_set_state(ctx, req_id, ZOO_SMB_MSG_ST_SENDING, NULL);
    TEST_ASSERT_NOT_NULL(zoo_smb_qos_ctx_get_state(ctx, req_id));

    zoo_smb_qos_ctx_remove_state(ctx, req_id);
    TEST_ASSERT_NULL(zoo_smb_qos_ctx_get_state(ctx, req_id));

    zoo_smb_destroy_qos_ctx(ctx);
}

/**
 * @brief Test or example function test_is_msg_status.
 */
void test_is_msg_status(void) {
    ZOO_SMB_QOS_CTX_HANDLE ctx = zoo_smb_create_qos_ctx(NULL);
    TEST_ASSERT_NOT_NULL(ctx);

    uint64_t req_id = 300;
    zoo_smb_qos_ctx_set_state(ctx, req_id, ZOO_SMB_MSG_ST_SENDING, NULL);

    TEST_ASSERT_TRUE(zoo_smb_qos_ctx_is_msg_status(ctx, req_id, ZOO_SMB_MSG_ST_SENDING));
    TEST_ASSERT_FALSE(zoo_smb_qos_ctx_is_msg_status(ctx, req_id, ZOO_SMB_MSG_ST_ACKED));

    zoo_smb_destroy_qos_ctx(ctx);
}

/**
 * @brief Test or example function test_destroy_null_ctx.
 */
void test_destroy_null_ctx(void) {
    zoo_smb_destroy_qos_ctx(NULL);
    // If we get here without crashing, the test passes
    TEST_ASSERT_TRUE(1);
}

/**
 * @brief Test or example function main.
 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_and_destroy_qos_ctx);
    RUN_TEST(test_get_qos_policy);
    RUN_TEST(test_set_and_get_state);
    RUN_TEST(test_set_msg_status_and_get_msg_status);
    RUN_TEST(test_remove_state);
    RUN_TEST(test_is_msg_status);
    RUN_TEST(test_destroy_null_ctx);
    
    return UNITY_END();
}
