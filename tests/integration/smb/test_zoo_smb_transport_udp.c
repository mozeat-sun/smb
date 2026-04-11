#include "unity.h"
#include "zoo_smb_transport_udp.h"
#include "zoo_smb_transport.h"
#include "zoo_smb_transport_observer.h"
#include "zoo_smb_config.h"
#include "zoo_log.h"
#include "zoo_smb_protocol.h"
#include "zoo_smb_message.h"
#include "../common/test_utils.h"
#include "zoo_thread_pool.h"
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
 * Traceability coverage:
 * - REQ-REL-001: UDP transport create/start/stop lifecycle validation.
 * - REQ-SAFE-003: requirement-linked transport integration evidence.
 */

// Test fixture data
static ZOO_SMB_TRANSPORT_CONFIG_STRUCT server_config;
static ZOO_SMB_TRANSPORT_CONFIG_STRUCT client_config;
static ZOO_SMB_TRANSPORT_HANDLE server_transport;
static ZOO_SMB_TRANSPORT_HANDLE client_transport;
static TRANSPORT_DATA_OBSERVER_HANDLE server_observer;
static TRANSPORT_DATA_OBSERVER_HANDLE client_observer;
static uint16_t udp_test_port_seed = 18000;

// Message tracking
static volatile ZOO_BOOL message_received = ZOO_FALSE;
static volatile int messages_count = 0;
static ZOO_SMB_MSG_STRUCT last_received_msg;

// Test message observer callback
static void message_observer(void* user_data, const ZOO_SMB_MSG_STRUCT* msg)
{
    (void)user_data;  // Suppress unused parameter warning
    message_received = ZOO_TRUE;
    messages_count++;
    if (msg) {
        last_received_msg = *msg;
    }
}

// Helper method to create and add observer
static TRANSPORT_DATA_OBSERVER_HANDLE create_and_add_observer(ZOO_SMB_TRANSPORT_HANDLE transport)
{
    TRANSPORT_DATA_OBSERVER_HANDLE observer = create_transport_data_observer(message_observer, NULL);
    if (observer && transport) {
        zoo_smb_transport_add_data_observer(transport, observer);
    }
    return observer;
}

void setUp(void)
{
    zoo_log_set_level(ZOO_LOG_LEVEL_TRACE);

    // Initialize UDP transport
    zoo_smb_transport_udp_init();

    // Setup server config
    memset(&server_config, 0, sizeof(server_config));
    server_config.type = ZOO_SMB_TRANSPORT_TYPE_UDP;
    server_config.is_server = ZOO_TRUE;
    strcpy(server_config.name, "udp_server");
    strcpy(server_config.address, "127.0.0.1");
    udp_test_port_seed++;
    server_config.port = udp_test_port_seed;
    server_config.timeout_ms = 5000;

    // Setup client config
    memset(&client_config, 0, sizeof(client_config));
    client_config.type = ZOO_SMB_TRANSPORT_TYPE_UDP;
    client_config.is_server = ZOO_FALSE;
    strcpy(client_config.name, "udp_client");
    strcpy(client_config.address, "127.0.0.1");
    client_config.port = udp_test_port_seed;
    client_config.timeout_ms = 5000;

    server_transport = NULL;
    client_transport = NULL;
    message_received = ZOO_FALSE;
    messages_count = 0;
    memset(&last_received_msg, 0, sizeof(last_received_msg));

    // Initialize observer handles
    server_observer = NULL;
    client_observer = NULL;
}

void tearDown(void)
{
    if (server_transport) {
        if (server_observer) {
            zoo_smb_transport_remove_data_observer(server_transport, server_observer);
            destroy_transport_data_observer(server_observer);
            server_observer = NULL;
        }
        zoo_smb_transport_stop(server_transport);
        zoo_smb_destroy_transport(server_transport);
        server_transport = NULL;
    }
    else if (server_observer) {
        destroy_transport_data_observer(server_observer);
        server_observer = NULL;
    }

    if (client_transport) {
        if (client_observer) {
            zoo_smb_transport_remove_data_observer(client_transport, client_observer);
            destroy_transport_data_observer(client_observer);
            client_observer = NULL;
        }
        zoo_smb_transport_stop(client_transport);
        zoo_smb_destroy_transport(client_transport);
        client_transport = NULL;
    }
    else if (client_observer) {
        destroy_transport_data_observer(client_observer);
        client_observer = NULL;
    }
}

void test_InitializationTest(void)
{
    // Test server transport creation
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Test client transport creation
    client_transport = zoo_smb_create_transport(&client_config);
    TEST_ASSERT_NOT_NULL(client_transport);

    // Test initial state
    TEST_ASSERT_FALSE(zoo_smb_transport_is_started(server_transport));
    TEST_ASSERT_FALSE(zoo_smb_transport_is_started(client_transport));
}

void test_InvalidParametersTest(void)
{
    // Test null config
    TEST_ASSERT_NULL(zoo_smb_create_transport(NULL));

    // Invalid address behavior is implementation-defined; verify it does not crash.
    ZOO_SMB_TRANSPORT_CONFIG_STRUCT invalid_config = server_config;
    strcpy(invalid_config.address, "invalid.ip.address");
    ZOO_SMB_TRANSPORT_HANDLE invalid_ip_transport = zoo_smb_create_transport(&invalid_config);
    if (invalid_ip_transport) {
        zoo_smb_destroy_transport(invalid_ip_transport);
    }

    // Test valid max port
    invalid_config = server_config;
    invalid_config.port = 65535;  // Valid max port
    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&invalid_config);
    if (transport) {
        zoo_smb_destroy_transport(transport);
    }
}

void test_ServerStartStopTest(void)
{
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Start server
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_start(server_transport, ZOO_TRUE));

    // Allow server to start
    usleep(1000000);  // 1 second
    TEST_ASSERT_TRUE(zoo_smb_transport_is_started(server_transport));

    // Stop server
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_stop(server_transport));
    TEST_ASSERT_FALSE(zoo_smb_transport_is_started(server_transport));
}

void test_ClientStartStopTest(void)
{
    client_transport = zoo_smb_create_transport(&client_config);
    TEST_ASSERT_NOT_NULL(client_transport);

    // Start client
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_start(client_transport, ZOO_TRUE));

    // Allow client to start
    usleep(100000);  // 100ms
    TEST_ASSERT_TRUE(zoo_smb_transport_is_started(client_transport));

    // Stop client
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_stop(client_transport));
    TEST_ASSERT_FALSE(zoo_smb_transport_is_started(client_transport));
}

void test_MessageSendingTest(void)
{
    // Create and start server
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Create and add observer to server
    server_observer = create_and_add_observer(server_transport);
    TEST_ASSERT_NOT_NULL(server_observer);

    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_start(server_transport, ZOO_TRUE));

    // Create and start client
    client_transport = zoo_smb_create_transport(&client_config);
    TEST_ASSERT_NOT_NULL(client_transport);

    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_start(client_transport, ZOO_TRUE));

    // Allow time for connection establishment
    usleep(200000);  // 200ms

    // Create test message
    ZOO_SMB_MSG_STRUCT test_msg = {0};
    test_msg.header.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    test_msg.header.version = ZOO_SMB_MSG_VERSION;
    test_msg.header.msg_type = ZOO_SMB_MSG_TYPE_PUB;
    test_msg.header.payload_size = 13;
    strcpy(test_msg.header.sender, "test_client");
    strcpy(test_msg.header.topic, "test_topic");
    test_msg.payload = (uint8_t*)"Hello World!";

    // Test sending message from client to server
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_send(client_transport, &test_msg, "udp_server"));

    // Wait for message to be received
    usleep(1000000);  // 1 second

    // Stop transports
    zoo_smb_transport_stop(server_transport);
    zoo_smb_transport_stop(client_transport);

    // Verify message was received
    TEST_ASSERT_TRUE(message_received);
    TEST_ASSERT_EQUAL(1, messages_count);
}

void test_ErrorConditionsTest(void)
{
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Test sending without starting
    ZOO_SMB_MSG_STRUCT test_msg = {0};
    test_msg.header.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    test_msg.header.version = ZOO_SMB_MSG_VERSION;
    test_msg.header.msg_type = ZOO_SMB_MSG_TYPE_PUB;
    test_msg.header.payload_size = 5;
    strcpy(test_msg.header.sender, "test");
    strcpy(test_msg.header.topic, "test");
    test_msg.payload = (uint8_t*)"Test";

    // Should fail because transport is not started
    TEST_ASSERT_NOT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_send(server_transport, &test_msg, "nonexistent"));

    // Test invalid message
    TEST_ASSERT_NOT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_send(server_transport, NULL, "target"));

    // Test invalid transport handle
    TEST_ASSERT_NOT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_send(NULL, &test_msg, "target"));

    // Test invalid receiver
    TEST_ASSERT_NOT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_send(server_transport, &test_msg, NULL));
}

void test_LargeMessageTest(void)
{
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Create oversized message
    ZOO_SMB_MSG_STRUCT large_msg = {0};
    large_msg.header.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    large_msg.header.version = ZOO_SMB_MSG_VERSION;
    large_msg.header.msg_type = ZOO_SMB_MSG_TYPE_PUB;
    large_msg.header.payload_size = 100000;  // Very large payload
    strcpy(large_msg.header.sender, "test");
    strcpy(large_msg.header.topic, "test");

    // Should fail due to size limit
    TEST_ASSERT_NOT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_send(server_transport, &large_msg, "target"));
}

void test_ObserverTest(void)
{
    // Test observer creation
    TRANSPORT_DATA_OBSERVER_HANDLE observer = create_transport_data_observer(message_observer, NULL);
    TEST_ASSERT_NOT_NULL(observer);

    // Test observer destruction
    destroy_transport_data_observer(observer);

    // Create transport for observer testing
    server_transport = zoo_smb_create_transport(&server_config);
    TEST_ASSERT_NOT_NULL(server_transport);

    // Test adding observer to transport
    observer = create_transport_data_observer(message_observer, NULL);
    TEST_ASSERT_NOT_NULL(observer);

    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_add_data_observer(server_transport, observer));

    // Test removing observer from transport (void return type)
    zoo_smb_transport_remove_data_observer(server_transport, observer);

    // Clean up
    destroy_transport_data_observer(observer);
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_InitializationTest);
    RUN_TEST(test_InvalidParametersTest);
    RUN_TEST(test_ServerStartStopTest);
    RUN_TEST(test_ClientStartStopTest);
    RUN_TEST(test_MessageSendingTest);
    RUN_TEST(test_ErrorConditionsTest);
    RUN_TEST(test_LargeMessageTest);
    RUN_TEST(test_ObserverTest);
    
    return UNITY_END();
}
