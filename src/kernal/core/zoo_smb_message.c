/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_MSG_STRUCT
 * File name: zoo_smb_message.c
 * Description: Implementation of SMB message creation, destruction, and utilities.
 *              Provides functions for message allocation, type conversion, and cleanup.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-14     weiwang.sun       created
 ******************************************************************************/

#include "zoo_smb_message.h"
#include "zoo_memory_pool.h"
#include "zoo_crc32.h"
#include "zoo_smb_config.h"
#include "zoo_util.h"
#include "zoo_smb_error.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/**
 * @brief Calculates the effective maximum payload size for SMB messages.
 *
 * Determines the maximum payload size that can be sent in a single SMB message,
 * accounting for the configured transport buffer size and the protocol header size.
 *
 * @return The maximum payload size in bytes that can be safely used for a message.
 *         Returns 0 if the transport budget is insufficient for a header.
 */
static size_t smb_effective_max_payload_size(void)
{
    size_t transport_budget = ZOO_SMB_DEFAULT_MAX_TRANSPORT_BUFFER_SIZE;
    const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_peek();

    if (config && config->sys.max_transport_buffer_size > 0)
    {
        transport_budget = config->sys.max_transport_buffer_size;
    }

    if (transport_budget <= sizeof(ZOO_SMB_MSG_HEADER_STRUCT))
    {
        return 0;
    }

    return transport_budget - sizeof(ZOO_SMB_MSG_HEADER_STRUCT);
}

/**
 * @brief Sets basic fields in message header
 *
 * This function initializes all fields in a ZOO SMB message header with
 * the provided parameters. It handles string field copying safely and
 * sets protocol-specific fields like magic number and version.
 *
 * @param header Pointer to header structure to initialize
 * @param msg_type Type of message (request, reply, etc.)
 * @param sender Sender identification string
 * @param target Target identification string
 * @param topic Topic/channel string
 * @param payload_size Size of payload in bytes
 * @param msg_id Unique message identifier
 * @param request_id Request correlation identifier
 *
 * @note Does nothing if header pointer is NULL
 * @note Sets timestamp to current time
 * @note CRC32 field is initialized to 0 (calculated separately)
 */
static void init_message_header(ZOO_SMB_MSG_HEADER_STRUCT* header, ZOO_SMB_MSG_TYPE_ENUM msg_type, const char* sender, const char* topic, size_t payload_size, uint32_t msg_id, uint64_t request_id)
{
    if (!header)
    {
        ZOO_LOG_ERROR("Cannot set header: NULL pointer");
        return;
    }

    ZOO_LOG_DEBUG("Setting message header: type=%u, payload_size=%zu, msg_id=%u",
                      msg_type,
                      payload_size,
                      msg_id);

    // Set string fields with safe copying
    zoo_safety_copy_string(header->sender, sender, sizeof(header->sender));
    zoo_safety_copy_string(header->topic, topic, sizeof(header->topic));

    // Set protocol fields
    header->magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    header->version = ZOO_SMB_MSG_VERSION;
    header->msg_type = msg_type;
    header->flags = 0;
    header->crc32 = 0;  // Will be set after payload CRC calculation
    header->payload_size = payload_size;
    header->msg_id = msg_id;
    header->timestamp = zoo_get_timestamp_milliseconds();
    header->request_id = request_id;

    ZOO_LOG_TRACE("Message header set successfully");
}

/**
 * @brief Allocates and copies payload data
 *
 * This function allocates memory for payload data and copies the provided
 * payload into the allocated buffer. It handles zero-length payloads
 * appropriately by setting the payload pointer to NULL.
 *
 * @param message Pointer to message structure to receive payload
 * @param payload Pointer to source payload data
 * @param payload_size Size of payload in bytes
 * @return ZOO_TRUE if allocation and copy succeed, ZOO_FALSE otherwise
 *
 * @note Sets payload pointer to NULL for zero-length payloads
 * @note Validates payload pointer for non-zero sizes
 * @note Uses memory pool allocation
 * @note Logs detailed information about allocation process
 */
static ZOO_BOOL allocate_and_copy_payload(ZOO_SMB_MSG_STRUCT* message, const void* payload, size_t payload_size)
{
    if (!message)
    {
        ZOO_LOG_ERROR("Cannot allocate payload: NULL message");
        return ZOO_FALSE;
    }

    if (payload_size == 0)
    {
        message->payload = NULL;
        ZOO_LOG_TRACE("No payload to allocate");
        return ZOO_TRUE;
    }

    if (!payload)
    {
        ZOO_LOG_ERROR("Payload data is NULL but payload_size > 0");
        return ZOO_FALSE;
    }

    message->payload = zoo_allocate_from_pool(payload_size);
    if (!message->payload)
    {
        ZOO_LOG_ERROR("Failed to allocate payload memory: size=%zu", payload_size);
        return ZOO_FALSE;
    }

    memcpy(message->payload, payload, payload_size);
    ZOO_LOG_TRACE("Payload allocated and copied: %zu bytes", payload_size);
    return ZOO_TRUE;
}

/**
 * @brief Calculates the CRC32 checksum for the given payload.
 *
 * This function computes the CRC32 checksum for the provided payload data.
 * It is used to ensure data integrity and detect errors in the message payload.
 *
 * @param header Pointer to the message header structure.
 * @param payload Pointer to the payload data.
 * @param payload_size Size of the payload in bytes.
 * @return The calculated CRC32 checksum.
 */
static uint32_t zoo_smb_calc_message_crc32(const uint8_t* payload, size_t payload_size)
{
    uint32_t crc = ~0U;
    if (payload && payload_size > 0)
        crc ^= zoo_calc_crc32(payload, payload_size);
    return crc;
}

/**
 * @brief Creates a new SMB message structure and allocates memory for payload.
 *
 * @param msg_type   The message type (enum).
 * @param sender     The sender name string.
 * @param topic      The topic name string.
 * @param payload    Pointer to the payload data.
 * @param payload_size Size of the payload in bytes.
 * @param msg_id     Unique message ID.
 * @param request_id Unique request ID.
 * @return Pointer to the newly created ZOO_SMB_MSG_STRUCT, or NULL on failure.
 */
ZOO_SMB_MSG_STRUCT* zoo_smb_create_message(
    IN ZOO_SMB_MSG_TYPE_ENUM msg_type,
    IN const char* sender,
    IN const char* topic,
    IN const void* payload,
    IN size_t payload_size,
    IN uint32_t msg_id,
    IN uint64_t request_id)
{
    if (!sender || !topic || (payload_size > 0 && !payload))
    {
        ZOO_LOG_ERROR("zoo_smb_create_message: Invalid parameters");
        return NULL;
    }

    if (payload_size > smb_effective_max_payload_size())
    {
        ZOO_LOG_ERROR("zoo_smb_create_message: Payload size exceeds configured transport budget: size=%zu max=%zu",
                      payload_size,
                      smb_effective_max_payload_size());
        return NULL;
    }

    ZOO_SMB_MSG_STRUCT* msg = (ZOO_SMB_MSG_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_MSG_STRUCT));
    if (!msg)
    {
        ZOO_LOG_ERROR("zoo_smb_create_message: Allocation failed");
        return NULL;
    }

    init_message_header(&msg->header, msg_type, sender, topic, payload_size, msg_id, request_id);
    if (!allocate_and_copy_payload(msg, payload, payload_size))
    {
        ZOO_LOG_ERROR("zoo_smb_create_message: Payload allocation failed");
        zoo_free_to_pool(msg);
        return NULL;
    }

    msg->header.crc32 = zoo_smb_calc_message_crc32(msg->payload, payload_size);
    ZOO_LOG_DEBUG("zoo_smb_create_message: Created msgid=%u, reqid=%llu, type=%u, sender=%s, topic=%s, crc32=%u, payload_size=%zu",
                      msg->header.msg_id,
                      msg->header.request_id,
                      msg->header.msg_type,
                      msg->header.sender,
                      msg->header.topic,
                      msg->header.crc32,
                      msg->header.payload_size);
    return msg;
}

/**
 * @brief Creates and returns a pointer to a default ZOO_SMB_MSG_STRUCT message.
 *
 * This function allocates and initializes a new ZOO_SMB_MSG_STRUCT with default values.
 * The caller is responsible for freeing the returned pointer when it is no longer needed.
 *
 * @return ZOO_SMB_MSG_STRUCT* Pointer to the newly created default message structure.
 */
ZOO_SMB_MSG_STRUCT* zoo_smb_default_message()
{
    ZOO_SMB_MSG_STRUCT* msg = (ZOO_SMB_MSG_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_MSG_STRUCT));
    if (!msg)
    {
        ZOO_LOG_ERROR("zoo_smb_default_message: Allocation failed");
        return NULL;
    }
    memset(msg, 0, sizeof(ZOO_SMB_MSG_STRUCT));
    ZOO_LOG_TRACE("Creating default message structure: %p", msg);
    return msg;
}

/**
 * @brief Converts a ZOO_SMB_MSG_TYPE_ENUM value to its corresponding string representation.
 *
 * @param msg_type The message type enumeration value to convert.
 * @return A constant character pointer to the string representation of the message type.
 */
const char* zoo_smb_msg_type_to_string(IN ZOO_SMB_MSG_TYPE_ENUM msg_type)
{
    switch (msg_type)
    {
        case ZOO_SMB_MSG_TYPE_REQ:
            return "REQ";
        case ZOO_SMB_MSG_TYPE_REPL:
            return "REPL";
        case ZOO_SMB_MSG_TYPE_REQACK:
            return "REQACK";
        case ZOO_SMB_MSG_TYPE_REPLACK:
            return "REPLACK";
        case ZOO_SMB_MSG_TYPE_PUB:
            return "PUB";
        case ZOO_SMB_MSG_TYPE_SUB:
            return "SUB";
        case ZOO_SMB_MSG_TYPE_UNSUB:
            return "UNSUB";
        case ZOO_SMB_MSG_TYPE_SUBACK:
            return "SUBACK";
        case ZOO_SMB_MSG_TYPE_PUBACK:
            return "PUBACK";
        case ZOO_SMB_MSG_TYPE_UNSUBACK:
            return "UNSUBACK";
        case ZOO_SMB_MSG_TYPE_HB:
            return "HB";
        case ZOO_SMB_MSG_TYPE_HBACK:
            return "HBACK";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Destroys a ZOO_SMB_MSG_STRUCT object and releases any associated resources.
 *
 * @param msg Pointer to the ZOO_SMB_MSG_STRUCT object to be destroyed.
 */
void zoo_smb_destroy_message(IN ZOO_SMB_MSG_STRUCT* msg)
{
    if (!msg)
        return;
    zoo_free_to_pool(msg->payload);
    zoo_free_to_pool(msg);
    ZOO_LOG_TRACE("Destroying message[%p]: msg_id=%u, type=%u, sender=%s, topic=%s", msg, msg->header.msg_id, msg->header.msg_type, msg->header.sender, msg->header.topic);
}

/**
 * @brief Creates and initializes a new SMB message header structure.
 *
 * This function allocates and sets up a new instance of the ZOO_SMB_MSG_HEADER_STRUCT,
 * which represents the header of an SMB message. The caller is responsible for freeing
 * the allocated memory when it is no longer needed.
 *
 * @return Pointer to the newly created ZOO_SMB_MSG_HEADER_STRUCT, or NULL on failure.
 */
ZOO_SMB_MSG_HEADER_STRUCT* zoo_smb_create_message_header(
    IN ZOO_SMB_MSG_TYPE_ENUM msg_type,
    IN const char* sender,
    IN const char* topic,
    IN size_t payload_size,
    IN uint32_t msg_id,
    IN uint64_t request_id)
{
    if (!sender || !topic)
    {
        ZOO_LOG_ERROR("zoo_smb_create_message_header: Invalid parameters");
        return NULL;
    }
    ZOO_SMB_MSG_HEADER_STRUCT* header = (ZOO_SMB_MSG_HEADER_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_MSG_HEADER_STRUCT));
    if (!header)
    {
        ZOO_LOG_ERROR("zoo_smb_create_message_header: Allocation failed");
        return NULL;
    }
    init_message_header(header, msg_type, sender, topic, payload_size, msg_id, request_id);
    return header;
}

/**
 * @brief Destroys and frees resources associated with a ZOO_SMB_MSG_HEADER_STRUCT message header.
 *
 * This function should be called to properly release any memory or resources
 * allocated for the specified message header structure.
 *
 * @param[in] msg_header Pointer to the ZOO_SMB_MSG_HEADER_STRUCT to be destroyed.
 */
void zoo_smb_destroy_message_header(IN ZOO_SMB_MSG_HEADER_STRUCT* msg_header)
{
    if (!msg_header)
    {
        ZOO_LOG_ERROR("zoo_smb_destroy_message_header: NULL pointer");
        return;
    }
    ZOO_LOG_DEBUG("Destroying message header: msg_id=%u, type=%u, sender=%s, topic=%s",
                      msg_header->msg_id,
                      msg_header->msg_type,
                      msg_header->sender,
                      msg_header->topic);
    zoo_free_to_pool(msg_header);
}

/**
 * @brief Copies the contents of one ZOO_SMB_MSG_STRUCT to another.
 *
 * @param from Pointer to the source ZOO_SMB_MSG_STRUCT to copy from.
 * @param to Pointer to destination ZOO_SMB_MSG_STRUCT to populate.
 *
 * This function copies header metadata and duplicates payload storage when
 * payload_size is non-zero.
 */
void zoo_smb_copy_message(IN const ZOO_SMB_MSG_STRUCT* from,
                          IN ZOO_SMB_MSG_STRUCT* to)
{
    if (!from || !to)
    {
        ZOO_LOG_ERROR("zoo_smb_copy_message: NULL pointer");
        return;
    }

    // Perform the copy (shallow copy in this case)
    memcpy(to, from, sizeof(ZOO_SMB_MSG_STRUCT));
    if (from->header.payload_size > 0)
    {
        to->payload = zoo_allocate_from_pool(from->header.payload_size);
        if (to->payload)
        {
            memcpy(to->payload, from->payload, from->header.payload_size);
        }
    }
}

/**
 * @brief Sets the fields of a ZOO SMB message header.
 *
 * @param header Pointer to a ZOO_SMB_MSG_HEADER_STRUCT structure to be initialized or modified.
 */
void zoo_smb_set_message_header(IN ZOO_SMB_MSG_HEADER_STRUCT* header,
                                IN ZOO_SMB_MSG_TYPE_ENUM msg_type,
                                IN const char* sender,
                                IN const char* topic,
                                IN size_t payload_size,
                                IN uint32_t msg_id,
                                IN uint64_t request_id)
{
    if (!header)
    {
        ZOO_LOG_ERROR("zoo_smb_set_message_header: NULL header pointer");
        return;
    }
    ZOO_LOG_DEBUG("Setting message header: type=%u, payload_size=%zu, msg_id=%u",
                      msg_type,
                      payload_size,
                      msg_id);
    init_message_header(header, msg_type, sender, topic, payload_size, msg_id, request_id);
    ZOO_LOG_TRACE("Message header set successfully: msg_id=%u, type=%u, sender=%s, topic=%s",
                      header->msg_id,
                      header->msg_type,
                      header->sender,
                      header->topic);
}