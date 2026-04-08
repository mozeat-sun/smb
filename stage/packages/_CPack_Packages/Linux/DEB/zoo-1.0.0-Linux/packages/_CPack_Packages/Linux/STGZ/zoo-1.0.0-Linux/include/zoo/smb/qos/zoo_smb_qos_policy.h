/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_QOS
 * File name: zoo_smb_qos_policy.h
 * Description: Quality of Service (QoS) definitions for ZOO SMB
 *              Similar to DDS QoS profiles, these settings control message
 *              delivery characteristics, reliability, and resource usage.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-18     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_QOS_POLICY_H
#define ZOO_SMB_QOS_POLICY_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include <stdbool.h>
#include "zoo_smb_types.h"
    typedef struct ZOO_SMB_QOS_POLICY_STRUCT* ZOO_SMB_QOS_POLICY_HANDLE;

    /**
     * @enum ZOO_SMB_QOS_PRIORITY
     * @brief Message priority levels for scheduling and resource allocation
     *
     * Higher priority messages are processed before lower priority ones.
     * This affects message queue ordering and thread scheduling.
     */
    typedef enum
    {
        ZOO_SMB_QOS_PRIORITY_MIN = 0,      /**< Minimum priority: lowest resource usage */
        ZOO_SMB_QOS_PRIORITY_LOW = 1,      /**< Low priority: background tasks */
        ZOO_SMB_QOS_PRIORITY_MEDIUM = 2,   /**< Medium priority: default value */
        ZOO_SMB_QOS_PRIORITY_HIGH = 3,     /**< High priority: important messages */
        ZOO_SMB_QOS_PRIORITY_REALTIME = 4, /**< Real-time priority: time-critical */
        ZOO_SMB_QOS_PRIORITY_MAX = 5       /**< Maximum priority: reserved for system use */
    } ZOO_SMB_QOS_PRIORITY_ENUM;

    typedef struct
    {
        ZOO_SMB_QOS_PRIORITY_ENUM kind; /**< Priority level */
    } ZOO_SMB_QOS_PRIORITY_POLICY_STRUCT;

    /**
     * @enum ZOO_SMB_RELIABILITY
     * @brief Reliability guarantees for message delivery
     *
     * Controls whether messages are guaranteed to be delivered and in what order.
     */
    typedef enum
    {
        ZOO_SMB_QOS_RELIABILITY_MIN = 0,         /**< Minimum reliability: no guarantees */
        ZOO_SMB_QOS_RELIABILITY_BEST_EFFORT = 1, /**< Best effort delivery: no guarantees */
        ZOO_SMB_QOS_RELIABILITY_RELIABLE = 2,    /**< Reliable delivery: with acknowledgment */
        ZOO_SMB_QOS_RELIABILITY_MAX = 3          /**< Maximum reliability: reserved for system use */
    } ZOO_SMB_QOS_RELIABILITY_ENUM;

    typedef struct
    {
        ZOO_SMB_QOS_RELIABILITY_ENUM kind; /**< Reliability guarantee */
        uint64_t max_blocking_time_ms;
        uint32_t attempts;           // Example field: number of attempts
        uint64_t retry_interval_ms;  // Example field: retry interval in milliseconds
    } ZOO_SMB_QOS_RELIABILITY_POLICY_STRUCT;

    /**
     * @enum ZOO_SMB_HISTORY
     * @brief History retention policies for messages
     *
     * Determines how many past messages are stored and available to new subscribers.
     */
    typedef enum
    {
        ZOO_SMB_QOS_HISTORY_MIN = 0,       /**< Minimum history: no samples retained */
        ZOO_SMB_QOS_HISTORY_KEEP_LAST = 1, /**< Keep only the last N samples (depth parameter) */
        ZOO_SMB_QOS_HISTORY_KEEP_ALL = 2,  /**< Keep all samples: no limit on history */
        ZOO_SMB_QOS_HISTORY_MAX = 3,       /**< Maximum history: reserved for system use */
    } ZOO_SMB_QOS_HISTORY_ENUM;

    typedef struct
    {
        ZOO_SMB_QOS_HISTORY_ENUM kind; /**< History retention policy */
        uint32_t depth;
    } ZOO_SMB_QOS_HISTORY_POLICY_STRUCT;

    /**
     * @enum ZOO_SMB_DURABILITY
     * @brief Durability policies for message delivery
     *
     * Controls how messages are persisted and delivered to late-joining subscribers.
     */
    typedef enum
    {
        ZOO_SMB_QOS_DURABILITY_MIN = 0,             /**< Minimum durability: no persistence */
        ZOO_SMB_QOS_DURABILITY_VOLATILE = 1,        /**< Volatile: messages are only sent to current subscribers */
        ZOO_SMB_QOS_DURABILITY_TRANSIENT_LOCAL = 2, /**< Transient local: late subscribers receive recent messages */
        ZOO_SMB_QOS_DURABILITY_PERSISTENT = 3,      /**< Persistent: late subscribers receive all historical messages */
        ZOO_SMB_QOS_DURABILITY_MAX
    } ZOO_SMB_QOS_DURABILITY_ENUM;

    typedef struct
    {
        ZOO_SMB_QOS_DURABILITY_ENUM kind; /**< Durability policy */
    } ZOO_SMB_QOS_DURABILITY_POLICY_STRUCT;

    typedef struct
    {
        uint64_t deadline_time_ms;      // Example field: time of the deadline
        uint64_t deadline_duration_ms;  // Example field: timeout in milliseconds
    } ZOO_SMB_QOS_DEADLINE_POLICY_STRUCT;

    /**
     * @enum ZOO_SMB_LIVELINESS
     * @brief DDS-like liveliness policy
     */
    typedef enum
    {
        ZOO_SMB_QOS_LIVELINESS_MIN = 0,
        ZOO_SMB_QOS_LIVELINESS_AUTOMATIC = 1,
        ZOO_SMB_QOS_LIVELINESS_MANUAL_BY_TOPIC = 2,
        ZOO_SMB_QOS_LIVELINESS_MAX = 3
    } ZOO_SMB_QOS_LIVELINESS_ENUM;

    typedef struct
    {
        ZOO_SMB_QOS_LIVELINESS_ENUM kind; /**< Liveliness assertion mode */
        uint64_t lease_duration_ms;       /**< Liveliness lease duration */
    } ZOO_SMB_QOS_LIVELINESS_POLICY_STRUCT;

    /**
     * @enum ZOO_SMB_OWNERSHIP
     * @brief DDS-like ownership policy
     */
    typedef enum
    {
        ZOO_SMB_QOS_OWNERSHIP_MIN = 0,
        ZOO_SMB_QOS_OWNERSHIP_SHARED = 1,
        ZOO_SMB_QOS_OWNERSHIP_EXCLUSIVE = 2,
        ZOO_SMB_QOS_OWNERSHIP_MAX = 3
    } ZOO_SMB_QOS_OWNERSHIP_ENUM;

    typedef struct
    {
        ZOO_SMB_QOS_OWNERSHIP_ENUM kind; /**< Ownership mode */
        uint32_t strength;               /**< Exclusive ownership strength */
    } ZOO_SMB_QOS_OWNERSHIP_POLICY_STRUCT;

    /**
     * @enum ZOO_SMB_DESTINATION_ORDER
     * @brief DDS-like destination order policy
     */
    typedef enum
    {
        ZOO_SMB_QOS_DESTINATION_ORDER_MIN = 0,
        ZOO_SMB_QOS_DESTINATION_ORDER_BY_RECEPTION_TIMESTAMP = 1,
        ZOO_SMB_QOS_DESTINATION_ORDER_BY_SOURCE_TIMESTAMP = 2,
        ZOO_SMB_QOS_DESTINATION_ORDER_MAX = 3
    } ZOO_SMB_QOS_DESTINATION_ORDER_ENUM;

    typedef struct
    {
        ZOO_SMB_QOS_DESTINATION_ORDER_ENUM kind; /**< Ordering mode */
    } ZOO_SMB_QOS_DESTINATION_ORDER_POLICY_STRUCT;

    typedef struct
    {
        uint64_t duration_ms; /**< Sample lifetime duration */
    } ZOO_SMB_QOS_LIFESPAN_POLICY_STRUCT;

    typedef struct
    {
        uint64_t duration_ms; /**< Desired latency budget */
    } ZOO_SMB_QOS_LATENCY_BUDGET_POLICY_STRUCT;

    typedef struct
    {
        uint32_t max_samples;               /**< Maximum tracked samples */
        uint32_t max_instances;             /**< Maximum instances */
        uint32_t max_samples_per_instance;  /**< Maximum samples per instance */
    } ZOO_SMB_QOS_RESOURCE_LIMITS_POLICY_STRUCT;

    /**
     * @brief QoS incompatibility reason bitmask
     */
    typedef enum
    {
        ZOO_SMB_QOS_INCOMPATIBLE_NONE = 0,
        ZOO_SMB_QOS_INCOMPATIBLE_RELIABILITY = 1 << 0,
        ZOO_SMB_QOS_INCOMPATIBLE_DURABILITY = 1 << 1,
        ZOO_SMB_QOS_INCOMPATIBLE_DEADLINE = 1 << 2,
        ZOO_SMB_QOS_INCOMPATIBLE_LIVELINESS = 1 << 3,
        ZOO_SMB_QOS_INCOMPATIBLE_HISTORY = 1 << 4,
        ZOO_SMB_QOS_INCOMPATIBLE_OWNERSHIP = 1 << 5,
        ZOO_SMB_QOS_INCOMPATIBLE_DESTINATION_ORDER = 1 << 6,
        ZOO_SMB_QOS_INCOMPATIBLE_RESOURCE_LIMITS = 1 << 7,
        ZOO_SMB_QOS_INCOMPATIBLE_LATENCY_BUDGET = 1 << 8,
        ZOO_SMB_QOS_INCOMPATIBLE_LIFESPAN = 1 << 9
    } ZOO_SMB_QOS_INCOMPATIBILITY_MASK_ENUM;

    /**
     * @brief DDS-like offered/requested QoS match result
     */
    typedef struct
    {
        ZOO_BOOL compatible;        /**< Whether offered policy satisfies requested policy */
        uint32_t incompatible_mask; /**< Incompatibility reason mask */
    } ZOO_SMB_QOS_MATCH_RESULT_STRUCT;

    typedef struct ZOO_SMB_QOS_POLICY_STRUCT
    {
        ZOO_BOOL priority_enabled;                             /**< Whether QoS is enabled for this policy */
        ZOO_SMB_QOS_PRIORITY_POLICY_STRUCT priority;       /**< Message priority policy */
        ZOO_BOOL reliability_enabled;                          /**< Whether reliability is enabled */
        ZOO_SMB_QOS_RELIABILITY_POLICY_STRUCT reliability; /**< Reliability guarantee policy */
        ZOO_BOOL history_enabled;                              /**< Whether history retention is enabled */
        ZOO_SMB_QOS_HISTORY_POLICY_STRUCT history;         /**< History retention policy */
        ZOO_BOOL durability_enabled;                           /**< Whether message durability is enabled */
        ZOO_SMB_QOS_DURABILITY_POLICY_STRUCT durability;   /**< Message durability policy */
        ZOO_BOOL deadline_enabled;                             /**< Whether message validity deadline is enabled */
        ZOO_SMB_QOS_DEADLINE_POLICY_STRUCT deadline;       /**< Message validity deadline */
        ZOO_BOOL liveliness_enabled;                           /**< Whether liveliness is enabled */
        ZOO_SMB_QOS_LIVELINESS_POLICY_STRUCT liveliness;   /**< Liveliness policy */
        ZOO_BOOL ownership_enabled;                            /**< Whether ownership is enabled */
        ZOO_SMB_QOS_OWNERSHIP_POLICY_STRUCT ownership;     /**< Ownership policy */
        ZOO_BOOL destination_order_enabled;                    /**< Whether destination order is enabled */
        ZOO_SMB_QOS_DESTINATION_ORDER_POLICY_STRUCT destination_order; /**< Destination order policy */
        ZOO_BOOL lifespan_enabled;                             /**< Whether lifespan is enabled */
        ZOO_SMB_QOS_LIFESPAN_POLICY_STRUCT lifespan;       /**< Lifespan policy */
        ZOO_BOOL latency_budget_enabled;                       /**< Whether latency budget is enabled */
        ZOO_SMB_QOS_LATENCY_BUDGET_POLICY_STRUCT latency_budget; /**< Latency budget policy */
        ZOO_BOOL resource_limits_enabled;                      /**< Whether resource limits are enabled */
        ZOO_SMB_QOS_RESOURCE_LIMITS_POLICY_STRUCT resource_limits; /**< Resource limits policy */
    } ZOO_SMB_QOS_POLICY_STRUCT;

    /**
     * @brief Creates a new QoS (Quality of Service) entity based on the specified policy.
     *
     * This function initializes and returns a handle to a QoS entity configured according to
     * the parameters defined in the provided ZOO_SMB_QOS_POLICY_STRUCT structure.
     *
     * @param policy Pointer to a ZOO_SMB_QOS_POLICY_STRUCT containing the desired QoS policy settings.
     * @return ZOO_SMB_QOS_HANDLE Handle to the newly created QoS entity, or NULL on failure.
     */
    ZOO_SMB_QOS_POLICY_HANDLE zoo_smb_init_qos_policy(IN ZOO_SMB_QOS_POLICY_STRUCT* policy);

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
    ZOO_BOOL zoo_smb_qos_policy_is_valid(IN const ZOO_SMB_QOS_POLICY_HANDLE qos_policy);

    /**
     * @brief Creates a new SMB QoS (Quality of Service) policy handle.
     *
     * This function initializes and returns a handle to a new QoS policy for SMB (Server Message Block).
     * The created policy handle can be used to configure and manage QoS settings for SMB operations.
     *
     * @return ZOO_SMB_QOS_POLICY_HANDLE A handle to the newly created QoS policy.
     *         Returns NULL or an invalid handle on failure.
     */
    ZOO_SMB_QOS_POLICY_HANDLE zoo_smb_create_qos_policy(
        IN const ZOO_SMB_QOS_POLICY_STRUCT* policy);

    /**
     * @brief Destroys a QoS (Quality of Service) entity associated with the given handle.
     *
     * This function releases all resources associated with the specified QoS entity.
     * After calling this function, the provided handle becomes invalid and should not be used.
     *
     * @param qos The handle to the QoS entity to be destroyed.
     */
    void zoo_smb_destroy_qos_policy(IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy);

    /**
     * @brief Sets the priority for SMB QoS (Quality of Service).
     *
     * This function configures the priority level for SMB (Server Message Block)
     * Quality of Service, which may affect resource allocation or traffic handling.
     *
        * @param qos_policy QoS policy handle to update.
        * @param priority Priority level to apply.
     */
    void zoo_smb_qos_policy_set_priority(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN ZOO_SMB_QOS_PRIORITY_ENUM priority);

    /**
     * @brief Sets the reliability level for SMB QoS (Quality of Service).
     *
     * This function configures the reliability parameters for SMB (Server Message Block)
     * Quality of Service, allowing control over how reliable the SMB service should be.
     *
    * @param qos_policy QoS policy handle to update.
    * @param reliability Reliability kind (best effort or reliable).
    * @param max_blocking_time_ms Maximum wait time used by reliability-sensitive waits.
    * @param attempts Maximum retry attempts used by retry-aware flows.
    * @param retry_interval_ms Retry interval between attempts.
     */
    void zoo_smb_qos_policy_set_reliability(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN ZOO_SMB_QOS_RELIABILITY_ENUM reliability,
        IN uint32_t max_blocking_time_ms,
        IN uint32_t attempts,
        IN uint64_t retry_interval_ms);

    /**
     * @brief Sets the history for SMB QoS (Quality of Service).
     *
     * This function configures or updates the history data used for SMB QoS management.
     *
    * @param qos_policy QoS policy handle to update.
    * @param history History kind.
    * @param depth Number of samples kept when using KEEP_LAST.
     */
    void zoo_smb_qos_policy_set_history(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN ZOO_SMB_QOS_HISTORY_ENUM history,
        IN uint32_t depth);

    /**
     * @brief Sets the durability level for SMB QoS (Quality of Service).
     *
     * This function configures the durability settings for SMB (Server Message Block)
     * Quality of Service, which may affect data consistency and reliability guarantees.
     *
    * @param qos_policy QoS policy handle to update.
    * @param durability Durability kind to apply.
     */
    void zoo_smb_qos_policy_set_durability(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN ZOO_SMB_QOS_DURABILITY_ENUM durability);

    /**
     * @brief Sets the deadline for SMB QoS (Quality of Service) operations.
     *
     * This function configures the deadline parameter used to control the timing or
     * scheduling of SMB (Server Message Block) operations to ensure quality of service.
     *
    * @param qos_policy QoS policy handle to update.
    * @param deadline_time_ms Deadline reference timestamp in milliseconds.
    * @param deadline_duration_ms Deadline duration in milliseconds.
     */
    void zoo_smb_qos_policy_set_deadline(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN uint64_t deadline_time_ms,
        IN uint32_t deadline_duration_ms);

    void zoo_smb_qos_policy_set_liveliness(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN ZOO_SMB_QOS_LIVELINESS_ENUM liveliness,
        IN uint64_t lease_duration_ms);

    void zoo_smb_qos_policy_set_ownership(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN ZOO_SMB_QOS_OWNERSHIP_ENUM ownership,
        IN uint32_t strength);

    void zoo_smb_qos_policy_set_destination_order(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN ZOO_SMB_QOS_DESTINATION_ORDER_ENUM destination_order);

    void zoo_smb_qos_policy_set_lifespan(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN uint64_t duration_ms);

    void zoo_smb_qos_policy_set_latency_budget(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN uint64_t duration_ms);

    void zoo_smb_qos_policy_set_resource_limits(
        IN ZOO_SMB_QOS_POLICY_HANDLE qos_policy,
        IN uint32_t max_samples,
        IN uint32_t max_instances,
        IN uint32_t max_samples_per_instance);

    ZOO_BOOL zoo_smb_qos_policy_match(
        IN const ZOO_SMB_QOS_POLICY_HANDLE offered_policy,
        IN const ZOO_SMB_QOS_POLICY_HANDLE requested_policy,
        OUT ZOO_SMB_QOS_MATCH_RESULT_STRUCT* result);

    ZOO_BOOL zoo_smb_qos_policy_is_compatible(
        IN const ZOO_SMB_QOS_POLICY_HANDLE offered_policy,
        IN const ZOO_SMB_QOS_POLICY_HANDLE requested_policy);

    ZOO_BOOL zoo_smb_qos_policy_is_equivalent(
        IN const ZOO_SMB_QOS_POLICY_HANDLE left_policy,
        IN const ZOO_SMB_QOS_POLICY_HANDLE right_policy);

#define ZOO_SMB_DEFAULT_QOS_POLICY()                                              \
    {                                                                             \
        ZOO_TRUE,                                   /* priority_enabled */             \
        { ZOO_SMB_QOS_PRIORITY_MEDIUM },        /* priority */                    \
        ZOO_TRUE,                                   /* reliability_enabled */          \
        {                                      /* reliability */                  \
            ZOO_SMB_QOS_RELIABILITY_RELIABLE,   /* kind */                        \
            5000,                              /* max_blocking_time_ms */         \
            3,                                 /* attempts */                     \
            1000                               /* retry_interval_ms */             \
        },                                                                         \
        ZOO_FALSE,                                   /* history_enabled */              \
        { ZOO_SMB_QOS_HISTORY_KEEP_LAST, 10 },  /* history */                     \
        ZOO_FALSE,                                   /* durability_enabled */           \
        { ZOO_SMB_QOS_DURABILITY_VOLATILE },    /* durability */                  \
        ZOO_TRUE,                                    /* deadline_enabled */             \
        { 0, 10000 },                             /* deadline: deadline_time_ms, deadline_duration_ms */ \
        ZOO_FALSE,                                /* liveliness_enabled */           \
        { ZOO_SMB_QOS_LIVELINESS_AUTOMATIC, 30000 }, /* liveliness */              \
        ZOO_FALSE,                                /* ownership_enabled */            \
        { ZOO_SMB_QOS_OWNERSHIP_SHARED, 0 },      /* ownership */                   \
        ZOO_FALSE,                                /* destination_order_enabled */    \
        { ZOO_SMB_QOS_DESTINATION_ORDER_BY_RECEPTION_TIMESTAMP }, /* destination_order */ \
        ZOO_FALSE,                                /* lifespan_enabled */             \
        { 0 },                                    /* lifespan */                    \
        ZOO_FALSE,                                /* latency_budget_enabled */       \
        { 0 },                                    /* latency_budget */              \
        ZOO_FALSE,                                /* resource_limits_enabled */      \
        { 1024, 1, 1024 }                         /* resource_limits */             \
    }

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_QOS_POLICY_H */