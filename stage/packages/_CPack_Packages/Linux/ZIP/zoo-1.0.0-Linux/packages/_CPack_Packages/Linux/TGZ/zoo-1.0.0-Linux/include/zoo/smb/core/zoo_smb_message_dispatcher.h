/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_MESSAGE_DISPATCHER
 * File name: zoo_smb_message_dispatcher.h
 * Description: Message dispatcher header (stub)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     assistant         created stub
 ******************************************************************************/

#ifndef ZOO_SMB_MESSAGE_DISPATCHER_H
#define ZOO_SMB_MESSAGE_DISPATCHER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo.h"
#include "zoo_queue.h"
#include "zoo_smb_error.h"

// Forward declarations
typedef struct ZOO_SMB_MESSAGE_DISPATCHER_STRUCT* ZOO_SMB_MESSAGE_DISPATCHER_HANDLE;

// Stub function declarations
ZOO_SMB_MESSAGE_DISPATCHER_HANDLE zoo_smb_create_message_dispatcher(ZOO_QUEUE_HANDLE queue);
void zoo_smb_destroy_message_dispatcher(ZOO_SMB_MESSAGE_DISPATCHER_HANDLE dispatcher);
ZOO_ERROR_TYPE zoo_smb_start_message_dispatcher(ZOO_SMB_MESSAGE_DISPATCHER_HANDLE dispatcher);
ZOO_ERROR_TYPE zoo_smb_stop_message_dispatcher(ZOO_SMB_MESSAGE_DISPATCHER_HANDLE dispatcher);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_MESSAGE_DISPATCHER_H */
