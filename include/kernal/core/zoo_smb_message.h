/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_MSG_STRUCT
 * File name: zoo_smb_message.h
 * Description: Message structure and operations for the ZOO SMB library
 *              Provides functions for creating, destroying, serializing, and
 *              deserializing message frames with variable-length payloads.
 *              Supports message types, flags, and status tracking.
 *              Includes QoS negotiation and message routing capabilities.
 *              Implements message ID generation and CRC32 checksum validation.
 *              Provides thread-safe operations for message handling.
 *              Designed for high-performance, low-latency communication in distributed systems.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-14     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_SMB_MESSAGE_H
#define ZOO_SMB_MESSAGE_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo_smb_error.h"
#include "zoo_log.h"
#include "zoo_smb_types.h"
#include <stdint.h>
#include <stdbool.h>

#define SMB_MULTICAST "SMB@MULTICAST"
#define SMB_BROADCAST "SMB@BROADCAST"

#define ZOO_SMB_MSG_MAGIC_NUMBER 0x12345678 /* Magic number for message format validation */
#define ZOO_SMB_MSG_VERSION 0x1001
#define ZOO_SMB_MAX_PAYLOAD_SIZE 0x1001
    /**
     * @enum ZOO_SMB_MSG_TYPE_ENUM
     * @brief Enumerates the different types of SMB messages used in the Zoo SMB protocol.
     *
     * This enumeration defines the various message types that can be exchanged in the protocol,
     * including requests, replies, acknowledgments, publish/subscribe operations, and heartbeat messages.
     *
     * @var ZOO_SMB_MSG_TYPE_MIN
     *      Minimum value for message type (reserved).
     * @var ZOO_SMB_MSG_TYPE_REQ
     *      Request message.
     * @var ZOO_SMB_MSG_TYPE_REPL
     *      Reply message.
     * @var ZOO_SMB_MSG_TYPE_ACK
     *      Acknowledgment message.
     * @var ZOO_SMB_MSG_TYPE_PUB
     *      Publish message.
     * @var ZOO_SMB_MSG_TYPE_SUB
     *      Subscribe message.
     * @var ZOO_SMB_MSG_TYPE_UNSUB
     *      Unsubscribe message.
     * @var ZOO_SMB_MSG_TYPE_SUBACK
     *      Subscribe acknowledgment message.
     * @var ZOO_SMB_MSG_TYPE_PUBACK
     *      Publish acknowledgment message.
     * @var ZOO_SMB_MSG_TYPE_UNSUBACK
     *      Unsubscribe acknowledgment message.
     * @var ZOO_SMB_MSG_TYPE_HB
     *      Heartbeat message.
     * @var ZOO_SMB_MSG_TYPE_HBACK
     *      Heartbeat acknowledgment message.
     * @var ZOO_SMB_MSG_TYPE_MAX
     *      Maximum value for message type (reserved).
     */
    typedef enum
    {
        ZOO_SMB_MSG_TYPE_MIN = 0x00, 
        ZOO_SMB_MSG_TYPE_REQ = 0x01,
        ZOO_SMB_MSG_TYPE_REPL = 0x02,
        ZOO_SMB_MSG_TYPE_REQACK = 0x03,
        ZOO_SMB_MSG_TYPE_REPLACK = 0x04,
        ZOO_SMB_MSG_TYPE_PUB = 0x10,
        ZOO_SMB_MSG_TYPE_SUB = 0x11,
        ZOO_SMB_MSG_TYPE_UNSUB = 0x12,
        ZOO_SMB_MSG_TYPE_SUBACK = 0x13,
        ZOO_SMB_MSG_TYPE_PUBACK = 0x14,
        ZOO_SMB_MSG_TYPE_UNSUBACK = 0x15,
        ZOO_SMB_MSG_TYPE_HB = 0x20,
        ZOO_SMB_MSG_TYPE_HBACK = 0x21,
        ZOO_SMB_MSG_TYPE_MAX = 0x55,
    } ZOO_SMB_MSG_TYPE_ENUM;

    /**
     * @enum ZOO_SMB_MSG_FLAG_ENUM
     * @brief Enumeration of message flags used in the SMB core module.
     *
     * This enum defines bitwise flags that can be set on SMB messages to indicate
     * various processing requirements such as compression, encryption, or serialization.
     *
     * @var ZOO_SMB_MSG_FLAG_COMPRESS
     *      Indicates that the message should be compressed.
     * @var ZOO_SMB_MSG_FLAG_ENCRYPT
     *      Indicates that the message should be encrypted.
     * @var ZOO_SMB_MSG_FLAG_SERIALIZE
     *      Indicates that the message should be serialized.
     */
    typedef enum
    {
        ZOO_SMB_MSG_FLAG_COMPRESS = 0x01,   // 压缩标志
        ZOO_SMB_MSG_FLAG_ENCRYPT = 0x02,    // 加密标志
        ZOO_SMB_MSG_FLAG_SERIALIZE = 0x03,  // 序列化标志
    } ZOO_SMB_MSG_FLAG_ENUM;

    /**
     * @enum ZOO_SMB_MSG_ST_ENUM
     * @brief Enumerates the possible states of an SMB message in the Zoo system.
     *
     * This enumeration defines the various states that an SMB (Soft Message Bus) message
     * can be in during its lifecycle within the Zoo system.
     *
     * @var ZOO_SMB_MSG_ST_CREATED
     *      The message has been created and is ready for processing.
     * @var ZOO_SMB_MSG_ST_SENDING
     *      The message is currently being sent.
     * @var ZOO_SMB_MSG_ST_WAITING_ACK
     *      The message has been sent and is waiting for an acknowledgment.
     * @var ZOO_SMB_MSG_ST_ACKED
     *      The message has been acknowledged as received.
     * @var ZOO_SMB_MSG_ST_FAILED
     *      The message failed to be sent or acknowledged.
     */
    typedef enum
    {
        ZOO_SMB_MSG_ST_CREATED = 0X01,
        ZOO_SMB_MSG_ST_SENDING = 0X02,
        ZOO_SMB_MSG_ST_WAITING_ACK = 0X03,
        ZOO_SMB_MSG_ST_ACKED = 0X04,
        ZOO_SMB_MSG_ST_TIMEOUT = 0X05,
        ZOO_SMB_MSG_ST_FAILED = 0X06,
        ZOO_SMB_MSG_ST_UNKNOWN = 0X07,
    } ZOO_SMB_MSG_ST_ENUM;

#pragma pack(push, 1)
    /**
     * @brief Message header structure (20 bytes, packed)
     */
    typedef struct __attribute__((packed))
    {
        uint32_t magic;        /* Magic number for message format validation (4 bytes) */
        uint16_t version;      /* Protocol version (2 bytes) */
        uint16_t msg_type;     /* Message type (2 bytes) */
        uint16_t flags;        /* Message flags (2 bytes) */
        uint32_t crc32;        /* CRC32 checksum of the message (4 bytes) */
        uint32_t payload_size; /* Payload size in bytes (4 bytes) */
        uint32_t msg_id;       /* Unique message ID (4 bytes) */
        uint32_t sequence;     /* Sequence number for message ordering (4 bytes) */
        uint64_t timestamp;    /* Timestamp (8 bytes) */
        uint64_t request_id;   /* Unique request ID (8 bytes) */
        char sender[32];       /* Sender name (32 bytes) */
        char topic[32];        /* Topic name (32 bytes) */
    } ZOO_SMB_MSG_HEADER_STRUCT;

    /**
     * @brief Structure representing an SMB message.
     *
     * This structure encapsulates an SMB message, including its header and payload.
     *
     * @typedef ZOO_SMB_MSG_STRUCT
     *
     * @member header
     *   The header of the SMB message, containing metadata and control information.
     *
     * @member payload
     *   Pointer to the message payload (data content).
     */
    typedef struct __attribute__((packed))
    {
        ZOO_SMB_MSG_HEADER_STRUCT header;
        uint8_t* payload;
    } ZOO_SMB_MSG_STRUCT;

#pragma pack(pop)

    /**
     * @brief Converts a ZOO_SMB_MSG_TYPE_ENUM value to its corresponding ZOO_SMB_NODE_TYPE_ENUM.
     *
     * This function takes a message type enumeration value and returns the appropriate
     * node type enumeration value that corresponds to it. It is typically used
     * to map message types to node types for further processing or communication
     * within the SMB zoo module.
     * @param memory_pool The memory pool handle to allocate memory from.
     * @param msg_type The message type to be converted (of type ZOO_SMB_MSG_TYPE_ENUM)
     * @param sender The sender name (of type const char*).
     * @param topic The topic name (of type const char*).
     * @param payload The payload data (of type const void*).
     * @return The corresponding node type (of type ZOO_SMB_NODE_TYPE_ENUM).
     * @note The function assumes that the input message type is valid and corresponds to a known node type.
     *       If the input message type is not recognized, the function will return ZOO_SMB_NODE_UNKNOWN.
     *       Ensure that the input message type is valid before calling this function.
     */
    ZOO_SMB_MSG_STRUCT* zoo_smb_create_message(
        IN ZOO_SMB_MSG_TYPE_ENUM msg_type,
        IN const char* sender,
        IN const char* topic,
        IN const void* payload,
        IN size_t payload_size,
        IN uint32_t msg_id,
        IN uint64_t request_id);

    /**
     * @brief Creates and returns a pointer to a default ZOO_SMB_MSG_STRUCT message.
     *
     * This function allocates and initializes a new ZOO_SMB_MSG_STRUCT with default values.
     * The caller is responsible for freeing the returned pointer when it is no longer needed.
     *
     * @return ZOO_SMB_MSG_STRUCT* Pointer to the newly created default message structure.
     */
    ZOO_SMB_MSG_STRUCT* zoo_smb_default_message();

    /**
     * @brief Destroys a ZOO_SMB_MSG_STRUCT object and releases any associated resources.
     *
     * This function should be called to properly clean up and free memory allocated
     * for a ZOO_SMB_MSG_STRUCT structure when it is no longer needed.
     *
     * @param msg Pointer to the ZOO_SMB_MSG_STRUCT object to be destroyed.
     */
    void zoo_smb_destroy_message(IN ZOO_SMB_MSG_STRUCT* msg);

    /**
     * @brief Converts a ZOO_SMB_MSG_TYPE_ENUM value to its corresponding string representation.
     *
     * @param msg_type The message type enumeration value to convert.
     * @return A constant character pointer to the string representation of the message type.
     */
    const char* zoo_smb_msg_type_to_string(IN ZOO_SMB_MSG_TYPE_ENUM msg_type);

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
        IN uint64_t request_id);

    /**
     * @brief Destroys and frees resources associated with a ZOO_SMB_MSG_HEADER_STRUCT message header.
     *
     * This function should be called to properly release any memory or resources
     * allocated for the specified message header structure.
     *
     * @param[in] msg_header Pointer to the ZOO_SMB_MSG_HEADER_STRUCT to be destroyed.
     */
    void zoo_smb_destroy_message_header(IN ZOO_SMB_MSG_HEADER_STRUCT* msg_header);

    /**
     * @brief Deep-copies one message into another.
     *
     * @param from Pointer to source message.
     * @param to Pointer to destination message.
     * @return ZOO_TRUE on success, ZOO_FALSE when copy fails.
     *
     * On failure, destination message is left in a safe zeroed state.
     */
    ZOO_BOOL zoo_smb_copy_message(IN const ZOO_SMB_MSG_STRUCT* from,
                                  IN ZOO_SMB_MSG_STRUCT* to);

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
                                    IN uint64_t request_id);    
#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_MESSAGE_H */