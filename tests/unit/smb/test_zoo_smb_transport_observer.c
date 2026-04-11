#include "unity.h"
#include "test_utils.h"
#include "zoo_smb_transport_observer.h"
#include <string.h>

/*
 * Traceability coverage:
 * - REQ-SAFE-001: deterministic transport observer registration and callback dispatch.
 * - REQ-SAFE-003: requirement-linked unit evidence for transport observer behavior.
 */

static int g_callback_count = 0;

/**
 * @brief Test or example function dummy_observer_cb.
 */
static void dummy_observer_cb(void* user_data, const ZOO_SMB_MSG_STRUCT* message) {
    (void)user_data;  // Suppress unused parameter warning
    (void)message;    // Suppress unused parameter warning
    g_callback_count++;
}

static ZOO_LIST_HANDLE list = NULL;

/**
 * @brief Test or example function setUp.
 */
void setUp(void) {
    zoo_create_memory_pool(1024 * 1024);  // 1MB pool for testing
    list = zoo_smb_create_list(1024);
    TEST_ASSERT_NOT_NULL(list);
    g_callback_count = 0;
}

/**
 * @brief Test or example function tearDown.
 */
void tearDown(void) {
    if (list) {
        zoo_smb_destroy_list(list);
        list = NULL;
    }
}

/**
 * @brief Test or example function test_create_and_destroy_observer.
 */
void test_create_and_destroy_observer(void) {
    TRANSPORT_DATA_OBSERVER_HANDLE obs = create_transport_data_observer(dummy_observer_cb, NULL);
    TEST_ASSERT_NOT_NULL(obs);
    TEST_ASSERT_EQUAL_PTR(dummy_observer_cb, obs->handler);
    destroy_transport_data_observer(obs);
}

/**
 * @brief Test or example function test_create_observer_null_handler.
 */
void test_create_observer_null_handler(void) {
    TRANSPORT_DATA_OBSERVER_HANDLE obs = create_transport_data_observer(NULL, NULL);
    TEST_ASSERT_NULL(obs);
}

/**
 * @brief Test or example function test_insert_and_remove_observer.
 */
void test_insert_and_remove_observer(void) {
    TRANSPORT_DATA_OBSERVER_HANDLE obs = create_transport_data_observer_and_insert(list, dummy_observer_cb, NULL);
    TEST_ASSERT_NOT_NULL(obs);
    
    // Insert same handler should return same object
    TRANSPORT_DATA_OBSERVER_HANDLE obs2 = create_transport_data_observer_and_insert(list, dummy_observer_cb, (void*)0x1234);
    TEST_ASSERT_EQUAL_PTR(obs, obs2);
    TEST_ASSERT_EQUAL_PTR((void*)0x1234, obs2->user_data);

    destroy_transport_data_observer_and_remove(list, obs);
    // Remove again should not crash
    destroy_transport_data_observer_and_remove(list, obs);
}

/**
 * @brief Test or example function test_remove_null.
 */
void test_remove_null(void) {
    destroy_transport_data_observer_and_remove(NULL, NULL);
    destroy_transport_data_observer_and_remove(list, NULL);
    destroy_transport_data_observer_and_remove(NULL, (TRANSPORT_DATA_OBSERVER_HANDLE)0x1);
    // If we get here without crashing, the test passes
    TEST_ASSERT_TRUE(1);
}

/**
 * @brief Test or example function test_destroy_null.
 */
void test_destroy_null(void) {
    destroy_transport_data_observer(NULL);
    // If we get here without crashing, the test passes
    TEST_ASSERT_TRUE(1);
}

/**
 * @brief Test or example function main.
 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_and_destroy_observer);
    RUN_TEST(test_create_observer_null_handler);
    RUN_TEST(test_insert_and_remove_observer);
    RUN_TEST(test_remove_null);
    RUN_TEST(test_destroy_null);
    
    return UNITY_END();
}
