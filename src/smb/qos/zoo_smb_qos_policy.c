/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_QOS
 * File name: zoo_smb_qos_policy.c
 * Description: Quality of Service (QoS) implementation for ZOO SMB
 *              Provides creation, configuration, and destruction of QoS entities.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-01     weiwang.sun       created
 ******************************************************************************/
#include "zoo_smb_qos_policy.h"
#include "zoo_log.h"
#include "zoo_memory_pool.h"
#include <string.h>
#include <stdlib.h>

// ZOO_SMB_QOS_POLICY_STRUCT initializer
#define ZOO_SMB_QOS_POLICY_STRUCT_INITIALIZER                                                                                                 \
    {                                                                                                                                         \
        .priority_enabled = ZOO_TRUE,                                                                                                             \
        .priority = {.kind = ZOO_SMB_QOS_PRIORITY_HIGH},                                                                                      \
        .reliability_enabled = ZOO_TRUE,                                                                                                          \
        .reliability = {.kind = ZOO_SMB_QOS_RELIABILITY_BEST_EFFORT, .max_blocking_time_ms = 5000, .attempts = 3, .retry_interval_ms = 1000}, \
        .history_enabled = ZOO_FALSE,                                                                                                             \
        .history = {.kind = ZOO_SMB_QOS_HISTORY_KEEP_LAST, .depth = 0},                                                                       \
        .durability_enabled = ZOO_FALSE,                                                                                                          \
        .durability = {.kind = ZOO_SMB_QOS_DURABILITY_VOLATILE},                                                                              \
        .deadline_enabled = ZOO_TRUE,                                                                                                             \
        .deadline = {.deadline_time_ms = 0,                                                                                                   \
                     .deadline_duration_ms = 5000,                                                                                            \
        },                                                                                                                                    \
        .liveliness_enabled = ZOO_FALSE,                                                                                                         \
        .liveliness = {.kind = ZOO_SMB_QOS_LIVELINESS_AUTOMATIC, .lease_duration_ms = 30000},                                                \
        .ownership_enabled = ZOO_FALSE,                                                                                                          \
        .ownership = {.kind = ZOO_SMB_QOS_OWNERSHIP_SHARED, .strength = 0},                                                                  \
        .destination_order_enabled = ZOO_FALSE,                                                                                                  \
        .destination_order = {.kind = ZOO_SMB_QOS_DESTINATION_ORDER_BY_RECEPTION_TIMESTAMP},                                                  \
        .lifespan_enabled = ZOO_FALSE,                                                                                                           \
        .lifespan = {.duration_ms = 0},                                                                                                        \
        .latency_budget_enabled = ZOO_FALSE,                                                                                                     \
        .latency_budget = {.duration_ms = 0},                                                                                                  \
        .resource_limits_enabled = ZOO_FALSE,                                                                                                    \
        .resource_limits = {.max_samples = 1024, .max_instances = 1, .max_samples_per_instance = 1024}                                       \
    }

/**
 * @brief Logs the details of a ZOO_SMB_QOS_POLICY_STRUCT policy.
 *
 * This static function outputs information about the provided QoS policy
 * structure for debugging or informational purposes.
 *
 * @param policy Pointer to the ZOO_SMB_QOS_POLICY_STRUCT to be logged.
 */
static void log_policy(const ZOO_SMB_QOS_POLICY_STRUCT* policy)
{
    if (!policy)
    {
        ZOO_LOG_ERROR("log_policy: policy is NULL");
        return;
    }
    ZOO_LOG_INFO("ZOO_SMB_QOS_POLICY_STRUCT {");
    ZOO_LOG_INFO("  priority.kind = %d", policy->priority.kind);
    ZOO_LOG_INFO("  reliability.kind = %d", policy->reliability.kind);
    ZOO_LOG_INFO("  reliability.max_blocking_time_ms = %u", policy->reliability.max_blocking_time_ms);
    ZOO_LOG_INFO("  history.kind = %d", policy->history.kind);
    ZOO_LOG_INFO("  history.depth = %u", policy->history.depth);
    ZOO_LOG_INFO("  durability.kind = %d", policy->durability.kind);
    ZOO_LOG_INFO("  deadline.deadline_time = %llu", (unsigned long long)policy->deadline.deadline_time_ms);
    ZOO_LOG_INFO("  deadline.deadline_duration = %llu", (unsigned long long)policy->deadline.deadline_duration_ms);
    ZOO_LOG_INFO("  liveliness.kind = %d", policy->liveliness.kind);
    ZOO_LOG_INFO("  liveliness.lease_duration_ms = %llu", (unsigned long long)policy->liveliness.lease_duration_ms);
    ZOO_LOG_INFO("  ownership.kind = %d", policy->ownership.kind);
    ZOO_LOG_INFO("  ownership.strength = %u", policy->ownership.strength);
    ZOO_LOG_INFO("  destination_order.kind = %d", policy->destination_order.kind);
    ZOO_LOG_INFO("  lifespan.duration_ms = %llu", (unsigned long long)policy->lifespan.duration_ms);
    ZOO_LOG_INFO("  latency_budget.duration_ms = %llu", (unsigned long long)policy->latency_budget.duration_ms);
    ZOO_LOG_INFO("  resource_limits.max_samples = %u", policy->resource_limits.max_samples);
    ZOO_LOG_INFO("  resource_limits.max_instances = %u", policy->resource_limits.max_instances);
    ZOO_LOG_INFO("  resource_limits.max_samples_per_instance = %u", policy->resource_limits.max_samples_per_instance);
    ZOO_LOG_INFO("}");
}

static ZOO_BOOL is_reliability_compatible(
    const ZOO_SMB_QOS_POLICY_STRUCT* offered,
    const ZOO_SMB_QOS_POLICY_STRUCT* requested)
{
    ZOO_SMB_QOS_RELIABILITY_ENUM offered_kind = offered->reliability_enabled ? offered->reliability.kind : ZOO_SMB_QOS_RELIABILITY_BEST_EFFORT;
    ZOO_SMB_QOS_RELIABILITY_ENUM requested_kind = requested->reliability_enabled ? requested->reliability.kind : ZOO_SMB_QOS_RELIABILITY_BEST_EFFORT;
    return offered_kind >= requested_kind;
}

/**
 * @brief Checks whether offered durability satisfies requested durability.
 *
 * @param offered Offered QoS policy.
 * @param requested Requested QoS policy.
 * @return ZOO_TRUE if the offered durability is compatible, ZOO_FALSE otherwise.
 */
static ZOO_BOOL is_durability_compatible(
    const ZOO_SMB_QOS_POLICY_STRUCT* offered,
    const ZOO_SMB_QOS_POLICY_STRUCT* requested)
{
    ZOO_SMB_QOS_DURABILITY_ENUM offered_kind = offered->durability_enabled ? offered->durability.kind : ZOO_SMB_QOS_DURABILITY_VOLATILE;
    ZOO_SMB_QOS_DURABILITY_ENUM requested_kind = requested->durability_enabled ? requested->durability.kind : ZOO_SMB_QOS_DURABILITY_VOLATILE;
    return offered_kind >= requested_kind;
}

/**
 * @brief Checks whether offered deadline satisfies requested deadline.
 *
 * @param offered Offered QoS policy.
 * @param requested Requested QoS policy.
 * @return ZOO_TRUE if the offered deadline is compatible, ZOO_FALSE otherwise.
 */
static ZOO_BOOL is_deadline_compatible(
    const ZOO_SMB_QOS_POLICY_STRUCT* offered,
    const ZOO_SMB_QOS_POLICY_STRUCT* requested)
{
    if (!requested->deadline_enabled)
    {
        return ZOO_TRUE;
    }
    if (!offered->deadline_enabled)
    {
        return ZOO_FALSE;
    }
    return offered->deadline.deadline_duration_ms <= requested->deadline.deadline_duration_ms;
}

/**
 * @brief Checks whether offered liveliness satisfies requested liveliness.
 *
 * @param offered Offered QoS policy.
 * @param requested Requested QoS policy.
 * @return ZOO_TRUE if the offered liveliness is compatible, ZOO_FALSE otherwise.
 */
static ZOO_BOOL is_liveliness_compatible(
    const ZOO_SMB_QOS_POLICY_STRUCT* offered,
    const ZOO_SMB_QOS_POLICY_STRUCT* requested)
{
    ZOO_SMB_QOS_LIVELINESS_ENUM offered_kind = offered->liveliness_enabled ? offered->liveliness.kind : ZOO_SMB_QOS_LIVELINESS_AUTOMATIC;
    ZOO_SMB_QOS_LIVELINESS_ENUM requested_kind = requested->liveliness_enabled ? requested->liveliness.kind : ZOO_SMB_QOS_LIVELINESS_AUTOMATIC;
    if (offered_kind < requested_kind)
    {
        return ZOO_FALSE;
    }
    if (!requested->liveliness_enabled)
    {
        return ZOO_TRUE;
    }
    return offered->liveliness.lease_duration_ms <= requested->liveliness.lease_duration_ms;
}

/**
 * @brief Checks whether offered history satisfies requested history.
 *
 * @param offered Offered QoS policy.
 * @param requested Requested QoS policy.
 * @return ZOO_TRUE if the offered history is compatible, ZOO_FALSE otherwise.
 */

static ZOO_BOOL is_history_compatible(
    const ZOO_SMB_QOS_POLICY_STRUCT* offered,
    const ZOO_SMB_QOS_POLICY_STRUCT* requested)
{
    if (!requested->history_enabled)
    {
        return ZOO_TRUE;
    }
    if (!offered->history_enabled)
    {
        return ZOO_FALSE;
    }
    if (offered->history.kind < requested->history.kind)
    {
        return ZOO_FALSE;
    }
    if (requested->history.kind == ZOO_SMB_QOS_HISTORY_KEEP_LAST)
    {
        return offered->history.depth >= requested->history.depth;
    }
    return ZOO_TRUE;
}

/**
 * @brief Creates a new QoS (Quality of Service) policy based on the specified policy.
 *
 * @param policy Pointer to a ZOO_SMB_QOS_POLICY_STRUCT containing the desired QoS policy settings.
 * @return ZOO_SMB_QOS_POLICY_HANDLE Handle to the newly created QoS policy, or NULL on failure.
 */
ZOO_SMB_QOS_POLICY_HANDLE zoo_smb_init_qos_policy(IN ZOO_SMB_QOS_POLICY_STRUCT* policy)
{
    if (!policy)
    {
        ZOO_LOG_ERROR("zoo_smb_init_qos_policy: allocation failed");
        return NULL;
    }

    memset(policy, 0, sizeof(ZOO_SMB_QOS_POLICY_STRUCT));
    *policy = (ZOO_SMB_QOS_POLICY_STRUCT)ZOO_SMB_QOS_POLICY_STRUCT_INITIALIZER;  // Initialize the QoS structure
    log_policy(policy);                                                          // Log the QoS policy details
    ZOO_LOG_DEBUG("zoo_smb_init_qos_policy: QoS policy created");
    return policy;
}

/**
 * @brief Checks if the given QoS policy handle is valid.
 *
 * This function verifies whether the specified ZOO_SMB_QOS_POLICY_HANDLE
 * represents a valid Quality of Service (QoS) policy.
 *
 * @param[in] qos_policy The handle to the QoS policy to validate.
 *
 * @return ZOO_TRUE if the QoS policy handle is valid, ZOO_FALSE otherwise.
 */
ZOO_BOOL zoo_smb_qos_policy_is_valid(IN const ZOO_SMB_QOS_POLICY_HANDLE qos_policy)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: qos_policy is NULL");
        return ZOO_FALSE;
    }

    if (qos_policy->priority_enabled)
    {
        if (qos_policy->priority.kind < ZOO_SMB_QOS_PRIORITY_MIN || qos_policy->priority.kind > ZOO_SMB_QOS_PRIORITY_MAX)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid priority kind %d", qos_policy->priority.kind);
            return ZOO_FALSE;
        }
    }

    if (qos_policy->reliability_enabled)
    {
        if (qos_policy->reliability.kind < ZOO_SMB_QOS_RELIABILITY_MIN || qos_policy->reliability.kind > ZOO_SMB_QOS_RELIABILITY_MAX)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid reliability kind %d", qos_policy->reliability.kind);
            return ZOO_FALSE;
        }
    }

    if (qos_policy->history_enabled )
    {
        if (qos_policy->history.kind < ZOO_SMB_QOS_HISTORY_MIN || qos_policy->history.kind > ZOO_SMB_QOS_HISTORY_MAX)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid history kind %d", qos_policy->history.kind);
            return ZOO_FALSE;
        }
    }

    if (qos_policy->durability_enabled)
    {
        if (qos_policy->durability.kind < ZOO_SMB_QOS_DURABILITY_MIN || qos_policy->durability.kind > ZOO_SMB_QOS_DURABILITY_MAX)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid durability kind %d", qos_policy->durability.kind);
            return ZOO_FALSE;
        }
    }

    if (qos_policy->deadline_enabled)
    {
        if (qos_policy->deadline.deadline_duration_ms == 0)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid deadline time or duration");
            return ZOO_FALSE;
        }
    }

    if (qos_policy->liveliness_enabled)
    {
        if (qos_policy->liveliness.kind <= ZOO_SMB_QOS_LIVELINESS_MIN || qos_policy->liveliness.kind >= ZOO_SMB_QOS_LIVELINESS_MAX)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid liveliness kind %d", qos_policy->liveliness.kind);
            return ZOO_FALSE;
        }
        if (qos_policy->liveliness.lease_duration_ms == 0)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid liveliness lease duration");
            return ZOO_FALSE;
        }
    }

    if (qos_policy->ownership_enabled)
    {
        if (qos_policy->ownership.kind <= ZOO_SMB_QOS_OWNERSHIP_MIN || qos_policy->ownership.kind >= ZOO_SMB_QOS_OWNERSHIP_MAX)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid ownership kind %d", qos_policy->ownership.kind);
            return ZOO_FALSE;
        }
    }

    if (qos_policy->destination_order_enabled)
    {
        if (qos_policy->destination_order.kind <= ZOO_SMB_QOS_DESTINATION_ORDER_MIN ||
            qos_policy->destination_order.kind >= ZOO_SMB_QOS_DESTINATION_ORDER_MAX)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid destination order kind %d", qos_policy->destination_order.kind);
            return ZOO_FALSE;
        }
    }

    if (qos_policy->lifespan_enabled && qos_policy->lifespan.duration_ms == 0)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid lifespan duration");
        return ZOO_FALSE;
    }

    if (qos_policy->latency_budget_enabled && qos_policy->latency_budget.duration_ms == 0)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid latency budget duration");
        return ZOO_FALSE;
    }

    if (qos_policy->resource_limits_enabled)
    {
        if (qos_policy->resource_limits.max_samples == 0 ||
            qos_policy->resource_limits.max_instances == 0 ||
            qos_policy->resource_limits.max_samples_per_instance == 0)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: Invalid resource limits");
            return ZOO_FALSE;
        }
        if (qos_policy->history_enabled &&
            qos_policy->history.kind == ZOO_SMB_QOS_HISTORY_KEEP_LAST &&
            qos_policy->history.depth > qos_policy->resource_limits.max_samples_per_instance)
        {
            ZOO_LOG_ERROR("zoo_smb_qos_policy_is_valid: history depth exceeds resource limits per instance");
            return ZOO_FALSE;
        }
    }

    ZOO_LOG_DEBUG("zoo_smb_qos_policy_is_valid: QoS policy is valid");
    return ZOO_TRUE;
}

/**
 * @brief Creates a new SMB QoS (Quality of Service) policy handle.
 *
 * This function initializes and returns a handle to a new QoS policy for SMB (Soft Message Bus).
 * The created policy handle can be used to configure and manage QoS settings for SMB operations.
 *
 * @return ZOO_SMB_QOS_POLICY_HANDLE A handle to the newly created QoS policy.
 *         Returns NULL or an invalid handle on failure.
 */
ZOO_SMB_QOS_POLICY_HANDLE zoo_smb_create_qos_policy(
    IN const ZOO_SMB_QOS_POLICY_STRUCT* policy)
{
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = (ZOO_SMB_QOS_POLICY_HANDLE)zoo_allocate_from_pool(sizeof(ZOO_SMB_QOS_POLICY_STRUCT));
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_create_qos_policy: allocation failed");
        return NULL;
    }
    memcpy(qos_policy, policy, sizeof(ZOO_SMB_QOS_POLICY_STRUCT));
    log_policy(qos_policy);  // Log the QoS policy details
    ZOO_LOG_DEBUG("zoo_smb_create_qos_policy: QoS policy created");
    return qos_policy;
}

/**
 * @brief Destroys a QoS (Quality of Service) policy associated with the given handle.
 *
 * @param qos The handle to the QoS policy to be destroyed.
 */
void zoo_smb_destroy_qos_policy(IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy)
{
    if (!qos_policy)
    {
        ZOO_LOG_WARN("zoo_smb_destroy_qos_policy: qos is NULL");
        return;
    }
    zoo_free_to_pool(qos_policy);
    ZOO_LOG_DEBUG("zoo_smb_destroy_qos_policy: QoS policy destroyed");
}

/**
 * @brief Sets the priority for SMB QoS (Quality of Service).
 *
 * @param qos      The QoS handle.
 * @param priority The priority level to set.
 */
void zoo_smb_qos_policy_set_priority(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN ZOO_SMB_QOS_PRIORITY_ENUM priority)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_priority: qos is NULL");
        return;
    }
    qos_policy->priority_enabled = ZOO_TRUE;
    qos_policy->priority.kind = priority;
    ZOO_LOG_DEBUG("zoo_smb_qos_set_priority: set priority=%d", priority);
}

/**
 * @brief Sets the reliability level for SMB QoS (Quality of Service).
 *
 * @param qos                The QoS handle.
 * @param reliability        The reliability level to set.
 * @param max_blocking_time_ms The maximum blocking time in milliseconds.
 */
void zoo_smb_qos_policy_set_reliability(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN ZOO_SMB_QOS_RELIABILITY_ENUM reliability,
    IN uint32_t max_blocking_time_ms,
    IN uint32_t attempts,
    IN uint64_t retry_interval_ms)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_reliability: qos is NULL");
        return;
    }
    qos_policy->reliability_enabled = ZOO_TRUE;
    qos_policy->reliability.kind = reliability;
    qos_policy->reliability.attempts = attempts;
    qos_policy->reliability.retry_interval_ms = retry_interval_ms;
    qos_policy->reliability.max_blocking_time_ms = max_blocking_time_ms;
    ZOO_LOG_DEBUG("zoo_smb_qos_set_reliability: set reliability=%d, attempts=%u, max_blocking_time_ms=%u", reliability, attempts, max_blocking_time_ms);
}

/**
 * @brief Sets the history for SMB QoS (Quality of Service).
 *
 * @param qos    The QoS handle.
 * @param history The history policy to set.
 * @param depth   The history depth.
 */
void zoo_smb_qos_policy_set_history(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN ZOO_SMB_QOS_HISTORY_ENUM history,
    IN uint32_t depth)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_history: qos is NULL");
        return;
    }
    qos_policy->history_enabled = ZOO_TRUE;
    qos_policy->history.kind = history;
    qos_policy->history.depth = depth;
    ZOO_LOG_DEBUG("zoo_smb_qos_set_history: set history=%d, depth=%u", history, depth);
}

/**
 * @brief Sets the durability level for SMB QoS (Quality of Service).
 *
 * @param qos        The QoS handle.
 * @param durability The durability policy to set.
 */
void zoo_smb_qos_policy_set_durability(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN ZOO_SMB_QOS_DURABILITY_ENUM durability)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_durability: qos is NULL");
        return;
    }
    qos_policy->durability_enabled = ZOO_TRUE;
    qos_policy->durability.kind = durability;
    ZOO_LOG_DEBUG("zoo_smb_qos_set_durability: set durability=%d", durability);
}

/**
 * @brief Sets the deadline for SMB QoS (Quality of Service) operations.
 *
 * @param qos             The QoS handle.
 * @param deadline_time   The deadline time (absolute or relative, implementation defined).
 * @param deadline_duration The deadline duration in milliseconds.
 */
void zoo_smb_qos_policy_set_deadline(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN uint64_t deadline_time_ms,
    IN uint32_t deadline_duration_ms)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_deadline: qos is NULL");
        return;
    }
    qos_policy->deadline_enabled = ZOO_TRUE;
    qos_policy->deadline.deadline_time_ms = deadline_time_ms;
    qos_policy->deadline.deadline_duration_ms = deadline_duration_ms;
    ZOO_LOG_DEBUG("zoo_smb_qos_set_deadline: set deadline_time=%llu, duration=%u", deadline_time_ms, deadline_duration_ms);
}

/**
 * @brief Sets the liveliness policy for SMB QoS.
 *
 * @param qos_policy The QoS handle.
 * @param liveliness The liveliness policy to set.
 * @param lease_duration_ms Liveliness lease duration in milliseconds.
 */
void zoo_smb_qos_policy_set_liveliness(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN ZOO_SMB_QOS_LIVELINESS_ENUM liveliness,
    IN uint64_t lease_duration_ms)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_liveliness: qos is NULL");
        return;
    }
    qos_policy->liveliness_enabled = ZOO_TRUE;
    qos_policy->liveliness.kind = liveliness;
    qos_policy->liveliness.lease_duration_ms = lease_duration_ms;
}

/**
 * @brief Sets the ownership policy for SMB QoS.
 *
 * @param qos_policy The QoS handle.
 * @param ownership The ownership policy to set.
 * @param strength Ownership strength used for exclusive ownership.
 */
void zoo_smb_qos_policy_set_ownership(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN ZOO_SMB_QOS_OWNERSHIP_ENUM ownership,
    IN uint32_t strength)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_ownership: qos is NULL");
        return;
    }
    qos_policy->ownership_enabled = ZOO_TRUE;
    qos_policy->ownership.kind = ownership;
    qos_policy->ownership.strength = strength;
}

/**
 * @brief Sets the destination order policy for SMB QoS.
 *
 * @param qos_policy The QoS handle.
 * @param destination_order Destination order policy to set.
 */
void zoo_smb_qos_policy_set_destination_order(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN ZOO_SMB_QOS_DESTINATION_ORDER_ENUM destination_order)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_destination_order: qos is NULL");
        return;
    }
    qos_policy->destination_order_enabled = ZOO_TRUE;
    qos_policy->destination_order.kind = destination_order;
}

/**
 * @brief Sets the lifespan policy for SMB QoS.
 *
 * @param qos_policy The QoS handle.
 * @param duration_ms Lifespan duration in milliseconds.
 */
void zoo_smb_qos_policy_set_lifespan(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN uint64_t duration_ms)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_lifespan: qos is NULL");
        return;
    }
    qos_policy->lifespan_enabled = ZOO_TRUE;
    qos_policy->lifespan.duration_ms = duration_ms;
}

/**
 * @brief Sets the latency budget policy for SMB QoS.
 *
 * @param qos_policy The QoS handle.
 * @param duration_ms Latency budget duration in milliseconds.
 */
void zoo_smb_qos_policy_set_latency_budget(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN uint64_t duration_ms)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_latency_budget: qos is NULL");
        return;
    }
    qos_policy->latency_budget_enabled = ZOO_TRUE;
    qos_policy->latency_budget.duration_ms = duration_ms;
}

/**
 * @brief Sets the resource limits policy for SMB QoS.
 *
 * @param qos_policy The QoS handle.
 * @param max_samples Maximum total tracked samples.
 * @param max_instances Maximum tracked instances.
 * @param max_samples_per_instance Maximum samples allowed per instance.
 */
void zoo_smb_qos_policy_set_resource_limits(
    IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
    IN uint32_t max_samples,
    IN uint32_t max_instances,
    IN uint32_t max_samples_per_instance)
{
    if (!qos_policy)
    {
        ZOO_LOG_ERROR("zoo_smb_qos_set_resource_limits: qos is NULL");
        return;
    }
    qos_policy->resource_limits_enabled = ZOO_TRUE;
    qos_policy->resource_limits.max_samples = max_samples;
    qos_policy->resource_limits.max_instances = max_instances;
    qos_policy->resource_limits.max_samples_per_instance = max_samples_per_instance;
}

/**
 * @brief Checks whether an offered policy is compatible with a requested policy.
 *
 * @param offered_policy Offered QoS policy.
 * @param requested_policy Requested QoS policy.
 * @return ZOO_TRUE if the policies are compatible, ZOO_FALSE otherwise.
 */
ZOO_BOOL zoo_smb_qos_policy_is_compatible(
    IN const ZOO_SMB_QOS_POLICY_HANDLE offered_policy,
    IN const ZOO_SMB_QOS_POLICY_HANDLE requested_policy)
{
    ZOO_SMB_QOS_MATCH_RESULT_STRUCT result;
    return zoo_smb_qos_policy_match(offered_policy, requested_policy, &result);
}

/**
 * @brief Performs detailed QoS policy matching and returns incompatibility reasons.
 *
 * @param offered_policy Offered QoS policy.
 * @param requested_policy Requested QoS policy.
 * @param result Output match result structure.
 * @return ZOO_TRUE if the policies are compatible, ZOO_FALSE otherwise.
 */
ZOO_BOOL zoo_smb_qos_policy_match(
    IN const ZOO_SMB_QOS_POLICY_HANDLE offered_policy,
    IN const ZOO_SMB_QOS_POLICY_HANDLE requested_policy,
    OUT ZOO_SMB_QOS_MATCH_RESULT_STRUCT* result)
{
    uint32_t incompatible_mask = ZOO_SMB_QOS_INCOMPATIBLE_NONE;

    if (!offered_policy || !requested_policy)
    {
        if (result)
        {
            result->compatible = ZOO_FALSE;
            result->incompatible_mask = 0xFFFFFFFFu;
        }
        return ZOO_FALSE;
    }

    if (!is_reliability_compatible(offered_policy, requested_policy))
    {
        incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_RELIABILITY;
    }

    if (!is_durability_compatible(offered_policy, requested_policy))
    {
        incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_DURABILITY;
    }

    if (!is_deadline_compatible(offered_policy, requested_policy))
    {
        incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_DEADLINE;
    }

    if (!is_liveliness_compatible(offered_policy, requested_policy))
    {
        incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_LIVELINESS;
    }

    if (!is_history_compatible(offered_policy, requested_policy))
    {
        incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_HISTORY;
    }

    if (requested_policy->ownership_enabled)
    {
        if (!offered_policy->ownership_enabled ||
            offered_policy->ownership.kind != requested_policy->ownership.kind)
        {
            incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_OWNERSHIP;
        }
    }

    if (requested_policy->destination_order_enabled)
    {
        if (!offered_policy->destination_order_enabled ||
            offered_policy->destination_order.kind < requested_policy->destination_order.kind)
        {
            incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_DESTINATION_ORDER;
        }
    }

    if (requested_policy->resource_limits_enabled)
    {
        if (!offered_policy->resource_limits_enabled ||
            offered_policy->resource_limits.max_samples < requested_policy->resource_limits.max_samples ||
            offered_policy->resource_limits.max_instances < requested_policy->resource_limits.max_instances ||
            offered_policy->resource_limits.max_samples_per_instance < requested_policy->resource_limits.max_samples_per_instance)
        {
            incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_RESOURCE_LIMITS;
        }
    }

    if (requested_policy->latency_budget_enabled)
    {
        if (!offered_policy->latency_budget_enabled ||
            offered_policy->latency_budget.duration_ms > requested_policy->latency_budget.duration_ms)
        {
            incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_LATENCY_BUDGET;
        }
    }

    if (requested_policy->lifespan_enabled)
    {
        if (!offered_policy->lifespan_enabled ||
            offered_policy->lifespan.duration_ms < requested_policy->lifespan.duration_ms)
        {
            incompatible_mask |= ZOO_SMB_QOS_INCOMPATIBLE_LIFESPAN;
        }
    }

    if (result)
    {
        result->compatible = (incompatible_mask == ZOO_SMB_QOS_INCOMPATIBLE_NONE);
        result->incompatible_mask = incompatible_mask;
    }

    return incompatible_mask == ZOO_SMB_QOS_INCOMPATIBLE_NONE;
}

/**
 * @brief Checks whether two QoS policies are byte-wise equivalent.
 *
 * @param left_policy Left QoS policy.
 * @param right_policy Right QoS policy.
 * @return ZOO_TRUE if both policies are equivalent, ZOO_FALSE otherwise.
 */
ZOO_BOOL zoo_smb_qos_policy_is_equivalent(
    IN const ZOO_SMB_QOS_POLICY_HANDLE left_policy,
    IN const ZOO_SMB_QOS_POLICY_HANDLE right_policy)
{
    if (!left_policy || !right_policy)
    {
        return ZOO_FALSE;
    }

    return memcmp(left_policy, right_policy, sizeof(ZOO_SMB_QOS_POLICY_STRUCT)) == 0;
}
