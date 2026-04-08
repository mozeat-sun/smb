/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TYPE_UDP_BROADCAST
 * File name: zoo_smb_transport_udp_broadcast.h
 * Description: UDP broadcast transport interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     weiwang.su         created
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_UDP_BROADCAST_H
#define ZOO_SMB_TRANSPORT_UDP_BROADCAST_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_transport.h"

    /**
     * @brief Initialize the UDP broadcast transport module.
     *
     * This function registers the UDP broadcast transport implementation with the SMB transport
     * framework, making UDP broadcast available as a transport option for SMB communication.
     */
    void zoo_smb_transport_udp_broadcast_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_UDP_BROADCAST_H */