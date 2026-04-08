/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_SERVICE
 * File name: zoo_smb_service.h
 * Description: Service information structure for ZOO SMB
 *              This structure holds basic service information such as name, topic,
 *              address, port, transport type, last seen timestamp, online status,
 *              and broadcast interval.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-18     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_SMB_SERVICE_H
#define ZOO_SMB_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_types.h"
#include "zoo_smb_error.h"
#include "zoo_list.h"
#include <stdint.h>

#define MAX_SERVICE_NAME_LENGTH 64
#define MAX_SERVICE_ADDRESS_LENGTH 64
#define MAX_SERVICE_TOPIC_LENGTH 64
#define MAX_SERVICE_INFO_LIST_SIZE 256

    typedef struct ZOO_SMB_SERVICE_STRUCT* ZOO_SMB_SERVICE_HANDLE;

    typedef enum ZOO_SMB_SERVICE_TYPE_ENUM
    {
        ZOO_SMB_SERVICE_TYPE_DISCOVERY,    /**< Service for discovery */
        ZOO_SMB_SERVICE_TYPE_REGISTRATION, /**< Service for registration */
        ZOO_SMB_SERVICE_TYPE_BROADCAST,    /**< Service for broadcast */
        ZOO_SMB_SERVICE_TYPE_MAX
    } ZOO_SMB_SERVICE_TYPE_ENUM;

    /**
     * @brief Service information structure for storing basic service information
     */
    typedef struct ZOO_SMB_SERVICE_STRUCT
    {
        ZOO_SMB_SERVICE_TYPE_ENUM service_type;     /**< Type of the service */
        char name[MAX_SERVICE_NAME_LENGTH];         /**< Service name */
        char topic[MAX_SERVICE_TOPIC_LENGTH];       /**< Service topic */
        char address[MAX_SERVICE_ADDRESS_LENGTH];   /**< Service address */
        int port;                                   /**< Service port */
        char multicast_address[MAX_SERVICE_ADDRESS_LENGTH]; /**< Multicast address, if applicable */
        int multicast_port;                         /**< Multicast port, if applicable */
        ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type; /**< Transport type. */
        uint64_t last_seen;                         /**< Last discovery timestamp */
        ZOO_BOOL is_online;                             /**< Service online status */
        ZOO_BOOL is_server;                             /**< Service server status */
        ZOO_BOOL is_publisher;                          /**< Service publisher status */
        uint32_t broadcast_interval_ms;             /**< Broadcast interval in milliseconds */
    } ZOO_SMB_SERVICE_STRUCT;

    /**
     * @brief Creates and initializes a new ZOO_SMB_SERVICE_HANDLE instance.
     *
     * This function allocates and sets up a new service info handle for use with the SMB service.
     *
     * @return ZOO_SMB_SERVICE_HANDLE A handle to the newly created service info instance.
     *         Returns NULL if the creation fails.
     */
    ZOO_SMB_SERVICE_HANDLE zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_ENUM service_type,
        const char* name,
        const char* topic,
        const char* address,
        int port,
        const char* multicast_address,
        int multicast_port,
        ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type,
        uint32_t broadcast_interval_ms);

    /**
     * @brief Registers SMB service information with the Zoo SMB service.
     *
     * This function is responsible for registering the service information required
     * for the Zoo SMB service to operate correctly. It typically involves providing
     * details such as service name, configuration, and other relevant metadata.
     *
     * @return ZOO_ERROR_TYPE Returns an error code indicating the result of the registration.
     */
    ZOO_ERROR_TYPE zoo_smb_register_service(
        ZOO_LIST_HANDLE service_list,
        ZOO_SMB_SERVICE_HANDLE service);

    /**
     * @brief Unregisters the SMB service information from the Zoo service registry.
     *
     * This function removes the previously registered SMB service information,
     * making it unavailable for discovery by other components or services.
     *
     * @note Ensure that the service was previously registered before calling this function.
     */
    void zoo_smb_unregister_service(
        ZOO_LIST_HANDLE service_list,
        ZOO_SMB_SERVICE_HANDLE service);

    /**
     * @brief Destroys and frees resources associated with a SMB service info structure.
     *
     * This function should be called to properly clean up and release any memory or resources
     * allocated for the SMB service information. After calling this function, the service info
     * pointer should not be used unless it is re-initialized.
     *
     * @param service Pointer to the SMB service info structure to be destroyed.
     */
    void zoo_smb_destroy_service(
        ZOO_SMB_SERVICE_HANDLE service);

    /**
     * @brief Destroys the SMB service information and removes associated resources.
     *
     * This function is responsible for cleaning up and deallocating any resources
     * related to the SMB service information, and ensures that all references are
     * properly removed from the system.
     *
     * @note The specific resources and removal mechanisms depend on the implementation.
     */
    void zoo_smb_destroy_service_and_remove(
        ZOO_LIST_HANDLE service_list,
        ZOO_SMB_SERVICE_HANDLE service);

    /**
     * @brief Destroys a list of SMB service information structures.
     *
     * This function releases all memory and resources associated with the list
     * of SMB service information structures. After calling this function, the
     * list pointer should not be used unless reinitialized.
     *
     * @param list Pointer to the list of SMB service information structures to destroy.
     */
    void zoo_smb_destroy_service_list(
        ZOO_LIST_HANDLE service_list);

    /**
     * @brief Finds and retrieves SMB service information.
     *
     * This function searches for and returns a handle to the SMB service information.
     *
     * @return ZOO_SMB_SERVICE_HANDLE Handle to the SMB service information, or NULL if not found.
     */
    ZOO_SMB_SERVICE_HANDLE zoo_smb_find_service(
        ZOO_LIST_HANDLE service_list,
        const char* service_name);

    /**
     * @brief Copies information from one zoo SMB service structure to another.
     *
     * This function duplicates the relevant information from a source
     * zoo SMB service structure to a destination structure. It is typically
     * used to ensure that service information is consistently replicated
     * or transferred between different parts of the application.
     *
     * @param dest Pointer to the destination zoo SMB service structure.
     * @param src Pointer to the source zoo SMB service structure.
     */
    void zoo_smb_service_copy(
        const ZOO_SMB_SERVICE_HANDLE from,
        ZOO_SMB_SERVICE_HANDLE to);

    /**
     * @brief Creates a duplicate of the given ZOO_SMB_SERVICE_HANDLE.
     *
     * This function returns a new handle that represents the same SMB service as the original,
     * allowing for independent management of the duplicated handle.
     *
     * @param handle The original ZOO_SMB_SERVICE_HANDLE to duplicate.
     * @return A new ZOO_SMB_SERVICE_HANDLE that is a duplicate of the input handle, or NULL on failure.
     */
    ZOO_SMB_SERVICE_HANDLE zoo_smb_service_duplicate(
        ZOO_SMB_SERVICE_HANDLE service);

    /**
     * @brief Sets the address information for the SMB service.
     *
     * This function updates the address details associated with the SMB service.
     *
        * @param service Service handle to update.
        * @param address Address string to store in the service descriptor.
     */
    void zoo_smb_service_set_address(
        ZOO_SMB_SERVICE_HANDLE service,
        const char* address);

    /**
     * @brief Sets the port information for the SMB service.
     *
     * This function updates the port configuration used by the SMB service.
     *
     * @param port The port number to be set for the SMB service.
     */
    void zoo_smb_service_set_port(
        ZOO_SMB_SERVICE_HANDLE service,
        int port);

    /**
     * @brief Sets the broadcast interval for the SMB service information.
     *
     * This function configures how frequently the SMB service information is broadcasted
     * to the network. Adjusting the interval can help balance network traffic and service
     * discovery responsiveness.
     *
     * @param interval The broadcast interval in milliseconds.
     */
    void zoo_smb_service_set_broadcast_interval(
        ZOO_SMB_SERVICE_HANDLE service,
        uint32_t broadcast_interval_ms);

    /**
     * @brief Sets the last seen timestamp for the SMB service.
     *
     * This function updates the record of when the SMB service was last accessed or observed.
     *
        * @param service Service handle to update.
        * @param last_seen Last-seen timestamp in milliseconds.
     */
    void zoo_smb_service_set_last_seen(
        ZOO_SMB_SERVICE_HANDLE service,
        uint64_t last_seen);

    /**
     * @brief Sets the online status for the SMB service information.
     *
     * This function updates the online status of the SMB (Server Message Block) service,
     * indicating whether the service is currently online or offline.
     *
    * @param service Service handle to update.
    * @param is_online Online state flag.
     */
    void zoo_smb_service_set_online_status(
        ZOO_SMB_SERVICE_HANDLE service,
        ZOO_BOOL is_online);

    /**
     * @brief Sets the transport type for the SMB service information.
     *
     * This function configures the transport type used by the SMB service.
     *
        * @param service Service handle to update.
        * @param transport_type Transport type value.
     */
    void zoo_smb_service_set_transport_type(
        ZOO_SMB_SERVICE_HANDLE service,
        ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type);

    /**
     * @brief Retrieves the name of the SMB service.
     *
     * @return A constant pointer to a null-terminated string representing the name of the SMB service.
     */
    const char* zoo_smb_service_get_name(
        ZOO_SMB_SERVICE_HANDLE service);

    /**
     * @brief Sets the type for the SMB service.
     *
     * This function configures the type of the SMB (Server Message Block) service.
     *
     * @param type The type to set for the SMB service.
     */
    void zoo_smb_service_set_type(
        ZOO_SMB_SERVICE_HANDLE service,
        ZOO_SMB_SERVICE_TYPE_ENUM service_type);
#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_SERVICE_H */