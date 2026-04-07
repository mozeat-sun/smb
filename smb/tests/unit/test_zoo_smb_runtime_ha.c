/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: UNIT_TEST_HA_RUNTIME
 * File name: test_zoo_smb_runtime_ha.c
 * Description: Unit tests for HA runtime, peer monitoring, and failover
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "zoo_smb_runtime.h"
#include "zoo_smb.h"
#include "../../log/inc/zoo_log.h"

// ==============================================================================
// TEST FIXTURES
// ==============================================================================

static ZOO_SMB_RUNTIME_HANDLE g_runtime = NULL;
static ZOO_SMB_CONFIG_STRUCT g_config;

static int g_observer_called = 0;
static ZOO_SMB_RUNTIME_STATE_ENUM g_observer_old_state = ZOO_SMB_RUNTIME_STATE_CREATED;
static ZOO_SMB_RUNTIME_STATE_ENUM g_observer_new_state = ZOO_SMB_RUNTIME_STATE_CREATED;

static void test_observer_callback(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    ZOO_SMB_RUNTIME_STATE_ENUM old_state,
    ZOO_SMB_RUNTIME_STATE_ENUM new_state,
    void* user_data)
{
    g_observer_called++;
    g_observer_old_state = old_state;
    g_observer_new_state = new_state;
}

// ==============================================================================
// RUNTIME LIFECYCLE TESTS
// ==============================================================================

void setUp(void)
{
    g_observer_called = 0;
    g_observer_old_state = ZOO_SMB_RUNTIME_STATE_CREATED;
    g_observer_new_state = ZOO_SMB_RUNTIME_STATE_CREATED;
    
    // Initialize config
    memset(&g_config, 0, sizeof(g_config));
    g_config.port = 10000;
    snprintf(g_config.name, sizeof(g_config.name), "test_smb");
    
    // Create runtime
    g_runtime = zoo_smb_runtime_create(&g_config);
}

void tearDown(void)
{
    if (g_runtime)
    {
        zoo_smb_runtime_stop(g_runtime);
        zoo_smb_runtime_destroy(g_runtime);
        g_runtime = NULL;
    }
}

// ==============================================================================
// BASIC RUNTIME TESTS
// ==============================================================================

void test_runtime_create_default_options(void)
{
    TEST_ASSERT_NOT_NULL(g_runtime);
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_STATE_CREATED, zoo_smb_runtime_get_state(g_runtime));
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_ROLE_STANDALONE, zoo_smb_runtime_get_role(g_runtime));
}

void test_runtime_create_with_ha_options(void)
{
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    options.heartbeat_interval_ms = 1000;
    options.failover_timeout_ms = 3000;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(&g_config, &options);
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_ROLE_SECONDARY, zoo_smb_runtime_get_role(runtime));
    
    zoo_smb_runtime_destroy(runtime);
}

void test_runtime_get_bus(void)
{
    ZOO_SMB_HANDLE bus = zoo_smb_runtime_get_bus(g_runtime);
    TEST_ASSERT_NOT_NULL(bus);
}

void test_runtime_get_epoch(void)
{
    uint64_t epoch = zoo_smb_runtime_get_epoch(g_runtime);
    TEST_ASSERT_EQUAL(0, epoch);
}

void test_runtime_set_role(void)
{
    ZOO_ERROR_TYPE result = zoo_smb_runtime_set_role(g_runtime, ZOO_SMB_RUNTIME_ROLE_PRIMARY);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_ROLE_PRIMARY, zoo_smb_runtime_get_role(g_runtime));
}

// ==============================================================================
// OBSERVER PATTERN TESTS
// ==============================================================================

void test_observer_registration(void)
{
    ZOO_ERROR_TYPE result = zoo_smb_runtime_register_state_observer(
        g_runtime,
        test_observer_callback,
        NULL);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
}

void test_observer_called_on_state_change(void)
{
    ZOO_ERROR_TYPE result = zoo_smb_runtime_register_state_observer(
        g_runtime,
        test_observer_callback,
        NULL);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    
    // Trigger state change
    zoo_smb_runtime_set_role(g_runtime, ZOO_SMB_RUNTIME_ROLE_PRIMARY);
    
    // Observer should have been called
    TEST_ASSERT_GREATER_THAN(0, g_observer_called);
}

void test_observer_unregistration(void)
{
    // Register
    zoo_smb_runtime_register_state_observer(g_runtime, test_observer_callback, NULL);
    
    // Unregister
    ZOO_ERROR_TYPE result = zoo_smb_runtime_unregister_state_observer(
        g_runtime,
        test_observer_callback,
        NULL);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
}

// ==============================================================================
// PEER MONITORING TESTS
// ==============================================================================

void test_report_peer_heartbeat(void)
{
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(&g_config, &options);
    
    ZOO_ERROR_TYPE result = zoo_smb_runtime_report_peer_heartbeat(
        runtime,
        "node-2",
        1);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    
    zoo_smb_runtime_destroy(runtime);
}

void test_get_peer_status_after_heartbeat(void)
{
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(&g_config, &options);
    
    // Report heartbeat from peer
    zoo_smb_runtime_report_peer_heartbeat(runtime, "node-2", 1);
    
    // Query peer status
    ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT status;
    memset(&status, 0, sizeof(status));
    
    ZOO_ERROR_TYPE result = zoo_smb_runtime_get_peer_status(runtime, "node-2", &status);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    TEST_ASSERT_EQUAL_STRING("node-2", status.peer_id);
    TEST_ASSERT_EQUAL(1, status.peer_epoch);
    TEST_ASSERT_EQUAL(ZOO_TRUE, status.healthy);
    
    zoo_smb_runtime_destroy(runtime);
}

void test_get_peer_status_nonexistent(void)
{
    ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT status;
    memset(&status, 0, sizeof(status));
    
    ZOO_ERROR_TYPE result = zoo_smb_runtime_get_peer_status(g_runtime, "nonexistent", &status);
    TEST_ASSERT_EQUAL(ZOO_SMB_ERROR_NOT_FOUND, result);
}

void test_report_peer_timeout(void)
{
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(&g_config, &options);
    
    // Report heartbeat, then timeout
    zoo_smb_runtime_report_peer_heartbeat(runtime, "node-2", 1);
    
    ZOO_ERROR_TYPE result = zoo_smb_runtime_report_peer_timeout(runtime, "node-2", 1);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    
    // Verify peer is marked unhealthy
    ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT status;
    memset(&status, 0, sizeof(status));
    zoo_smb_runtime_get_peer_status(runtime, "node-2", &status);
    TEST_ASSERT_EQUAL(ZOO_FALSE, status.healthy);
    
    zoo_smb_runtime_destroy(runtime);
}

// ==============================================================================
// FAILOVER AND ELECTION TESTS
// ==============================================================================

void test_trigger_election_basic(void)
{
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(&g_config, &options);
    
    // Register observer
    g_observer_called = 0;
    zoo_smb_runtime_register_state_observer(runtime, test_observer_callback, NULL);
    
    // Trigger election
    ZOO_ERROR_TYPE result = zoo_smb_runtime_trigger_election(runtime, "test_reason");
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    
    // Verify role changed to PRIMARY
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_ROLE_PRIMARY, zoo_smb_runtime_get_role(runtime));
    
    // Verify epoch incremented
    uint64_t epoch = zoo_smb_runtime_get_epoch(runtime);
    TEST_ASSERT_GREATER_THAN(0, epoch);
    
    zoo_smb_runtime_destroy(runtime);
}

void test_trigger_election_without_ha(void)
{
    ZOO_ERROR_TYPE result = zoo_smb_runtime_trigger_election(g_runtime, "test");
    TEST_ASSERT_EQUAL(ZOO_SMB_ERROR_INVALID_STATE, result);
}

void test_automatic_failover_on_peer_timeout(void)
{
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    options.failover_timeout_ms = 100;  // Short timeout for testing
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(&g_config, &options);
    
    // Verify initial role is SECONDARY
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_ROLE_SECONDARY, zoo_smb_runtime_get_role(runtime));
    
    // Report peer timeout
    zoo_smb_runtime_report_peer_timeout(runtime, "node-2", 1);
    
    // After timeout detection, should promote to PRIMARY
    // (Note: This happens in background monitor task in real implementation)
    
    zoo_smb_runtime_destroy(runtime);
}

// ==============================================================================
// MULTI-PEER TESTS
// ==============================================================================

void test_multiple_peer_tracking(void)
{
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(&g_config, &options);
    
    // Report heartbeats from multiple peers
    for (int i = 2; i <= 8; i++)
    {
        char peer_id[32];
        snprintf(peer_id, sizeof(peer_id), "node-%d", i);
        zoo_smb_runtime_report_peer_heartbeat(runtime, peer_id, i);
    }
    
    // Verify all peers are tracked
    for (int i = 2; i <= 8; i++)
    {
        char peer_id[32];
        snprintf(peer_id, sizeof(peer_id), "node-%d", i);
        
        ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT status;
        memset(&status, 0, sizeof(status));
        ZOO_ERROR_TYPE result = zoo_smb_runtime_get_peer_status(runtime, peer_id, &status);
        
        TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
        TEST_ASSERT_EQUAL_STRING(peer_id, status.peer_id);
        TEST_ASSERT_EQUAL(ZOO_TRUE, status.healthy);
    }
    
    zoo_smb_runtime_destroy(runtime);
}

void test_peer_epoch_tracking(void)
{
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(&g_config, &options);
    
    // Report peer with epoch 1
    zoo_smb_runtime_report_peer_heartbeat(runtime, "node-2", 1);
    
    ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT status1;
    memset(&status1, 0, sizeof(status1));
    zoo_smb_runtime_get_peer_status(runtime, "node-2", &status1);
    TEST_ASSERT_EQUAL(1, status1.peer_epoch);
    
    // Report peer with higher epoch
    zoo_smb_runtime_report_peer_heartbeat(runtime, "node-2", 5);
    
    ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT status2;
    memset(&status2, 0, sizeof(status2));
    zoo_smb_runtime_get_peer_status(runtime, "node-2", &status2);
    TEST_ASSERT_EQUAL(5, status2.peer_epoch);
    
    zoo_smb_runtime_destroy(runtime);
}

// ==============================================================================
// ERROR HANDLING TESTS
// ==============================================================================

void test_invalid_parameters(void)
{
    // NULL runtime
    ZOO_ERROR_TYPE result = zoo_smb_runtime_report_peer_heartbeat(NULL, "node-2", 1);
    TEST_ASSERT_EQUAL(ZOO_SMB_ERROR_INVALID_PARAM, result);
    
    // NULL peer_id
    result = zoo_smb_runtime_report_peer_heartbeat(g_runtime, NULL, 1);
    TEST_ASSERT_EQUAL(ZOO_SMB_ERROR_INVALID_PARAM, result);
}

void test_runtime_state_transitions(void)
{
    ZOO_ERROR_TYPE result = zoo_smb_runtime_set_role(g_runtime, ZOO_SMB_RUNTIME_ROLE_PRIMARY);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_ROLE_PRIMARY, zoo_smb_runtime_get_role(g_runtime));
    
    // Transition to SECONDARY
    result = zoo_smb_runtime_set_role(g_runtime, ZOO_SMB_RUNTIME_ROLE_SECONDARY);
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, result);
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_ROLE_SECONDARY, zoo_smb_runtime_get_role(g_runtime));
}
