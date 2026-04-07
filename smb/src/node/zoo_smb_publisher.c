/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: zoo_smb_
 * File name: zoo_smb_publisher.c
 * Description: Implementation of publisher node for ZOO Soft Message Bus (SMB)
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-28     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_publisher.h"
#include "zoo_smb_node.h"
#include "zoo_smb_qos.h"
#include "zoo_smb.h"
#include <string.h>

#define MAX_SUBSCRIBER_NUMBER 1024
#define MAX_PUBLISHER_SUBSCRIBER_NAME_LENGTH 32
#define SUBSCRIBER_INDEX_BUCKET_COUNT 257

typedef struct ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT
{
    char subscriber_name[MAX_PUBLISHER_SUBSCRIBER_NAME_LENGTH];
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy;
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx;
    ZOO_BOOL matched;
    uint32_t incompatibility_mask;
} ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT;

typedef struct ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT
{
    char subscriber_name[MAX_PUBLISHER_SUBSCRIBER_NAME_LENGTH];
    ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* subscriber_ctx;
} ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT;

typedef struct ZOO_SMB_PUBLISHER_STRUCT
{
    int32_t id[2];
    ZOO_SMB_NODE_HANDLE node;
    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity;
    ZOO_LIST_HANDLE subscribers;
    ZOO_LIST_HANDLE subscriber_index_buckets[SUBSCRIBER_INDEX_BUCKET_COUNT];
} ZOO_SMB_PUBLISHER_STRUCT;

static ZOO_USIZE subscriber_index_hash(const char* name)
{
    if (!name)
    {
        return 0;
    }

    ZOO_USIZE hash = 5381;
    while (*name)
    {
        hash = ((hash << 5) + hash) + (unsigned char)(*name);
        name++;
    }

    return hash % SUBSCRIBER_INDEX_BUCKET_COUNT;
}

static ZOO_BOOL compare_subscriber_index_entry_by_name(const void* data, const void* target)
{
    const ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT* entry = (const ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT*)data;
    const char* name = (const char*)target;

    if (!entry || !name)
    {
        return ZOO_FALSE;
    }

    return strcmp(entry->subscriber_name, name) == 0;
}

static ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* subscriber_index_find(
    ZOO_SMB_PUBLISHER_STRUCT* publisher,
    const char* name)
{
    if (!publisher || !name)
    {
        return NULL;
    }

    ZOO_LIST_HANDLE bucket = publisher->subscriber_index_buckets[subscriber_index_hash(name)];
    if (!bucket)
    {
        return NULL;
    }

    ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT*)zoo_list_find_if(
        bucket,
        compare_subscriber_index_entry_by_name,
        name);

    return entry ? entry->subscriber_ctx : NULL;
}

static ZOO_ERROR_TYPE subscriber_index_upsert(
    ZOO_SMB_PUBLISHER_STRUCT* publisher,
    const char* name,
    ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* subscriber_ctx)
{
    if (!publisher || !name || !subscriber_ctx)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_LIST_HANDLE bucket = publisher->subscriber_index_buckets[subscriber_index_hash(name)];
    if (!bucket)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT*)zoo_list_find_if(
        bucket,
        compare_subscriber_index_entry_by_name,
        name);
    if (entry)
    {
        entry->subscriber_ctx = subscriber_ctx;
        return ZOO_SMB_OK;
    }

    entry = (ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT));
    if (!entry)
    {
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    memset(entry, 0, sizeof(ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT));
    snprintf(entry->subscriber_name, sizeof(entry->subscriber_name), "%s", name);
    entry->subscriber_name[sizeof(entry->subscriber_name) - 1] = '\0';
    entry->subscriber_ctx = subscriber_ctx;

    ZOO_ERROR_TYPE ret = zoo_list_push_back(bucket, entry);
    if (ret != ZOO_SMB_OK)
    {
        zoo_free_to_pool(entry);
    }

    return ret;
}

static void subscriber_index_remove(
    ZOO_SMB_PUBLISHER_STRUCT* publisher,
    const char* name)
{
    if (!publisher || !name)
    {
        return;
    }

    ZOO_LIST_HANDLE bucket = publisher->subscriber_index_buckets[subscriber_index_hash(name)];
    if (!bucket)
    {
        return;
    }

    ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT*)zoo_list_remove_if(
        bucket,
        compare_subscriber_index_entry_by_name,
        name);

    if (entry)
    {
        zoo_free_to_pool(entry);
    }
}

static void subscriber_index_destroy(ZOO_SMB_PUBLISHER_STRUCT* publisher)
{
    if (!publisher)
    {
        return;
    }

    for (ZOO_USIZE i = 0; i < SUBSCRIBER_INDEX_BUCKET_COUNT; i++)
    {
        ZOO_LIST_HANDLE bucket = publisher->subscriber_index_buckets[i];
        if (!bucket)
        {
            continue;
        }

        while (!zoo_list_empty(bucket))
        {
            ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT* entry = (ZOO_SMB_SUBSCRIBER_INDEX_ENTRY_STRUCT*)zoo_list_pop_front(bucket);
            if (entry)
            {
                zoo_free_to_pool(entry);
            }
        }

        zoo_list_destroy(bucket);
        publisher->subscriber_index_buckets[i] = NULL;
    }
}

/**
 * @brief Sets the QoS message status for all subscribers associated with the given publisher.
 *
 * This function updates the QoS message status for all subscribers managed by the specified
 * publisher, based on the provided request ID and status value.
 *
 * @param publisher   Pointer to the ZOO_SMB_PUBLISHER_STRUCT representing the publisher.
 * @param request_id  The unique identifier for the request whose status is being updated.
 * @param status      The new QoS message status to set for all subscribers.
 */
static void publisher_set_all_subscribers_qos_msg_status(ZOO_SMB_PUBLISHER_STRUCT* publisher, uint64_t request_id, ZOO_SMB_MSG_ST_ENUM status, ZOO_SMB_MSG_STRUCT* msg)
{
    for (size_t i = 0; i < zoo_list_size(publisher->subscribers); i++)
    {
        ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* subscriber = (ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT*)zoo_list_at(publisher->subscribers, i);
        if (subscriber && subscriber->qos_ctx)
        {
            zoo_smb_qos_ctx_set_state(subscriber->qos_ctx, request_id, status, msg);
        }
    }
}

/**
 * @brief Find subscriber QoS context entry by subscriber name.
 * @param publisher Publisher handle
 * @param name Subscriber name string
 * @return ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* Subscriber QoS context entry, NULL if not found
 */
static ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* publisher_find_subscriber_ctx_by_name(ZOO_SMB_PUBLISHER_STRUCT* publisher, const char* name)
{
    return subscriber_index_find(publisher, name);
}

/**
 * @brief Destroy all subscriber QoS context entries managed by publisher.
 * @param publisher Publisher handle
 * @return void
 */
static void publisher_destroy_subscriber_ctx_list(ZOO_SMB_PUBLISHER_STRUCT* publisher)
{
    if (!publisher || !publisher->subscribers)
    {
        return;
    }

    while (!zoo_list_empty(publisher->subscribers))
    {
        ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* subscriber_ctx = (ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT*)zoo_list_pop_front(publisher->subscribers);
        if (!subscriber_ctx)
        {
            continue;
        }

        zoo_smb_destroy_qos_ctx(subscriber_ctx->qos_ctx);
        zoo_smb_destroy_qos_policy(subscriber_ctx->qos_policy);
        zoo_free_to_pool(subscriber_ctx);
    }

    subscriber_index_destroy(publisher);
}

/**
 * @brief Register or update subscriber QoS context after DDS-style QoS negotiation.
 * @param publisher Publisher handle
 * @param subscriber_name Subscriber name string
 * @param subscriber_qos_policy Subscriber requested QoS policy
 * @param match_result DDS-like QoS match result
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure
 */
static ZOO_ERROR_TYPE publisher_register_or_update_subscriber_qos_ctx(
    ZOO_SMB_PUBLISHER_STRUCT* publisher,
    const char* subscriber_name,
    const ZOO_SMB_QOS_POLICY_STRUCT* subscriber_qos_policy,
    const ZOO_SMB_QOS_MATCH_RESULT_STRUCT* match_result)
{
    if (!publisher || !subscriber_name || !subscriber_qos_policy || !match_result)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* subscriber_ctx = publisher_find_subscriber_ctx_by_name(publisher, subscriber_name);
    if (!match_result->compatible)
    {
        if (subscriber_ctx)
        {
            zoo_list_remove(publisher->subscribers, subscriber_ctx);
            subscriber_index_remove(publisher, subscriber_ctx->subscriber_name);
            zoo_smb_destroy_qos_ctx(subscriber_ctx->qos_ctx);
            zoo_smb_destroy_qos_policy(subscriber_ctx->qos_policy);
            zoo_free_to_pool(subscriber_ctx);
        }
        return ZOO_SMB_ERROR_PUBLICATION_QOS_VIOLATION;
    }

    if (!subscriber_ctx)
    {
        subscriber_ctx = (ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT));
        if (!subscriber_ctx)
        {
            return ZOO_SMB_ERROR_OUT_OF_MEMORY;
        }
        memset(subscriber_ctx, 0, sizeof(ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT));
        snprintf(subscriber_ctx->subscriber_name, sizeof(subscriber_ctx->subscriber_name), "%s", subscriber_name);
        subscriber_ctx->subscriber_name[sizeof(subscriber_ctx->subscriber_name) - 1] = '\0';

        subscriber_ctx->qos_policy = zoo_smb_create_qos_policy(subscriber_qos_policy);
        if (!subscriber_ctx->qos_policy)
        {
            zoo_free_to_pool(subscriber_ctx);
            return ZOO_SMB_ERROR_OUT_OF_MEMORY;
        }

        subscriber_ctx->qos_ctx = zoo_smb_create_qos_ctx(subscriber_ctx->qos_policy);
        if (!subscriber_ctx->qos_ctx)
        {
            zoo_smb_destroy_qos_policy(subscriber_ctx->qos_policy);
            zoo_free_to_pool(subscriber_ctx);
            return ZOO_SMB_ERROR_OUT_OF_MEMORY;
        }

        if (zoo_list_push_back(publisher->subscribers, subscriber_ctx) != ZOO_SMB_OK)
        {
            zoo_smb_destroy_qos_ctx(subscriber_ctx->qos_ctx);
            zoo_smb_destroy_qos_policy(subscriber_ctx->qos_policy);
            zoo_free_to_pool(subscriber_ctx);
            return ZOO_SMB_ERROR_LIST_FULL;
        }

        if (subscriber_index_upsert(publisher, subscriber_ctx->subscriber_name, subscriber_ctx) != ZOO_SMB_OK)
        {
            zoo_list_remove(publisher->subscribers, subscriber_ctx);
            zoo_smb_destroy_qos_ctx(subscriber_ctx->qos_ctx);
            zoo_smb_destroy_qos_policy(subscriber_ctx->qos_policy);
            zoo_free_to_pool(subscriber_ctx);
            return ZOO_SMB_ERROR_OUT_OF_MEMORY;
        }
    }
    else if (!zoo_smb_qos_policy_is_equivalent(
                 subscriber_ctx->qos_policy,
                 (ZOO_SMB_QOS_POLICY_HANDLE)subscriber_qos_policy))
    {
        zoo_smb_destroy_qos_ctx(subscriber_ctx->qos_ctx);
        zoo_smb_destroy_qos_policy(subscriber_ctx->qos_policy);
        subscriber_ctx->qos_policy = zoo_smb_create_qos_policy(subscriber_qos_policy);
        subscriber_ctx->qos_ctx = subscriber_ctx->qos_policy ? zoo_smb_create_qos_ctx(subscriber_ctx->qos_policy) : NULL;
        if (!subscriber_ctx->qos_policy || !subscriber_ctx->qos_ctx)
        {
            if (subscriber_ctx->qos_policy)
            {
                zoo_smb_destroy_qos_policy(subscriber_ctx->qos_policy);
            }
            subscriber_ctx->qos_policy = NULL;
            subscriber_ctx->qos_ctx = NULL;
            return ZOO_SMB_ERROR_OUT_OF_MEMORY;
        }

        (void)subscriber_index_upsert(publisher, subscriber_ctx->subscriber_name, subscriber_ctx);
    }

    subscriber_ctx->matched = match_result->compatible;
    subscriber_ctx->incompatibility_mask = match_result->incompatible_mask;
    return ZOO_SMB_OK;
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
static void publisher_send_ack(
    ZOO_SMB_PUBLISHER_STRUCT* publisher,
    uint32_t msg_id,
    uint64_t request_id,
    const char* receiver,
    const ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT* negotiation_result)
{
    ZOO_SMB_MSG_STRUCT* message = zoo_smb_create_message(
        ZOO_SMB_MSG_TYPE_SUBACK,
        publisher->node->target,
        publisher->node->topic,
        negotiation_result,
        negotiation_result ? sizeof(ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT) : 0,
        msg_id,
        request_id);
    if (message == NULL)
    {
        ZOO_LOG_ERROR("Failed to create request acknowledgment message for request_id=%llu", request_id);
        return;
    }

    zoo_smb_send_message(publisher->node, message, receiver, ZOO_TRUE);
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
static void publisher_on_handle_SUB_message_cb(IN void* context, IN const void* msg, IN size_t msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p, msg_len=%zu", msg, context, msg_len);
        return;
    }

    ZOO_SMB_PUBLISHER_STRUCT* publisher = (ZOO_SMB_PUBLISHER_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;
    ZOO_SMB_QOS_MATCH_RESULT_STRUCT match_result = {ZOO_TRUE, 0};
    ZOO_SMB_QOS_NEGOTIATION_RESULT_STRUCT negotiation_result = {ZOO_TRUE, 0};
    ZOO_LOG_INFO("Receive SUB message for subscriber: %s,msg_id=%u request_id=%llu",
                     message->header.sender,
                     message->header.msg_id,
                     message->header.request_id);

    ZOO_SMB_QOS_POLICY_HANDLE publisher_qos_policy = zoo_smb_qos_get_policy(publisher->qos_entity);
    if (message->header.payload_size > 0)
    {
        ZOO_SMB_QOS_POLICY_HANDLE subscriber_qos_policy = message->payload ? (ZOO_SMB_QOS_POLICY_HANDLE)message->payload : NULL;
        if (subscriber_qos_policy && publisher_qos_policy)
        {
            (void)zoo_smb_qos_policy_match(publisher_qos_policy, subscriber_qos_policy, &match_result);
            negotiation_result.matched = match_result.compatible;
            negotiation_result.incompatibility_mask = match_result.incompatible_mask;

            if (ZOO_SMB_OK != publisher_register_or_update_subscriber_qos_ctx(
                    publisher,
                    message->header.sender,
                    subscriber_qos_policy,
                    &match_result))
            {
                ZOO_LOG_ERROR("QoS policy mismatch for subscriber: %s", message->header.sender);
            }
            publisher_send_ack(publisher, message->header.msg_id, message->header.request_id, message->header.sender, &negotiation_result);
            return;
        }
    }

    publisher_send_ack(publisher, message->header.msg_id, message->header.request_id, message->header.sender, &negotiation_result);
}

/**
 * @brief Callback function invoked when a PUBACK message is received by the publisher.
 *
 * This function handles the PUBACK (Publish Acknowledgement) message received from the broker,
 * typically as part of the QoS 1 publish flow in MQTT or similar protocols.
 *
 * @param context Pointer to user-defined context or state information.
 * @param msg Pointer to the received PUBACK message data.
 * @param msg_len Length of the received message in bytes.
 */
static void publisher_on_handle_PUBACK_msg_cb(IN void* context, IN const void* msg, IN size_t msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p, msg_len=%zu", msg, context, msg_len);
        return;
    }

    ZOO_SMB_PUBLISHER_STRUCT* publisher = (ZOO_SMB_PUBLISHER_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;
    ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* subscriber_ctx = publisher_find_subscriber_ctx_by_name(publisher, message->header.sender);
    if (subscriber_ctx && subscriber_ctx->qos_ctx)
    {
        zoo_smb_qos_ctx_set_state(subscriber_ctx->qos_ctx, message->header.request_id, ZOO_SMB_MSG_ST_ACKED, NULL);
    }
    ZOO_LOG_INFO("Received PUBACK message for subscriber: %s, request_id=%llu",
                     publisher->node->name,
                     message->header.request_id);
}

/**
 * @brief Create a new publisher node.
 *
 * This function creates a new publisher node for the Soft Message Bus.
 *
 * @param name  Name of the publisher node.
 * @param topic Name of the topic to publish.
 * @return ZOO_SMB_NODE_HANDLE Handle to the newly created publisher node, or NULL on error.
 */
ZOO_SMB_PUBLISHER_HANDLE zoo_smb_create_publisher(IN const char* name,
                                                  IN const char* target,
                                                  IN const char* topic,
                                                  IN ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type,
                                                  IN const ZOO_SMB_QOS_POLICY_STRUCT* policy)
{
    if (topic == NULL || name == NULL || target == NULL)
    {
        ZOO_LOG_ERROR("Invalid param cannot be NULL");
        return NULL;
    }

    ZOO_SMB_PUBLISHER_STRUCT* publisher = (ZOO_SMB_PUBLISHER_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_PUBLISHER_STRUCT));
    if (publisher == NULL)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for publisher");
        return NULL;
    }

    publisher->node = zoo_smb_create_node(name, target, topic, ZOO_SMB_NODE_TYPE_PUBLISHER, transport_type);
    if (NULL == publisher->node)
    {
        ZOO_LOG_ERROR("Fail to create server : %s", name);
        zoo_free_to_pool(publisher);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_HANDLE sub_observer = zoo_smb_create_node_observer_and_insert(
        publisher->node->observers, publisher->node->name, 0, ZOO_SMB_MSG_TYPE_SUB, publisher_on_handle_SUB_message_cb, publisher, &publisher->id[0]);
    if (sub_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create publisher pub_observer");
        zoo_smb_destroy_node(publisher->node);
        zoo_free_to_pool(publisher);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_HANDLE suback_observer = zoo_smb_create_node_observer_and_insert(
        publisher->node->observers, publisher->node->name, 0, ZOO_SMB_MSG_TYPE_PUBACK, publisher_on_handle_PUBACK_msg_cb, publisher, &publisher->id[1]);
    if (suback_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create publisher puback_observer");
        zoo_smb_destroy_node(publisher->node);
        zoo_free_to_pool(publisher);
        return NULL;
    }

    if (policy)
    {
        publisher->qos_entity = zoo_smb_create_qos_entity(policy);
    }
    else
    {
        publisher->qos_entity = zoo_smb_create_qos_default_entity();
    }

    if (!publisher->qos_entity)
    {
        ZOO_LOG_ERROR("Failed to create QoS entity");
        zoo_smb_destroy_node(publisher->node);
        zoo_free_to_pool(publisher);
        return NULL;
    }

    publisher->subscribers = zoo_list_create(MAX_SUBSCRIBER_NUMBER);
    if (!publisher->subscribers)
    {
        ZOO_LOG_ERROR("Failed to create subscribers list");
        zoo_smb_destroy_qos_entity(publisher->qos_entity);
        zoo_smb_destroy_node(publisher->node);
        zoo_free_to_pool(publisher);
        return NULL;
    }

    memset(publisher->subscriber_index_buckets, 0, sizeof(publisher->subscriber_index_buckets));
    for (ZOO_USIZE i = 0; i < SUBSCRIBER_INDEX_BUCKET_COUNT; i++)
    {
        publisher->subscriber_index_buckets[i] = zoo_list_create(8);
        if (!publisher->subscriber_index_buckets[i])
        {
            ZOO_LOG_ERROR("Failed to create subscriber index bucket");
            for (ZOO_USIZE j = 0; j < i; j++)
            {
                zoo_list_destroy(publisher->subscriber_index_buckets[j]);
                publisher->subscriber_index_buckets[j] = NULL;
            }
            zoo_list_destroy(publisher->subscribers);
            zoo_smb_destroy_qos_entity(publisher->qos_entity);
            zoo_smb_destroy_node(publisher->node);
            zoo_free_to_pool(publisher);
            return NULL;
        }
    }

    if (ZOO_SMB_OK != zoo_smb_register_node(publisher->node))
    {
        ZOO_LOG_ERROR("Failed to register node");
        publisher_destroy_subscriber_ctx_list(publisher);
        zoo_list_destroy(publisher->subscribers);
        zoo_smb_destroy_qos_entity(publisher->qos_entity);
        zoo_smb_destroy_node(publisher->node);
        zoo_free_to_pool(publisher);
        return NULL;
    }
    ZOO_LOG_INFO("Publisher node %s target %s topic %s created successfully", name, target, topic);
    return publisher;
}

/**
 * @brief Destroys a SMB publisher node and frees associated resources
 *
 * This function cleans up and deallocates all resources associated with the
 * specified SMB publisher node. The node handle becomes invalid after this call.
 *
 * @param[in] node Handle to the SMB publisher node to destroy
 */
void zoo_smb_destroy_publisher(IN ZOO_SMB_PUBLISHER_HANDLE publisher)
{
    if (publisher == NULL)
    {
        ZOO_LOG_ERROR("Invalid publisher handle: %p", publisher);
        return;
    }
    publisher_destroy_subscriber_ctx_list(publisher);
    zoo_list_destroy(publisher->subscribers);
    zoo_smb_destroy_qos_entity(publisher->qos_entity);
    zoo_smb_destroy_node(publisher->node);
    zoo_free_to_pool(publisher);
    ZOO_LOG_DEBUG("SMB publisher node destroyed: %p", publisher);
}

/**
 * @brief Publish a message to a topic.
 *
 * This function publishes a message to the specified topic using the given publisher node.
 *
 * @param node  Handle to the publisher node.
 * @param topic Name of the message topic.
 * @param msg   Pointer to the message data.
 * @param size  Size of the message in bytes.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 */
ZOO_ERROR_TYPE zoo_smb_publish_message(
    IN ZOO_SMB_PUBLISHER_HANDLE publisher,
    IN uint32_t msg_id,
    IN const void* payload,
    IN size_t payload_size)
{
    if (publisher == NULL || (payload_size > 0 && payload == NULL))
    {
        ZOO_LOG_ERROR("Invalid parameters: node=%p", publisher);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    uint64_t request_id = zoo_generate_uuid64();
    ZOO_SMB_MSG_STRUCT* message = zoo_smb_create_message(ZOO_SMB_MSG_TYPE_PUB, publisher->node->target, publisher->node->topic, payload, payload_size, msg_id, request_id);
    if (message == NULL)
    {
        ZOO_LOG_ERROR("Failed to create reply message");
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    ZOO_BOOL delete_message_after_send = ZOO_TRUE;
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(publisher->qos_entity);
    if (qos_policy && qos_policy->reliability_enabled && zoo_list_size(publisher->subscribers) > 0)
    {
        delete_message_after_send = ZOO_FALSE;
        publisher_set_all_subscribers_qos_msg_status(publisher, request_id, ZOO_SMB_MSG_ST_CREATED, message);
    }

    // Prefer negotiated unicast delivery to known subscribers; fallback to multicast when none are tracked.
    const char* first_receiver = NULL;
    for (size_t i = 0; i < zoo_list_size(publisher->subscribers); i++)
    {
        ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT* subscriber = (ZOO_SMB_SUBSCRIBER_QOS_CTX_STRUCT*)zoo_list_at(publisher->subscribers, i);
        if (!subscriber || !subscriber->matched || subscriber->subscriber_name[0] == '\0')
        {
            continue;
        }

        first_receiver = subscriber->subscriber_name;
        break;
    }

    if (first_receiver)
    {
        return zoo_smb_send_message(publisher->node, message, first_receiver, delete_message_after_send);
    }

    return zoo_smb_send_message(publisher->node, message, SMB_MULTICAST, delete_message_after_send);
}
