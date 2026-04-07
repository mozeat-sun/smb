/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_SHM
 * File name: zoo_smb_transport_shm.c
 * Description: Shared Memory Transport Main Interface
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-23     weiwang.sun       Created
 * 2.0       2025-06-23     weiwang.sun       Modularized with sync interface
 ******************************************************************************/

#include "zoo_smb_transport_shm.h"
#include "zoo_smb_transport_shm_client.h"
#include "zoo_smb_transport_shm_server.h"
#include "../../log/inc/zoo_log.h"

// ==============================================================================
// MAIN TRANSPORT OPERATIONS
// ==============================================================================

/**
 * @brief Initialize SHM transport based on mode
 */
static ZOO_ERROR_TYPE shm_init(ZOO_SMB_TRANSPORT_STRUCT* transport)
{
    ZOO_SMB_VALIDATE_PTR(transport, ZOO_SMB_ERROR_INVALID_PARAM);
    ZOO_SMB_VALIDATE_PTR(transport->config, ZOO_SMB_ERROR_INVALID_PARAM);

    if (transport->config->is_server)
    {
        return shm_server_init(transport);
    }
    else
    {
        return shm_client_init(transport);
    }
}

/**
 * @brief Destroy SHM transport
 */
static void shm_destroy(void* impl_ptr)
{
    if (!impl_ptr)
        return;

    // Check if it's server or client based on implementation structure
    // We can determine this by checking the first field or using a flag
    ZOO_SMB_SHM_COMMON_IMPL* common = (ZOO_SMB_SHM_COMMON_IMPL*)impl_ptr;

    if (common->is_server)
    {
        shm_server_destroy(impl_ptr);
    }
    else
    {
        shm_client_destroy(impl_ptr);
    }
}

/**
 * @brief Start SHM transport (blocking)
 */
static ZOO_ERROR_TYPE shm_start(void* impl_ptr)
{
    ZOO_SMB_VALIDATE_PTR(impl_ptr, ZOO_SMB_ERROR_INVALID_PARAM);

    ZOO_SMB_SHM_COMMON_IMPL* common = (ZOO_SMB_SHM_COMMON_IMPL*)impl_ptr;

    if (common->is_server)
    {
        return shm_server_start(impl_ptr);
    }
    else
    {
        return shm_client_start(impl_ptr);
    }
}

/**
 * @brief Stop SHM transport
 */
static ZOO_ERROR_TYPE shm_stop(void* impl_ptr)
{
    ZOO_SMB_VALIDATE_PTR(impl_ptr, ZOO_SMB_ERROR_INVALID_PARAM);

    ZOO_SMB_SHM_COMMON_IMPL* common = (ZOO_SMB_SHM_COMMON_IMPL*)impl_ptr;

    if (common->is_server)
    {
        return shm_server_stop(impl_ptr);
    }
    else
    {
        return shm_client_stop(impl_ptr);
    }
}

/**
 * @brief Send message through SHM transport
 */
static ZOO_ERROR_TYPE shm_send(void* impl_ptr, const ZOO_SMB_MSG_STRUCT* msg, const char* receiver)
{
    ZOO_SMB_VALIDATE_PTR(impl_ptr, ZOO_SMB_ERROR_INVALID_PARAM);

    ZOO_SMB_SHM_COMMON_IMPL* common = (ZOO_SMB_SHM_COMMON_IMPL*)impl_ptr;

    if (common->is_server)
    {
        return shm_server_send(impl_ptr, msg, receiver);
    }
    else
    {
        return shm_client_send(impl_ptr, msg, receiver);
    }
}

/**
 * @brief Check if SHM transport is started
 */
static ZOO_BOOL shm_is_started(void* impl_ptr)
{
    if (!impl_ptr)
        return ZOO_FALSE;

    ZOO_SMB_SHM_COMMON_IMPL* common = (ZOO_SMB_SHM_COMMON_IMPL*)impl_ptr;

    if (common->is_server)
    {
        return shm_server_is_started(impl_ptr);
    }
    else
    {
        return shm_client_is_started(impl_ptr);
    }
}

// ==============================================================================
// PUBLIC INTERFACE IMPLEMENTATION
// ==============================================================================

/**
 * @brief Initialize and register SHM transport with the transport registry
 */
void zoo_smb_transport_shm_init(void)
{
    static ZOO_SMB_TRANSPORT_OPS_STRUCT shm_ops = {
        .init = shm_init,
        .destroy = shm_destroy,
        .start = shm_start,
        .stop = shm_stop,
        .send = shm_send,
        .is_started = shm_is_started};

    zoo_smb_transport_register(ZOO_SMB_TRANSPORT_TYPE_SHM, &shm_ops);
    ZOO_LOG_INFO("SHM transport registered successfully");
}