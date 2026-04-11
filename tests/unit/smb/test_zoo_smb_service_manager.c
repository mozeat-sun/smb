#include "unity.h"
#include "test_utils.h"
#include "zoo_smb_service_manager.h"
#include "zoo_smb_service.h"
#include <string.h>

/*
 * Traceability coverage:
 * - REQ-REL-001: service manager create/register/unregister lifecycle validation.
 * - REQ-REL-004: service lookup and observer flow support availability-state handling.
 * - REQ-SAFE-003: requirement-linked unit evidence for service management.
 */

static ZOO_SMB_CONFIG_STRUCT config;

/**
 * @brief Test or example function setUp.
 */
void setUp(void) {
    test_memory_tracker_init();
    memset(&config, 0, sizeof(config));
}

/**
 * @brief Test or example function tearDown.
 */
void tearDown(void) {
    test_memory_tracker_cleanup();
}

/**
 * @brief Test or example function test_create_and_destroy_manager.
 */
void test_create_and_destroy_manager(void) {
    ZOO_SMB_SERVICE_MANAGER_HANDLE mgr = zoo_smb_create_service_manager(&config);
    TEST_ASSERT_NOT_NULL(mgr);
    zoo_smb_destroy_service_manager(mgr);
}

/**
 * @brief Test or example function test_register_and_unregister_service.
 */
void test_register_and_unregister_service(void) {
    ZOO_SMB_SERVICE_MANAGER_HANDLE mgr = zoo_smb_create_service_manager(&config);
    TEST_ASSERT_NOT_NULL(mgr);

    ZOO_SMB_SERVICE_HANDLE svc = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_REGISTRATION, "svc", "topic", "127.0.0.1", 1234, 
        NULL, 0, ZOO_SMB_TRANSPORT_TYPE_SHM, 1000);
    TEST_ASSERT_NOT_NULL(svc);

    zoo_smb_service_manager_register_service(mgr, ZOO_SMB_SERVICE_TYPE_REGISTRATION, svc);
    ZOO_SMB_SERVICE_HANDLE found = zoo_smb_service_manager_get_service(mgr, "svc", ZOO_SMB_SERVICE_TYPE_REGISTRATION);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("svc", found->name);

    zoo_smb_service_manager_unregister_service(mgr, ZOO_SMB_SERVICE_TYPE_REGISTRATION, svc);
    TEST_ASSERT_NULL(zoo_smb_service_manager_get_service(mgr, "svc", ZOO_SMB_SERVICE_TYPE_REGISTRATION));

    zoo_smb_destroy_service(svc);
    zoo_smb_destroy_service_manager(mgr);
}

/**
 * @brief Test or example function test_get_service_not_found.
 */
void test_get_service_not_found(void) {
    ZOO_SMB_SERVICE_MANAGER_HANDLE mgr = zoo_smb_create_service_manager(&config);
    TEST_ASSERT_NOT_NULL(mgr);
    TEST_ASSERT_NULL(zoo_smb_service_manager_get_service(mgr, "notexist", ZOO_SMB_SERVICE_TYPE_REGISTRATION));
    zoo_smb_destroy_service_manager(mgr);
}

/**
 * @brief Test or example function test_get_service_list.
 */
void test_get_service_list(void) {
    ZOO_SMB_SERVICE_MANAGER_HANDLE mgr = zoo_smb_create_service_manager(&config);
    TEST_ASSERT_NOT_NULL(mgr);
    ZOO_LIST_HANDLE list = zoo_smb_service_manager_get_service_list(mgr, ZOO_SMB_SERVICE_TYPE_REGISTRATION);
    TEST_ASSERT_NOT_NULL(list);
    zoo_smb_destroy_service_manager(mgr);
}

/**
 * @brief Test or example function test_register_and_unregister_observer.
 */
void test_register_and_unregister_observer(void) {
    ZOO_SMB_SERVICE_MANAGER_HANDLE mgr = zoo_smb_create_service_manager(&config);
    TEST_ASSERT_NOT_NULL(mgr);

    ZOO_SMB_SERVICE_OBSERVER_STRUCT observer = {0};
    observer.handler_cb = NULL;
    observer.user_data = NULL;

    zoo_smb_service_manager_register_observer(mgr, &observer);
    zoo_smb_service_manager_unregister_observer(mgr, &observer);

    zoo_smb_destroy_service_manager(mgr);
}

/**
 * @brief Test or example function test_get_service_wait_timeout.
 */
void test_get_service_wait_timeout(void) {
    ZOO_SMB_SERVICE_MANAGER_HANDLE mgr = zoo_smb_create_service_manager(&config);
    TEST_ASSERT_NOT_NULL(mgr);

    // Should timeout and return NULL
    ZOO_SMB_SERVICE_HANDLE svc = zoo_smb_service_manager_get_service_wait(mgr, "notexist", 200);
    TEST_ASSERT_NULL(svc);

    zoo_smb_destroy_service_manager(mgr);
}

/**
 * @brief Test or example function test_null_and_invalid_params.
 */
void test_null_and_invalid_params(void) {
    // Create with null config
    TEST_ASSERT_NULL(zoo_smb_create_service_manager(NULL));

    // Destroy null manager - should not crash
    zoo_smb_destroy_service_manager(NULL);

    // Register/unregister service with nulls - should not crash
    zoo_smb_service_manager_register_service(NULL, ZOO_SMB_SERVICE_TYPE_REGISTRATION, NULL);
    zoo_smb_service_manager_register_service(NULL, ZOO_SMB_SERVICE_TYPE_REGISTRATION, (ZOO_SMB_SERVICE_HANDLE)0x1);
    zoo_smb_service_manager_register_service((ZOO_SMB_SERVICE_MANAGER_HANDLE)0x1, ZOO_SMB_SERVICE_TYPE_MAX, NULL);

    zoo_smb_service_manager_unregister_service(NULL, ZOO_SMB_SERVICE_TYPE_REGISTRATION, NULL);
    zoo_smb_service_manager_unregister_service(NULL, ZOO_SMB_SERVICE_TYPE_REGISTRATION, (ZOO_SMB_SERVICE_HANDLE)0x1);
    zoo_smb_service_manager_unregister_service((ZOO_SMB_SERVICE_MANAGER_HANDLE)0x1, ZOO_SMB_SERVICE_TYPE_MAX, NULL);

    // Get service with nulls
    TEST_ASSERT_NULL(zoo_smb_service_manager_get_service(NULL, "svc", ZOO_SMB_SERVICE_TYPE_REGISTRATION));
    TEST_ASSERT_NULL(zoo_smb_service_manager_get_service((ZOO_SMB_SERVICE_MANAGER_HANDLE)0x1, NULL, ZOO_SMB_SERVICE_TYPE_REGISTRATION));

    // Get service list with null
    TEST_ASSERT_NULL(zoo_smb_service_manager_get_service_list(NULL, ZOO_SMB_SERVICE_TYPE_REGISTRATION));

    // Register/unregister observer with nulls - should not crash
    zoo_smb_service_manager_register_observer(NULL, NULL);
    zoo_smb_service_manager_unregister_observer(NULL, NULL);
    zoo_smb_service_manager_register_observer((ZOO_SMB_SERVICE_MANAGER_HANDLE)0x1, NULL);
    zoo_smb_service_manager_unregister_observer((ZOO_SMB_SERVICE_MANAGER_HANDLE)0x1, NULL);

    // If we get here without crashing, test passes
    TEST_ASSERT_TRUE(1);
}

/**
 * @brief Test or example function main.
 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_and_destroy_manager);
    RUN_TEST(test_register_and_unregister_service);
    RUN_TEST(test_get_service_not_found);
    RUN_TEST(test_get_service_list);
    RUN_TEST(test_register_and_unregister_observer);
    RUN_TEST(test_get_service_wait_timeout);
    RUN_TEST(test_null_and_invalid_params);
    
    return UNITY_END();
}
