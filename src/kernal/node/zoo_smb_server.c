/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SERVER
 * File name: zoo_smb_server.c
 * Description: Implementation of server node interface for ZOO SMB
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_server.h"
#include "zoo_types.h"
#include "zoo_smb_node.h"
#include "zoo_smb_qos.h"
#include "zoo_smb.h"
typedef struct ZOO_SMB_SERVER_STRUCT
{
    ZOO_S32_T id[2]; /**< Unique identifier for the server */
    ZOO_SMB_MSG_HANDLER message_handler;
    void* user_data;
    ZOO_SMB_NODE_HANDLE node;
    ZOO_SMB_QOS_CTX_HANDLE qos_ctx;       /**< QoS context for the subscriber */
    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity; /**< List of QoS entities associated with */
} ZOO_SMB_SERVER_STRUCT;

/**
 * @brief Sends an acknowledgment for a received request to the client.
 *
 * Builds and sends a REQACK message carrying msg_id/request_id correlation.
 *
 * @param server Server runtime object.
 * @param msg_id Message identifier being acknowledged.
 * @param request_id Request identifier being acknowledged.
 * @param receiver Request sender identity that receives ACK.
 */
static void server_send_request_ack(
    IN ZOO_SMB_SERVER_HANDLE server,
    IN ZOO_U32_T msg_id,
    IN ZOO_U64_T request_id,
    IN ZOO_STRING_T receiver)
{
    ZOO_SMB_MSG_STRUCT* message = zoo_smb_create_message(ZOO_SMB_MSG_TYPE_REQACK, server->node->target, server->node->topic, NULL, 0, msg_id, request_id);
    if (message == NULL)
    {
        ZOO_LOG_ERROR("Failed to create request acknowledgment message for request_id=%llu", request_id);
        return;
    }

    zoo_smb_send_message(server->node, message, receiver, ZOO_TRUE);
}

/**
 * @brief Handles incoming REPLACK messages.
 *
 * When reliability is enabled, this callback marks the corresponding QoS
 * request state as ACKED.
 *
 * @param context Server context pointer.
 * @param msg Incoming message pointer.
 * @param msg_len Received message size in bytes.
 */
static void server_on_handle_REPLACK_msg_cb(void* context, const void* msg, ZOO_USIZE_T msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p, msg_len=%zu", msg, context, msg_len);
        return;
    }

    ZOO_SMB_SERVER_STRUCT* server = (ZOO_SMB_SERVER_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;
    ZOO_LOG_INFO("Received REPLACK message: msg_id=%u, request_id=%llu, sender=%s",
                      message->header.msg_id,
                      message->header.request_id,
                      message->header.sender);
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(server->qos_entity);
    if (qos_policy && qos_policy->reliability_enabled)
    {
        ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(server->qos_entity);
        zoo_smb_qos_ctx_set_msg_status(qos_ctx, message->header.request_id, ZOO_SMB_MSG_ST_ACKED);
    }

    return;
}

/**
 * @brief Handles incoming REQ messages and dispatches user callback.
 *
 * For reliability-enabled policies this callback emits REQACK before invoking
 * the registered message_handler.
 *
 * @param context Server context pointer.
 * @param msg Incoming message pointer.
 * @param msg_len Received message size in bytes.
 */
static void server_on_handle_REQ_msg_cb(void* context, const void* msg, ZOO_USIZE_T msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p, msg_len=%zu", msg, context, msg_len);
        return;
    }

    ZOO_SMB_SERVER_STRUCT* server = (ZOO_SMB_SERVER_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(server->qos_entity);
    if (qos_policy && qos_policy->reliability_enabled)
    {
        server_send_request_ack(
            server, message->header.msg_id, message->header.request_id, message->header.sender);
    }

    ZOO_LOG_INFO("Received request message: msg_id=%u, request_id=%llu, sender=%s",
                      message->header.msg_id,
                      message->header.request_id,
                      message->header.sender);

    if (server->message_handler != NULL)
    {
        if (ZOO_SMB_OK ==
            server->message_handler(server->user_data, message->header.msg_id, message->header.request_id, message->header.timestamp, message->header.sender, message->payload, message->header.payload_size))
        {
            ZOO_LOG_DEBUG("Message handled successfully: msg_id=%llu, payload_size=%zu", message->header.request_id, message->header.payload_size);
        }
    }

}

/**
 * @brief Create a new server node.
 *
 * This function creates a new server node for the Soft Message Bus.
 *
 * @param name   Name of the server node.
 * @param target Target address or identifier for the server.
 * @param topic  Name of the topic to serve.
 * @return ZOO_SMB_SERVER_HANDLE Handle to the newly created server node, or NULL on error.
 */
ZOO_SMB_SERVER_HANDLE zoo_smb_create_server(
    IN ZOO_STRING_T name,
    IN ZOO_STRING_T target,
    IN ZOO_STRING_T topic,
    IN ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type,
    IN const ZOO_SMB_QOS_POLICY_STRUCT* policy)
{
    if (name == NULL || target == NULL || topic == NULL || transport_type >= ZOO_SMB_TRANSPORT_TYPE_MAX)
    {
        ZOO_LOG_ERROR("Invalid parameters: name=%p, target=%p, topic=%p, transport_type=%d", name, target, topic, transport_type);
        return NULL;
    }

    ZOO_SMB_SERVER_STRUCT* server = (ZOO_SMB_SERVER_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_SERVER_STRUCT));
    server->node = zoo_smb_create_node(name, target, topic, ZOO_SMB_NODE_TYPE_SERVER, transport_type);
    if (NULL == server->node)
    {
        ZOO_LOG_ERROR("Fail to create server : %s", name);
        return NULL;
    }

    if (policy)
    {
        server->qos_entity = zoo_smb_create_qos_entity(policy);
    }
    else
    {
        server->qos_entity = zoo_smb_create_qos_default_entity();
    }

    if (server->qos_entity == NULL)
    {
        ZOO_LOG_ERROR("Failed to create QoS entity for server: %s", name);
        zoo_smb_destroy_node(server->node);
        zoo_free_to_pool(server);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_HANDLE repl_observer = zoo_smb_create_node_observer_and_insert(
        server->node->observers, server->node->name, 0, ZOO_SMB_MSG_TYPE_REQ, server_on_handle_REQ_msg_cb, server, &server->id[0]);
    if (repl_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create server observer");
        zoo_smb_destroy_node(server->node);
        zoo_free_to_pool(server);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_HANDLE replack_observer = zoo_smb_create_node_observer_and_insert(
        server->node->observers, server->node->name, 0, ZOO_SMB_MSG_TYPE_REPLACK, server_on_handle_REPLACK_msg_cb, server, &server->id[1]);
    if (replack_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create server observer");
        zoo_smb_destroy_node(server->node);
        zoo_free_to_pool(server);
        return NULL;
    }

    if (ZOO_SMB_OK != zoo_smb_register_node(server->node))
    {
        ZOO_LOG_ERROR("Failed to register node");
        zoo_smb_destroy_node(server->node);
        zoo_free_to_pool(server);
        return NULL;
    }

    ZOO_LOG_INFO("Created SMB server server: %s, target: %s, topic: %s", name, target, topic);
    return (ZOO_SMB_SERVER_HANDLE)server;
}

/**
 * @brief Destroys an SMB server node and cleans up associated resources
 *
 * This function handles the cleanup and destruction of an SMB server node,
 * freeing all allocated resources and ensuring proper shutdown.
 *
 * @param server The handle to the SMB server node to be destroyed
 *
 * @note After calling this function, the server handle becomes invalid and
 *       should not be used
 */
void zoo_smb_destroy_server(IN ZOO_SMB_SERVER_HANDLE server)
{
    if (server == NULL)
    {
        ZOO_LOG_ERROR("Invalid server handle: %p", server);
        return;
    }

    zoo_smb_destroy_qos_entity(server->qos_entity);
    zoo_smb_destroy_node(server->node);
    zoo_free_to_pool(server);
    ZOO_LOG_DEBUG("SMB server server destroyed: %p", server);
}

/**
 * @brief Send a reply message from the server.
 *
 * This function sends a reply message to a client from the server node.
 *
 * @param server         Handle to the server node.
 * @param receiver       Target receiver identity.
 * @param msg_id       Message ID to reply to.
 * @param payload      Pointer to the reply payload data.
 * @param payload_size Size of the reply payload in bytes.
 * @param request_id   Request ID to reply to.
 * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
 */
ZOO_ERROR_T zoo_smb_server_send_reply(
    IN ZOO_SMB_SERVER_HANDLE server,
    IN ZOO_STRING_T receiver,
    IN ZOO_U32_T msg_id,
    IN const void* payload,
    IN ZOO_USIZE_T payload_size,
    IN ZOO_S64_T request_id)
{
    if (server == NULL || payload == NULL)
    {
        ZOO_LOG_ERROR(
            "Invalid parameters: server=%p, payload=%p, request_id=%ld", server, payload, request_id);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_SMB_MSG_STRUCT* repl_msg = zoo_smb_create_message(
        ZOO_SMB_MSG_TYPE_REPL, server->node->name, server->node->topic, payload, payload_size, msg_id, request_id);
    if (repl_msg == NULL)
    {
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    ZOO_BOOL delete_message_after_send = ZOO_TRUE;
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(server->qos_entity);
    if (qos_policy && qos_policy->reliability_enabled)
    {
        delete_message_after_send = ZOO_FALSE;
        ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(server->qos_entity);
        zoo_smb_qos_ctx_set_state(qos_ctx, repl_msg->header.request_id, ZOO_SMB_MSG_ST_CREATED, repl_msg);
    }

    ZOO_LOG_INFO("receiver:%s, topic:%s, msg_id: %u, payload_size: %zu", receiver, server->node->topic, msg_id, payload_size);
    return zoo_smb_send_message(server->node, repl_msg, receiver, delete_message_after_send);
}

/**
 * @brief Set the event handler for the server node.
 *
 * This function sets the event handler callback for the server node to handle incoming requests.
 *
 * @param server          Handle to the server node.
 * @param message_handler Pointer to the event handler function.
 * @param user_data     Pointer to user-defined data to be passed to the event handler.
 */
ZOO_ERROR_TYPE zoo_smb_server_set_message_handler(
    IN ZOO_SMB_SERVER_HANDLE server,
    IN ZOO_SMB_MSG_HANDLER message_handler,
    IN void* user_data)
{
    if (server == NULL || message_handler == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: server=%p, message_handler=%p", server, message_handler);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (server->message_handler != message_handler)
    {
        server->message_handler = message_handler;
        server->user_data = user_data;
    }

    ZOO_LOG_INFO("Set message handler for server server: %p, handler: %p", server, message_handler);
    return ZOO_SMB_OK;
}