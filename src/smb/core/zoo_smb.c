/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: zoo_smb
 * File name: zoo_smb.c
 * Description: Core implementation of the Soft Message Bus
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 ******************************************************************************/

#include "zoo_smb.h"
#include "zoo_util.h"
#include "zoo_smb_config.h"
#include "zoo_list.h"
#include "zoo_memory_pool.h"
#include "zoo_thread_pool.h"
#include "zoo_smb_service_manager.h"
#include "zoo_smb_transport_manager.h"
#include "zoo_smb_rule_manager.h"
#include "zoo_smb_routing_engine.h"
#include "zoo_smb_service_discovery.h"
#include "zoo_smb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(_MSC_VER)
#define ZOO_SMB_CONSTRUCTOR
#define ZOO_SMB_DESTRUCTOR
#else
#define ZOO_SMB_CONSTRUCTOR __attribute__((constructor, weak))
#define ZOO_SMB_DESTRUCTOR __attribute__((destructor, weak))
#endif

#define __SMB__ (g_smb_instance)
#define _ROUTING_ENGINE_ (g_smb_instance->routing_engine)
#define _THREAD_POOL_ (g_smb_instance->thread_pool)
#define _DISCOVERY_ (g_smb_instance->discovery)
#define _NODE_LIST_ (g_smb_instance->node_list)
#define _SERVICE_MANAGER_ (g_smb_instance->service_manager)
#define _TRANSPORT_MANAGER_ (g_smb_instance->transport_manager)
#define _RULE_MANAGER_ (g_smb_instance->rule_manager)

/**
 * @brief Definition of a structure type.
 *
 * This typedef begins the declaration of a struct, which is used to group related data together.
 * The actual members and purpose of the struct should be documented after the full definition is provided.
 */
typedef struct
{
    ZOO_THREAD_POOL_HANDLE thread_pool;  // Thread pool for handling concurrent operations
    ZOO_SMB_SERVICE_DISCOVERY_HANDLE discovery;
    ZOO_SMB_ROUTING_ENGINE_HANDLE routing_engine;        // Routing engine for managing message routing
    ZOO_SMB_SERVICE_MANAGER_HANDLE service_manager;      // Service manager for managing services
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE transport_manager;  // Transport manager for handling transports
    ZOO_SMB_RULE_MANAGER_HANDLE rule_manager;            // Rule manager for managing routing rules
    ZOO_LIST_HANDLE node_list;
    ZOO_SMB_CONFIG_STRUCT* global_cfg;
    ZOO_BOOL is_running;  // Flag to indicate if the SMB instance is running
} ZOO_SMB_STRUCT;

/**
 * @brief Global instance of the SMB structure.
 *
 * This variable holds the singleton instance of the ZOO_SMB_STRUCT, which is used throughout
 * the application to manage the state and operations of the Soft Message Bus.
 */
static ZOO_SMB_STRUCT* g_smb_instance = NULL;

/**
 * @brief Initializes the broadcast service for the SMB bus internally.
 *
 * This function sets up the broadcast service for the SMB bus using the provided configuration.
 * It is typically called during the initialization phase of the SMB module.
 *
 * @param[in] bus Pointer to the SMB bus structure containing configuration data.
 * @param[in] service_info Pointer to the service information structure.
 *
 * @return ZOO_ERROR_TYPE Error code indicating success or failure of initialization.
 */
static ZOO_SMB_HANDLE smb_sys_init(const ZOO_SMB_CONFIG_STRUCT* cfg);
static void start_smb(ZOO_SMB_STRUCT* smb);
static void stop_smb(ZOO_SMB_STRUCT* smb);
static void destroy_smb(ZOO_SMB_STRUCT* smb, IN ZOO_BOOL force);

/**
 * @def ZOO_SMB_STRUCT_INITIALIZER
 * @brief Macro to provide a default initializer for SMB-related structures.
 *
 * This macro is intended to be used for initializing structures related to SMB (Soft Message Bus)
 * functionality within the zoo_smb module. The specific initialization values or behavior should be
 * defined where the macro is implemented.
 */
#define ZOO_SMB_STRUCT_INITIALIZER \
    {                              \
        .thread_pool = NULL,       \
        .discovery = NULL,         \
        .routing_engine = NULL,    \
        .service_manager = NULL,   \
        .transport_manager = NULL, \
        .rule_manager = NULL,      \
        .node_list = NULL,         \
        .global_cfg = NULL,        \
        .is_running = ZOO_FALSE}

/**
 * @brief Constructor function to initialize global variables.
 *
 * This function is automatically called before the main() function
 * when the shared object or executable is loaded. It is used to
 * perform any necessary initialization of global variables required
 * by the program.
 */
ZOO_SMB_CONSTRUCTOR void init_smb()
{
    const ZOO_SMB_CONFIG_STRUCT* cfg = zoo_smb_config_init();
    g_smb_instance = (ZOO_SMB_STRUCT*)smb_sys_init(cfg);
    if (g_smb_instance)
    {
        start_smb(g_smb_instance);
    }
}

/**
 * @brief Destructor function for SMB module.
 *
 * This function is automatically called when the shared object or executable is unloaded.
 * It is used to perform cleanup operations for the SMB module, such as releasing resources,
 * closing connections, or freeing memory allocated during the module's lifetime.
 *
 * The __attribute__((destructor)) ensures that this function is executed after main() exits
 * or when the shared library is unloaded.
 */
ZOO_SMB_DESTRUCTOR void terminate_smb()
{
    if (g_smb_instance)
    {
        stop_smb(g_smb_instance);
        destroy_smb(g_smb_instance, ZOO_FALSE);
        g_smb_instance = NULL;
    }
}

/**
 * @brief Initializes a ZOO SMB handle.
 *
 * This function sets up and returns a handle for interacting with the SMB (Soft Message Bus) protocol
 * in the ZOO system. It prepares the necessary resources and configurations for SMB operations.
 *
 * @return ZOO_SMB_HANDLE A handle to the initialized SMB instance.
 *         Returns NULL or an equivalent error value if initialization fails.
 */
static ZOO_SMB_HANDLE smb_sys_init(const ZOO_SMB_CONFIG_STRUCT* cfg)
{
    ZOO_LOG_TRACE("Initializing SMB instance...");
    ZOO_SMB_STRUCT* smb = NULL;

    if (cfg && cfg->sys.enable_deterministic_memory_profile)
    {
        if (cfg->sys.max_transport_buffer_size == 0 ||
            cfg->sys.max_transport_buffer_size > cfg->sys.mem_pool_size)
        {
            ZOO_LOG_ERROR("Deterministic memory profile invalid: transport buffer=%u pool=%u",
                          cfg->sys.max_transport_buffer_size,
                          cfg->sys.mem_pool_size);
            return NULL;
        }

        if (cfg->sys.max_thread_queue_size > 0 &&
            cfg->sys.send_high_watermark > cfg->sys.max_thread_queue_size)
        {
            ZOO_LOG_ERROR("Deterministic memory profile invalid: send high watermark=%u exceeds thread queue=%u",
                          cfg->sys.send_high_watermark,
                          cfg->sys.max_thread_queue_size);
            return NULL;
        }

        if (cfg->sys.max_queue_size > 0 &&
            cfg->sys.ingress_high_watermark > cfg->sys.max_queue_size)
        {
            ZOO_LOG_ERROR("Deterministic memory profile invalid: ingress high watermark=%u exceeds queue=%u",
                          cfg->sys.ingress_high_watermark,
                          cfg->sys.max_queue_size);
            return NULL;
        }
    }
#if defined(ZOO_SMB_AUTO_INIT)
    if (ZOO_SMB_OK != zoo_create_memory_pool(cfg->sys.mem_pool_size))
    {
        ZOO_LOG_ERROR("Failed to create memory pool for SMB instance");
        return NULL;
    }
    assert(ZOO_SMB_OK == zoo_validate_memory_pool());

    smb = (ZOO_SMB_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_STRUCT));
    if (!smb)
    {
        ZOO_LOG_ERROR("Failed to allocate SMB instance from memory pool");
        zoo_destroy_memory_pool();
        return NULL;
    }
#else
    smb = (ZOO_SMB_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_STRUCT));
    if (!smb)
    {
        ZOO_LOG_ERROR("Failed to allocate SMB instance from memory pool");
        return NULL;
    }
#endif

    memcpy(smb, &(ZOO_SMB_STRUCT)ZOO_SMB_STRUCT_INITIALIZER, sizeof(ZOO_SMB_STRUCT));
    smb->global_cfg = (ZOO_SMB_CONFIG_STRUCT*)cfg;
#if defined(ZOO_SMB_AUTO_INIT)
    smb->thread_pool = zoo_create_thread_pool(cfg->sys.default_thread_numbers, cfg->sys.max_thread_queue_size);
    assert(smb->thread_pool != NULL);
    smb->rule_manager = zoo_smb_create_rule_manager();
    assert(smb->rule_manager != NULL);
    smb->service_manager = zoo_smb_create_service_manager(cfg);
    assert(smb->service_manager != NULL);
    smb->transport_manager = zoo_smb_create_transport_manager(smb->service_manager, cfg);
    assert(smb->transport_manager != NULL);
    smb->routing_engine = zoo_smb_create_routing_engine(cfg);
    assert(smb->routing_engine != NULL);
    smb->discovery = zoo_smb_create_service_discovery(cfg, smb->service_manager, smb->transport_manager);
    assert(smb->discovery != NULL);
    smb->node_list = zoo_list_create(cfg->sys.max_node_size == 0 ? ZOO_SMB_MAX_NODE_SIZE : cfg->sys.max_node_size);
    assert(smb->node_list != NULL);
    zoo_smb_routing_engine_set_transport_manager(smb->routing_engine, smb->transport_manager);
    zoo_smb_routing_engine_set_rule_manager(smb->routing_engine, smb->rule_manager);
    zoo_smb_routing_engine_set_service_manager(smb->routing_engine, smb->service_manager);
#endif  // ZOO_SMB_AUTO_INIT
    ZOO_LOG_INFO("SMB initialized successfully");
    return (ZOO_SMB_HANDLE)smb;
}

/**
 * @brief Starts the SMB (Soft Message Bus) service for the zoo application.
 *
 * This function initializes and starts the SMB service, allowing network file sharing
 * capabilities within the zoo application context. It sets up necessary resources and
 * configurations required for SMB operations.
 *
 * @return ZOO_TRUE if the SMB service started successfully, ZOO_FALSE otherwise.
 */
static void start_smb(ZOO_SMB_STRUCT* smb)
{
    if (!smb)
    {
        return;
    }

    if (!smb->is_running)
    {
#if defined(ZOO_SMB_AUTO_INIT)
        zoo_smb_start_service_discovery(smb->discovery);
        zoo_smb_start_routing_engine(smb->routing_engine);
#endif  // ZOO_SMB_AUTO_INIT
        ZOO_LOG_INFO("SMB instance started successfully: %p", smb);
        smb->is_running = ZOO_TRUE;
    }
}

/**
 * @brief Stops the specified SMB (Soft Message Bus) handle.
 *
 * This function is responsible for stopping or shutting down the SMB handle
 * represented by the given ZOO_SMB_HANDLE. It should perform any necessary
 * cleanup or resource deallocation associated with the SMB session.
 *
 * @param smb The handle to the SMB session to be stopped.
 */
static void stop_smb(ZOO_SMB_STRUCT* smb)
{
    if (!smb)
    {
        return;
    }

    if (smb->is_running)
    {
        smb->is_running = ZOO_FALSE;
#if defined(ZOO_SMB_AUTO_INIT)
        zoo_smb_stop_service_discovery(smb->discovery);
#endif  // ZOO_SMB_AUTO_INIT
    }
}

/**
 * @brief Destroys the specified SMB handle and releases associated resources.
 *
 * This function cleans up and deallocates resources associated with the given
 * ZOO_SMB_HANDLE. If the 'force' parameter is set to ZOO_TRUE, the destruction
 * will proceed even if there are pending operations or references.
 *
 * @param[in] smb   The SMB handle to be destroyed.
 * @param[in] force If ZOO_TRUE, forces destruction regardless of pending operations.
 */
static void destroy_smb(ZOO_SMB_STRUCT* smb, IN ZOO_BOOL force)
{
    if (!smb)
    {
        return;
    }

    ZOO_LOG_DEBUG("Destroying SMB instance: %p, force: %d", smb, force);
#if defined(ZOO_SMB_AUTO_INIT)
    zoo_smb_destroy_service_discovery(smb->discovery);
    zoo_smb_destroy_routing_engine(smb->routing_engine);
    zoo_smb_destroy_service_manager(smb->service_manager);
    zoo_smb_destroy_transport_manager(smb->transport_manager);
    zoo_smb_destroy_rule_manager(smb->rule_manager);
    zoo_destroy_thread_pool(force);
    zoo_free_to_pool(smb);
    zoo_destroy_memory_pool();
#else
    zoo_free_to_pool(smb);
#endif  // ZOO_SMB_AUTO_INIT
}

/**
 * @brief Associates a node with the SMB bus.
 *
 * This function links the specified node to the SMB bus structure,
 * establishing any necessary relationships or references between
 * the node and the bus.
 *
 * @param smb Pointer to the ZOO_SMB_STRUCT representing the SMB bus.
 * @param node Handle to the node to be associated with the bus.
 */
static ZOO_ERROR_TYPE make_node_associate_with_bus(ZOO_SMB_STRUCT* smb, const ZOO_SMB_NODE_HANDLE node)
{
    uint32_t timeout_ms = smb->global_cfg->sys.max_timeout;  // Default timeout for rule preparation
    ZOO_SMB_TRANSPORT_HANDLE transport = NULL;
    ZOO_SMB_RULE_HANDLE rule = NULL;
    ZOO_SMB_SERVICE_HANDLE service = zoo_smb_service_manager_make_service(_SERVICE_MANAGER_, node);
    if (service == NULL)
    {
        service = zoo_smb_service_manager_get_service_wait(_SERVICE_MANAGER_, node->target, timeout_ms);
        if (!service)
        {
            ZOO_LOG_WARN("Service '%s' not found within timeout:%d", node->target, timeout_ms);
            return ZOO_SMB_ERROR_SERVICE_NOT_FOUND;
        }
    }

    transport = zoo_smb_transport_manager_make_transport(_TRANSPORT_MANAGER_, service);
    rule = zoo_smb_rule_manager_make_rule(_RULE_MANAGER_, node, service, transport);
    if (rule == NULL)
    {
        ZOO_LOG_ERROR("Failed to create routing rule for node: %p", node);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (ZOO_SMB_OK != zoo_smb_routing_engine_prepare_rule(_ROUTING_ENGINE_, rule, timeout_ms))
    {
        ZOO_LOG_DEBUG("Registered node: %p, total nodes: %zu", node, zoo_list_size(_NODE_LIST_));
        return ZOO_SMB_ERROR_ROUTING_RULE_PREPARE_FAILED;
    }
    node->active = ZOO_TRUE;           // Mark the node as active
    node->rule_name = rule->name;  // Associate the rule name with the node
    return ZOO_SMB_OK;
}

/**
 * @brief Retrieves the singleton instance handle for the ZOO SMB module.
 *
 * This function returns a handle to the current instance of the ZOO SMB module.
 * It is typically used to access shared resources or perform operations that
 * require a reference to the module's context.
 *
 * @return ZOO_SMB_HANDLE Handle to the ZOO SMB instance.
 */
ZOO_SMB_HANDLE zoo_smb_get_instance(void)
{
    return (ZOO_SMB_HANDLE)g_smb_instance;
}

/**
 * @brief Creates a new ZOO_SMB_BASE_NODE_STRUCT* instance associated with the given SMB handle.
 *
 * @param smb The SMB handle to associate with the new node.
 * @return Pointer to the newly created ZOO_SMB_BASE_NODE_STRUCT*, or NULL on failure.
 */
ZOO_BOOL zoo_smb_is_ready()
{
    return g_smb_instance->is_running;
}

/**
 * @brief Registers a SMB node with the system.
 *
 * This function registers the specified SMB node, making it available for further operations.
 *
 * @param node The handle to the SMB node to be registered.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the registration operation.
 */
ZOO_ERROR_TYPE zoo_smb_register_node(IN ZOO_SMB_NODE_HANDLE node)
{
    if (node == NULL)
    {
        ZOO_LOG_ERROR("Invalid node handle");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (zoo_smb_find_node(_NODE_LIST_, node) != NULL)
    {
        ZOO_LOG_WARN("Node is already registered: %p", node);
        return ZOO_SMB_OK;
    }

    zoo_list_push_back(_NODE_LIST_, node);
    ZOO_LOG_INFO("Node registered successfully: %s, topic: %s, transport type: %d",
                     node->target,
                     node->topic,
                     node->transport_type);

    if (node->node_type == ZOO_SMB_NODE_TYPE_SERVER)
    {
        ZOO_ERROR_TYPE associate_result = make_node_associate_with_bus(g_smb_instance, node);
        if (associate_result != ZOO_SMB_OK)
        {
            zoo_list_remove(_NODE_LIST_, node);
            ZOO_LOG_ERROR("Failed to activate server node '%s': %d", node->name, associate_result);
            return associate_result;
        }
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Unregisters a node from the Zoo SMB system.
 *
 * This function removes the specified node from the Zoo SMB registry,
 * effectively unregistering it and releasing any associated resources.
 *
 * @param[in] node The handle to the node to be unregistered.
 */
void zoo_smb_unregister_node(IN ZOO_SMB_NODE_HANDLE node)
{
    zoo_list_remove(_NODE_LIST_, node);
    ZOO_LOG_DEBUG("Unregistered node: %p, remaining nodes: %zu", node, zoo_list_size(_NODE_LIST_));
}

/**
 * @brief Checks if a specified SMB service is currently online.
 *
 * This function verifies the operational status of a named SMB service
 * by attempting to establish a connection and validate its availability.
 *
 * @param service_name The name of the SMB service to check. Must not be NULL.
 * @return ZOO_TRUE if the service is online and responsive,
 *         ZOO_FALSE if the service is offline or cannot be reached
 */
ZOO_BOOL zoo_smb_service_is_online(const char* service_name)
{
    uint32_t timeout_ms = __SMB__->global_cfg->sys.max_timeout;  // Default timeout for rule preparation
    ZOO_SMB_SERVICE_HANDLE service = zoo_smb_service_manager_get_service_wait(_SERVICE_MANAGER_, service_name, timeout_ms);
    return service != NULL && service->is_online;
}

ZOO_BOOL zoo_smb_service_is_available(const char* service_name)
{
    if (!g_smb_instance || !service_name)
    {
        return ZOO_FALSE;
    }

    ZOO_SMB_SERVICE_HANDLE service =
        zoo_smb_service_manager_get_service(_SERVICE_MANAGER_, service_name, ZOO_SMB_SERVICE_TYPE_REGISTRATION);
    return service != NULL && service->is_online;
}

ZOO_ERROR_TYPE zoo_smb_add_service_observer(IN ZOO_SMB_SERVICE_OBSERVER_HANDLE observer)
{
    if (!g_smb_instance || !observer)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    zoo_smb_service_manager_register_observer(_SERVICE_MANAGER_, observer);
    return ZOO_SMB_OK;
}

void zoo_smb_remove_service_observer(IN ZOO_SMB_SERVICE_OBSERVER_HANDLE observer)
{
    if (!g_smb_instance || !observer)
    {
        return;
    }

    zoo_smb_service_manager_unregister_observer(_SERVICE_MANAGER_, observer);
}

/**
 * @brief Routes a message to the specified SMB node and consumer.
 *
 * This function sends a message with the given message ID and payload to the specified
 * node and consumer within the SMB system. It is typically used to facilitate communication
 * between different components or services managed by the SMB core.
 *
 * @param[in] node         Handle to the target SMB node.
 * @param[in] msg_id       Identifier for the message type or command.
 * @param[in] payload      Pointer to the message payload data.
 * @param[in] payload_size Size of the payload data in bytes.
 * @param[in] consumer     Handle to the target consumer that should receive the message.
 * @param[in] request_id   Unique identifier for the request, used for tracking or correlation.
 *
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE zoo_smb_send_message(IN const ZOO_SMB_NODE_HANDLE node,
                                        IN const ZOO_SMB_MSG_STRUCT* msg,
                                        IN const char* receiver,
                                        IN ZOO_BOOL delete_after_send)
{
    if (!(__SMB__)->is_running)
    {
        ZOO_LOG_ERROR("SMB instance is not running, cannot route message.");
        return ZOO_SMB_ERROR_NOT_INITIALIZED;
    }

    if (!node->active)
    {
        ZOO_ERROR_TYPE associate_result = make_node_associate_with_bus(g_smb_instance, node);
        if (associate_result != ZOO_SMB_OK)
        {
            if (associate_result == (ZOO_ERROR_TYPE)ZOO_SMB_ERROR_SERVICE_NOT_FOUND)
            {
                ZOO_LOG_WARN("Target service not available yet for node: %s", node->name);
            }
            else
            {
                ZOO_LOG_ERROR("Failed to associate node with SMB bus: %p", node);
            }
            return associate_result;
        }
    }

    ZOO_SMB_RULE_HANDLE rule = zoo_smb_rule_manager_get_rule(_RULE_MANAGER_, node->rule_name);
    return zoo_smb_routing_engine_handle_route_message(_ROUTING_ENGINE_, rule, msg, receiver, delete_after_send);
}
