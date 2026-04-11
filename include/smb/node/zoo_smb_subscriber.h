/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_NODE
 * File name: zoo_smb_subscriber.h
 * Description: Subscriber node interface for ZOO Soft Message Bus (SMB)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_SMB_SUBSCRIBER_H
#define ZOO_SMB_SUBSCRIBER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include "zoo_smb_error.h"
#include "zoo_smb_types.h"
#include "zoo_smb_qos.h"
    typedef struct ZOO_SMB_SUBSCRIBER_STRUCT* ZOO_SMB_SUBSCRIBER_HANDLE;

    /**
     * @brief Create a new subscriber node.
     *
     * This function is used to create a new subscriber node on the Soft Message Bus.
     * The created node can be used to subscribe to messages on specific topics.
     *
     * @param name A pointer to a null-terminated string representing the name of the subscriber node.
     * @return ZOO_SMB_SUBSCRIBER_HANDLE A handle to the newly created subscriber node.
     *         Returns NULL if the node creation fails.
     */
    ZOO_SMB_SUBSCRIBER_HANDLE zoo_smb_create_subscriber(
        IN const char* name,
        IN const char* target,
        IN const char* topic,
        IN const ZOO_SMB_QOS_POLICY_STRUCT* policy);

    /**
     * @brief Destroys a subscriber node and releases associated resources
     *
     * This function cleans up and deallocates all resources associated with the given
     * subscriber node handle. After calling this function, the node handle becomes invalid
     * and should not be used.
     *
     * @param[in] subscriber Handle to the subscriber node to be destroyed
     *
     * @note The caller is responsible for ensuring no other operations are performed on the
     *       node after calling this function
     */
    void zoo_smb_destroy_subscriber(IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber);

    /**
     * @brief Registers or reactivates subscription intent for a message id.
     *
     * The API accepts intent immediately and triggers asynchronous SUB/SUBACK
     * reconciliation in background worker flow.
     *
     * @param subscriber Subscriber handle.
     * @param msg_id Message identifier to receive.
     * @param message_handler Callback invoked on matching PUB messages.
     * @param user_data User context passed to message_handler.
     * @param handle Output stable session handle used by unsubscribe.
     * @return ZOO_SMB_OK when intent is accepted; otherwise an error code.
     */
    ZOO_ERROR_TYPE zoo_smb_subscribe_message(
        IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber,
        IN uint32_t msg_id,
        IN ZOO_SMB_MSG_HANDLER message_handler,
        IN void* user_data,
        OUT int32_t* handle);

    /**
     * @brief Cancels and removes one subscription session.
     *
     * @param subscriber Subscriber handle.
     * @param handle Session handle returned by zoo_smb_subscribe_message().
     */
    void zoo_smb_unsubscribe_message(IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber, IN int32_t handle);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_SUBSCRIBER_H */
