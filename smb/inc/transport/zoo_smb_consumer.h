/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_CONSUMER
 * File name: zoo_smb_consumer.h
 * Description: Common definitions and types for the ZOO SMB library
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_SMB_CONSUMER_H
#define ZOO_SMB_CONSUMER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo_smb_types.h"
#include "zoo_smb_error.h"
#include "../../buffer/inc/zoo_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#define MAX_CONSUMER_NAME_LENGTH 64
#define MAX_CONSUMER_NUMBER 64

    typedef struct ZOO_SMB_CONSUMER_STRUCT* ZOO_SMB_CONSUMER_HANDLE;

    /**
     * @enum ZOO_SMB_CONSUMER_TYPE
     * @brief Enumeration of consumer types in the ZOO SMB system.
     */
    typedef struct ZOO_SMB_CONSUMER_STRUCT
    {
        char name[MAX_CONSUMER_NAME_LENGTH]; /**< Name of the consumer node */
        int fd;                              /**< Unique identifier for the consumer, can be a pointer or an integer */
        struct sockaddr_in addr;            /**< Address of the consumer */
    } ZOO_SMB_CONSUMER_STRUCT;

    /**
     * @brief Creates a new SMB consumer handle.
     *
     * This function initializes and returns a handle for an SMB consumer,
     * which can be used to interact with the SMB transport layer.
     *
     * @return ZOO_SMB_CONSUMER_HANDLE A handle to the newly created SMB consumer.
     */
    ZOO_SMB_CONSUMER_HANDLE zoo_smb_create_consumer(
        const char* name,
        int fd,
        struct sockaddr_in* addr);

    /**
     * @brief Finds and returns a handle to a SMB consumer.
     *
     * This function searches for an existing SMB consumer and returns a handle
     * to it if found. The criteria for finding the consumer depend on the
     * implementation details.
     *
     * @return ZOO_SMB_CONSUMER_HANDLE Handle to the found SMB consumer, or
     *         an invalid handle if not found.
     */
    ZOO_SMB_CONSUMER_HANDLE zoo_smb_consumer_find_by_name(
        ZOO_LIST_HANDLE list,
        const char* name);

    ZOO_SMB_CONSUMER_HANDLE zoo_smb_find_consumer_by_name(
        ZOO_LIST_HANDLE list,
        const char* name);

    ZOO_SMB_CONSUMER_HANDLE zoo_smb_find_consumer_by_fd(
        ZOO_LIST_HANDLE list,
        int32_t fd);

    /**
     * @brief Creates a new SMB consumer and inserts it into the specified linked list.
     *
     * @param list The handle to the linked list where the new consumer will be inserted.
     * @return ZOO_SMB_CONSUMER_HANDLE Handle to the newly created SMB consumer, or NULL on failure.
     */
    ZOO_SMB_CONSUMER_HANDLE zoo_smb_create_consumer_and_insert(ZOO_LIST_HANDLE list,
                                                               const char* name,
                                                               int32_t fd,
                                                               struct sockaddr_in* addr);
    /**
     * @brief Sets the ID for the SMB consumer.
     *
     * This function assigns a unique identifier to the SMB consumer instance.
     *
     * @param id The identifier to set for the consumer.
     * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the operation.
     */
    ZOO_ERROR_TYPE zoo_smb_consumer_set_fd(
        ZOO_SMB_CONSUMER_HANDLE consumer,
        int32_t fd);

    /**
     * @brief Retrieves the unique identifier for the SMB consumer.
     *
     * This function returns the ID associated with a specific SMB consumer instance.
     *
     * @return int32_t The unique identifier of the SMB consumer.
     */
    int32_t zoo_smb_consumer_get_fd(
        ZOO_SMB_CONSUMER_HANDLE consumer);

    /**
     * @brief Destroys a SMB consumer instance and releases associated resources.
     *
     * This function cleans up and deallocates any resources held by the specified
     * SMB consumer handle. After calling this function, the consumer handle should
     * not be used in any further operations.
     *
     * @param consumer The handle to the SMB consumer to be destroyed.
     * @return ZOO_ERROR_TYPE Error code indicating the result of the operation.
     *         Returns ZOO_SMB_ERROR_NONE on success, or an appropriate error code on failure.
     */
    ZOO_ERROR_TYPE zoo_smb_destroy_consumer(ZOO_SMB_CONSUMER_HANDLE consumer);
#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_CONSUMER_H */