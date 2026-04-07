/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: zoo_smb_
 * File name: zoo_smb_subscriber.c
 * Description: Implementation of subscriber node for ZOO Soft Message Bus (SMB)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-28     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_subscriber.h"
#include "zoo_smb_node.h"
#include "zoo_smb_qos.h"
#include "zoo_smb.h"
#include "zoo.h"
#include <string.h>

typedef struct USER_OBSERVER_STRUCT
{
    void* usr_data;
    uint32_t msg_id;
    int32_t id;
    ZOO_SMB_MSG_HANDLER handler;
} USER_OBSERVER_STRUCT;

typedef struct ZOO_SMB_SUBSCRIBER_STRUCT
{
    int32_t id[2];
    ZOO_SMB_NODE_HANDLE node;
    ZOO_LIST_HANDLE observers;
    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity; /**< List of QoS entities associated with */
    ZOO_MUTEX_T mutex;                    /**< Mutex for thread safety */
} ZOO_SMB_SUBSCRIBER_STRUCT;

/**
 * @brief Callback function invoked when an observer notification is received by the subscriber.
 *
 * This function handles notifications sent to the subscriber by an observer.
 * It is typically used to process updates or events that the observer wants to communicate.
 *
 * @param ... (Specify the parameters of the function here, if known)
 *
 * @note Fill in parameter and return value details as appropriate.
 */
static void subscriber_on_notify_observer(
    IN const USER_OBSERVER_STRUCT* observer,
    IN const ZOO_SMB_MSG_STRUCT* message)
{
    if (observer == NULL || message == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: observer=%p, msg=%p", observer, message);
        return;
    }

    // Notify the observer with the message
    if (observer->handler != NULL && message->header.msg_id == observer->msg_id)
    {
        observer->handler(
            observer->usr_data, message->header.msg_id, message->header.request_id, message->header.timestamp, message->header.sender, message->payload, message->header.payload_size);
    }

    return;
}

/**
 * @brief Callback function to handle PublishAck messages received by the subscriber.
 *
 * This function is invoked when a PublishAck message is received. It processes
 * the message and performs any necessary actions based on its contents.
 *
 * @param context Pointer to user-defined context or state information.
 * @param msg Pointer to the received message data.
 * @param msg_len Length of the received message in bytes.
 */
static void subscriber_on_handle_SUBACK_msg_cb(IN void* context, IN const void* msg, IN size_t msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p, msg_len=%zu", msg, context, msg_len);
        return;
    }
    ZOO_SMB_SUBSCRIBER_STRUCT* subscriber = (ZOO_SMB_SUBSCRIBER_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;

    ZOO_LOG_INFO("Received SUBACK message for subscriber: %s, request_id=%llu",
                     subscriber->node->name,
                     message->header.request_id);
    if (subscriber->qos_entity)
    {
        ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(subscriber->qos_entity);
        ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT negotiation_result = {ZOO_TRUE, 0};
        if (message->payload && message->header.payload_size >= sizeof(ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT))
        {
            memcpy(&negotiation_result, message->payload, sizeof(ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT));
        }

        (void)zoo_smb_qos_ctx_set_match_result(
            qos_ctx,
            message->header.request_id,
            negotiation_result.matched,
            negotiation_result.incompatibility_mask);

        if (negotiation_result.matched)
        {
            zoo_smb_qos_ctx_set_msg_status(qos_ctx, message->header.request_id, ZOO_SMB_MSG_ST_ACKED);
        }
        else
        {
            zoo_smb_qos_ctx_set_msg_status(qos_ctx, message->header.request_id, ZOO_SMB_MSG_ST_FAILED);
        }
    }
    return;
}

/**
 * @brief Sends an acknowledgment message from the publisher.
 *
 * This function is responsible for sending an acknowledgment (ACK) for a specific message,
 * identified by its message ID and request ID, using the provided publisher structure.
 *
 * @param publisher Pointer to the ZOO_SMB_PUBLISHER_STRUCT representing the publisher context.
 * @param msg_id    The ID of the message to acknowledge.
 * @param request_id The unique identifier for the request being acknowledged.
 */
static void subscriber_send_ack(ZOO_SMB_SUBSCRIBER_STRUCT* subscriber, uint32_t msg_id, uint64_t request_id, const char* receiver)
{
    ZOO_SMB_MSG_STRUCT* message = zoo_smb_create_message(ZOO_SMB_MSG_TYPE_PUBACK, subscriber->node->target, subscriber->node->topic, NULL, 0, msg_id, request_id);
    if (message == NULL)
    {
        ZOO_LOG_ERROR("Failed to create request acknowledgment message for request_id=%llu", request_id);
        return;
    }

    zoo_smb_send_message(subscriber->node, message, receiver, ZOO_TRUE);
}

/**
 * @brief Callback function to handle incoming SMB messages for a server.
 *
 * This function is invoked when a new SMB message is received. It processes
 * the message in the context of the specified server.
 *
 * @param context Pointer to user-defined context data associated with the server.
 * @param message Pointer to the received ZOO_SMB_MSG_STRUCT structure containing
 *                the message data to be handled.
 */
static void subscriber_on_handle_PUB_msg_cb(IN void* context, IN const void* msg, IN size_t msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p, msg_len=%zu", msg, context, msg_len);
        return;
    }
    ZOO_SMB_SUBSCRIBER_STRUCT* subscriber = (ZOO_SMB_SUBSCRIBER_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;

    ZOO_LOG_INFO("Received PUB message: msg_id=%u, req_id=%llu, payload_size=%zu",
                     message->header.msg_id,
                     message->header.request_id,
                     message->header.payload_size);

    if (subscriber->qos_entity)
    {
        ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(subscriber->qos_entity);
        if (qos_policy->reliability_enabled)
        {
            subscriber_send_ack(subscriber, message->header.msg_id, message->header.request_id, message->header.sender);
        }
    }

    ZOO_MUTEX_LOCK(&subscriber->mutex);
    for (size_t i = 0; i < zoo_list_size(subscriber->observers); i++)
    {
        USER_OBSERVER_STRUCT* observer = (USER_OBSERVER_STRUCT*)zoo_list_at(subscriber->observers, i);
        subscriber_on_notify_observer(observer, message);
    }
    ZOO_MUTEX_UNLOCK(&subscriber->mutex);
}

/**
 * @brief Create a new subscriber node.
 *
 * This function creates a new subscriber node for the Soft Message Bus.
 *
 * @param name Name of the subscriber node.
 * @return ZOO_SMB_SUBSCRIBER_HANDLE Handle to the newly created subscriber node, or NULL on error.
 */
ZOO_SMB_SUBSCRIBER_HANDLE zoo_smb_create_subscriber(
    IN const char* name,
    IN const char* target,
    IN const char* topic,
    IN const ZOO_SMB_QOS_POLICY_STRUCT* policy)
{
    if (topic == NULL)
    {
        ZOO_LOG_ERROR("Topic cannot be NULL");
        return NULL;
    }

    ZOO_SMB_SUBSCRIBER_STRUCT* subscriber = (ZOO_SMB_SUBSCRIBER_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_SUBSCRIBER_STRUCT));
    subscriber->node = zoo_smb_create_node(name, target, topic, ZOO_SMB_NODE_TYPE_SUBSCRIBER, ZOO_SMB_TRANSPORT_TYPE_DEFAULT);
    if (NULL == subscriber->node)
    {
        ZOO_LOG_ERROR("Fail to create server : %s", name);
        return NULL;
    }

    subscriber->observers = zoo_list_create(MAX_NODE_OBSERVER_SIZE);
    if (subscriber->observers == NULL)
    {
        ZOO_LOG_ERROR("Failed to create observer list for subscriber: %s", name);
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_HANDLE pub_observer = zoo_smb_create_node_observer_and_insert(
        subscriber->node->observers, subscriber->node->name, 0, ZOO_SMB_MSG_TYPE_PUB, subscriber_on_handle_PUB_msg_cb, subscriber, &subscriber->id[0]);
    if (pub_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create subscriber pub_observer");
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_HANDLE puback_observer = zoo_smb_create_node_observer_and_insert(
        subscriber->node->observers, subscriber->node->name, 0, ZOO_SMB_MSG_TYPE_SUBACK, subscriber_on_handle_SUBACK_msg_cb, subscriber, &subscriber->id[1]);
    if (puback_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create subscriber puback_observer");
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    if (policy)
    {
        subscriber->qos_entity = zoo_smb_create_qos_entity(policy);
    }
    else
    {
        subscriber->qos_entity = zoo_smb_create_qos_default_entity();
    }

    if (subscriber->qos_entity == NULL)
    {
        ZOO_LOG_ERROR("Failed to create QoS entity for subscriber: %s", name);
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }

    if (ZOO_SMB_OK != zoo_smb_register_node(subscriber->node))
    {
        ZOO_LOG_ERROR("Failed to register node");
        zoo_smb_destroy_node(subscriber->node);
        zoo_free_to_pool(subscriber);
        return NULL;
    }
    ZOO_MUTEX_INIT(&subscriber->mutex);
    ZOO_LOG_DEBUG("Created subscriber node: %s, target: %s, topic: %s", name, target, topic);
    return subscriber;
}

/**
 * @brief Destroys and cleans up a subscriber node.
 *
 * This function releases all resources associated with the given subscriber node handle.
 * After calling this function, the node handle becomes invalid and should not be used.
 *
 * @param subscriber [in] Handle to the subscriber node to be destroyed
 */
void zoo_smb_destroy_subscriber(IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber)
{
    ZOO_LOG_DEBUG("Destroying subscriber node: %p", subscriber);
    if (subscriber == NULL)
    {
        ZOO_LOG_ERROR("Invalid node handle: %p", subscriber);
        return;
    }

    ZOO_MUTEX_DESTROY(&subscriber->mutex);
    while (!zoo_list_empty(subscriber->observers))
    {
        USER_OBSERVER_STRUCT* observer = (USER_OBSERVER_STRUCT*)zoo_list_pop_front(subscriber->observers);
        if (observer)
        {
            zoo_free_to_pool(observer);
        }
    }
    zoo_list_destroy(subscriber->observers);
    zoo_smb_destroy_qos_entity(subscriber->qos_entity);
    zoo_smb_destroy_node(subscriber->node);
    zoo_free_to_pool(subscriber);
}

/**
 * @brief Finds an observer in the linked list of observers that matches the specified handler.
 *
 * @param observers A handle to the linked list of observer nodes.
 * @param handler The message handler function to search for.
 * @return ZOO_SMB_NODE_OBSERVER_HANDLE The handle to the found observer, or NULL if not found.
 */
static USER_OBSERVER_STRUCT* subscriber_find_observer(ZOO_LIST_HANDLE observers, ZOO_SMB_MSG_HANDLER handler)
{
    for (size_t i = 0; i < zoo_list_size(observers); ++i)
    {
        USER_OBSERVER_STRUCT* observer = (USER_OBSERVER_STRUCT*)zoo_list_at(observers, i);
        if (observer->handler == handler)
        {
            return observer;
        }
    }
    return NULL;
}

/**
 * Finds and returns a pointer to a USER_OBSERVER_STRUCT in the observers list by its unique ID.
 *
 * @param observers The linked list handle containing USER_OBSERVER_STRUCT observers.
 * @param id The unique identifier of the observer to find.
 * @return Pointer to the USER_OBSERVER_STRUCT with the specified ID, or NULL if not found.
 */
static USER_OBSERVER_STRUCT* subscriber_find_observer_by_id(ZOO_LIST_HANDLE observers, int32_t id)
{
    for (size_t i = 0; i < zoo_list_size(observers); ++i)
    {
        USER_OBSERVER_STRUCT* observer = (USER_OBSERVER_STRUCT*)zoo_list_at(observers, i);
        if (observer->id == id)
        {
            return observer;
        }
    }
    return NULL;
}
/**
 * @brief Registers a subscriber for the SMB Zoo service.
 *
 * This function handles the registration process for a subscriber,
 * allowing it to receive updates or notifications from the SMB Zoo service.
 *
 * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the registration.
 */
static ZOO_ERROR_TYPE subscriber_register_data_observer(
    IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER handler,
    IN void* user_data,
    OUT int32_t* handle)
{
    if (subscriber == NULL || handler == NULL || handle == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    USER_OBSERVER_STRUCT* observer = subscriber_find_observer(subscriber->observers, handler);
    if (observer != NULL)
    {
        *handle = observer->id;          // Assuming id is used as handle
        observer->usr_data = user_data;  // Update user data if needed
        observer->msg_id = msg_id;       // Update message ID if needed
        return ZOO_SMB_OK;
    }

    // Create a new observer for the subscriber
    observer = zoo_allocate_from_pool(sizeof(USER_OBSERVER_STRUCT));
    if (observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create observer for subscriber: %s", subscriber->node->name);
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    observer->usr_data = user_data;
    observer->msg_id = msg_id;
    observer->handler = handler;
    observer->id = (int32_t)(zoo_generate_uuid64() & 0x7FFFFFFF);
    zoo_list_push_back(subscriber->observers, observer);
    *handle = observer->id;

    ZOO_LOG_DEBUG("Registered subscriber: %s, topic: %s, handle: %u",
                      subscriber->node->name,
                      subscriber->node->topic,
                      *handle);
    return ZOO_SMB_OK;
}

/**
 * @brief Checks if the subscriber has a data observer.
 *
 * This function determines whether a data observer is associated with the subscriber.
 *
 * @return ZOO_TRUE if a data observer is present, ZOO_FALSE otherwise.
 */
static ZOO_BOOL subscriber_has_data_observer(
    IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER handler)
{
    if (subscriber == NULL || handler == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: subscriber=%p, msg_id=%d, handler=%p", subscriber, msg_id, handler);
        return ZOO_FALSE;
    }

    USER_OBSERVER_STRUCT* observer = subscriber_find_observer(subscriber->observers, handler);
    if (observer != NULL && observer->msg_id == msg_id && observer->handler == handler)
    {
        return ZOO_TRUE;
    }
    return ZOO_FALSE;
}

/**
 * @brief Subscribe to a message topic.
 *
 * This function allows a subscriber node to subscribe to a specific message topic.
 * When a message is published on the specified topic, the provided event handler will be called.
 *
 * @param subscriber      Handle to the subscriber node.
 * @param topic     Name of the topic to subscribe to.
 * @param handler   Pointer to the event handler function.
 * @param user_data Pointer to user-defined data to be passed to the event handler.
 * @param handle    Pointer to an unsigned 32-bit integer where the subscription handle will be stored.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE zoo_smb_subscribe_message(
    IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER message_handler,
    IN void* user_data,
    OUT int32_t* handle)
{
    if (subscriber == NULL || message_handler == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: node=%p, msg_id=%d, handler=%p", subscriber, msg_id, message_handler);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_ERROR_TYPE result = ZOO_SMB_OK;
    if (subscriber_has_data_observer(subscriber, msg_id, message_handler))
    {
        return ZOO_SMB_OK;
    }

    uint64_t request_id = zoo_generate_uuid64();
    ZOO_LOG_INFO(
        "topic: %s, msg_id: %d, req_id:%llu, handler: %p", subscriber->node->topic, msg_id, request_id, message_handler);
    ZOO_SMB_MSG_STRUCT* msg = zoo_smb_qos_build_msg(
        subscriber->qos_entity, subscriber->node->name, subscriber->node->topic, ZOO_SMB_MSG_TYPE_SUB, msg_id, request_id);
    if (msg == NULL)
    {
        ZOO_LOG_ERROR("Failed to create subscription message for subscriber: %s", subscriber->node->name);
        return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    }

    ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(subscriber->qos_entity);
    zoo_smb_qos_ctx_set_state(qos_ctx, msg->header.request_id, ZOO_SMB_MSG_ST_SENDING, msg);
    if (result == ZOO_SMB_OK)
    {
        zoo_smb_qos_ctx_set_msg_status(qos_ctx, msg->header.request_id, ZOO_SMB_MSG_ST_WAITING_ACK);
        result = zoo_smb_send_message(subscriber->node, msg, NULL, ZOO_TRUE);
    }

    if (result == ZOO_SMB_OK)
    {
        result = zoo_smb_qos_wait_for_negotiation_completed(subscriber->qos_entity, request_id);
    }

    if (result == ZOO_SMB_OK)
    {
        result = subscriber_register_data_observer(subscriber, msg_id, message_handler, user_data, handle);
    }

    if (result != ZOO_SMB_OK)
    {
        ZOO_LOG_ERROR("Failed to register data observer for subscriber: %s", subscriber->node->name);
    }
    else
    {
        ZOO_LOG_INFO(
            "Subscribed to topic success: %s, msg_id: %d, handler: %p, handle: %u, result:%d", subscriber->node->topic, msg_id, message_handler, *handle, result);
    }
    zoo_smb_qos_ctx_remove_state(qos_ctx, request_id);
    return result;
}

/**
 * @brief Unsubscribe from a message topic.
 *
 * This function allows a subscriber node to unsubscribe from a previously subscribed topic.
 * The subscription is identified by the provided handle.
 *
 * @param node   Handle to the subscriber node.
 * @param handle Pointer to an integer representing the subscription handle.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 */
void zoo_smb_unsubscribe_message(IN ZOO_SMB_SUBSCRIBER_HANDLE subscriber, IN int32_t handle)
{
    if (!subscriber)
    {
        ZOO_LOG_ERROR("Invalid node handle: %p", subscriber);
        return;
    }

    USER_OBSERVER_STRUCT* observer = subscriber_find_observer_by_id(subscriber->observers, handle);
    if (observer == NULL)
    {
        ZOO_LOG_ERROR("Observer not found for handle: %d", handle);
        return;
    }

    zoo_list_remove(subscriber->observers, observer);
    zoo_free_to_pool(observer);
    ZOO_LOG_INFO("Unsubscribed from topic: %s, handle: %d", subscriber->node->topic, handle);
}