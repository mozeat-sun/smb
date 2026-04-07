#include "unity.h"
#include "test_utils.h"
#include "zoo_smb_rule_manager.h"
#include "zoo_smb_routing_rule.h"
#include "zoo_smb_node.h"
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
 * @brief Test or example function test_create_and_destroy_manager.
 */
void test_create_and_destroy_manager(void) {
    ZOO_SMB_RULE_MANAGER_HANDLE mgr = zoo_smb_create_rule_manager();
    TEST_ASSERT_NOT_NULL(mgr);
    zoo_smb_destroy_rule_manager(mgr);
}

/**
 * @brief Test or example function test_make_and_get_rule.
 */
void test_make_and_get_rule(void) {
    ZOO_SMB_RULE_MANAGER_HANDLE mgr = zoo_smb_create_rule_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    const char* rule_name = "ruleA";
    const char* service_name = "svcA";
    const char* topic = "topicA";
    ZOO_SMB_NODE_HANDLE node = zoo_smb_create_node(rule_name, service_name, topic, 
        ZOO_SMB_NODE_TYPE_SERVER, ZOO_SMB_TRANSPORT_TYPE_SHM);
    TEST_ASSERT_NOT_NULL(node);

    ZOO_SMB_SERVICE_HANDLE service = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        service_name,
        topic,
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
    strncpy(config.name, "ruleA_transport", sizeof(config.name) - 1);

    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&config);
    TEST_ASSERT_NOT_NULL(transport);

    ZOO_SMB_RULE_HANDLE rule = zoo_smb_rule_manager_make_rule(mgr, node, service, transport);
    TEST_ASSERT_NOT_NULL(rule);

    // Making same rule again should return same object
    ZOO_SMB_RULE_HANDLE rule2 = zoo_smb_rule_manager_make_rule(mgr, node, service, transport);
    TEST_ASSERT_EQUAL_PTR(rule, rule2);

    // Test get interface
    const char* rule_key = zoo_smb_make_name_3(node->target, node->topic, node->transport_type);
    ZOO_SMB_RULE_HANDLE got = zoo_smb_rule_manager_get_rule(mgr, rule_key);
    TEST_ASSERT_EQUAL_PTR(got, rule);

    zoo_smb_destroy_rule_manager(mgr);
    zoo_smb_destroy_node(node);
    zoo_smb_destroy_service(service);
    zoo_smb_destroy_transport(transport);
}

/**
 * @brief Test or example function test_make_rule_invalid_params.
 */
void test_make_rule_invalid_params(void) {
    ZOO_SMB_RULE_MANAGER_HANDLE mgr = zoo_smb_create_rule_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    // All parameters NULL should return NULL
    TEST_ASSERT_NULL(zoo_smb_rule_manager_make_rule(mgr, NULL, NULL, NULL));

    ZOO_SMB_NODE_HANDLE node = zoo_smb_create_node("n", "svc", "t", 
        ZOO_SMB_NODE_TYPE_SERVER, ZOO_SMB_TRANSPORT_TYPE_SHM);
    TEST_ASSERT_NOT_NULL(node);
    
    ZOO_SMB_SERVICE_HANDLE service = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        "svc",
        "t",
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

    // service/transport NULL cases
    TEST_ASSERT_NULL(zoo_smb_rule_manager_make_rule(mgr, node, NULL, NULL));
    TEST_ASSERT_NULL(zoo_smb_rule_manager_make_rule(mgr, node, service, NULL));

    zoo_smb_destroy_rule_manager(mgr);
    zoo_smb_destroy_node(node);
    zoo_smb_destroy_service(service);
    zoo_smb_destroy_transport(transport);
}

/**
 * @brief Test or example function test_add_and_remove_rule.
 */
void test_add_and_remove_rule(void) {
    ZOO_SMB_RULE_MANAGER_HANDLE mgr = zoo_smb_create_rule_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    ZOO_SMB_SERVICE_HANDLE service = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        "svcB",
        "topicB",
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
    strncpy(config.name, "ruleB_transport", sizeof(config.name) - 1);

    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&config);
    TEST_ASSERT_NOT_NULL(transport);

    ZOO_SMB_RULE_HANDLE rule = zoo_smb_create_routing_rule("ruleB", service, transport, observers);
    TEST_ASSERT_NOT_NULL(rule);

    TEST_ASSERT_EQUAL_INT(ZOO_SMB_OK, zoo_smb_rule_manager_add_rule(mgr, rule));
    // Adding again should return OK (already exists)
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_OK, zoo_smb_rule_manager_add_rule(mgr, rule));

    // Remove
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_OK, zoo_smb_rule_manager_remove_rule(mgr, rule));
    // Remove again should return NOT_FOUND
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_ERROR_NOT_FOUND, zoo_smb_rule_manager_remove_rule(mgr, rule));

    zoo_smb_destroy_rule_manager(mgr);
    zoo_smb_destroy_service(service);
    zoo_smb_destroy_transport(transport);
}

/**
 * @brief Test or example function test_find_rule.
 */
void test_find_rule(void) {
    ZOO_SMB_RULE_MANAGER_HANDLE mgr = zoo_smb_create_rule_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    ZOO_SMB_NODE_HANDLE node = zoo_smb_create_node("findme", "svc", "topic", 
        ZOO_SMB_NODE_TYPE_SERVER, ZOO_SMB_TRANSPORT_TYPE_SHM);
    TEST_ASSERT_NOT_NULL(node);
    
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
    strncpy(config.name, "findme_transport", sizeof(config.name) - 1);

    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&config);
    TEST_ASSERT_NOT_NULL(transport);

    ZOO_SMB_RULE_HANDLE rule = zoo_smb_rule_manager_make_rule(mgr, node, service, transport);
    TEST_ASSERT_NOT_NULL(rule);

    const char* rule_key = zoo_smb_make_name_3(node->target, node->topic, node->transport_type);
    ZOO_SMB_RULE_HANDLE found = zoo_smb_rule_manager_get_rule(mgr, rule_key);
    TEST_ASSERT_EQUAL_PTR(found, rule);

    // Not found case
    TEST_ASSERT_NULL(zoo_smb_rule_manager_get_rule(mgr, "notexist"));

    zoo_smb_destroy_rule_manager(mgr);
    zoo_smb_destroy_node(node);
    zoo_smb_destroy_service(service);
    zoo_smb_destroy_transport(transport);
}

/**
 * @brief Test or example function test_destroy_null_manager.
 */
void test_destroy_null_manager(void) {
    // Should not crash
    zoo_smb_destroy_rule_manager(NULL);
    // If we get here without crashing, test passes
    TEST_ASSERT_TRUE(1);
}

/**
 * @brief Test or example function test_add_remove_invalid_params.
 */
void test_add_remove_invalid_params(void) {
    ZOO_SMB_RULE_MANAGER_HANDLE mgr = zoo_smb_create_rule_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    TEST_ASSERT_EQUAL_INT(ZOO_SMB_ERROR_INVALID_PARAM, zoo_smb_rule_manager_add_rule(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_ERROR_INVALID_PARAM, zoo_smb_rule_manager_add_rule(mgr, NULL));
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_ERROR_INVALID_PARAM, zoo_smb_rule_manager_remove_rule(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_ERROR_INVALID_PARAM, zoo_smb_rule_manager_remove_rule(mgr, NULL));

    zoo_smb_destroy_rule_manager(mgr);
}

/**
 * @brief Test or example function main.
 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_and_destroy_manager);
    RUN_TEST(test_make_and_get_rule);
    RUN_TEST(test_make_rule_invalid_params);
    RUN_TEST(test_add_and_remove_rule);
    RUN_TEST(test_find_rule);
    RUN_TEST(test_destroy_null_manager);
    RUN_TEST(test_add_remove_invalid_params);
    
    return UNITY_END();
}
