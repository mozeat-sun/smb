/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_TYPE_TCP
 * File name: zoo_smb_transport_tcp.h
 * Description: TCP transport interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_TCP_H
#define ZOO_SMB_TRANSPORT_TCP_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_transport.h"

    /**
     * @brief Initialize the TCP transport module.
     *
     * This function registers the TCP transport implementation with the SMB transport
     * framework, making TCP available as a transport option for SMB communication.
     */
    void zoo_smb_transport_tcp_init(void);

#ifdef __cplusplus
}
#endif
#endif /* ZOO_SMB_TRANSPORT_TCP_H */