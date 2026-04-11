/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_METRICS_REPORT
 * File name: zoo_smb_metrics_report.h
 * Description: Unified metrics snapshot and reporting API for ZOO SMB.
 *              Aggregates transport-manager and routing-engine telemetry
 *              into a single printable snapshot.
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-01     AI Assistant      created
 ******************************************************************************/

#ifndef ZOO_SMB_METRICS_REPORT_H
#define ZOO_SMB_METRICS_REPORT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_transport_manager.h"
#include "zoo_smb_routing_engine.h"
#include "zoo_smb_error.h"

    typedef struct
    {
        size_t pool_size_bytes;
        size_t used_size_bytes;
        size_t free_size_bytes;
        size_t max_free_pages;
        uint32_t used_pct;
        uint32_t high_watermark_pct;
        uint32_t low_watermark_pct;
        ZOO_BOOL high_watermark_active;
    } ZOO_SMB_MEMORY_POOL_METRICS_STRUCT;

    /**
     * @brief Unified metrics snapshot combining transport and routing telemetry.
     */
    typedef struct
    {
        ZOO_SMB_TRANSPORT_MANAGER_METRICS_STRUCT transport; /**< Send-path metrics  */
        ZOO_SMB_ROUTING_ENGINE_METRICS_STRUCT    routing;   /**< Ingress-path metrics */
        ZOO_SMB_MEMORY_POOL_METRICS_STRUCT       memory;    /**< Memory-pool pressure metrics */
    } ZOO_SMB_METRICS_SNAPSHOT_STRUCT;

    /**
     * @brief Collect a single atomic snapshot of all metrics.
     *
     * Reads metrics from both the transport manager and the routing engine into
     * @p out_snapshot.
     *
     * @param transport_manager  Handle to the transport manager (may be NULL – that
     *                          section of the snapshot will be zeroed).
     * @param routing_engine     Handle to the routing engine (may be NULL – that
     *                          section of the snapshot will be zeroed).
     * @param out_snapshot       Caller-allocated output structure; must not be NULL.
     * @return ZOO_SMB_OK on success, or an error code if @p out_snapshot is NULL.
     */
    ZOO_ERROR_TYPE zoo_smb_collect_metrics_snapshot(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE       transport_manager,
        ZOO_SMB_ROUTING_ENGINE_HANDLE          routing_engine,
        ZOO_SMB_METRICS_SNAPSHOT_STRUCT*       out_snapshot);

    /**
     * @brief Print a human-readable metrics report to stdout.
     *
     * Internally calls zoo_smb_collect_metrics_snapshot() then formats the
     * result using printf.  Suitable for test-script output and debug builds.
     *
     * @param tag                Short prefix label printed with each line (e.g.
     *                          "POST_TEST" or "PERIODIC").  If NULL, "[METRICS]"
     *                          is used.
     * @param transport_manager  Handle to the transport manager (may be NULL).
     * @param routing_engine     Handle to the routing engine (may be NULL).
     */
    void zoo_smb_print_metrics_report(
        const char*                            tag,
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE       transport_manager,
        ZOO_SMB_ROUTING_ENGINE_HANDLE          routing_engine);

    /**
     * @brief Reset all telemetry counters on both subsystems.
     *
     * @param transport_manager  Handle to the transport manager (may be NULL).
     * @param routing_engine     Handle to the routing engine (may be NULL).
     */
    void zoo_smb_reset_all_metrics(
        ZOO_SMB_TRANSPORT_MANAGER_HANDLE       transport_manager,
        ZOO_SMB_ROUTING_ENGINE_HANDLE          routing_engine);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_METRICS_REPORT_H */
