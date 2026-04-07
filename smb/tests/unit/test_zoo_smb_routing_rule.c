#include "unity.h"
#include "test_utils.h"
#include "zoo_smb_routing_rule.h"
#include "zoo_smb_service.h"
#include "zoo_smb_transport.h"
#include <string.h>

static ZOO_LIST_HANDLE observers;

/**
 * @brief Test or example function setUp.
 */
void setUp(void) {
    test_memory_tracker_init();
    observers = zoo_smb_create_list(1024);
    TEST_ASSERT_NOT_NULL(observers);
}

/**
 * @brief Test or example function tearDown.
 */
void tearDown(void) {
    if (observers) {
        zoo_smb_destroy_list(observers);
        observers = NULL;
    }
    test_memory_tracker_cleanup();
}

/**
 * @brief Test or example function test_create_and_destroy_routing_rule.
 */
void test_create_and_destroy_routing_rule(void) {
    const char* name = "rule1";
    
    // Create service
    ZOO_SMB_SERVICE_HANDLE service = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        "svc",
        "topic",
        "127.0.0.1",
        12345,
        NULL, 0,
        ZOO_SMB_TRANSPORT_TYPE_SHM,
        1000
    );
    TEST_ASSERT_NOT_NULL(service);

    // Create transport
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT config = {0};
    config.type = ZOO_SMB_TRANSPORT_TYPE_SHM;
    strncpy(config.address, "127.0.0.1", sizeof(config.address) - 1);
    config.port = 12345;
    config.timeout_ms = 1000;
    config.is_server = ZOO_TRUE;
    strncpy(config.name, "rule1_transport", sizeof(config.name) - 1);

    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&config);
    TEST_ASSERT_NOT_NULL(transport);

    ZOO_SMB_RULE_HANDLE rule = zoo_smb_create_routing_rule(
        name, service, transport, observers);
    TEST_ASSERT_NOT_NULL(rule);

    // Check name
    const char* got_name = zoo_smb_get_routing_rule_name(rule);
    TEST_ASSERT_EQUAL_STRING(name, got_name);

    zoo_smb_destroy_routing_rule(rule);
    zoo_smb_destroy_service(service);
    zoo_smb_destroy_transport(transport);
}

/**
 * @brief Test or example function test_create_routing_rule_invalid_params.
 */
void test_create_routing_rule_invalid_params(void) {
    // Create service and transport for partial parameter testing
    ZOO_SMB_SERVICE_HANDLE service = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        "svc",
        "topic",
        "127.0.0.1",
        12345,
        NULL, 0,
        ZOO_SMB_TRANSPORT_TYPE_SHM,
        1000
    );
    TEST_ASSERT_NOT_NULL(service);

    ZOO_SMB_TRANSPORT_CONFIG_STRUCT config = {0};
    config.type = ZOO_SMB_TRANSPORT_TYPE_SHM;
    strncpy(config.address, "127.0.0.1", sizeof(config.address) - 1);
    config.port = 12345;
    config.timeout_ms = 1000;
    config.is_server = ZOO_TRUE;
    strncpy(config.name, "invalid_transport", sizeof(config.name) - 1);

    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&config);
    TEST_ASSERT_NOT_NULL(transport);

    // name is NULL
    ZOO_SMB_RULE_HANDLE rule1 = zoo_smb_create_routing_rule(
        NULL, service, transport, observers);
    TEST_ASSERT_NULL(rule1);
    
    // service is NULL
    ZOO_SMB_RULE_HANDLE rule2 = zoo_smb_create_routing_rule(
        "rule", NULL, transport, observers);
    TEST_ASSERT_NULL(rule2);
    
    // transport is NULL
    ZOO_SMB_RULE_HANDLE rule3 = zoo_smb_create_routing_rule(
        "rule", service, NULL, observers);
    TEST_ASSERT_NULL(rule3);

    zoo_smb_destroy_service(service);
    zoo_smb_destroy_transport(transport);
}

/**
 * @brief Test or example function test_get_routing_rule_name_null.
 */
void test_get_routing_rule_name_null(void) {
    const char* name = zoo_smb_get_routing_rule_name(NULL);
    TEST_ASSERT_NULL(name);
}

/**
 * @brief Test or example function test_destroy_null_rule.
 */
void test_destroy_null_rule(void) {
    // Should not crash
    zoo_smb_destroy_routing_rule(NULL);
    // If we get here without crashing, test passes
    TEST_ASSERT_TRUE(1);
}

/**
 * @brief Test or example function main.
 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_and_destroy_routing_rule);
    RUN_TEST(test_create_routing_rule_invalid_params);
    RUN_TEST(test_get_routing_rule_name_null);
    RUN_TEST(test_destroy_null_rule);
    
    return UNITY_END();
}
