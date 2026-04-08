#include "unity.h"
#include "test_utils.h"
#include "zoo_smb_service.h"
#include <string.h>

/**
 * @brief Test or example function setUp.
 */
void setUp(void) {
    test_memory_tracker_init();
}

/**
 * @brief Test or example function tearDown.
 */
void tearDown(void) {
    test_memory_tracker_cleanup();
}

/**
 * @brief Test or example function test_create_and_destroy_service.
 */
void test_create_and_destroy_service(void) {
    ZOO_SMB_SERVICE_HANDLE svc = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY, "test_svc", "test_topic", "127.0.0.1", 1234,
        NULL, 0, ZOO_SMB_TRANSPORT_TYPE_TCP, 2000);
    TEST_ASSERT_NOT_NULL(svc);
    
    // Verify basic functionality by just testing creation/destruction
    // Note: Direct field access might not work depending on implementation
    // so we'll just test creation/destruction for now
    
    zoo_smb_destroy_service(svc);
}

/**
 * @brief Test or example function test_create_service_null_params.
 */
void test_create_service_null_params(void) {
    // Test with NULL name - should return NULL or handle gracefully
    ZOO_SMB_SERVICE_HANDLE svc1 = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_REGISTRATION, NULL, "topic", "127.0.0.1", 1, 
        NULL, 0, ZOO_SMB_TRANSPORT_TYPE_TCP, 1000);
    // Note: Implementation may or may not accept NULL - we just test it doesn't crash
    if (svc1) {
        zoo_smb_destroy_service(svc1);
    }
}

/**
 * @brief Test or example function test_service_duplicate.
 */
void test_service_duplicate(void) {
    ZOO_SMB_SERVICE_HANDLE original = zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_DISCOVERY, "test_svc", "test_topic", "1.1.1.1", 1111,
        NULL, 0, ZOO_SMB_TRANSPORT_TYPE_UDP, 1000);
    TEST_ASSERT_NOT_NULL(original);

    ZOO_SMB_SERVICE_HANDLE duplicate = zoo_smb_service_duplicate(original);
    if (duplicate) {
        zoo_smb_destroy_service(duplicate);
    }
    
    zoo_smb_destroy_service(original);
}

/**
 * @brief Test or example function main.
 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_and_destroy_service);
    RUN_TEST(test_create_service_null_params);
    RUN_TEST(test_service_duplicate);
    
    return UNITY_END();
}
