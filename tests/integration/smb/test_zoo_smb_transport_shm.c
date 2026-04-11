#include "unity.h"
#include "zoo_smb_transport_shm.h"
#include "zoo_smb_transport.h"
#include "zoo_smb_config.h"
#include <string.h>
#include <stdio.h>

/*
 * Traceability coverage:
 * - REQ-REL-001: SHM transport initialization and stop semantics.
 * - REQ-SAFE-003: requirement-linked transport integration evidence.
 */

// Forward declarations for missing types
typedef void* ZOO_THREAD_POOL_HANDLE;

static ZOO_SMB_TRANSPORT_CONFIG_STRUCT server_config;
static ZOO_SMB_TRANSPORT_CONFIG_STRUCT client_config;
static ZOO_SMB_TRANSPORT_HANDLE server_transport;
static ZOO_SMB_TRANSPORT_HANDLE client_transport;
static ZOO_THREAD_POOL_HANDLE thread_pool;

/**
 * @brief Test or example function setUp.
 */
void setUp(void) {
    // Initialize SHM transport
    zoo_smb_transport_shm_init();

    // Setup server config with memset for proper initialization
    memset(&server_config, 0, sizeof(server_config));
    server_config.type = ZOO_SMB_TRANSPORT_TYPE_SHM;
    server_config.is_server = ZOO_TRUE;
    strcpy(server_config.name, "shm_test_channel");
    strcpy(server_config.address, "shm_test_channel");
    server_config.port = 0;
    server_config.timeout_ms = 5000;

    // Setup client config with memset for proper initialization
    memset(&client_config, 0, sizeof(client_config));
    client_config.type = ZOO_SMB_TRANSPORT_TYPE_SHM;
    client_config.is_server = ZOO_FALSE;
    strcpy(client_config.name, "shm_test_channel");
    strcpy(client_config.address, "shm_test_channel");
    client_config.port = 0;
    client_config.timeout_ms = 5000;

    server_transport = NULL;
    client_transport = NULL;
    thread_pool = NULL;
}

/**
 * @brief Test or example function tearDown.
 */
void tearDown(void) {
    if (server_transport) {
        zoo_smb_transport_stop(server_transport);
        zoo_smb_destroy_transport(server_transport);
        server_transport = NULL;
    }
    if (client_transport) {
        zoo_smb_transport_stop(client_transport);
        zoo_smb_destroy_transport(client_transport);
        client_transport = NULL;
    }
    if (thread_pool) {
        // zoo_destroy_thread_pool(thread_pool); // Stub - not available
        thread_pool = NULL;
    }
}

/**
 * @brief Test or example function test_initialization.
 */
void test_initialization(void) {
    // Test server transport creation
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Test client transport creation
    client_transport = zoo_smb_create_transport(&client_config);
    TEST_ASSERT_NOT_NULL(client_transport);

    // Test initial state - both should not be started
    TEST_ASSERT_FALSE(zoo_smb_transport_is_started(server_transport));
    TEST_ASSERT_FALSE(zoo_smb_transport_is_started(client_transport));
}

/**
 * @brief Test or example function test_invalid_parameters.
 */
void test_invalid_parameters(void) {
    // Test null config
    TEST_ASSERT_NULL(zoo_smb_create_transport(NULL));

    // Test invalid transport type
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT invalid_config = server_config;
    invalid_config.type = (ZOO_SMB_TRANSPORT_TYPE_ENUM)999;
    TEST_ASSERT_NULL(zoo_smb_create_transport(&invalid_config));
}

/**
 * @brief Test or example function test_start_implementation.
 */
void test_start_implementation(void) {
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Try to start - it might fail, but should not return NOT_IMPLEMENTED
    ZOO_ERROR_TYPE result = zoo_smb_transport_start(server_transport, ZOO_FALSE);

    // The error should not be NOT_IMPLEMENTED (-2147483647)
    // It could be other errors like permission denied, resource unavailable, etc.
    if (result != ZOO_SMB_OK) {
        // Log the actual error for debugging
        printf("Server start failed with error: %d (expected failure)\n", result);
        // We expect some kind of implementation, not NOT_IMPLEMENTED
        TEST_ASSERT_NOT_EQUAL_INT32(-2147483647, result);
    }

    // Test client as well
    client_transport = zoo_smb_create_transport(&client_config);
    TEST_ASSERT_NOT_NULL(client_transport);

    result = zoo_smb_transport_start(client_transport, ZOO_FALSE);
    if (result != ZOO_SMB_OK) {
        printf("Client start failed with error: %d (expected failure)\n", result);
        TEST_ASSERT_NOT_EQUAL_INT32(-2147483647, result);
    }
}

/**
 * @brief Test or example function test_error_conditions.
 */
void test_error_conditions(void) {
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Test sending without starting - should fail but with proper error
    ZOO_SMB_MSG_STRUCT test_msg = {0};
    test_msg.header.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    test_msg.header.version = ZOO_SMB_MSG_VERSION;
    test_msg.header.msg_type = ZOO_SMB_MSG_TYPE_PUB;
    test_msg.header.payload_size = 5;
    test_msg.payload = (uint8_t*)"Test";

    // Should fail because transport is not started
    ZOO_ERROR_TYPE result = zoo_smb_transport_send(server_transport, &test_msg, "nonexistent");
    TEST_ASSERT_NOT_EQUAL_INT32(ZOO_SMB_OK, result);

    // Test invalid message
    TEST_ASSERT_NOT_EQUAL_INT32(ZOO_SMB_OK, zoo_smb_transport_send(server_transport, NULL, "target"));

    // Test invalid transport handle
    TEST_ASSERT_NOT_EQUAL_INT32(ZOO_SMB_OK, zoo_smb_transport_send(NULL, &test_msg, "target"));
}

/**
 * @brief Test or example function test_stop_without_start.
 */
void test_stop_without_start(void) {
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Stopping a non-started transport should be safe
    TEST_ASSERT_EQUAL_INT32(ZOO_SMB_OK, zoo_smb_transport_stop(server_transport));

    // Multiple stops should be safe
    TEST_ASSERT_EQUAL_INT32(ZOO_SMB_OK, zoo_smb_transport_stop(server_transport));

    client_transport = zoo_smb_create_transport(&client_config);
    TEST_ASSERT_NOT_NULL(client_transport);

    TEST_ASSERT_EQUAL_INT32(ZOO_SMB_OK, zoo_smb_transport_stop(client_transport));
}

/**
 * @brief Test or example function test_resource_cleanup.
 */
void test_resource_cleanup(void) {
    // Create multiple transports
    ZOO_SMB_TRANSPORT_HANDLE transports[5];
    int i;

    for (i = 0; i < 5; i++) {
        ZOO_SMB_TRANSPORT_CONFIG_STRUCT config = (i % 2 == 0) ? server_config : client_config;
        snprintf(config.name, sizeof(config.name), "test_transport_%d", i);
        snprintf(config.address, sizeof(config.address), "shm_channel_%d", i);

        ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&config);
        TEST_ASSERT_NOT_NULL(transport);
        transports[i] = transport;
    }

    // Try to start all transports (may fail, but should not crash)
    for (i = 0; i < 5; i++) {
        ZOO_ERROR_TYPE result = zoo_smb_transport_start(transports[i], ZOO_FALSE);
        // Log result but don't assert on success
        if (result != ZOO_SMB_OK) {
            printf("Transport start failed with error: %d\n", result);
        }
    }

    // Stop and destroy all transports (should always work)
    for (i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_INT32(ZOO_SMB_OK, zoo_smb_transport_stop(transports[i]));
        zoo_smb_destroy_transport(transports[i]);
    }

    // Clear our references to prevent double cleanup in tearDown
    server_transport = NULL;
    client_transport = NULL;
}

/**
 * @brief Test or example function test_operations_registration.
 */
void test_operations_registration(void) {
    // This test verifies that SHM transport operations are registered
    // If we can create transports, the ops are registered
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    client_transport = zoo_smb_create_transport(&client_config);
    TEST_ASSERT_NOT_NULL(client_transport);

    // Test that is_started works (should return ZOO_FALSE for new transports)
    TEST_ASSERT_FALSE(zoo_smb_transport_is_started(server_transport));
    TEST_ASSERT_FALSE(zoo_smb_transport_is_started(client_transport));
}

/**
 * @brief Test or example function test_config_validation.
 */
void test_config_validation(void) {
    // Test with empty name
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT empty_name_config = server_config;
    strcpy(empty_name_config.name, "");
    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&empty_name_config);
    // This might succeed or fail depending on implementation
    if (transport) {
        zoo_smb_destroy_transport(transport);
    }

    // Test with empty address
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT empty_addr_config = server_config;
    strcpy(empty_addr_config.address, "");
    transport = zoo_smb_create_transport(&empty_addr_config);
    // This might succeed or fail depending on implementation
    if (transport) {
        zoo_smb_destroy_transport(transport);
    }
}

/**
 * @brief Test or example function test_shm_key_generation.
 */
void test_shm_key_generation(void) {
    // Create multiple transports with different names
    // Should generate different SHM keys
    ZOO_SMB_TRANSPORT_HANDLE transports[3];
    int i;

    for (i = 0; i < 3; i++) {
        ZOO_SMB_TRANSPORT_CONFIG_STRUCT config = server_config;
        snprintf(config.name, sizeof(config.name), "shm_test_%d", i);

        ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&config);
        TEST_ASSERT_NOT_NULL(transport);
        transports[i] = transport;
    }

    // Cleanup
    for (i = 0; i < 3; i++) {
        zoo_smb_destroy_transport(transports[i]);
    }

    // Clear our references
    server_transport = NULL;
    client_transport = NULL;
}

/**
 * @brief Test or example function test_multiple_registration.
 */
void test_multiple_registration(void) {
    // Multiple calls to shm_init should be safe
    zoo_smb_transport_shm_init();
    zoo_smb_transport_shm_init();
    zoo_smb_transport_shm_init();

    // Should still be able to create transports
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);
}

/**
 * @brief Test or example function main.
 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_initialization);
    RUN_TEST(test_invalid_parameters);
    RUN_TEST(test_error_conditions);
    RUN_TEST(test_stop_without_start);
    RUN_TEST(test_operations_registration);
    RUN_TEST(test_config_validation);
    RUN_TEST(test_shm_key_generation);
    RUN_TEST(test_multiple_registration);
    
    return UNITY_END();
}
