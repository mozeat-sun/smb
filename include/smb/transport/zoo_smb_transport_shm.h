/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_SHM
 * File name: zoo_smb_transport_shm.h
 * Description: Shared Memory Transport Implementation for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-23     weiwang.sun       Created
 * 2.0       2025-06-23     weiwang.sun       Simplified interface
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_SHM_H
#define ZOO_SMB_TRANSPORT_SHM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_transport.h"

    // ==============================================================================
    // PUBLIC INTERFACE
    // ==============================================================================

    /**
     * @brief Initialize and register SHM transport with the transport registry
     *
     * This is the only public interface for the SHM transport layer.
     * Call this function once during application initialization to make
     * SHM transport available for use.
     *
     * @note This function registers the SHM transport implementation with
     *       the transport layer, making it available for creation via
     *       zoo_smb_create_transport() with type ZOO_SMB_TRANSPORT_TYPE_SHM.
     */
    void zoo_smb_transport_shm_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_SHM_H */