/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_CLIENT
 * File name: zoo_smb_client.c
 * Description: RPC interface for ZOO Soft Message Bus (SMB)
 *              Defines APIs for request/reply messaging, payload access,
 *              reply sending, and event handler registration.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     weiwang.sun       created
 ******************************************************************************/
#include "zoo_smb_client.h"
#include "zoo_smb_node.h"
#include "zoo_smb_qos.h"
#include "zoo_smb.h"
#include "zoo_util.h"
#include <errno.h>
typedef struct ZOO_SMB_CLIENT_STRUCT
{
    int32_t id[2]; /**< Unique identifier for the server */
    ZOO_SMB_NODE_HANDLE node;
    ZOO_LIST_HANDLE reply_message_list; /**< Message queue for reply messages */
    ZOO_SMB_QOS_ENTITY_HANDLE qos_entity;     /**< List of QoS entities associated with */
    ZOO_MUTEX_T mutex;                        /**< Mutex for thread safety */
    ZOO_COND_T reply_cond;
} ZOO_SMB_CLIENT_STRUCT;

/**
 * @brief Predicate used to match reply messages by request id.
 *
 * @param data Candidate message item from reply list.
 * @param user_data Pointer to expected request id (uint64_t*).
 * @return ZOO_TRUE when message request id matches expected request id.
 */
static ZOO_BOOL client_compare_message(const void* data, const void* user_data)
{
    if (data == NULL || user_data == NULL)
    {
        return ZOO_FALSE;
    }

    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)data;
    uint64_t request_id = *(uint64_t*)user_data;
    if (message->header.request_id == request_id)
    {
        ZOO_LOG_DEBUG("req_id=%llu FOUND", message->header.request_id);
        return ZOO_TRUE;
    }

    return ZOO_FALSE;
}

/**
 * @brief Removes one buffered reply from list and destroys message storage.
 *
 * Caller must hold client mutex while manipulating reply_message_list.
 *
 * @param client Client handle owning reply list.
 * @param reply_message Reply message entry to remove.
 */
static void client_remove_reply_message_locked(
    IN ZOO_SMB_CLIENT_HANDLE client,
    IN ZOO_SMB_MSG_STRUCT* reply_message)
{
    if (client == NULL)
    {
        ZOO_LOG_ERROR("Client handle is NULL");
        return;
    }

    ZOO_SMB_CLIENT_STRUCT* c = (ZOO_SMB_CLIENT_STRUCT*)client;
    zoo_list_remove(c->reply_message_list, reply_message);
    zoo_smb_destroy_message(reply_message); /**< Free the message payload */
    ZOO_LOG_DEBUG("msg_id=%u, req_id=%llu", reply_message->header.msg_id, reply_message->header.request_id);
}

/**
 * @brief Finds buffered reply matching both message id and request id.
 *
 * Caller must hold client mutex while searching reply_message_list.
 *
 * @param c Client runtime pointer.
 * @param msg_id Expected message identifier.
 * @param request_id Expected request identifier.
 * @return Matching message pointer or NULL if not found.
 */
static ZOO_SMB_MSG_STRUCT* client_find_reply_message_locked(
    IN ZOO_SMB_CLIENT_STRUCT* c,
    IN uint32_t msg_id,
    IN uint64_t request_id)
{
    if (c == NULL || c->reply_message_list == NULL)
    {
        return NULL;
    }

    ZOO_SMB_MSG_STRUCT* data = zoo_list_find_if(c->reply_message_list, client_compare_message, &request_id);
    return (data && data->header.msg_id == msg_id) ? data : NULL;
}

/**
 * @brief Sends an acknowledgment reply for a specific request.
 *
 * This function is responsible for sending an acknowledgment (ACK) message
 * back to the server or peer, indicating that the client has received and
 * processed a request identified by the given request ID.
 *
 * @param client        The handle to the SMB client instance.
 * @param request_id    The unique identifier of the request to acknowledge.
 */
static void client_send_replied_ack(ZOO_SMB_CLIENT_HANDLE client, uint32_t msg_id, uint64_t request_id)
{
    ZOO_SMB_MSG_STRUCT* ack_msg = zoo_smb_create_message(
        ZOO_SMB_MSG_TYPE_REPLACK, client->node->target, client->node->topic, NULL, 0, msg_id, request_id);
    if (ack_msg == NULL)
    {
        ZOO_LOG_ERROR("Failed to create ACK message for request_id=%llu", request_id);
        return;
    }

    // Send the ACK message to the target
    if (ZOO_SMB_OK != zoo_smb_send_message(client->node, ack_msg, NULL, ZOO_TRUE))
    {
        ZOO_LOG_ERROR("Failed to send ACK message for request_id=%llu", request_id);
    }
}

/**
 * @brief Callback function to handle incoming SMB messages for a node.
 *
 * This function is invoked when a new SMB message is received. It processes
 * the message in the context of the specified node.
 *
 * @param context Pointer to user-defined context data associated with the node.
 * @param message Pointer to the received ZOO_SMB_MSG_STRUCT structure containing
 *                the message data to be handled.
 */
static void client_on_handle_REPL_message_cb(IN void* context, IN const void* msg, IN size_t msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p,msg_len=%zu", msg, context, msg_len);
        return;
    }

    ZOO_SMB_CLIENT_STRUCT* client = (ZOO_SMB_CLIENT_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* reply_message = (ZOO_SMB_MSG_STRUCT*)msg;
    ZOO_LOG_INFO("Received REPL message for sender=%s,msg_id=%llu, req_id=%llu, payload_size=%zu",
                     reply_message->header.sender,
                     reply_message->header.msg_id,
                     reply_message->header.request_id,
                     reply_message->header.payload_size);
    if (reply_message->payload == NULL || reply_message->header.payload_size == 0)
    {
        ZOO_LOG_ERROR("Received reply message has no payload");
        return;
    }
    ZOO_SMB_MSG_STRUCT* message = zoo_smb_default_message();
    if (message == NULL)
    {
        ZOO_LOG_ERROR("Failed to allocate reply message container");
        return;
    }

    if (!zoo_smb_copy_message(reply_message, message))
    {
        ZOO_LOG_ERROR("Failed to copy reply message");
        zoo_smb_destroy_message(message);
        return;
    }
    ZOO_MUTEX_LOCK(&client->mutex);
    if (ZOO_SMB_OK != zoo_list_push_back(client->reply_message_list, message))
    {
        ZOO_MUTEX_UNLOCK(&client->mutex);
        ZOO_LOG_ERROR("Failed to push reply message to list");
        return;
    }
    ZOO_COND_SIGNAL(&client->reply_cond);
    ZOO_MUTEX_UNLOCK(&client->mutex);

    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(client->qos_entity);
    if (qos_policy && qos_policy->reliability_enabled)
    {
        client_send_replied_ack(client, message->header.msg_id, message->header.request_id);
        ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(client->qos_entity);
        zoo_smb_qos_ctx_set_state(qos_ctx, message->header.request_id, ZOO_SMB_MSG_ST_ACKED, NULL);
    }
}

/**
 * @brief Callback function to handle REQACK (Request Acknowledgement) messages from the client.
 *
 * This function processes incoming REQACK messages, typically used to acknowledge
 * the receipt or processing of a previous request in the SMB client protocol.
 *
 * @param context Pointer to user-defined context or state information.
 * @param msg Pointer to the received message data.
 * @param msg_len Length of the received message in bytes.
 */
static void client_on_handle_REQACK_message_cb(IN void* context, IN const void* msg, IN size_t msg_len)
{
    if (msg == NULL || context == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: msg=%p, context=%p, msg_len=%zu", msg, context, msg_len);
        return;
    }

    ZOO_SMB_CLIENT_STRUCT* client = (ZOO_SMB_CLIENT_STRUCT*)context;
    ZOO_SMB_MSG_STRUCT* message = (ZOO_SMB_MSG_STRUCT*)msg;
    ZOO_LOG_INFO("Received REQACK message for sender=%s, request_id=%llu, msg_id=%llu",
                     message->header.sender,
                     message->header.request_id,
                     message->header.msg_id);
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(client->qos_entity);
    if (qos_policy && qos_policy->reliability_enabled)
    {
        ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(client->qos_entity);
        zoo_smb_qos_ctx_set_state(
            qos_ctx, message->header.request_id, ZOO_SMB_MSG_ST_ACKED, NULL);
    }
}

/**
 * @brief Creates a new SMB client node.
 *
 * This function initializes and returns a new SMB client node with the specified name and target.
 *
 * @param name   The name to assign to the SMB client node.
 * @param target The target address or identifier for the SMB client connection.
 * @return ZOO_SMB_CLIENT_HANDLE  The created SMB client node.
 */

ZOO_SMB_CLIENT_HANDLE zoo_smb_create_client(
    IN const char* name,
    IN const char* target,
    IN const char* topic,
    IN const ZOO_SMB_QOS_POLICY_STRUCT* policy)
{
    ZOO_SMB_CLIENT_STRUCT* client = (ZOO_SMB_CLIENT_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_CLIENT_STRUCT));
    client->node = zoo_smb_create_node(name, target, topic, ZOO_SMB_NODE_TYPE_CLIENT, ZOO_SMB_TRANSPORT_TYPE_DEFAULT);
    if (client->node == NULL)
    {
        ZOO_LOG_ERROR("Failed to create client node");
        zoo_free_to_pool(client);
        return NULL;
    }

    client->reply_message_list = zoo_list_create(1024);
    if (client->reply_message_list == NULL)
    {
        ZOO_LOG_ERROR("Failed to create reply message list");
        zoo_smb_destroy_node(client->node);
        zoo_free_to_pool(client);
        return NULL;
    }

    if (policy)
    {
        client->qos_entity = zoo_smb_create_qos_entity(policy);
    }
    else
    {
        client->qos_entity = zoo_smb_create_qos_default_entity();
    }

    if (client->qos_entity == NULL)
    {
        ZOO_LOG_ERROR("Failed to create QoS entity for client: %s", name);
        zoo_smb_destroy_node(client->node);
        zoo_free_to_pool(client);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_STRUCT* repl_observer = zoo_smb_create_node_observer_and_insert(
        client->node->observers, client->node->name, 0, ZOO_SMB_MSG_TYPE_REPL, client_on_handle_REPL_message_cb, client, &client->id[0]);
    if (repl_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create reply observer");
        zoo_smb_destroy_node(client->node);
        zoo_free_to_pool(client);
        return NULL;
    }

    ZOO_SMB_NODE_OBSERVER_STRUCT* reqack_observer = zoo_smb_create_node_observer_and_insert(
        client->node->observers, client->node->name, 0, ZOO_SMB_MSG_TYPE_REQACK, client_on_handle_REQACK_message_cb, client, &client->id[1]);
    if (reqack_observer == NULL)
    {
        ZOO_LOG_ERROR("Failed to create request acknowledgment observer");
        zoo_smb_destroy_node(client->node);
        zoo_free_to_pool(client);
        return NULL;
    }

    if (ZOO_SMB_OK != zoo_smb_register_node(client->node))
    {
        ZOO_LOG_ERROR("Failed to register node");
        zoo_smb_destroy_node(client->node);
        zoo_free_to_pool(client);
        return NULL;
    }

    if (!ZOO_MUTEX_INIT(&client->mutex) || !ZOO_COND_INIT(&client->reply_cond))
    {
        ZOO_LOG_ERROR("Failed to initialize client synchronization primitives");
        zoo_smb_destroy_node(client->node);
        zoo_free_to_pool(client);
        return NULL;
    }
    ZOO_LOG_INFO("Created SMB client node: %s, target: %s, topic: %s", client->node->name, client->node->target, client->node->topic);
    return (ZOO_SMB_CLIENT_HANDLE)client;
}

/**
 * @brief Checks if the SMB server is ready and accessible
 *
 * This function verifies whether the specified SMB server is ready to accept
 * connections and handle requests.
 *
 * @param target The target SMB server address or hostname to check
 *
 * @return ZOO_TRUE if the server is ready and accessible, ZOO_FALSE otherwise
 */
ZOO_BOOL zoo_smb_is_server_ready(const char* target)
{
    return zoo_smb_service_is_online(target);
}

/**
 * @brief Destroys and cleans up an SMB client node handle
 *
 * This function releases all resources associated with the given SMB node handle,
 * including closing any open connections and freeing allocated memory.
 *
 * @param[in] client The SMB client handle to be destroyed.
 *
 * @note After calling this function, the node handle becomes invalid and should not be used
 */
void zoo_smb_destroy_client(IN ZOO_SMB_CLIENT_HANDLE client)
{
    ZOO_SMB_CLIENT_STRUCT* c = (ZOO_SMB_CLIENT_STRUCT*)client;
    if (c == NULL)
    {
        ZOO_LOG_ERROR("Invalid node handle: %p", c);
        return;
    }
    zoo_smb_destroy_qos_entity(c->qos_entity); /**< Clean up QoS entity */
    ZOO_COND_DESTROY(&c->reply_cond);
    ZOO_MUTEX_DESTROY(&c->mutex);
    zoo_smb_destroy_node(c->node);
    zoo_free_to_pool(client);
    ZOO_LOG_DEBUG("SMB client node destroyed: %p", c);
}

/**
 * @brief Sends one REQ message from client context.
 *
 * This call validates parameters and service availability, creates/assigns a
 * request id, updates QoS state, and dispatches asynchronously through transport.
 *
 * @param client Client handle.
 * @param msg_id Application message identifier.
 * @param payload Request payload pointer.
 * @param payload_size Request payload size in bytes.
 * @param request_id Output request identifier assigned by this call.
 * @return ZOO_SMB_OK on send success; otherwise a module error code.
 */
ZOO_ERROR_TYPE zoo_smb_client_send_request(ZOO_SMB_CLIENT_HANDLE client,
                                               IN uint32_t msg_id,
                                               IN const void* payload,
                                               IN size_t payload_size,
                                               INOUT int64_t* request_id)
{
    if (client == NULL || payload == NULL || request_id == NULL)
    {
        ZOO_LOG_ERROR("Invalid parameters: node=%s, request_id=%p",
                          client == NULL ? 0 : client->node->name,
                          request_id);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (!zoo_smb_service_is_online(client->node->target))
    {
        ZOO_LOG_ERROR("SMB service is offline: %s", client->node->target);
        return ZOO_SMB_ERROR_SERVICE_UNAVAILABLE;
    }

    *request_id = zoo_generate_uuid64();
    ZOO_SMB_MSG_STRUCT* req_msg = zoo_smb_create_message(
        ZOO_SMB_MSG_TYPE_REQ, client->node->name, client->node->topic, payload, payload_size, msg_id, *request_id);
    if (req_msg == NULL)
    {
        ZOO_LOG_ERROR("Failed to create request message");
        return ZOO_SMB_ERROR_OUT_OF_MEMORY;
    }

    ZOO_BOOL delete_message_after_send = ZOO_TRUE;
    ZOO_SMB_QOS_POLICY_HANDLE qos_policy = zoo_smb_qos_get_policy(client->qos_entity);
    if (qos_policy && qos_policy->reliability_enabled)
    {
        delete_message_after_send = ZOO_FALSE;
        ZOO_SMB_QOS_CTX_HANDLE qos_ctx = zoo_smb_qos_get_ctx(client->qos_entity);
        zoo_smb_qos_ctx_set_state(
            qos_ctx, *request_id, ZOO_SMB_MSG_ST_CREATED, req_msg);
    }

    ZOO_LOG_INFO("target:%s, topic:%s, msg_id: %u, payload_size: %zu, request_id: %lld", client->node->target, client->node->topic, msg_id, payload_size, *request_id);
    return zoo_smb_send_message(client->node, req_msg, NULL, delete_message_after_send);
}

/**
 * @brief Waits for buffered REPL matching msg_id and request_id.
 *
 * This call may block up to timeout_ms. On success, payload bytes are copied
 * into caller-provided output buffer and the buffered reply entry is removed.
 *
 * @param client Client handle.
 * @param msg_id Expected reply message identifier.
 * @param request_id Expected request identifier.
 * @param payload Output buffer pointer receiving copied payload bytes.
 * @param payload_size Output payload length in bytes.
 * @param timeout_ms Maximum wait duration in milliseconds.
 * @return ZOO_SMB_OK on success, or timeout/invalid-param/other module error code.
 */
ZOO_ERROR_TYPE zoo_smb_client_recv_reply(ZOO_SMB_CLIENT_HANDLE client,
                                             IN uint32_t msg_id,
                                             IN int64_t request_id,
                                             OUT void** payload,
                                             OUT size_t* payload_size,
                                             IN uint32_t timeout_ms)
{
    if (client == NULL || payload == NULL || payload_size == NULL)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }
    ZOO_LOG_DEBUG("Waiting for reply message: client:%s, request_id=%llu, timeout=%u ms",
                      client->node->name,
                      request_id,
                      timeout_ms);

    uint64_t start_time = zoo_get_timestamp_milliseconds(); /**< Record the start time for timeout calculation */
    ZOO_SMB_CLIENT_STRUCT* c = (ZOO_SMB_CLIENT_STRUCT*)client;
    ZOO_MUTEX_LOCK(&c->mutex);
    ZOO_SMB_MSG_STRUCT* replys_message = client_find_reply_message_locked(c, msg_id, request_id);
    while (!replys_message)
    {
        uint64_t elapsed = zoo_get_timestamp_milliseconds() - start_time;
        if (elapsed >= timeout_ms)
        {
            ZOO_MUTEX_UNLOCK(&c->mutex);
            ZOO_LOG_ERROR("Timeout waiting for reply message: client:%s, request_id=%llu, waited=%u ms",
                              client->node->name,
                              request_id,
                              timeout_ms);
            return ZOO_SMB_ERROR_TIMEOUT;
        }

        uint32_t wait_ms = (uint32_t)(timeout_ms - elapsed);
        if (!ZOO_COND_WAIT_TIMEOUT(&c->reply_cond, &c->mutex, wait_ms))
        {
            ZOO_MUTEX_UNLOCK(&c->mutex);
            ZOO_LOG_ERROR("Timeout waiting for reply message: client:%s, request_id=%llu, waited=%u ms",
                              client->node->name,
                              request_id,
                              timeout_ms);
            return ZOO_SMB_ERROR_TIMEOUT;
        }

        replys_message = client_find_reply_message_locked(c, msg_id, request_id);
    }

    memcpy(*payload, replys_message->payload, replys_message->header.payload_size); /**< Copy the payload to the output buffer */
    *payload_size = replys_message->header.payload_size;
    client_remove_reply_message_locked(client, replys_message); /**< Remove the reply message from the list */
    ZOO_MUTEX_UNLOCK(&c->mutex);
    ZOO_LOG_INFO("Received reply message: msg_id=%u, req_id=%llu, payload_size=%zu", msg_id, request_id, *payload_size);
    return ZOO_SMB_OK;
}
