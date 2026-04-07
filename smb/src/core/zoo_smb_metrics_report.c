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
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */

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

    return ZOO_SMB_OK;
}

/* -------------------------------------------------------------------------- */

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

    printf("=================================================\n");
    fflush(stdout);
}

/* -------------------------------------------------------------------------- */

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
