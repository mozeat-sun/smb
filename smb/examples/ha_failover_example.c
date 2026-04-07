/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: EXAMPLE_HA_FAILOVER
 * File name: example_ha_failover.c
 * Description: Complete example demonstrating HA failover and peer monitoring
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "zoo_smb.h"
#include "zoo_smb_runtime.h"
#include "zoo_log.h"

// ==============================================================================
// EXAMPLE CONSTANTS
// ==============================================================================

#define EXAMPLE_PEER_COUNT 3
#define EXAMPLE_HEARTBEAT_INTERVAL_MS 2000
#define EXAMPLE_FAILOVER_TIMEOUT_MS 5000
#define EXAMPLE_RUNTIME_DURATION_SEC 30

// ==============================================================================
// GLOBAL STATE
// ==============================================================================

static volatile int g_running = 1;
static ZOO_SMB_RUNTIME_HANDLE g_runtime = NULL;
static int g_state_change_count = 0;
static int g_peer_status_checks = 0;

// ==============================================================================
// OBSERVER CALLBACKS
// ==============================================================================

/*
 * Observe and print runtime state transitions.
 *
 * The example uses this callback to make HA role and lifecycle changes visible
 * in stdout. It stays intentionally lightweight because observer callbacks may
 * run in timing-sensitive runtime paths.
 */
void runtime_state_observer(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    ZOO_SMB_RUNTIME_STATE_ENUM old_state,
    ZOO_SMB_RUNTIME_STATE_ENUM new_state,
    void* user_data)
{
    // Keep this callback lightweight; examples only log role/state transitions.
    g_state_change_count++;
    
    const char* state_names[] = {
        "CREATED", "STARTING", "RUNNING", "DEGRADED", "STOPPING", "STOPPED", "FAULTED"
    };
    
    printf("[STATE CHANGE] %s -> %s (count=%d)\n",
           state_names[old_state],
           state_names[new_state],
           g_state_change_count);
}

/*
 * Convert process signals into a cooperative stop request.
 *
 * The long-running cluster simulation polls this flag so Ctrl+C or termination
 * signals can stop the demo cleanly without abruptly interrupting cleanup.
 */
void signal_handler(int sig)
{
    printf("\n[SIGNAL] Received signal %d, shutting down...\n", sig);
    g_running = 0;
}

// ==============================================================================
// EXAMPLE: BASIC HA SETUP
// ==============================================================================

/*
 * Create a runtime with HA enabled and print the effective setup.
 *
 * This is the simplest HA example. It demonstrates the runtime creation API,
 * the key HA-related options, and observer registration without introducing any
 * peer interactions or election traffic.
 */
void example_basic_ha_setup(void)
{
    printf("\n=== Example 1: Basic HA Setup ===\n");

    const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_init();
    
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    options.heartbeat_interval_ms = EXAMPLE_HEARTBEAT_INTERVAL_MS;
    options.failover_timeout_ms = EXAMPLE_FAILOVER_TIMEOUT_MS;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "local-node");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(config, &options);
    if (!runtime)
    {
        printf("[ERROR] Failed to create runtime\n");
        return;
    }
    
    printf("[SETUP] Runtime created\n");
    printf("  - Peer ID: %s\n", options.local_peer_id);
    printf("  - Heartbeat Interval: %d ms\n", options.heartbeat_interval_ms);
    printf("  - Failover Timeout: %d ms\n", options.failover_timeout_ms);
    printf("  - Initial Role: %s\n",
           zoo_smb_runtime_get_role(runtime) == ZOO_SMB_RUNTIME_ROLE_PRIMARY ? "PRIMARY" : "SECONDARY");
    
    // Register observer before teardown so transitions are visible in output.
    zoo_smb_runtime_register_state_observer(runtime, runtime_state_observer, NULL);
    printf("[SETUP] State observer registered\n");
    
    zoo_smb_runtime_destroy(runtime);
}

// ==============================================================================
// EXAMPLE: PEER MONITORING
// ==============================================================================

/*
 * Demonstrate peer status tracking with synthetic heartbeat input.
 *
 * The example reports heartbeats for a small set of peers, queries their health
 * state, and then injects a timeout for one peer. This shows how monitoring and
 * peer inspection APIs can be used by control-plane logic.
 */
void example_peer_monitoring(void)
{
    printf("\n=== Example 2: Peer Monitoring ===\n");

    const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_init();
    
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(config, &options);
    if (!runtime)
    {
        printf("[ERROR] Failed to create runtime\n");
        return;
    }
    
    printf("[MONITOR] Simulating peer heartbeats...\n");
    
    // Seed runtime with synthetic heartbeats from peer nodes.
    for (int i = 2; i <= EXAMPLE_PEER_COUNT; i++)
    {
        char peer_id[32];
        snprintf(peer_id, sizeof(peer_id), "node-%d", i);
        
        ZOO_ERROR_TYPE result = zoo_smb_runtime_report_peer_heartbeat(runtime, peer_id, i);
        if (result == ZOO_SMB_OK)
        {
            printf("[HEARTBEAT] Peer %s registered (epoch=%d)\n", peer_id, i);
        }
    }
    
    // Query peer status snapshot after heartbeat ingestion.
    printf("\n[STATUS] Current peer status:\n");
    for (int i = 2; i <= EXAMPLE_PEER_COUNT; i++)
    {
        char peer_id[32];
        snprintf(peer_id, sizeof(peer_id), "node-%d", i);
        
        ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT status;
        memset(&status, 0, sizeof(status));
        
        ZOO_ERROR_TYPE result = zoo_smb_runtime_get_peer_status(runtime, peer_id, &status);
        if (result == ZOO_SMB_OK)
        {
            printf("  - %s: epoch=%llu, healthy=%s\n",
                   status.peer_id,
                   (unsigned long long)status.peer_epoch,
                   status.healthy ? "YES" : "NO");
        }
    }
    
    // Mark one peer as timed out to demonstrate failover input path.
    printf("\n[TIMEOUT] Simulating timeout for node-2...\n");
    zoo_smb_runtime_report_peer_timeout(runtime, "node-2", 2);
    
    // Query status again
    printf("[STATUS] After timeout:\n");
    ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT status;
    memset(&status, 0, sizeof(status));
    zoo_smb_runtime_get_peer_status(runtime, "node-2", &status);
    printf("  - node-2: healthy=%s\n", status.healthy ? "YES" : "NO");
    
    zoo_smb_runtime_destroy(runtime);
}

// ==============================================================================
// EXAMPLE: MANUAL ELECTION
// ==============================================================================

/*
 * Trigger a manual election and display the resulting state changes.
 *
 * This scenario is meant to illustrate the operator-driven failover path. It
 * prints the role and epoch before and after the election request so callers can
 * see how manual promotion or re-election is surfaced by the runtime APIs.
 */
void example_manual_election(void)
{
    printf("\n=== Example 3: Manual Election ===\n");

    const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_init();
    
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create_ex(config, &options);
    if (!runtime)
    {
        printf("[ERROR] Failed to create runtime\n");
        return;
    }
    
    // Register observer
    g_state_change_count = 0;
    zoo_smb_runtime_register_state_observer(runtime, runtime_state_observer, NULL);
    
    printf("[ELECTION] Current role: %s\n",
           zoo_smb_runtime_get_role(runtime) == ZOO_SMB_RUNTIME_ROLE_PRIMARY ? "PRIMARY" : "SECONDARY");
    printf("[ELECTION] Current epoch: %llu\n", (unsigned long long)zoo_smb_runtime_get_epoch(runtime));
    
    // Manual election demonstrates operator-driven epoch advancement.
    printf("[ELECTION] Triggering manual election...\n");
    ZOO_ERROR_TYPE result = zoo_smb_runtime_trigger_election(runtime, "operator_request");
    if (result == ZOO_SMB_OK)
    {
        printf("[ELECTION] Election triggered successfully\n");
        printf("[ELECTION] New role: %s\n",
               zoo_smb_runtime_get_role(runtime) == ZOO_SMB_RUNTIME_ROLE_PRIMARY ? "PRIMARY" : "SECONDARY");
        printf("[ELECTION] New epoch: %llu\n", (unsigned long long)zoo_smb_runtime_get_epoch(runtime));
        printf("[ELECTION] State changes observed: %d\n", g_state_change_count);
    }
    else
    {
        printf("[ERROR] Election failed with code %d\n", result);
    }
    
    zoo_smb_runtime_destroy(runtime);
}

// ==============================================================================
// EXAMPLE: LONG-RUNNING HA CLUSTER SIMULATION
// ==============================================================================

/*
 * Simulate a small HA cluster over a fixed runtime window.
 *
 * The loop periodically emits peer heartbeats, prints cluster health snapshots,
 * and temporarily suppresses one peer to emulate an outage. It is intended to
 * show how HA monitoring behaves over time rather than in a single API call.
 */
void example_ha_cluster_simulation(void)
{
    printf("\n=== Example 4: HA Cluster Simulation ===\n");

    const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_init();
    
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.enable_ha = ZOO_TRUE;
    options.heartbeat_interval_ms = 1000;
    options.failover_timeout_ms = 3000;
    snprintf(options.local_peer_id, sizeof(options.local_peer_id), "node-1");
    
    g_runtime = zoo_smb_runtime_create_ex(config, &options);
    if (!g_runtime)
    {
        printf("[ERROR] Failed to create runtime\n");
        return;
    }
    
    // Register observer
    zoo_smb_runtime_register_state_observer(g_runtime, runtime_state_observer, NULL);
    
    printf("[CLUSTER] Simulation starting for %d seconds\n", EXAMPLE_RUNTIME_DURATION_SEC);
    printf("[CLUSTER] Peer count: %d\n", EXAMPLE_PEER_COUNT);
    
    // Allow Ctrl+C to stop the long-running simulation loop.
    signal(SIGINT, signal_handler);
    
    time_t start_time = time(NULL);
    int heartbeat_count = 0;
    int status_check_count = 0;
    
    while (g_running)
    {
        time_t current_time = time(NULL);
        int elapsed_sec = current_time - start_time;
        
        if (elapsed_sec >= EXAMPLE_RUNTIME_DURATION_SEC)
        {
            printf("[CLUSTER] Duration reached, stopping...\n");
            break;
        }
        
        // Every second, simulate peer heartbeats.
        if ((heartbeat_count % 10) == 0)
        {
            for (int i = 2; i <= EXAMPLE_PEER_COUNT; i++)
            {
                char peer_id[32];
                snprintf(peer_id, sizeof(peer_id), "node-%d", i);
                
                // Simulate occasional peer failures
                if (i == 2 && elapsed_sec > 10 && elapsed_sec < 15)
                {
                    // node-2 is down between 10-15 seconds
                    printf("[CLUSTER] node-2 is offline (simulated)\n");
                }
                else
                {
                    zoo_smb_runtime_report_peer_heartbeat(g_runtime, peer_id, i + elapsed_sec);
                }
            }
        }
        
        // Every 5 seconds, print cluster health and current role.
        if ((status_check_count % 50) == 0)
        {
            printf("\n[CLUSTER] Peer status at t=%ds:\n", elapsed_sec);
            for (int i = 2; i <= EXAMPLE_PEER_COUNT; i++)
            {
                char peer_id[32];
                snprintf(peer_id, sizeof(peer_id), "node-%d", i);
                
                ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT status;
                memset(&status, 0, sizeof(status));
                
                ZOO_ERROR_TYPE result = zoo_smb_runtime_get_peer_status(g_runtime, peer_id, &status);
                if (result == ZOO_SMB_OK)
                {
                    printf("  - %s: healthy=%s, epoch=%llu\n",
                           peer_id,
                           status.healthy ? "YES" : "NO",
                           (unsigned long long)status.peer_epoch);
                }
            }
            printf("  - Local: role=%s, epoch=%llu\n",
                   zoo_smb_runtime_get_role(g_runtime) == ZOO_SMB_RUNTIME_ROLE_PRIMARY ? "PRIMARY" : "SECONDARY",
                   (unsigned long long)zoo_smb_runtime_get_epoch(g_runtime));
        }
        
        heartbeat_count++;
        status_check_count++;
        usleep(100000);  // 100ms
    }
    
    printf("\n[CLUSTER] Simulation statistics:\n");
    printf("  - State changes: %d\n", g_state_change_count);
    printf("  - Status checks: %d\n", g_peer_status_checks);
    printf("  - Total heartbeats sent: %d\n", heartbeat_count);
    
    zoo_smb_runtime_destroy(g_runtime);
    g_runtime = NULL;
}

// ==============================================================================
// EXAMPLE: OBSERVER SNAPSHOT PATTERN
// ==============================================================================

/*
 * Show how multiple observers receive the same runtime transition.
 *
 * This pattern is useful when different subsystems need to react to leadership
 * or lifecycle changes independently. The example registers several observers
 * and then forces a state transition to demonstrate fan-out behavior.
 */
void example_observer_snapshot_pattern(void)
{
    printf("\n=== Example 5: Observer Snapshot Pattern ===\n");

    const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_init();

    ZOO_SMB_RUNTIME_HANDLE runtime = zoo_smb_runtime_create(config);
    if (!runtime)
    {
        printf("[ERROR] Failed to create runtime\n");
        return;
    }
    
    printf("[OBSERVER] Registering 5 observers...\n");
    
    for (int i = 0; i < 5; i++)
    {
        zoo_smb_runtime_register_state_observer(runtime, runtime_state_observer, (void*)(intptr_t)i);
        printf("[OBSERVER] Observer %d registered\n", i);
    }
    
    printf("[OBSERVER] Triggering state changes...\n");
    
    // Trigger a role update to fan out observer callbacks.
    zoo_smb_runtime_set_role(runtime, ZOO_SMB_RUNTIME_ROLE_PRIMARY);
    
    printf("[OBSERVER] State changes observed by all registered observers\n");
    printf("[OBSERVER] Total state changes: %d\n", g_state_change_count);
    
    zoo_smb_runtime_destroy(runtime);
}

// ==============================================================================
// MAIN ENTRY POINT
// ==============================================================================

/*
 * Parse the requested HA demo and run the matching scenario.
 *
 * When no argument is provided, the entry point runs the non-blocking examples
 * as a quick walkthrough. The longer cluster simulation remains opt-in because
 * it intentionally occupies the process for a fixed duration.
 */
int main(int argc, char* argv[])
{
    printf("=== ZOO SMB HA Examples ===\n");
    printf("Version: 1.0\n");
    printf("Date: 2026-04-01\n\n");
    
    if (argc > 1)
    {
        int example_num = atoi(argv[1]);
        
        switch (example_num)
        {
            case 1:
                example_basic_ha_setup();
                break;
            case 2:
                example_peer_monitoring();
                break;
            case 3:
                example_manual_election();
                break;
            case 4:
                example_ha_cluster_simulation();
                break;
            case 5:
                example_observer_snapshot_pattern();
                break;
            default:
                printf("Unknown example: %d\n", example_num);
                printf("Available examples: 1-5\n");
                break;
        }
    }
    else
    {
        printf("Usage: %s <example_number>\n\n", argv[0]);
        printf("Available examples:\n");
        printf("  1 - Basic HA Setup\n");
        printf("  2 - Peer Monitoring\n");
        printf("  3 - Manual Election\n");
        printf("  4 - HA Cluster Simulation (30 seconds)\n");
        printf("  5 - Observer Snapshot Pattern\n");
        
        // Run all examples
        example_basic_ha_setup();
        example_peer_monitoring();
        example_manual_election();
        example_observer_snapshot_pattern();
    }
    
    printf("\n=== Examples Complete ===\n");
    return 0;
}
