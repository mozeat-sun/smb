/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_NODE
 * File name: zoo_smb_subsciber.h
 * Description: subsciber node header file
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
     * @brief Subscribe to a message topic.
     *
     * This function allows a subscriber node to subscribe to a specific message topic.
     * When a message is published on the specified topic, the provided event handler will be called.
     *
     * @param subscriber A handle to the subscriber node that wants to subscribe to the topic.
     * @param topic A pointer to a null-terminated string representing the topic to subscribe to.
     * @param handler A pointer to the event handler function that will be called when a message is received.
     * @param user_data A pointer to user-defined data that will be passed to the event handler.
     * @param handle A pointer to an unsigned 32-bit integer where the subscription handle will be stored.
     *               This handle can be used later to unsubscribe from the topic.
     */
    ZOO_ERROR_TYPE zoo_smb_subscribe_message(
        IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber,
        IN uint32_t msg_id,
        IN ZOO_SMB_MSG_HANDLER message_handler,
        IN void* user_data,
        OUT int32_t* handle);

    /**
     * @brief Unsubscribe from a message topic.
     *
     * This function allows a subscriber node to unsubscribe from a previously subscribed topic.
     * The subscription is identified by the provided handle.
     *
     * @param node A handle to the subscriber node that wants to unsubscribe.
     * @param handle A pointer to an integer representing the subscription handle.
     */
    void zoo_smb_unsubscribe_message(IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber, IN int32_t handle);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_SUBSCRIBER_H */
