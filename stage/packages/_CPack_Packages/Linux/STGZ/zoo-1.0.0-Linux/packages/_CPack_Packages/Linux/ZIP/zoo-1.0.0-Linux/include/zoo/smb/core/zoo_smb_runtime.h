/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_RUNTIME
 * File name: zoo_smb_runtime.h
 * Description: Runtime lifecycle and HA scaffolding interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-03-31     AI Assistant      created
 ******************************************************************************/

#ifndef ZOO_SMB_RUNTIME_H
#define ZOO_SMB_RUNTIME_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb.h"
#include "zoo_smb_config.h"

    /**
     * @defgroup ZOO_SMB_RUNTIME Runtime Module
     * @brief Runtime lifecycle and HA scaffolding APIs for ZOO SMB
     * @{
     */

    /* Forward declaration of runtime handle */
    typedef struct ZOO_SMB_RUNTIME_STRUCT* ZOO_SMB_RUNTIME_HANDLE;

    /**
     * @brief Runtime lifecycle state enumeration
     */
    typedef enum
    {
        ZOO_SMB_RUNTIME_STATE_CREATED = 0,
        ZOO_SMB_RUNTIME_STATE_STARTING,
        ZOO_SMB_RUNTIME_STATE_RUNNING,
        ZOO_SMB_RUNTIME_STATE_DEGRADED,
        ZOO_SMB_RUNTIME_STATE_STOPPING,
        ZOO_SMB_RUNTIME_STATE_STOPPED,
        ZOO_SMB_RUNTIME_STATE_FAULTED
    } ZOO_SMB_RUNTIME_STATE_ENUM;

    /**
     * @brief Runtime HA role enumeration
     */
    typedef enum
    {
        ZOO_SMB_RUNTIME_ROLE_STANDALONE = 0,
        ZOO_SMB_RUNTIME_ROLE_PRIMARY,
        ZOO_SMB_RUNTIME_ROLE_SECONDARY
    } ZOO_SMB_RUNTIME_ROLE_ENUM;

    /**
     * @brief Runtime HA options
     */
    typedef struct
    {
        ZOO_BOOL enable_ha;               /**< Enable HA runtime behavior */
        uint32_t heartbeat_interval_ms;   /**< Peer heartbeat interval in milliseconds */
        uint32_t failover_timeout_ms;     /**< Peer timeout threshold in milliseconds */
        char local_peer_id[32];           /**< Local peer identifier */
    } ZOO_SMB_RUNTIME_OPTIONS_STRUCT;

    /**
     * @brief Runtime HA peer status snapshot
     */
    typedef struct
    {
        char peer_id[32];
        uint64_t peer_epoch;
        uint64_t last_heartbeat_ms;
        ZOO_BOOL healthy;
    } ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT;

    /**
     * @brief Runtime state observer callback type
     * @param runtime Runtime handle
     * @param old_state Previous runtime state
     * @param new_state Current runtime state
     * @param user_data User context pointer
     */
    typedef void (*ZOO_SMB_RUNTIME_STATE_OBSERVER)(
        ZOO_SMB_RUNTIME_HANDLE runtime,
        ZOO_SMB_RUNTIME_STATE_ENUM old_state,
        ZOO_SMB_RUNTIME_STATE_ENUM new_state,
        void* user_data);

    /**
     * @brief Create runtime instance with default runtime options
     * @param config SMB configuration pointer
     * @return ZOO_SMB_RUNTIME_HANDLE Runtime handle, NULL on failure
     */
    ZOO_SMB_RUNTIME_HANDLE zoo_smb_runtime_create(const ZOO_SMB_CONFIG_STRUCT* config);

    /**
     * @brief Create runtime instance with explicit runtime options
     * @param config SMB configuration pointer
     * @param options Runtime options pointer, NULL to use defaults
     * @return ZOO_SMB_RUNTIME_HANDLE Runtime handle, NULL on failure
     */
    ZOO_SMB_RUNTIME_HANDLE zoo_smb_runtime_create_ex(
        const ZOO_SMB_CONFIG_STRUCT* config,
        const ZOO_SMB_RUNTIME_OPTIONS_STRUCT* options);

    /**
     * @brief Start runtime instance
     * @param runtime Runtime handle
     * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_runtime_start(ZOO_SMB_RUNTIME_HANDLE runtime);

    /**
     * @brief Stop runtime instance
     * @param runtime Runtime handle
     * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_runtime_stop(ZOO_SMB_RUNTIME_HANDLE runtime);

    /**
     * @brief Destroy runtime instance
     * @param runtime Runtime handle to destroy
     * @return void
     */
    void zoo_smb_runtime_destroy(ZOO_SMB_RUNTIME_HANDLE runtime);

    /**
     * @brief Get SMB bus handle owned by runtime
     * @param runtime Runtime handle
     * @return ZOO_SMB_HANDLE SMB bus handle, NULL on failure
     */
    ZOO_SMB_HANDLE zoo_smb_runtime_get_bus(ZOO_SMB_RUNTIME_HANDLE runtime);

    /**
     * @brief Get current runtime state
     * @param runtime Runtime handle
     * @return ZOO_SMB_RUNTIME_STATE_ENUM Current runtime state
     */
    ZOO_SMB_RUNTIME_STATE_ENUM zoo_smb_runtime_get_state(ZOO_SMB_RUNTIME_HANDLE runtime);

    /**
     * @brief Get current runtime role
     * @param runtime Runtime handle
     * @return ZOO_SMB_RUNTIME_ROLE_ENUM Current runtime role
     */
    ZOO_SMB_RUNTIME_ROLE_ENUM zoo_smb_runtime_get_role(ZOO_SMB_RUNTIME_HANDLE runtime);

    /**
     * @brief Get current runtime epoch
     * @param runtime Runtime handle
     * @return uint64_t Current runtime epoch
     */
    uint64_t zoo_smb_runtime_get_epoch(ZOO_SMB_RUNTIME_HANDLE runtime);

    /**
     * @brief Set runtime role explicitly
     * @param runtime Runtime handle
     * @param role Target runtime role
     * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_runtime_set_role(ZOO_SMB_RUNTIME_HANDLE runtime, ZOO_SMB_RUNTIME_ROLE_ENUM role);

    /**
     * @brief Register runtime state observer
     * @param runtime Runtime handle
     * @param observer Observer callback
     * @param user_data User context pointer
     * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_runtime_register_state_observer(
        ZOO_SMB_RUNTIME_HANDLE runtime,
        ZOO_SMB_RUNTIME_STATE_OBSERVER observer,
        void* user_data);

    /**
     * @brief Unregister runtime state observer
     * @param runtime Runtime handle
     * @param observer Observer callback
     * @param user_data User context pointer
     * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_runtime_unregister_state_observer(
        ZOO_SMB_RUNTIME_HANDLE runtime,
        ZOO_SMB_RUNTIME_STATE_OBSERVER observer,
        void* user_data);

    /**
     * @brief Report peer heartbeat to runtime HA logic
     * @param runtime Runtime handle
     * @param peer_id Peer identifier string
     * @param peer_epoch Peer epoch value
     * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_runtime_report_peer_heartbeat(
        ZOO_SMB_RUNTIME_HANDLE runtime,
        const char* peer_id,
        uint64_t peer_epoch);

    /**
     * @brief Report peer timeout to runtime HA logic
     * @param runtime Runtime handle
     * @param peer_id Peer identifier string
     * @param peer_epoch Peer epoch value
     * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_runtime_report_peer_timeout(
        ZOO_SMB_RUNTIME_HANDLE runtime,
        const char* peer_id,
        uint64_t peer_epoch);

    /**
     * @brief Trigger explicit HA election and promote local runtime to PRIMARY.
     * @param runtime Runtime handle
     * @param reason Election reason string (optional)
     * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
     */
    ZOO_ERROR_TYPE zoo_smb_runtime_trigger_election(
        ZOO_SMB_RUNTIME_HANDLE runtime,
        const char* reason);

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
        ZOO_SMB_RUNTIME_PEER_STATUS_STRUCT* out_status);

    /** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_RUNTIME_H */
