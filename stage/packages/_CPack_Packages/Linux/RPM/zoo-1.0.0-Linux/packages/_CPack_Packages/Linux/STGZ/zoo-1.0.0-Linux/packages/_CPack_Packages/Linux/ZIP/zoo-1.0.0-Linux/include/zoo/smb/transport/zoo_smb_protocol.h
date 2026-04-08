/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_PROTOCOL
 * File name: zoo_smb_protocol.h
 * Description: Message protocol structure and operations for the ZOO SMB library
 *              Provides functions for creating, destroying, serializing, and
 *              deserializing message frames with variable-length payloads.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_SMB_PROTOCOL_H
#define ZOO_SMB_PROTOCOL_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo_smb_message.h"
#include "zoo_smb_service.h"
#include "zoo_smb_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef MAKE_DISCOVERY_PAYLOAD_FRAME
#define MAKE_DISCOVERY_PAYLOAD_FRAME(service, topic, addr, port, transport) \
    TOSTRING(service)                                                       \
    "-" TOSTRING(topic) "-" TOSTRING(addr) "-" TOSTRING(port) "-" TOSTRING(transport)
#endif

    /**
     * @brief Constructs a discovery message for the Zoo SMB protocol.
     *
     * This function creates and initializes a discovery message that can be used
     * to announce the presence or capabilities of a node within the Zoo SMB network.
        *
        * @param msg Message structure to populate.
        * @param service_name Service identifier to encode.
        * @param address Service address string.
        * @param port Service port.
        * @param topic Service topic name.
        * @param transport_type Transport type associated with the service.
        * @return ZOO_TRUE on success, ZOO_FALSE on failure.
     */
    ZOO_BOOL zoo_smb_protocol_make_discovery_payload_message(
        ZOO_SMB_MSG_STRUCT* msg,
        const char* service_name,
        const char* address,
        uint16_t port,
        const char* topic,
        ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type);

    /**
     * @brief Constructs a discovery message for the Zoo SMB protocol with multicast support.
     *
     * This function creates and initializes a discovery message that can be used
     * to announce the presence or capabilities of a node within the Zoo SMB network.
     * For UDP multicast services, it includes multicast group information.
     *
     * @param msg Pointer to message structure to populate
     * @param service_name Name of the service
     * @param address Service address (unicast) or multicast group address
     * @param port Service port
     * @param topic Service topic
     * @param transport_type Transport type (UDP_CLIENT, UDP_SERVER, UDP_PUBLISHER, UDP_SUBSCRIBER)
     * @param multicast_addr Multicast group address (for UDP_PUBLISHER/UDP_SUBSCRIBER, NULL for others)
     * @param multicast_port Multicast group port (for UDP_PUBLISHER/UDP_SUBSCRIBER, 0 for others)
     * @return ZOO_TRUE on success, ZOO_FALSE on failure
     */
    ZOO_BOOL zoo_smb_protocol_make_discovery_payload_message_ex(
        ZOO_SMB_MSG_STRUCT* msg,
        const char* service_name,
        const char* address,
        uint16_t port,
        const char* topic,
        ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type,
        const char* multicast_addr,
        uint16_t multicast_port);

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
        ZOO_SMB_TRANSPORT_TYPE_ENUM* transport_type);

    /**
     * @brief Parses a discovery message in the Zoo SMB protocol with multicast support.
     *
     * This function processes the incoming discovery message and extracts relevant
     * information according to the Zoo SMB protocol specification, including
     * multicast group information for UDP multicast services.
     *
     * @param msg Pointer to the message structure containing the discovery message
     * @param service_name Buffer to store service name
     * @param address Buffer to store service address
     * @param port Pointer to store service port
     * @param topic Buffer to store service topic
     * @param transport_type Pointer to store transport type
     * @param multicast_addr Buffer to store multicast group address (can be NULL)
     * @param multicast_port Pointer to store multicast group port (can be NULL)
     * @return ZOO_TRUE on success, ZOO_FALSE on failure
     */
    ZOO_BOOL zoo_smb_protocol_parse_discovery_payload_message_ex(
        const ZOO_SMB_MSG_STRUCT* msg,
        char* service_name,
        char* address,
        uint16_t* port,
        char* topic,
        ZOO_SMB_TRANSPORT_TYPE_ENUM* transport_type,
        char* multicast_addr,
        uint16_t* multicast_port);

    /**
     * @brief Swap the byte order of the message header fields if needed.
     * @param header Pointer to the original header.
     * @return Swapped header.
     */
    ZOO_SMB_MSG_HEADER_STRUCT zoo_smb_protocol_swap_header(const ZOO_SMB_MSG_HEADER_STRUCT* header);

    /**
     * @brief Validates the SMB message header
     *
     * Checks if the provided SMB message header structure contains valid values
     * according to the SMB protocol specification.
     *
     * @param header Pointer to the SMB message header structure to validate
     * @return ZOO_TRUE if the header is valid, ZOO_FALSE otherwise
     */
    ZOO_BOOL zoo_smb_protocol_is_header_valid(const ZOO_SMB_MSG_HEADER_STRUCT* header);

    /**
     * @brief Checks if the given SMB protocol message header is valid and matches the specified message type.
     *
     * This function validates the provided ZOO_SMB_MSG_HEADER_STRUCT structure and verifies
     * whether its message type corresponds to the expected msg_type.
     *
     * @param header Pointer to the SMB message header to validate.
     * @param msg_type The expected message type to check against the header.
     * @return ZOO_TRUE if the header is valid and the message type matches; ZOO_FALSE otherwise.
     */
    ZOO_BOOL zoo_smb_protocol_is_header_valid_with_msg_type(const ZOO_SMB_MSG_HEADER_STRUCT* header, int msg_type);

    /**
     * @brief Check if byte order swapping is needed (endianness check).
     * @return ZOO_TRUE if swap is needed, ZOO_FALSE otherwise.
     */
    ZOO_BOOL zoo_smb_protocol_need_swap(void);

    /**
     * @brief Serialize a message into a buffer.
     * @param msg Pointer to the message.
     * @param buffer Output buffer.
     * @param buffer_size Size of the output buffer.
     * @return Number of bytes written to the buffer.
     */
    size_t zoo_smb_protocol_serialize(const ZOO_SMB_MSG_STRUCT* msg,
                                      uint8_t* buffer,
                                      size_t buffer_size);

    /**
     * @brief Deserialize a buffer into a ZOO_SMB_MSG_STRUCT structure (memory provided by caller).
     *
     * This function parses a buffer containing serialized Zoo SMB protocol data and fills
     * the provided ZOO_SMB_MSG_STRUCT structure with the header and payload. The caller must
     * allocate sufficient memory for msg_out (at least sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + payload size).
     *
     * @param buffer        Pointer to the input buffer containing the serialized message.
     * @param buffer_size   Size of the input buffer in bytes.
     * @param msg_out       Pointer to the user-allocated ZOO_SMB_MSG_STRUCT structure to fill.
     * @param msg_out_size  Size of the msg_out buffer in bytes.
     * @return ZOO_SMB_OK on success, or an error code (e.g., ZOO_SMB_ERROR_INVALID_PARAM, ZOO_SMB_ERROR_ALLOCATION_FAILED).
     *
     * Example usage:
     *   size_t total_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + expected_payload_size;
     *   ZOO_SMB_MSG_STRUCT *msg = malloc(total_size);
     *   ZOO_ERROR_TYPE err = zoo_smb_protocol_deserialize(buf, buf_len, msg, total_size);
     */
    ZOO_ERROR_TYPE zoo_smb_protocol_deserialize(
        const uint8_t* buffer,
        size_t buffer_size,
        ZOO_SMB_MSG_STRUCT* msg_out,
        size_t msg_out_size);

    /**
     * @brief Converts a ZOO_SMB_NODE_TYPE_ENUM value to its corresponding ZOO_SMB_MSG_TYPE_ENUM.
     *
     * This function takes a node type enumeration value and returns the appropriate
     * message type enumeration value that corresponds to it. It is typically used
     * to map node types to message types for further processing or communication
     * within the SMB zoo module.
     *
     * @param node_type The node type to be converted (of type ZOO_SMB_NODE_TYPE_ENUM).
     * @return The corresponding message type (of type ZOO_SMB_MSG_TYPE_ENUM).
     */
    ZOO_SMB_MSG_TYPE_ENUM zoo_smb_protocol_convert_to_msg_type(ZOO_SMB_NODE_TYPE_ENUM node_type);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_PROTOCOL_H */