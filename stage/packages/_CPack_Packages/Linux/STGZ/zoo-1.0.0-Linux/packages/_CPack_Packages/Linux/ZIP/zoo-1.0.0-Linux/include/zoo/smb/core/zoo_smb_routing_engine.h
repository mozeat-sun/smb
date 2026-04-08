/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_ROUTING_ENGINE
 * File name: zoo_smb_routing_engine.h
 * Description: Routing engine interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-30     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_ROUTING_ENGINE_H
#define ZOO_SMB_ROUTING_ENGINE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_routing_rule.h"
#include "zoo_smb_transport_manager.h"
#include "zoo_smb_service_manager.h"
#include "zoo_smb_rule_manager.h"
#include <stdio.h>
#include <stdbool.h>

    typedef enum
    {
        ZOO_SMB_INGRESS_DROP_NONE = 0,
        ZOO_SMB_INGRESS_DROP_QUEUE_FULL,
        ZOO_SMB_INGRESS_DROP_BACKPRESSURE,
        ZOO_SMB_INGRESS_DROP_INVALID_HEADER,
        ZOO_SMB_INGRESS_DROP_SECURITY_POLICY,
        ZOO_SMB_INGRESS_DROP_INVALID_PARAM
    } ZOO_SMB_INGRESS_DROP_REASON_ENUM;

    typedef struct
    {
        uint64_t ingress_received;
        uint64_t ingress_enqueued;
        uint64_t ingress_dequeued;
        uint64_t ingress_dropped_total;
        uint64_t ingress_drop_queue_full;
        uint64_t ingress_drop_backpressure;
        uint64_t ingress_drop_invalid_header;
        uint64_t ingress_drop_security_policy;
        uint32_t ingress_in_flight;
        uint32_t ingress_high_watermark;
        uint32_t ingress_low_watermark;
        ZOO_BOOL ingress_backpressure_active;
    } ZOO_SMB_ROUTING_ENGINE_METRICS_STRUCT;

    typedef struct ZOO_SMB_ROUTING_ENGINE_STRUCT* ZOO_SMB_ROUTING_ENGINE_HANDLE;

    /**
     * @brief Creates and initializes a new SMB routing engine instance.
     *
     * This function allocates and configures a routing engine based on the provided configuration.
     *
     * @param[in] name Pointer to a string containing the name of the routing engine.
     * @return ZOO_SMB_ROUTING_ENGINE_HANDLE Handle to the newly created routing engine instance, or NULL on failure.
     */
    ZOO_SMB_ROUTING_ENGINE_HANDLE zoo_smb_create_routing_engine(const ZOO_SMB_CONFIG_STRUCT* config);

    /**
     * @brief Sets the transport manager for the specified SMB routing engine.
     *
     * This function associates a transport manager with the given SMB routing engine handle.
     * The transport manager is responsible for managing the underlying transport mechanisms
     * used by the routing engine to send and receive SMB traffic.
     *
     * @param engine The handle to the SMB routing engine instance.
     * @param transport_manager The transport manager to be set for the routing engine.
     */
    void zoo_smb_routing_engine_set_transport_manager(ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                                      ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager);

    /**
     * @brief Sets the rule manager for the specified SMB routing engine.
     *
     * This function associates a rule manager with the given SMB routing engine handle,
     * allowing the engine to use the provided rule manager for routing decisions.
     *
     * @param engine The handle to the SMB routing engine instance.
     * @param rule_manager The rule manager to be set for the routing engine.
     */
    void zoo_smb_routing_engine_set_rule_manager(ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                                 ZOO_SMB_RULE_MANAGER_HANDLE rule_manager);
                                                 
    /**
     * @brief Sets the service manager for the specified SMB routing engine.
     *
     * This function associates a service manager with the given SMB routing engine handle.
     *
     * @param engine The handle to the SMB routing engine instance.
     * 
     * @note The service manager must be properly initialized before calling this function.
     */
    void zoo_smb_routing_engine_set_service_manager(ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                                 ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager);

    /**
     * @brief Destroys and cleans up resources used by the SMB routing engine.
     *
     * This function should be called to properly release any memory or resources
     * allocated by the SMB routing engine before program termination or when the
     * routing engine is no longer needed.
     */
    void zoo_smb_destroy_routing_engine(ZOO_SMB_ROUTING_ENGINE_HANDLE engine);

    /**
     * @brief Starts the SMB routing engine.
     *
     * This function initializes and starts the specified SMB routing engine,
     * allowing it to begin processing routing tasks.
     *
     * @param engine Handle to the SMB routing engine to be started.
     */
    void zoo_smb_start_routing_engine(ZOO_SMB_ROUTING_ENGINE_HANDLE engine);

    /**
     * @brief Stop the SMB routing engine.
     *
     * This function initializes and stops the specified SMB routing engine,
     * allowing it to begin processing routing tasks.
     *
     * @param engine Handle to the SMB routing engine to be stopped.
     */
    void zoo_smb_stop_routing_engine(ZOO_SMB_ROUTING_ENGINE_HANDLE engine);

    /**
     * @brief Prepares a routing rule for the specified SMB routing engine.
     *
     * This function initializes or sets up a routing rule within the given SMB routing engine handle.
     *
     * @param engine The handle to the SMB routing engine where the rule will be prepared.
     * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_routing_engine_prepare_rule(IN ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                                           IN ZOO_SMB_RULE_HANDLE rule,
                                                           IN uint32_t timeout_ms);
    /**
     * @brief Sends a message using the specified SMB routing engine.
     *
     * @param engine The handle to the SMB routing engine.
     * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_routing_engine_handle_route_message(IN ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
                                                                   IN const ZOO_SMB_RULE_HANDLE rule,
                                                                   IN const ZOO_SMB_MSG_STRUCT* message,
                                                                   IN const char* receiver,
                                                                   IN ZOO_BOOL delete_message_after_send);

    ZOO_ERROR_TYPE zoo_smb_routing_engine_get_metrics(
        IN ZOO_SMB_ROUTING_ENGINE_HANDLE engine,
        OUT ZOO_SMB_ROUTING_ENGINE_METRICS_STRUCT* out_metrics);

    ZOO_ERROR_TYPE zoo_smb_routing_engine_reset_metrics(
        IN ZOO_SMB_ROUTING_ENGINE_HANDLE engine);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_ROUTING_ENGINE_H */