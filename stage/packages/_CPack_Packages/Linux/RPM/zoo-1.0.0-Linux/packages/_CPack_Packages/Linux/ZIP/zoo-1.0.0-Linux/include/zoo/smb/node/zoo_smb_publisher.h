/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_NODE
 * File name: zoo_smb_publisher.h
 * Description: publisher node header file
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_SMB_PUBLISHER_H
#define ZOO_SMB_PUBLISHER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include "zoo_smb_error.h"
#include "zoo_smb_types.h"
#include "zoo_smb_qos.h"
    typedef struct ZOO_SMB_PUBLISHER_STRUCT* ZOO_SMB_PUBLISHER_HANDLE;

    /**
     * @brief Create a new publisher node.
     *
     * @param name Name of the publisher node.
     * @param topic Name of the topic to publish.
     * @return ZOO_SMB_NODE_HANDLE Handle to the newly created publisher node, or NULL on error.
     */
    ZOO_SMB_PUBLISHER_HANDLE zoo_smb_create_publisher(IN const char* name,
                                                      IN const char* target,
                                                      IN const char* topic,
                                                      IN ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type,
                                                      IN const ZOO_SMB_QOS_POLICY_STRUCT* profile);

    /**
     * @brief Destroys a ZOO SMB publisher
     *
     * Destroys and cleans up resources associated with a ZOO SMB publisher node.
     *
     * @param[in] node The handle to the ZOO SMB node to be destroyed
     *
     * @note After calling this function, the node handle becomes invalid and should not be used.
     */
    void zoo_smb_destroy_publisher(IN ZOO_SMB_PUBLISHER_HANDLE publisher);

    /**
     * @brief Publish a message to a topic.
     *
     * @param node Handle to the publisher node.
     * @param topic Name of the message topic.
     * @param payload Pointer to the message data.
     * @param payload_size Size of the message in bytes.
     * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_publish_message(
        IN ZOO_SMB_PUBLISHER_HANDLE publisher,
        IN uint32_t msg_id,
        IN const void* payload,
        IN size_t payload_size);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_PUBLISHER_H */
