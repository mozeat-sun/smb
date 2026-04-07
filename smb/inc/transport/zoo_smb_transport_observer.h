/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_TRANSPORT_OBSERVER
 * File name: zoo_smb_transport_observer.h
 * Description: Observer interface for ZOO Soft Message Bus (SMB)
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 ******************************************************************************/

#ifndef ZOO_SMB_TRANSPORT_OBSERVER_H
#define ZOO_SMB_TRANSPORT_OBSERVER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo_smb_protocol.h"
#include "zoo_smb_types.h"
#include "../../buffer/inc/zoo_list.h"

    typedef struct TRANSPORT_DATA_OBSERVER_STRUCT* TRANSPORT_DATA_OBSERVER_HANDLE;

    /**
     * @brief Callback function type for handling arrival of transport messages.
     *
     * This callback is invoked when a new message arrives via the transport layer.
     *
     * @param context      Pointer to the transport context or instance.
     * @param header       Pointer to the message header structure.
     * @param payoad       Pointer to the message payload data.
     * @param payoad_size  Size of the payload data in bytes.
     * @param user_data    Pointer to user-defined data provided during callback registration.
     */
    typedef void (*TRANSPORT_DATA_OBSERVER_CB)(
        IN void* user_data,
        IN const ZOO_SMB_MSG_STRUCT* message);

    /**
     * @brief Context structure for SMB transport data observer.
     *
     * This structure holds the callback function and user data for observing
     * transport messages in the SMB transport layer.
     *
     * @var observer
     *   Callback function to be invoked when a transport message arrives.
     *
     * @var user_data
     *   Pointer to user-defined data that will be passed to the callback.
     */
    typedef struct TRANSPORT_DATA_OBSERVER_STRUCT
    {
        TRANSPORT_DATA_OBSERVER_CB handler;
        void* user_data;
    } TRANSPORT_DATA_OBSERVER_STRUCT;

    /**
     * @brief Creates a new transport data observer context.
     *
     * This function allocates and initializes a context structure for observing
     * transport data events. The observer callback will be invoked with the provided
     * user data whenever relevant transport data events occur.
     *
     * @param handler    Callback function to be called on transport data events.
     * @param user_data  Pointer to user-defined data to be passed to the callback.
     * @return Pointer to the newly created TRANSPORT_DATA_OBSERVER_STRUCT, or
     *         nullptr on failure.
     */
    TRANSPORT_DATA_OBSERVER_HANDLE create_transport_data_observer(
        IN TRANSPORT_DATA_OBSERVER_CB handler,
        IN void* user_data);

    /**
     * @brief Creates a new transport data observer context and inserts it into the relevant data structure.
     *
     * This function allocates and initializes a TRANSPORT_DATA_OBSERVER_STRUCT instance,
     * and inserts it into the appropriate collection for managing transport data observers.
     *
     * @return Pointer to the newly created TRANSPORT_DATA_OBSERVER_STRUCT, or NULL on failure.
     */
    TRANSPORT_DATA_OBSERVER_HANDLE create_transport_data_observer_and_insert(
        IN ZOO_LIST_HANDLE list,
        IN TRANSPORT_DATA_OBSERVER_CB handler,
        IN void* user_data);

    /**
     * @brief Destroys and cleans up the resources associated with a TRANSPORT_DATA_OBSERVER_STRUCT context.
     *
     * This function releases any memory and resources held by the specified transport data observer context.
     * After calling this function, the context pointer should not be used unless it is reinitialized.
     *
     * @param observer Pointer to the TRANSPORT_DATA_OBSERVER_STRUCT to be destroyed.
     */
    void destroy_transport_data_observer(
        IN TRANSPORT_DATA_OBSERVER_HANDLE observer);

    /**
     * @brief Destroys the transport data observer context and removes it from the observer list.
     *
     * This function is responsible for cleaning up and deallocating any resources associated
     * with a transport data observer context, and ensuring it is properly removed from any
     * internal tracking structures or lists.
     *
     * @param observer Pointer to the transport data observer context to be destroyed and removed.
     */
    void destroy_transport_data_observer_and_remove(
        IN ZOO_LIST_HANDLE list,
        IN TRANSPORT_DATA_OBSERVER_HANDLE observer);

    /**
     * @brief Removes a previously registered transport data observer.
     *
     * This function unregisters an observer that was monitoring transport data events.
     * After calling this function, the specified observer will no longer receive notifications
     * about transport data changes.
     *
     * @param observer Pointer to the observer instance to be removed.
     */
    void remove_transport_data_observer(
        IN ZOO_LIST_HANDLE list,
        IN TRANSPORT_DATA_OBSERVER_HANDLE observer);

    /**
     * @brief Registers a new observer for transport data events.
     *
     * This function allows clients to add an observer that will be notified
     * when transport data events occur. The observer should implement the
     * appropriate callback interface to handle these events.
     *
     * @param observer Pointer to the observer instance to be added.
     */
    void add_transport_data_observer(
        IN ZOO_LIST_HANDLE list,
        IN TRANSPORT_DATA_OBSERVER_HANDLE observer);

    /**
     * @brief Finds and returns a handle to a data observer.
     *
     * This function searches for a data observer and returns its handle.
     *
     * @return TRANSPORT_DATA_OBSERVER_HANDLE Handle to the found data observer, or NULL if not found.
     */
    TRANSPORT_DATA_OBSERVER_HANDLE find_transport_data_observer(
        IN ZOO_LIST_HANDLE list,
        IN TRANSPORT_DATA_OBSERVER_CB handler);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_TRANSPORT_OBSERVER_H */