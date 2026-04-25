/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_METRICS_REPORT
 * File name: zoo_smb_metrics_report.c
 * Description: Implementation of unified metrics snapshot and reporting API.
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/

#include "zoo_smb_metrics_report.h"
#include "zoo_memory_pool.h"
#include "zoo_smb_config.h"
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */

/**
 * @brief Collects a unified snapshot of SMB metrics.
 *
 * Gathers metrics from the transport manager, routing engine, and memory pool,
 * and populates the provided snapshot structure with the latest values.
 *
 * @param transport_manager Handle to the transport manager (may be NULL).
 * @param routing_engine    Handle to the routing engine (may be NULL).
 * @param out_snapshot      Pointer to the snapshot struct to fill (must not be NULL).
 * @return ZOO_SMB_OK on success, ZOO_SMB_ERROR_INVALID_PARAM if out_snapshot is NULL.
 */
ZOO_ERROR_TYPE zoo_smb_collect_metrics_snapshot(
       ZOO_SMB_TRANSPORT_MANAGER_HANDLE  transport_manager,
       ZOO_SMB_ROUTING_ENGINE_HANDLE     routing_engine,
       ZOO_SMB_METRICS_SNAPSHOT_STRUCT*  out_snapshot)
{
    if (!out_snapshot)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    memset(out_snapshot, 0, sizeof(*out_snapshot));

    if (transport_manager)
    {
        (void)zoo_smb_transport_manager_get_metrics(transport_manager,
                                                    &out_snapshot->transport);
    }

    if (routing_engine)
    {
        (void)zoo_smb_routing_engine_get_metrics(routing_engine,
                                                 &out_snapshot->routing);
    }

       {
              ZOO_MEMORY_USAGE_T usage;
              const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_peek();

              memset(&usage, 0, sizeof(usage));
              zoo_memory_pool_get_usage(&usage);

              out_snapshot->memory.pool_size_bytes = usage.pool_size;
              out_snapshot->memory.used_size_bytes = usage.used_size;
              out_snapshot->memory.free_size_bytes = usage.free_size;
              out_snapshot->memory.max_free_pages = usage.max_free_pages;
              out_snapshot->memory.used_pct = (uint32_t)usage.used_pct;
              out_snapshot->memory.high_watermark_pct = config
                     ? config->sys.memory_high_watermark_pct
                     : ZOO_SMB_MEMORY_HIGH_WATERMARK_PCT_DEFAULT;
              out_snapshot->memory.low_watermark_pct = config
                     ? config->sys.memory_low_watermark_pct
                     : ZOO_SMB_MEMORY_LOW_WATERMARK_PCT_DEFAULT;
              out_snapshot->memory.high_watermark_active =
                     out_snapshot->memory.used_pct >= out_snapshot->memory.high_watermark_pct;
       }

    return ZOO_SMB_OK;
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Prints a formatted metrics report to stdout.
 *
 * Collects a metrics snapshot and prints a human-readable report including
 * transport, routing, and memory pool statistics. Useful for diagnostics and monitoring.
 *
 * @param tag              Optional label for the report (NULL or empty for default).
 * @param transport_manager Handle to the transport manager (may be NULL).
 * @param routing_engine    Handle to the routing engine (may be NULL).
 */
void zoo_smb_print_metrics_report(
       const char*                       tag,
       ZOO_SMB_TRANSPORT_MANAGER_HANDLE  transport_manager,
       ZOO_SMB_ROUTING_ENGINE_HANDLE     routing_engine)
{
    ZOO_SMB_METRICS_SNAPSHOT_STRUCT snap;
    zoo_smb_collect_metrics_snapshot(transport_manager, routing_engine, &snap);

    const char* label = (tag && tag[0] != '\0') ? tag : "[METRICS]";

    printf("=== %s =============================================\n", label);

    /* Transport-manager section */
    printf("  [TRANSPORT SEND]\n");
    printf("    send_success          : %llu\n",
           (unsigned long long)snap.transport.total_send_success);
    printf("    send_failures         : %llu\n",
           (unsigned long long)snap.transport.total_send_failures);
    printf("    backpressure_drops    : %llu\n",
           (unsigned long long)snap.transport.backpressure_drops);
    printf("    circuit_open_rejects  : %llu\n",
           (unsigned long long)snap.transport.circuit_open_rejections);
    printf("    in_flight             : %u  (high=%u low=%u)\n",
           snap.transport.in_flight_sends,
           snap.transport.send_high_watermark,
           snap.transport.send_low_watermark);
    printf("    backpressure_active   : %s\n",
           snap.transport.backpressure_active ? "YES" : "no");

    /* Routing-engine section */
    printf("  [ROUTING INGRESS]\n");
    printf("    received              : %llu\n",
           (unsigned long long)snap.routing.ingress_received);
    printf("    enqueued              : %llu\n",
           (unsigned long long)snap.routing.ingress_enqueued);
    printf("    dequeued              : %llu\n",
           (unsigned long long)snap.routing.ingress_dequeued);
    printf("    dropped_total         : %llu\n",
           (unsigned long long)snap.routing.ingress_dropped_total);
    printf("      drop_queue_full     : %llu\n",
           (unsigned long long)snap.routing.ingress_drop_queue_full);
    printf("      drop_backpressure   : %llu\n",
           (unsigned long long)snap.routing.ingress_drop_backpressure);
    printf("      drop_invalid_header : %llu\n",
           (unsigned long long)snap.routing.ingress_drop_invalid_header);
    printf("      drop_security_policy: %llu\n",
           (unsigned long long)snap.routing.ingress_drop_security_policy);
    printf("    in_flight             : %u  (high=%u low=%u)\n",
           snap.routing.ingress_in_flight,
           snap.routing.ingress_high_watermark,
           snap.routing.ingress_low_watermark);
    printf("    backpressure_active   : %s\n",
           snap.routing.ingress_backpressure_active ? "YES" : "no");

    printf("  [MEMORY POOL]\n");
    printf("    pool_size_bytes       : %zu\n", snap.memory.pool_size_bytes);
    printf("    used_size_bytes       : %zu\n", snap.memory.used_size_bytes);
    printf("    free_size_bytes       : %zu\n", snap.memory.free_size_bytes);
    printf("    used_pct              : %u  (high=%u low=%u)\n",
           snap.memory.used_pct,
           snap.memory.high_watermark_pct,
           snap.memory.low_watermark_pct);
    printf("    high_watermark_active : %s\n",
           snap.memory.high_watermark_active ? "YES" : "no");

    printf("=================================================\n");
    fflush(stdout);
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Resets all metrics for transport manager and routing engine.
 *
 * Calls the reset function for both the transport manager and routing engine,
 * clearing all counters and statistics. Safe to call with NULL handles.
 *
 * @param transport_manager Handle to the transport manager (may be NULL).
 * @param routing_engine    Handle to the routing engine (may be NULL).
 */
void zoo_smb_reset_all_metrics(
       ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager,
       ZOO_SMB_ROUTING_ENGINE_HANDLE    routing_engine)
{
    if (transport_manager)
    {
        (void)zoo_smb_transport_manager_reset_metrics(transport_manager);
    }
    if (routing_engine)
    {
        (void)zoo_smb_routing_engine_reset_metrics(routing_engine);
    }
}
