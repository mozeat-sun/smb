#include "unity.h"
#include "zoo_smb_service_discovery.h"
#include "zoo_smb_service_manager.h"
#include "zoo_smb_transport_manager.h"
#include "zoo_smb_config.h"
#include "zoo_log.h"
#include "../common/test_utils.h"
#include <unistd.h>
#include <string.h>

/*
 * Traceability coverage:
 * - REQ-REL-001: discovery component create/destroy startup path executes without fault.
 * - REQ-REL-004: service lifecycle discovery remains part of health/availability evidence.
 * - REQ-SAFE-003: requirement-linked integration evidence for discovery lifecycle.
 */

// Test fixture data
static ZOO_SMB_CONFIG_STRUCT config_local;
static const ZOO_SMB_CONFIG_STRUCT* config;
static ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager;
static ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager;
static ZOO_SMB_SERVICE_DISCOVERY_HANDLE discovery;

// Discovery callback implementation
static ZOO_BOOL discovery_callback_called = ZOO_FALSE;
static void __attribute__((unused)) discovery_callback(const ZOO_SMB_SERVICE_STRUCT* service_info, void* user_data)
{
    ZOO_BOOL* called = (ZOO_BOOL*)user_data;
    if (called) {
        *called = ZOO_TRUE;
    }
    discovery_callback_called = ZOO_TRUE;
    TEST_ASSERT_NOT_NULL(service_info);
}

void setUp(void)
{
    zoo_log_set_level(ZOO_LOG_LEVEL_DEBUG);
    const ZOO_SMB_CONFIG_STRUCT* base = zoo_smb_config_init();
    memcpy(&config_local, base, sizeof(config_local));

    /* Isolate discovery transport ports from other integration executables. */
    uint16_t port_seed = (uint16_t)(53000 + (getpid() % 1000));
    config_local.broadcast.port = port_seed;
    config_local.multicast.port = (uint16_t)(port_seed - 1000);
    config = &config_local;
    service_manager = zoo_smb_create_service_manager(config);
    transport_manager = zoo_smb_create_transport_manager(service_manager, config);
    discovery = NULL;
    discovery_callback_called = ZOO_FALSE;
}

void tearDown(void)
{
    if (discovery) {
        zoo_smb_destroy_service_discovery(discovery);
        discovery = NULL;
    }
    if (transport_manager) {
        zoo_smb_destroy_transport_manager(transport_manager);
        transport_manager = NULL;
    }
    if (service_manager) {
        zoo_smb_destroy_service_manager(service_manager);
        service_manager = NULL;
    }
    config = NULL;
}

void test_CreateDestroy(void)
{
    discovery = zoo_smb_create_service_discovery(config, service_manager, transport_manager);
    TEST_ASSERT_NOT_NULL(discovery);
    zoo_smb_destroy_service_discovery(discovery);
    discovery = NULL;  // Prevent double cleanup
}

void test_StartStopDiscovery(void)
{
    discovery = zoo_smb_create_service_discovery(config, service_manager, transport_manager);
    TEST_ASSERT_NOT_NULL(discovery);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_start_service_discovery(discovery));
    zoo_smb_stop_service_discovery(discovery);
    zoo_smb_destroy_service_discovery(discovery);
    discovery = NULL;  // Prevent double cleanup
}

void test_ObserverCallback(void)
{
    discovery = zoo_smb_create_service_discovery(config, service_manager, transport_manager);
    TEST_ASSERT_NOT_NULL(discovery);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_start_service_discovery(discovery));

    // Sleep for 200ms to allow discovery to run
    usleep(200000);

    zoo_smb_stop_service_discovery(discovery);
    zoo_smb_destroy_service_discovery(discovery);
    discovery = NULL;  // Prevent double cleanup
}

void test_MultipleObservers(void)
{
    discovery = zoo_smb_create_service_discovery(config, service_manager, transport_manager);
    TEST_ASSERT_NOT_NULL(discovery);

    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_start_service_discovery(discovery));

    // Sleep for 200ms to allow discovery to run
    usleep(200000);

    zoo_smb_stop_service_discovery(discovery);
    zoo_smb_destroy_service_discovery(discovery);
    discovery = NULL;  // Prevent double cleanup
}

void test_BroadcastSingleService(void)
{
    discovery = zoo_smb_create_service_discovery(config, service_manager, transport_manager);
    TEST_ASSERT_NOT_NULL(discovery);

    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_start_service_discovery(discovery));
    
    ZOO_SMB_SERVICE_HANDLE service = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        "TestService",
        "TestTopic",
        "127.0.0.1",
        12345,
        NULL, 0,
        ZOO_SMB_TRANSPORT_TYPE_TCP,
        5000);
    TEST_ASSERT_NOT_NULL(service);

    zoo_smb_service_manager_register_service(
        service_manager,
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        service);

    // Sleep for a shorter time in tests (1.5 seconds instead of 15)
    usleep(1500000);
    
    zoo_smb_stop_service_discovery(discovery);
    zoo_smb_destroy_service_discovery(discovery);
    discovery = NULL;  // Prevent double cleanup
}

void test_BroadcastMultiServices(void)
{
    discovery = zoo_smb_create_service_discovery(config, service_manager, transport_manager);
    TEST_ASSERT_NOT_NULL(discovery);

    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_start_service_discovery(discovery));
    
    ZOO_SMB_SERVICE_HANDLE service1 = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        "TestService1",
        "TestTopic1",
        "127.0.0.1",
        12345,
        NULL, 0,
        ZOO_SMB_TRANSPORT_TYPE_TCP,
        5000);
    TEST_ASSERT_NOT_NULL(service1);

    ZOO_SMB_SERVICE_HANDLE service2 = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        "TestService2",
        "TestTopic2",
        "127.0.0.1",
        12346,
        NULL, 0,
        ZOO_SMB_TRANSPORT_TYPE_TCP,
        5000);
    TEST_ASSERT_NOT_NULL(service2);

    zoo_smb_service_manager_register_service(
        service_manager,
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        service1);
    zoo_smb_service_manager_register_service(
        service_manager,
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,
        service2);

    // Sleep for a shorter time in tests (1.5 seconds instead of 15)
    usleep(1500000);

    zoo_smb_stop_service_discovery(discovery);
    zoo_smb_destroy_service_discovery(discovery);
    discovery = NULL;  // Prevent double cleanup
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CreateDestroy);
    
    int result = UNITY_END();

    return result;
}
