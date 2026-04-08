
/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_PORT_MANAGER
 * File name: zoo_smb_port_manager.h
 * Description: Port manager interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-30     weiwang.sun         created
 ******************************************************************************/
#ifndef ZOO_SMB_PORT_MANAGER_H
#define ZOO_SMB_PORT_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#ifndef TRANSPORT_PORT
#define MIN_TRANSPORT_PORT 13800
#define MAX_TRANSPORT_PORT 15800
#endif
#define ZOO_SMB_INVALID_PORT -1
    /**
     * @brief Find an available port within the specified range.
     *
     * This function searches for an available port between MIN_TRANSPORT_PORT and MAX_TRANSPORT_PORT.
     *
     * @return The available port number on success, or -1 on failure.
     */
    int zoo_smb_find_available_port(int min, int max);

    /**
     * @brief Release port resources.
     *
     * This function releases the resources associated with the specified port.
     *
     * @param port The port number to release.
     */
    void zoo_smb_release_port_resources(int port);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_PORT_MANAGER_H */
