/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_PROTOCOL
 * File name: zoo_smb_protocol.c
 * Description: Implementation of the ZOO SMB message protocol.
 *              Provides serialization, deserialization, endianness handling,
 *              and CRC32 checksum calculation for message frames.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun       created
 * 1.1       2025-06-19     weiwang.sun       optimized with atomic functions and enhanced logging
 ******************************************************************************/

#include "zoo_smb_protocol.h"
#include "zoo_memory_pool.h"
#include "zoo_smb_config.h"
#include "zoo_util.h"
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

/**
 * @brief Validates the protocol magic number
 *
 * This function checks if the provided magic number matches the expected
 * ZOO SMB protocol magic number. This is used to verify that received
 * data is indeed a valid ZOO SMB protocol message.
 *
 * @param magic The magic number to validate
 * @return ZOO_TRUE if the magic number is valid, ZOO_FALSE otherwise
 *
 * @note Logs ERROR level message on validation failure
 * @note Logs TRACE level message on validation success
 */
static ZOO_BOOL validate_magic_number(uint32_t magic)
{
    if (magic != ZOO_SMB_MSG_MAGIC_NUMBER)
    {
        ZOO_LOG_ERROR("Invalid magic number: expected=0x%04X, actual=0x%04X",
                          ZOO_SMB_MSG_MAGIC_NUMBER,
                          magic);
        return ZOO_FALSE;
    }

    ZOO_LOG_TRACE("Magic number validation passed: 0x%04X", magic);
    return ZOO_TRUE;
}

/**
 * @brief Validates the protocol version
 *
 * This function verifies that the protocol version in the message header
 * is compatible with the current implementation. Version mismatches
 * indicate incompatible protocol implementations.
 *
 * @param version The protocol version to validate
 * @return ZOO_TRUE if the version is valid, ZOO_FALSE otherwise
 *
 * @note Logs ERROR level message on validation failure
 * @note Logs TRACE level message on validation success
 */
static ZOO_BOOL validate_protocol_version(uint16_t version)
{
    if (version != ZOO_SMB_MSG_VERSION)
    {
        ZOO_LOG_ERROR("Invalid protocol version: expected=%u, actual=%u",
                          ZOO_SMB_PROTOCOL_VERSION,
                          version);
        return ZOO_FALSE;
    }

    ZOO_LOG_TRACE("Protocol version validation passed: %u", version);
    return ZOO_TRUE;
}

/**
 * @brief Validates the payload size
 *
 * This function ensures that the payload size specified in the message
 * header does not exceed the maximum allowed payload size. This prevents
 * buffer overflow attacks and ensures memory safety.
 *
 * @param payload_size The payload size to validate in bytes
 * @return ZOO_TRUE if the payload size is within limits, ZOO_FALSE otherwise
 *
 * @note Logs ERROR level message if payload size exceeds maximum
 * @note Logs TRACE level message on successful validation
 */
static ZOO_BOOL validate_payload_size(uint32_t payload_size)
{
    size_t max_payload_size = ZOO_SMB_MAX_PAYLOAD_SIZE;
    const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_peek();

    if (config && config->sys.max_transport_buffer_size > sizeof(ZOO_SMB_MSG_HEADER_STRUCT))
    {
        max_payload_size = config->sys.max_transport_buffer_size - sizeof(ZOO_SMB_MSG_HEADER_STRUCT);
    }

    if (payload_size > max_payload_size)
    {
        ZOO_LOG_ERROR("Payload size exceeds maximum: max=%zu, actual=%u",
                          max_payload_size,
                          payload_size);
        return ZOO_FALSE;
    }

    ZOO_LOG_TRACE("Payload size validation passed: %u bytes", payload_size);
    return ZOO_TRUE;
}

/**
 * @brief Constructs a discovery message for the Zoo SMB protocol.
 *
 * This function creates and initializes a discovery message that can be used
 * to announce the presence or capabilities of a node within the Zoo SMB network.
 *
 * @note The specific parameters and return type should be documented based on the
 *       full function signature and implementation details.
 */
ZOO_BOOL zoo_smb_protocol_make_discovery_payload_message(
    ZOO_SMB_MSG_STRUCT* msg,
    const char* service_name,
    const char* address,
    uint16_t port,
    const char* topic,
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type)
{
    if (!msg || !service_name || !address || !topic)
    {
        ZOO_LOG_ERROR("Invalid parameters for making discovery message");
        return ZOO_FALSE;
    }

    msg->payload = zoo_allocate_from_pool(128);
    if (!msg->payload)
    {
        ZOO_LOG_ERROR("Failed to allocate memory for discovery message payload");
        return ZOO_FALSE;
    }
    memset(msg->payload, 0, 128);  // Ensure null-termination
    int n = snprintf((char*)msg->payload, 128, "%s-%s-%s-%hu-%d", service_name, topic, address, port, (int)transport_type);
    msg->header.payload_size = n;
    return ZOO_TRUE;
}

/**
 * @brief Parses a discovery message in the Zoo SMB protocol.
 *
 * This function processes the incoming discovery message and extracts relevant
 * information according to the Zoo SMB protocol specification.
 *
 * @param message Pointer to the buffer containing the discovery message.
 * @param length Length of the discovery message buffer in bytes.
 * @param[out] result Pointer to a structure where the parsed information will be stored.
 *
 * @return 0 on success, or a negative error code on failure.
 */
ZOO_BOOL zoo_smb_protocol_parse_discovery_payload_message(
    const ZOO_SMB_MSG_STRUCT* msg,
    char* service_name,
    char* address,
    uint16_t* port,
    char* topic,
    ZOO_SMB_TRANSPORT_TYPE_ENUM* transport_type)
{
    return sscanf((char*)msg->payload, "%31[^-]-%31[^-]-%31[^-]-%hu-%d", service_name, topic, address, port, (int*)transport_type) == 5;
}

/**
 * @brief Checks if the given SMB protocol message header is valid and matches the specified message type.
 *
 * This function verifies the validity of the provided ZOO_SMB_MSG_HEADER_STRUCT structure and determines
 * whether its message type corresponds to the expected msg_type.
 *
 * @param header Pointer to the ZOO_SMB_MSG_HEADER_STRUCT structure to validate.
 * @param msg_type The expected message type to check against the header.
 * @return ZOO_TRUE if the header is valid and the message type matches; ZOO_FALSE otherwise.
 */
ZOO_BOOL zoo_smb_protocol_is_header_valid_with_msg_type(const ZOO_SMB_MSG_HEADER_STRUCT* header, int msg_type)
{
    if (!zoo_smb_protocol_is_header_valid(header))
    {
        return ZOO_FALSE;
    }

    return header->msg_type == msg_type;
}

/**
 * @brief Determines if byte order swapping is needed
 *
 * This function checks the endianness of the current system and determines
 * whether byte order conversion is necessary for network byte order compliance.
 * The ZOO SMB protocol uses big-endian (network) byte order.
 *
 * @return ZOO_TRUE if byte swapping is needed (little-endian system), ZOO_FALSE otherwise
 *
 * @note Uses a compile-time constant for efficient endianness detection
 * @note Logs TRACE level message with endianness information
 */
ZOO_BOOL zoo_smb_protocol_need_swap(void)
{
    static const int endian_check = 1;
    ZOO_BOOL need_swap = (*(const char*)&endian_check == 0);

    ZOO_LOG_TRACE("Endianness check: need_swap=%s", need_swap ? "ZOO_TRUE" : "ZOO_FALSE");
    return need_swap;
}

/**
 * @brief Converts 16-bit value byte order if needed
 *
 * This function performs byte order conversion for 16-bit values when
 * operating on little-endian systems. It converts from host byte order
 * to big-endian (network) byte order or vice versa.
 *
 * @param value The 16-bit value to potentially swap
 * @param need_swap Whether byte swapping is required
 * @return The value in the correct byte order
 *
 * @note Uses be16toh() for efficient conversion
 * @note Logs TRACE level message when conversion is performed
 */
static uint32_t swap_uint16(uint16_t value, ZOO_BOOL need_swap)
{
    if (!need_swap)
        return value;

    uint16_t swapped = be16toh(value);
    ZOO_LOG_TRACE("Swapped uint16: 0x%04X -> 0x%04X", value, swapped);
    return swapped;
}

/**
 * @brief Converts 32-bit value byte order if needed
 *
 * This function performs byte order conversion for 32-bit values when
 * operating on little-endian systems. It handles conversion between
 * host byte order and big-endian (network) byte order.
 *
 * @param value The 32-bit value to potentially swap
 * @param need_swap Whether byte swapping is required
 * @return The value in the correct byte order
 *
 * @note Uses be32toh() for efficient conversion
 * @note Logs TRACE level message when conversion is performed
 */
static uint32_t swap_uint32(uint32_t value, ZOO_BOOL need_swap)
{
    if (!need_swap)
        return value;

    uint32_t swapped = be32toh(value);
    ZOO_LOG_TRACE("Swapped uint32: 0x%08X -> 0x%08X", value, swapped);
    return swapped;
}

/**
 * @brief Validates message header integrity
 *
 * This function performs comprehensive validation of a ZOO SMB message header,
 * including magic number verification, protocol version checking, and payload
 * size validation. It handles byte order conversion as needed.
 *
 * @param header Pointer to the message header to validate
 * @return ZOO_TRUE if the header is valid, ZOO_FALSE otherwise
 *
 * @note Returns ZOO_FALSE immediately if header pointer is NULL
 * @note Performs endianness-aware validation
 * @note Logs DEBUG level messages for validation process
 */
ZOO_BOOL zoo_smb_protocol_is_header_valid(const ZOO_SMB_MSG_HEADER_STRUCT* header)
{
    if (!header)
    {
        ZOO_LOG_ERROR("Header validation failed: NULL pointer");
        return ZOO_FALSE;
    }

    ZOO_BOOL need_swap = zoo_smb_protocol_need_swap();

    // Validate magic number
    uint32_t magic = swap_uint32(header->magic, need_swap);
    if (!validate_magic_number(magic))
        return ZOO_FALSE;

    // Validate protocol version
    uint16_t version = swap_uint16(header->version, need_swap);
    if (!validate_protocol_version(version))
        return ZOO_FALSE;

    // Validate payload size
    uint32_t payload_size = swap_uint32(header->payload_size, need_swap);
    if (!validate_payload_size(payload_size))
        return ZOO_FALSE;

    ZOO_LOG_DEBUG("Header validation successful: magic=0x%04X, version=%u, payload_size=%u",
                      magic,
                      version,
                      payload_size);
    return ZOO_TRUE;
}

/**
 * @brief Swaps byte order of message header fields
 *
 * This function converts all multi-byte numeric fields in a message header
 * from host byte order to big-endian (network) byte order or vice versa.
 * String fields are not affected by byte order conversion.
 *
 * @param header Pointer to the header to swap
 * @return A new header structure with swapped byte order
 *
 * @note Returns an empty header if input pointer is NULL
 * @note String fields (sender, target, topic) are copied without conversion
 * @note Uses htobe*() functions for host-to-big-endian conversion
 */
ZOO_SMB_MSG_HEADER_STRUCT zoo_smb_protocol_swap_header(const ZOO_SMB_MSG_HEADER_STRUCT* header)
{
    if (!header)
    {
        ZOO_LOG_ERROR("Cannot swap header: NULL pointer");
        ZOO_SMB_MSG_HEADER_STRUCT empty_header = {0};
        return empty_header;
    }

    ZOO_LOG_DEBUG("Swapping header byte order");

    ZOO_SMB_MSG_HEADER_STRUCT swapped;
    swapped.magic = htobe16(header->magic);
    swapped.version = htobe16(header->version);
    swapped.msg_type = htobe16(header->msg_type);
    swapped.flags = htobe16(header->flags);
    swapped.payload_size = htobe32(header->payload_size);
    swapped.crc32 = htobe32(header->crc32);
    swapped.msg_id = htobe32(header->msg_id);
    swapped.request_id = htobe64(header->request_id);
    swapped.timestamp = htobe64(header->timestamp);

    // String fields do not require byte order conversion
    memcpy(swapped.sender, header->sender, sizeof(swapped.sender));
    memcpy(swapped.topic, header->topic, sizeof(swapped.topic));

    ZOO_LOG_TRACE("Header byte order swapped successfully");
    return swapped;
}

/**
 * @brief Calculates required buffer size for message serialization
 *
 * This function determines the total buffer size needed to serialize a
 * complete ZOO SMB message, including both header and payload data.
 *
 * @param msg Pointer to the message structure
 * @return Total buffer size required in bytes, or 0 if message is NULL
 *
 * @note The calculated size includes header size plus payload size
 * @note Logs TRACE level message with calculated size
 */
static size_t calculate_required_buffer_size(const ZOO_SMB_MSG_STRUCT* msg)
{
    if (!msg)
        return 0;

    size_t required_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + msg->header.payload_size;
    ZOO_LOG_TRACE("Calculated required buffer size: %zu bytes", required_size);
    return required_size;
}

/**
 * @brief Copies message payload to buffer
 *
 * This function copies the message payload data to the buffer immediately
 * following the header. It handles cases where there is no payload data
 * (payload_size = 0) gracefully.
 *
 * @param msg Pointer to the message structure containing payload
 * @param buffer Pointer to the destination buffer
 *
 * @note Does nothing if either pointer is NULL
 * @note Payload is copied to buffer + sizeof(ZOO_SMB_MSG_HEADER_STRUCT)
 * @note Handles zero-length payloads safely
 */
static void copy_payload_to_buffer(const ZOO_SMB_MSG_STRUCT* msg, uint8_t* buffer)
{
    if (msg->header.payload_size > 0 && msg->payload)
    {
        memcpy(buffer + sizeof(ZOO_SMB_MSG_HEADER_STRUCT), msg->payload, msg->header.payload_size);
    }
}

/**
 * @brief Serializes a ZOO SMB message into a buffer
 *
 * This function converts a ZOO SMB message structure into a binary format
 * suitable for network transmission. It handles byte order conversion,
 * buffer size validation, and complete message serialization.
 *
 * @param msg Pointer to the message structure to serialize
 * @param buffer Pointer to the destination buffer
 * @param buffer_size Size of the destination buffer in bytes
 * @return Number of bytes written to buffer, or 0 on failure
 *
 * @note Returns 0 if buffer is too small or parameters are invalid
 * @note Clears the buffer before writing
 * @note Handles endianness conversion automatically
 * @note Logs INFO level message on successful serialization
 */
size_t zoo_smb_protocol_serialize(const ZOO_SMB_MSG_STRUCT* msg, uint8_t* buffer, size_t buffer_size)
{
    if (!msg || !buffer)
    {
        ZOO_LOG_ERROR("Serialization failed: NULL parameters");
        return 0;
    }

    // Calculate required buffer size
    size_t required_size = calculate_required_buffer_size(msg);
    if (buffer_size < required_size)
    {
        ZOO_LOG_ERROR("Buffer too small: required=%zu, available=%zu", required_size, buffer_size);
        return 0;
    }

    // Clear buffer
    memset(buffer, 0, buffer_size);

    // Prepare header with byte order conversion if needed
    ZOO_SMB_MSG_HEADER_STRUCT header = msg->header;
    if (zoo_smb_protocol_need_swap())
    {
        header = zoo_smb_protocol_swap_header(&header);
        ZOO_LOG_TRACE("Header byte order swapped for serialization");
    }

    // Copy header and payload to buffer
    memcpy(buffer, &header, sizeof(ZOO_SMB_MSG_HEADER_STRUCT));
    copy_payload_to_buffer(msg, buffer);

    ZOO_LOG_DEBUG("Message serialized successfully: type=%d, msgid=%u, reqid=%llu, topic=%s, sender=%s, payload_size=%u",
                      msg->header.msg_type,
                      msg->header.msg_id,
                      msg->header.request_id,
                      msg->header.topic,
                      msg->header.sender,
                      msg->header.payload_size);

    return required_size;
}

/**
 * @brief Validates parameters for message deserialization
 *
 * This function performs comprehensive parameter validation before
 * attempting to deserialize a message from a buffer. It checks for
 * NULL pointers and sufficient buffer sizes.
 *
 * @param buffer Pointer to the source buffer
 * @param buffer_size Size of the source buffer in bytes
 * @param msg_out Pointer to the output message structure
 * @param msg_out_size Size of the output message structure
 * @return ZOO_SMB_OK if parameters are valid, error code otherwise
 *
 * @note Checks for minimum buffer size to contain header
 * @note Validates output structure size
 * @note Logs ERROR level messages for validation failures
 */
static ZOO_ERROR_TYPE validate_deserialize_params(const uint8_t* buffer, size_t buffer_size, ZOO_SMB_MSG_STRUCT* msg_out, size_t msg_out_size)
{
    if (!buffer || !msg_out)
    {
        ZOO_LOG_ERROR("Deserialization failed: NULL parameters");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (buffer_size < sizeof(ZOO_SMB_MSG_HEADER_STRUCT))
    {
        ZOO_LOG_ERROR("Buffer too small for header: size=%zu, required=%zu",
                          buffer_size,
                          sizeof(ZOO_SMB_MSG_HEADER_STRUCT));
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (msg_out_size < sizeof(ZOO_SMB_MSG_STRUCT))
    {
        ZOO_LOG_ERROR("Output buffer too small: size=%zu, required=%zu",
                          msg_out_size,
                          sizeof(ZOO_SMB_MSG_STRUCT));
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Extracts and validates header from buffer
 *
 * This function extracts the message header from the beginning of a buffer
 * and performs validation including magic number verification. It handles
 * byte order conversion as needed.
 *
 * @param buffer Pointer to the source buffer containing header
 * @param header_out Pointer to store the extracted header
 * @return ZOO_SMB_OK if extraction succeeds, error code otherwise
 *
 * @note Validates magic number before accepting header
 * @note Performs endianness conversion if required
 * @note Logs TRACE level messages for conversion status
 */
static ZOO_ERROR_TYPE extract_and_validate_header(const uint8_t* buffer,
                                                      ZOO_SMB_MSG_HEADER_STRUCT* header_out)
{
    const ZOO_SMB_MSG_HEADER_STRUCT* header = (const ZOO_SMB_MSG_HEADER_STRUCT*)buffer;

    // Validate magic number
    ZOO_BOOL need_swap = zoo_smb_protocol_need_swap();
    uint32_t magic = swap_uint32(header->magic, need_swap);
    if (!validate_magic_number(magic))
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // Copy header with byte order conversion if needed
    if (need_swap)
    {
        *header_out = zoo_smb_protocol_swap_header(header);
        ZOO_LOG_TRACE("Header byte order swapped during extraction");
    }
    else
    {
        *header_out = *header;
        ZOO_LOG_TRACE("Header extracted without byte order conversion");
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Copies payload data during deserialization
 *
 * This function copies payload data from the buffer to the message structure
 * during deserialization. It validates buffer size and handles zero-length
 * payloads appropriately.
 *
 * @param buffer Pointer to the source buffer
 * @param buffer_size Size of the source buffer in bytes
 * @param header Pointer to the validated header structure
 * @param msg_out Pointer to the output message structure
 * @return ZOO_SMB_OK if copy succeeds, error code otherwise
 *
 * @note Validates total buffer size before copying
 * @note Handles zero-length payloads gracefully
 * @note Requires pre-allocated payload buffer in msg_out if payload_size > 0
 */
static ZOO_ERROR_TYPE copy_payload_data(const uint8_t* buffer, size_t buffer_size, ZOO_SMB_MSG_STRUCT* msg_out)
{
    if (msg_out->header.payload_size > 0 && msg_out->header.payload_size + sizeof(ZOO_SMB_MSG_HEADER_STRUCT) > buffer_size)
    {
        ZOO_LOG_ERROR("Buffer too small for payload: size=%zu, required=%zu",
                          buffer_size,
                          msg_out->header.payload_size + sizeof(ZOO_SMB_MSG_HEADER_STRUCT));
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (msg_out->header.payload_size > 0)
    {
        msg_out->payload = zoo_allocate_from_pool(msg_out->header.payload_size);
        if (msg_out->payload == NULL)
        {
            ZOO_LOG_ERROR("Failed to allocate memory for payload: size=%u", msg_out->header.payload_size);
            return ZOO_SMB_ERROR_ALLOCATION_FAILED;
        }
    }

    memcpy(msg_out->payload, buffer + sizeof(ZOO_SMB_MSG_HEADER_STRUCT), msg_out->header.payload_size);
    return ZOO_SMB_OK;
}

/**
 * @brief Deserializes a buffer into a ZOO SMB message
 *
 * This function converts binary data from a buffer back into a ZOO SMB
 * message structure. It handles header validation, byte order conversion,
 * and payload extraction.
 *
 * @param buffer Pointer to the source buffer containing serialized message
 * @param buffer_size Size of the source buffer in bytes
 * @param msg_out Pointer to the output message structure
 * @param msg_out_size Size of the output message structure
 * @return ZOO_SMB_OK on success, error code on failure
 *
 * @note The output message structure must have payload buffer pre-allocated
 * @note Performs comprehensive validation before deserialization
 * @note Handles endianness conversion automatically
 * @note Logs INFO level message on successful deserialization
 */
ZOO_ERROR_TYPE zoo_smb_protocol_deserialize(const uint8_t* buffer, size_t buffer_size, ZOO_SMB_MSG_STRUCT* msg_out, size_t msg_out_size)
{
    ZOO_LOG_DEBUG("[proto-debug] Starting message deserialization: buffer_size=%zu", buffer_size);
    if (!buffer || !msg_out || msg_out_size < sizeof(ZOO_SMB_MSG_STRUCT))
    {
        ZOO_LOG_ERROR("Deserialization failed: NULL parameters or insufficient output size");
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    ZOO_ERROR_TYPE result = validate_deserialize_params(buffer, buffer_size, msg_out, msg_out_size);
    if (result == ZOO_SMB_OK)
    {
        result = extract_and_validate_header(buffer, &msg_out->header);
    }

    if (result == ZOO_SMB_OK)
    {
        result = copy_payload_data(buffer, buffer_size, msg_out);
        ZOO_LOG_DEBUG("[proto-debug] After header extraction: msg_type=%u, payload_size=%u", msg_out->header.msg_type, msg_out->header.payload_size);
    }

    return result;
}

/**
 * @brief Create discovery payload with multicast support
 * Format: "service_name topic address:port transport_type multicast_addr:multicast_port"
 */
ZOO_BOOL zoo_smb_protocol_make_discovery_payload_message_ex(
    ZOO_SMB_MSG_STRUCT* msg,
    const char* service_name,
    const char* address,
    uint16_t port,
    const char* topic,
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type,
    const char* multicast_addr,
    uint16_t multicast_port)
{
    if (!msg || !service_name || !address || !topic)
        return ZOO_FALSE;

    char payload_buffer[512] = {0};
    int payload_len;

    // Check if multicast information should be included
    if (multicast_addr && strlen(multicast_addr) > 0 && multicast_port > 0)
    {
        payload_len = snprintf(payload_buffer, sizeof(payload_buffer), "%s %s %s:%d %d %s:%d", service_name, topic, address, port, transport_type, multicast_addr, multicast_port);
    }
    else
    {
        // Standard format without multicast info
        payload_len = snprintf(payload_buffer, sizeof(payload_buffer), "%s %s %s:%d 0:0 %d", service_name, topic, address, port, transport_type);
    }

    if (payload_len <= 0 || (size_t)payload_len >= sizeof(payload_buffer))
        return ZOO_FALSE;

    // Set payload
    if (msg->payload)
    {
        zoo_free_to_pool(msg->payload);
    }

    msg->payload = zoo_allocate_from_pool(payload_len + 1);
    if (!msg->payload)
        return ZOO_FALSE;

    memcpy(msg->payload, payload_buffer, payload_len);
    msg->payload[payload_len] = '\0';
    msg->header.payload_size = payload_len;
    ZOO_LOG_TRACE("Discovery payload created: %s", msg->payload);
    return ZOO_TRUE;
}

/**
 * @brief Parse discovery payload with multicast support
 * Format: "service_name topic address:port transport_type multicast_addr:multicast_port"
 */
ZOO_BOOL zoo_smb_protocol_parse_discovery_payload_message_ex(
    const ZOO_SMB_MSG_STRUCT* msg,
    char* service_name,
    char* address,
    uint16_t* port,
    char* topic,
    ZOO_SMB_TRANSPORT_TYPE_ENUM* transport_type,
    char* multicast_addr,
    uint16_t* multicast_port)
{
    if (!msg || !msg->payload || !service_name || !address || !port || !topic || !transport_type)
        return ZOO_FALSE;

    char* payload_copy = zoo_allocate_from_pool(msg->header.payload_size + 1);
    if (!payload_copy)
        return ZOO_FALSE;

    ZOO_LOG_TRACE("payload :%s", msg->payload);
    memcpy(payload_copy, msg->payload, msg->header.payload_size);
    payload_copy[msg->header.payload_size] = '\0';

    // Parse basic fields
    char* tokens[5];
    int token_count = 0;
    char* token = strtok(payload_copy, " ");
    while (token && token_count < 5)
    {
        tokens[token_count++] = token;
        token = strtok(NULL, " ");
    }
    if (token_count < 4)
    {
        ZOO_LOG_ERROR("Invalid discovery payload format: %s", payload_copy);
        zoo_free_to_pool(payload_copy);
        return ZOO_FALSE;
    }
    // Extract basic fields
    strncpy(service_name, tokens[0], 31);
    service_name[31] = '\0';  // Ensure null-termination
    strncpy(topic, tokens[1], 31);
    topic[31] = '\0';  // Ensure null-termination
    if (sscanf(tokens[2], "%31[^:]:%hu", address, port) != 2)
    {
        ZOO_LOG_ERROR("Invalid address format in payload: %s", tokens[2]);
        zoo_free_to_pool(payload_copy);
        return ZOO_FALSE;
    }
    *transport_type = (ZOO_SMB_TRANSPORT_TYPE_ENUM)atoi(tokens[3]);
    if (sscanf(tokens[4], "%31[^:]:%hu", multicast_addr, multicast_port) != 2)
    {
        ZOO_LOG_ERROR("Invalid address format in payload: %s", tokens[4]);
        zoo_free_to_pool(payload_copy);
        return ZOO_FALSE;
    }
    // Check for multicast information

    zoo_free_to_pool(payload_copy);
    return ZOO_TRUE;
}
