/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_RUNTIME
 * File name: zoo_smb_runtime.c
 * Description: Runtime lifecycle and HA scaffolding implementation for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-03-31     AI Assistant      created
 ******************************************************************************/

#include "zoo_smb_runtime.h"
#include "zoo_smb_error.h"
#include "domain/zoo_domain_profile.h"
#include "domain/zoo_domain_policy.h"
#include "assurance/zoo_assurance_mesh.h"
#include "zoo_log.h"
#include "zoo_memory_pool.h"
#include "zoo_thread_pool.h"
#include "zoo_platform.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

#define ZOO_SMB_RUNTIME_OBSERVER_CAPACITY 8
#define ZOO_SMB_RUNTIME_PEER_CAPACITY 8

typedef struct
{
    ZOO_SMB_RUNTIME_STATE_OBSERVER observer;
    void* user_data;
} ZOO_SMB_RUNTIME_OBSERVER_SLOT_STRUCT;

typedef struct
{
    char peer_id[32];
    uint64_t peer_epoch;
    uint64_t last_heartbeat_ms;
    ZOO_BOOL healthy;
} ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT;

typedef struct ZOO_SMB_RUNTIME_STRUCT
{
    const ZOO_SMB_CONFIG_STRUCT* config;
    ZOO_SMB_HANDLE bus;
    ZOO_BOOL started;
    ZOO_BOOL ha_monitor_running;
    ZOO_MUTEX_T lock;
    ZOO_SMB_RUNTIME_STATE_ENUM state;
    ZOO_SMB_RUNTIME_ROLE_ENUM role;
    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    uint64_t epoch;
    ZOO_SMB_RUNTIME_OBSERVER_SLOT_STRUCT observers[ZOO_SMB_RUNTIME_OBSERVER_CAPACITY];
    ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT peers[ZOO_SMB_RUNTIME_PEER_CAPACITY];
} ZOO_SMB_RUNTIME_STRUCT;

/**
 * @brief Check whether the runtime role value is valid.
 * @param role Runtime role to validate
 * @return ZOO_TRUE if valid, otherwise ZOO_FALSE
 */
static ZOO_BOOL is_valid_role(ZOO_SMB_RUNTIME_ROLE_ENUM role)
{
    return role == ZOO_SMB_RUNTIME_ROLE_STANDALONE ||
           role == ZOO_SMB_RUNTIME_ROLE_PRIMARY ||
           role == ZOO_SMB_RUNTIME_ROLE_SECONDARY;
}

/**
 * @brief Resolve effective domain profile for runtime policy decisions.
 * @param runtime Runtime handle
 * @return ZOO_DOMAIN_PROFILE_ENUM Effective domain profile
 */
static ZOO_DOMAIN_PROFILE_ENUM runtime_get_effective_profile(
    ZOO_SMB_RUNTIME_HANDLE runtime)
{
    if (runtime && runtime->options.domain_profile_override_enabled &&
        zoo_domain_profile_is_valid(runtime->options.domain_profile_override))
    {
        return runtime->options.domain_profile_override;
    }

    return zoo_domain_profile_get_active();
}

/**
 * @brief Get current monotonic-ish runtime timestamp in milliseconds.
 * @return uint64_t Timestamp in milliseconds
 */
static uint64_t runtime_now_ms(void)
{
    return (uint64_t)time(NULL) * 1000ULL;
}

/**
 * @brief Find peer slot by peer id.
 * @param runtime Runtime handle
 * @param peer_id Peer id string
 * @return ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT* Slot pointer, NULL if not found
 */
static ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT* find_peer_slot(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    const char* peer_id)
{
    if (!runtime || !peer_id)
    {
        return NULL;
    }

    for (ZOO_USIZE i = 0; i < ZOO_SMB_RUNTIME_PEER_CAPACITY; ++i)
    {
        if (runtime->peers[i].peer_id[0] != '\0' && strcmp(runtime->peers[i].peer_id, peer_id) == 0)
        {
            return &runtime->peers[i];
        }
    }

    return NULL;
}

/**
 * @brief Find or create peer slot by peer id.
 * @param runtime Runtime handle
 * @param peer_id Peer id string
 * @return ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT* Slot pointer, NULL if unavailable
 */
static ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT* find_or_create_peer_slot(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    const char* peer_id)
{
    ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT* slot = find_peer_slot(runtime, peer_id);
    if (slot)
    {
        return slot;
    }

    for (ZOO_USIZE i = 0; i < ZOO_SMB_RUNTIME_PEER_CAPACITY; ++i)
    {
        if (runtime->peers[i].peer_id[0] == '\0')
        {
            snprintf(runtime->peers[i].peer_id, sizeof(runtime->peers[i].peer_id), "%s", peer_id);
            runtime->peers[i].peer_id[sizeof(runtime->peers[i].peer_id) - 1] = '\0';
            runtime->peers[i].healthy = ZOO_FALSE;
            runtime->peers[i].peer_epoch = 0;
            runtime->peers[i].last_heartbeat_ms = 0;
            return &runtime->peers[i];
        }
    }

    return NULL;
}

/**
 * @brief Promote runtime to PRIMARY and bump epoch.
 * @param runtime Runtime handle
 * @param reason Promotion reason string
 * @return void
 */
static void promote_to_primary(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    const char* reason)
{
    runtime->role = ZOO_SMB_RUNTIME_ROLE_PRIMARY;
    runtime->epoch++;
    ZOO_LOG_WARN("Runtime promoted to PRIMARY, epoch=%llu, reason=%s",
                     (unsigned long long)runtime->epoch,
                     reason ? reason : "none");
}

/**
 * @brief HA monitor task for peer timeout detection and role transitions.
 * @param user_data Runtime handle
 * @param argument Unused argument
 * @return ZOO_ERROR_TYPE Task result
 */
static ZOO_ERROR_TYPE runtime_ha_monitor_task(void* user_data, void* argument)
{
    ZOO_SMB_UNUSED(argument);
    ZOO_SMB_RUNTIME_HANDLE runtime = (ZOO_SMB_RUNTIME_HANDLE)user_data;
    if (!runtime)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    while (runtime->ha_monitor_running)
    {
        uint64_t now_ms = runtime_now_ms();
        ZOO_BOOL any_healthy_peer = ZOO_FALSE;

        ZOO_MUTEX_LOCK(&runtime->lock);
        for (ZOO_USIZE i = 0; i < ZOO_SMB_RUNTIME_PEER_CAPACITY; ++i)
        {
            ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT* peer = &runtime->peers[i];
            if (peer->peer_id[0] == '\0')
            {
                continue;
            }

            if (peer->healthy)
            {
                uint64_t age_ms = (now_ms > peer->last_heartbeat_ms) ? (now_ms - peer->last_heartbeat_ms) : 0;
                if (age_ms > runtime->options.failover_timeout_ms)
                {
                    peer->healthy = ZOO_FALSE;
                    ZOO_LOG_WARN("HA peer timeout detected: peer=%s age_ms=%llu",
                                     peer->peer_id,
                                     (unsigned long long)age_ms);
                }
                else
                {
                    any_healthy_peer = ZOO_TRUE;
                }
            }
        }

        if (runtime->options.enable_ha && !any_healthy_peer && runtime->role != ZOO_SMB_RUNTIME_ROLE_PRIMARY)
        {
            promote_to_primary(runtime, "peer timeout monitor");
            runtime->state = ZOO_SMB_RUNTIME_STATE_DEGRADED;
        }
        else if (runtime->options.enable_ha && any_healthy_peer && runtime->started)
        {
            if (runtime->state == ZOO_SMB_RUNTIME_STATE_DEGRADED)
            {
                runtime->state = ZOO_SMB_RUNTIME_STATE_RUNNING;
            }
        }

        ZOO_MUTEX_UNLOCK(&runtime->lock);
        ZOO_SMB_SLEEP_MS(runtime->options.heartbeat_interval_ms > 0 ? runtime->options.heartbeat_interval_ms : 1000);
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Notify registered observers about a runtime state change.
 * @param runtime Runtime handle
 * @param old_state Previous runtime state
 * @param new_state New runtime state
 * @return void
 */
static void notify_state_change(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    ZOO_SMB_RUNTIME_STATE_ENUM old_state,
    ZOO_SMB_RUNTIME_STATE_ENUM new_state)
{
    ZOO_SMB_RUNTIME_STATE_OBSERVER local_observers[ZOO_SMB_RUNTIME_OBSERVER_CAPACITY] = {0};
    void* local_user_data[ZOO_SMB_RUNTIME_OBSERVER_CAPACITY] = {0};
    ZOO_USIZE local_count = 0;

    ZOO_MUTEX_LOCK(&runtime->lock);
    for (ZOO_USIZE i = 0; i < ZOO_SMB_RUNTIME_OBSERVER_CAPACITY; ++i)
    {
        if (runtime->observers[i].observer)
        {
            local_observers[local_count] = runtime->observers[i].observer;
            local_user_data[local_count] = runtime->observers[i].user_data;
            local_count++;
        }
    }
    ZOO_MUTEX_UNLOCK(&runtime->lock);

    for (ZOO_USIZE i = 0; i < local_count; ++i)
    {
        local_observers[i](runtime, old_state, new_state, local_user_data[i]);
    }
}

/**
 * @brief Update runtime state and emit observer notifications when changed.
 * @param runtime Runtime handle
 * @param new_state Target runtime state
 * @return void
 */
static void set_runtime_state(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    ZOO_SMB_RUNTIME_STATE_ENUM new_state)
{
    ZOO_SMB_RUNTIME_STATE_ENUM old_state;

    ZOO_MUTEX_LOCK(&runtime->lock);
    old_state = runtime->state;
    if (old_state == new_state)
    {
        ZOO_MUTEX_UNLOCK(&runtime->lock);
        return;
    }
    runtime->state = new_state;
    ZOO_MUTEX_UNLOCK(&runtime->lock);

    notify_state_change(runtime, old_state, new_state);
}

/**
 * @brief Create runtime instance with default runtime options.
 * @param config SMB configuration pointer
 * @return ZOO_SMB_RUNTIME_HANDLE Runtime handle, NULL on failure
 */
ZOO_SMB_RUNTIME_HANDLE zoo_smb_runtime_create(const ZOO_SMB_CONFIG_STRUCT* config)
{
    return zoo_smb_runtime_create_ex(config, NULL);
}

/**
 * @brief Create runtime instance with explicit runtime options.
 * @param config SMB configuration pointer
 * @param options Runtime options pointer, NULL to use defaults
 * @return ZOO_SMB_RUNTIME_HANDLE Runtime handle, NULL on failure
 */
ZOO_SMB_RUNTIME_HANDLE zoo_smb_runtime_create_ex(
    const ZOO_SMB_CONFIG_STRUCT* config,
    const ZOO_SMB_RUNTIME_OPTIONS_STRUCT* options)
{
    ZOO_SMB_RUNTIME_STRUCT* runtime = (ZOO_SMB_RUNTIME_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_RUNTIME_STRUCT));
    if (!runtime)
    {
        return NULL;
    }

    memset(runtime, 0, sizeof(ZOO_SMB_RUNTIME_STRUCT));
    if (!ZOO_MUTEX_INIT(&runtime->lock))
    {
        zoo_free_to_pool(runtime);
        return NULL;
    }

    runtime->config = config;
    runtime->bus = zoo_smb_get_instance();
    runtime->started = ZOO_FALSE;
    runtime->state = ZOO_SMB_RUNTIME_STATE_CREATED;
    runtime->role = ZOO_SMB_RUNTIME_ROLE_STANDALONE;
    runtime->epoch = 1;

    runtime->options.enable_ha = ZOO_FALSE;
    runtime->options.heartbeat_interval_ms = 1000;
    runtime->options.failover_timeout_ms = 3000;
    runtime->options.local_wire_major = 1U;
    runtime->options.local_wire_minor = 0U;
    runtime->options.peer_wire_major = 1U;
    runtime->options.peer_wire_minor = 0U;
    runtime->options.domain_profile_override_enabled = ZOO_FALSE;
    runtime->options.domain_profile_override = ZOO_DOMAIN_PROFILE_GENERIC;
    memset(runtime->options.local_peer_id, 0, sizeof(runtime->options.local_peer_id));
    runtime->ha_monitor_running = ZOO_FALSE;

    if (options)
    {
        runtime->options = *options;
        if (runtime->options.local_wire_major == 0U)
        {
            runtime->options.local_wire_major = 1U;
        }
        if (runtime->options.peer_wire_major == 0U)
        {
            runtime->options.peer_wire_major = 1U;
        }
        if (runtime->options.domain_profile_override_enabled &&
            !zoo_domain_profile_is_valid(runtime->options.domain_profile_override))
        {
            runtime->options.domain_profile_override_enabled = ZOO_FALSE;
            runtime->options.domain_profile_override = ZOO_DOMAIN_PROFILE_GENERIC;
        }
    }
    else
    {
        snprintf(runtime->options.local_peer_id, sizeof(runtime->options.local_peer_id), "local");
    }

    if (runtime->options.enable_ha)
    {
        runtime->role = ZOO_SMB_RUNTIME_ROLE_SECONDARY;
    }

    return runtime;
}

/**
 * @brief Start runtime instance.
 * @param runtime Runtime handle
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_runtime_start(ZOO_SMB_RUNTIME_HANDLE runtime)
{
    if (!runtime)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (runtime->started)
    {
        return ZOO_SMB_OK;
    }

    set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_STARTING);

    runtime->bus = zoo_smb_get_instance();
    if (!runtime->bus || !zoo_smb_is_ready())
    {
        set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_FAULTED);
        return ZOO_SMB_ERROR_NOT_INITIALIZED;
    }

    ZOO_DOMAIN_PROFILE_ENUM active_profile = runtime_get_effective_profile(runtime);
    ZOO_ASSURANCE_POLICY_SNAPSHOT_STRUCT policy_snapshot;
    ZOO_ASSURANCE_STARTUP_CONTEXT_STRUCT assurance_context;
    memset(&assurance_context, 0, sizeof(assurance_context));
    assurance_context.domain_profile = active_profile;
    assurance_context.protocol.local_wire_major = runtime->options.local_wire_major;
    assurance_context.protocol.local_wire_minor = runtime->options.local_wire_minor;
    assurance_context.protocol.peer_wire_major = runtime->options.peer_wire_major;
    assurance_context.protocol.peer_wire_minor = runtime->options.peer_wire_minor;

    if (zoo_assurance_resolve_policy(active_profile, &policy_snapshot) != ZOO_SMB_OK)
    {
        set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_FAULTED);
        return ZOO_SMB_ERROR_OPERATION_FAILED;
    }

    ZOO_LOG_INFO("Runtime assurance policy: profile=%s decision=%u class=%u partition=%u",
                 zoo_domain_profile_to_string(active_profile),
                 (unsigned)policy_snapshot.startup_decision,
                 (unsigned)policy_snapshot.assurance_class,
                 (unsigned)policy_snapshot.partition_id);

    if (zoo_domain_policy_requires_identity(active_profile) &&
        runtime->options.local_peer_id[0] == '\0')
    {
        set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_FAULTED);
        ZOO_LOG_ERROR("Runtime startup blocked: identity required for profile=%s",
                      zoo_domain_profile_to_string(active_profile));
        return ZOO_SMB_ERROR_INVALID_STATE;
    }

    ZOO_ERROR_TYPE assurance_result = zoo_assurance_evaluate_startup(&assurance_context);
    if (assurance_result != ZOO_SMB_OK)
    {
        set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_FAULTED);
        ZOO_LOG_ERROR("Runtime startup blocked by assurance mesh: profile=%s local=%u.%u peer=%u.%u err=%d",
                      zoo_domain_profile_to_string(active_profile),
                      (unsigned)assurance_context.protocol.local_wire_major,
                      (unsigned)assurance_context.protocol.local_wire_minor,
                      (unsigned)assurance_context.protocol.peer_wire_major,
                      (unsigned)assurance_context.protocol.peer_wire_minor,
                      assurance_result);
        return assurance_result;
    }

    runtime->started = ZOO_TRUE;
    set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_RUNNING);

    /* Start HA monitor task if HA is enabled */
    if (runtime->options.enable_ha && !runtime->ha_monitor_running)
    {
        runtime->ha_monitor_running = ZOO_TRUE;

        ZOO_ERROR_TYPE thread_result = zoo_thread_pool_submit_task_ex(
            "runtime_ha_monitor",
            runtime_ha_monitor_task,
            (void*)runtime,
            NULL,
            NULL,
            NULL,
            NULL,
            ZOO_FALSE);
        if (thread_result != ZOO_OK)
        {
            runtime->ha_monitor_running = ZOO_FALSE;
            ZOO_LOG_WARN("Failed to submit HA monitor task, result=%d", thread_result);
        }
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Stop runtime instance.
 * @param runtime Runtime handle
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_runtime_stop(ZOO_SMB_RUNTIME_HANDLE runtime)
{
    if (!runtime)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_STOPPING);
    
    /* Stop HA monitor task */
    if (runtime->ha_monitor_running)
    {
        runtime->ha_monitor_running = ZOO_FALSE;
        /* Give task time to exit gracefully */
        ZOO_SLEEP_MS(100);
    }
    
    runtime->started = ZOO_FALSE;
    set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_STOPPED);
    return ZOO_SMB_OK;
}

/**
 * @brief Destroy runtime instance and release owned resources.
 * @param runtime Runtime handle to destroy
 * @return void
 */
void zoo_smb_runtime_destroy(ZOO_SMB_RUNTIME_HANDLE runtime)
{
    if (!runtime)
    {
        return;
    }

    (void)zoo_smb_runtime_stop(runtime);
    ZOO_MUTEX_DESTROY(&runtime->lock);
    zoo_free_to_pool(runtime);
}

/**
 * @brief Get SMB bus handle associated with runtime.
 * @param runtime Runtime handle
 * @return ZOO_SMB_HANDLE SMB bus handle, NULL on failure
 */
ZOO_SMB_HANDLE zoo_smb_runtime_get_bus(ZOO_SMB_RUNTIME_HANDLE runtime)
{
    if (!runtime)
    {
        return NULL;
    }

    return runtime->bus;
}

/**
 * @brief Get current runtime state.
 * @param runtime Runtime handle
 * @return ZOO_SMB_RUNTIME_STATE_ENUM Current runtime state
 */
ZOO_SMB_RUNTIME_STATE_ENUM zoo_smb_runtime_get_state(ZOO_SMB_RUNTIME_HANDLE runtime)
{
    if (!runtime)
    {
        return ZOO_SMB_RUNTIME_STATE_FAULTED;
    }

    return runtime->state;
}

/**
 * @brief Get current runtime role.
 * @param runtime Runtime handle
 * @return ZOO_SMB_RUNTIME_ROLE_ENUM Current runtime role
 */
ZOO_SMB_RUNTIME_ROLE_ENUM zoo_smb_runtime_get_role(ZOO_SMB_RUNTIME_HANDLE runtime)
{
    if (!runtime)
    {
        return ZOO_SMB_RUNTIME_ROLE_STANDALONE;
    }

    return runtime->role;
}

/**
 * @brief Get current runtime epoch.
 * @param runtime Runtime handle
 * @return uint64_t Current runtime epoch value
 */
uint64_t zoo_smb_runtime_get_epoch(ZOO_SMB_RUNTIME_HANDLE runtime)
{
    if (!runtime)
    {
        return 0;
    }

    return runtime->epoch;
}

/**
 * @brief Set runtime role explicitly.
 * @param runtime Runtime handle
 * @param role Target runtime role
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_runtime_set_role(ZOO_SMB_RUNTIME_HANDLE runtime, ZOO_SMB_RUNTIME_ROLE_ENUM role)
{
    if (!runtime || !is_valid_role(role))
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_MUTEX_LOCK(&runtime->lock);
    runtime->role = role;
    ZOO_MUTEX_UNLOCK(&runtime->lock);
    return ZOO_SMB_OK;
}

/**
 * @brief Register a runtime state observer.
 * @param runtime Runtime handle
 * @param observer Observer callback
 * @param user_data User context pointer
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_runtime_register_state_observer(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    ZOO_SMB_RUNTIME_STATE_OBSERVER observer,
    void* user_data)
{
    if (!runtime || !observer)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_MUTEX_LOCK(&runtime->lock);

    for (ZOO_USIZE i = 0; i < ZOO_SMB_RUNTIME_OBSERVER_CAPACITY; ++i)
    {
        if (runtime->observers[i].observer == observer && runtime->observers[i].user_data == user_data)
        {
            ZOO_MUTEX_UNLOCK(&runtime->lock);
            return ZOO_SMB_ERROR_ALREADY_EXISTS;
        }
    }

    for (ZOO_USIZE i = 0; i < ZOO_SMB_RUNTIME_OBSERVER_CAPACITY; ++i)
    {
        if (!runtime->observers[i].observer)
        {
            runtime->observers[i].observer = observer;
            runtime->observers[i].user_data = user_data;
            ZOO_MUTEX_UNLOCK(&runtime->lock);
            return ZOO_SMB_OK;
        }
    }

    ZOO_MUTEX_UNLOCK(&runtime->lock);
    return ZOO_SMB_ERROR_LIST_FULL;
}

/**
 * @brief Unregister a runtime state observer.
 * @param runtime Runtime handle
 * @param observer Observer callback
 * @param user_data User context pointer
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_runtime_unregister_state_observer(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    ZOO_SMB_RUNTIME_STATE_OBSERVER observer,
    void* user_data)
{
    if (!runtime || !observer)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_MUTEX_LOCK(&runtime->lock);
    for (ZOO_USIZE i = 0; i < ZOO_SMB_RUNTIME_OBSERVER_CAPACITY; ++i)
    {
        if (runtime->observers[i].observer == observer && runtime->observers[i].user_data == user_data)
        {
            runtime->observers[i].observer = NULL;
            runtime->observers[i].user_data = NULL;
            ZOO_MUTEX_UNLOCK(&runtime->lock);
            return ZOO_SMB_OK;
        }
    }
    ZOO_MUTEX_UNLOCK(&runtime->lock);
    return ZOO_SMB_ERROR_NOT_FOUND;
}

/**
 * @brief Report peer heartbeat to runtime HA logic.
 * @param runtime Runtime handle
 * @param peer_id Peer identifier string
 * @param peer_epoch Peer epoch value
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_runtime_report_peer_heartbeat(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    const char* peer_id,
    uint64_t peer_epoch)
{
    ZOO_DOMAIN_PROFILE_ENUM active_profile;
    ZOO_ASSURANCE_ADMISSION_CONTEXT_STRUCT admission_context;
    ZOO_ERROR_TYPE admission_result;

    if (!runtime || !peer_id)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (!runtime->options.enable_ha)
    {
        return ZOO_SMB_ERROR_INVALID_STATE;
    }

    active_profile = runtime_get_effective_profile(runtime);
    memset(&admission_context, 0, sizeof(admission_context));
    admission_context.domain_profile = active_profile;
    admission_context.local_peer_id = runtime->options.local_peer_id;
    admission_context.remote_peer_id = peer_id;
    admission_context.partition_id = zoo_domain_policy_get_partition_id(active_profile);
    admission_result = zoo_assurance_evaluate_admission(&admission_context);
    if (admission_result != ZOO_SMB_OK)
    {
        ZOO_LOG_WARN("Runtime heartbeat admission blocked: profile=%s peer=%s err=%d",
                     zoo_domain_profile_to_string(active_profile),
                     peer_id,
                     admission_result);
        return admission_result;
    }

    ZOO_MUTEX_LOCK(&runtime->lock);
    ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT* slot = find_or_create_peer_slot(runtime, peer_id);
    if (!slot)
    {
        ZOO_MUTEX_UNLOCK(&runtime->lock);
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    slot->peer_epoch = peer_epoch;
    slot->last_heartbeat_ms = runtime_now_ms();
    slot->healthy = ZOO_TRUE;

    if (peer_epoch >= runtime->epoch)
    {
        runtime->epoch = peer_epoch;
        runtime->role = ZOO_SMB_RUNTIME_ROLE_SECONDARY;
        if (runtime->started && runtime->state == ZOO_SMB_RUNTIME_STATE_DEGRADED)
        {
            runtime->state = ZOO_SMB_RUNTIME_STATE_RUNNING;
        }
    }
    ZOO_MUTEX_UNLOCK(&runtime->lock);
    return ZOO_SMB_OK;
}

/**
 * @brief Report peer timeout to runtime HA logic.
 * @param runtime Runtime handle
 * @param peer_id Peer identifier string
 * @param peer_epoch Peer epoch value
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_runtime_report_peer_timeout(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    const char* peer_id,
    uint64_t peer_epoch)
{
    ZOO_SMB_UNUSED(peer_epoch);
    if (!runtime || !peer_id)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (!runtime->options.enable_ha)
    {
        return ZOO_SMB_ERROR_INVALID_STATE;
    }

    ZOO_MUTEX_LOCK(&runtime->lock);
    if (runtime->role != ZOO_SMB_RUNTIME_ROLE_PRIMARY)
    {
        runtime->role = ZOO_SMB_RUNTIME_ROLE_PRIMARY;
        runtime->epoch++;
        ZOO_MUTEX_UNLOCK(&runtime->lock);
        set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_DEGRADED);
        ZOO_LOG_WARN("Runtime failover promoted to PRIMARY, epoch=%llu", (unsigned long long)runtime->epoch);
        return ZOO_SMB_OK;
    }
    ZOO_MUTEX_UNLOCK(&runtime->lock);
    return ZOO_SMB_OK;
}
/**
 * @brief Trigger explicit HA election and promote local runtime to PRIMARY.
 * @param runtime Runtime handle
 * @param reason Election reason string (optional)
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_runtime_trigger_election(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    const char* reason)
{
    if (!runtime)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (!runtime->options.enable_ha)
    {
        return ZOO_SMB_ERROR_INVALID_STATE;
    }

    ZOO_MUTEX_LOCK(&runtime->lock);
    
    /* Promote to PRIMARY if not already */
    if (runtime->role != ZOO_SMB_RUNTIME_ROLE_PRIMARY)
    {
        runtime->role = ZOO_SMB_RUNTIME_ROLE_PRIMARY;
        runtime->epoch++;
    }

    ZOO_MUTEX_UNLOCK(&runtime->lock);
    
    set_runtime_state(runtime, ZOO_SMB_RUNTIME_STATE_RUNNING);
    
    ZOO_LOG_INFO("Runtime election triggered, promoted to PRIMARY, epoch=%llu, reason=%s",
                 (unsigned long long)runtime->epoch,
                 reason ? reason : "manual");

    return ZOO_SMB_OK;
}

/**
 * @brief Query peer status snapshot by peer id.
 * @param runtime Runtime handle
 * @param peer_id Peer identifier
 * @param out_status Output peer status snapshot
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
ZOO_ERROR_TYPE zoo_smb_runtime_get_peer_status(
    ZOO_SMB_RUNTIME_HANDLE runtime,
    const char* peer_id,
    ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT* out_status)
{
    if (!runtime || !peer_id || !out_status)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_MUTEX_LOCK(&runtime->lock);

    ZOO_SMB_RUNTIME_PEER_SLOT_STRUCT* slot = find_peer_slot(runtime, peer_id);
    if (!slot)
    {
        ZOO_MUTEX_UNLOCK(&runtime->lock);
        return ZOO_SMB_ERROR_NOT_FOUND;
    }

    out_status->peer_epoch = slot->peer_epoch;
    out_status->last_heartbeat_ms = slot->last_heartbeat_ms;
    out_status->healthy = slot->healthy;
    memcpy(out_status->peer_id, slot->peer_id, sizeof(out_status->peer_id));
    out_status->peer_id[sizeof(out_status->peer_id) - 1] = '\0';

    ZOO_MUTEX_UNLOCK(&runtime->lock);

    return ZOO_SMB_OK;
}