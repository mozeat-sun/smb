/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: buffer
 * Component id: list
 * File name: zoo_list.c
 * Description: Cross-platform doubly linked list implementation for ZOO
 *              Supports Windows, Linux, FreeRTOS, CMSIS-RTOS, and bare metal
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 * 1.1       2025-07-31     weiwang.sun         added cross-platform support
 ******************************************************************************/

// Suppress compiler warnings for unused mutex return values
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"

#include "zoo_list.h"
#include "zoo.h"
#include "zoo_error.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Node structure for linked list implementation
 */
typedef struct NODE_STRUCT
{
    void* data;               /**< Pointer to stored data */
    struct NODE_STRUCT* next; /**< Pointer to next node */
} NODE_STRUCT;

/**
 * @brief List control structure
 */
typedef struct ZOO_LIST_STRUCT
{
    NODE_STRUCT* head;      /**< Head node pointer */
    NODE_STRUCT* tail;      /**< Tail node pointer */
    ZOO_SIZE_T size;            /**< Current list size */
    ZOO_SIZE_T max_list_length; /**< Maximum allowed size */
    ZOO_MUTEX_T mutex;      /**< Cross-platform thread synchronization */
    ZOO_COND_T cond;        /**< Cross-platform condition variable for signaling */
} ZOO_LIST_STRUCT;

/**
 * @brief Helper function to check if list is full
 */
static ZOO_BOOL is_list_full(const ZOO_LIST_STRUCT* list)
{
    return (list->size >= list->max_list_length);
}

/**
 * @brief Helper function to lock list mutex
 */
static ZOO_BOOL lock_list(ZOO_LIST_STRUCT* list)
{
    return ZOO_MUTEX_LOCK(&list->mutex);
}

/**
 * @brief Helper function to unlock list mutex
 */
static ZOO_BOOL unlock_list(ZOO_LIST_STRUCT* list)
{
    return ZOO_MUTEX_UNLOCK(&list->mutex);
}

/**
 * @brief Creates a new linked list for SMB (Server Message Block) handling
 *
 * @param max_list_length Maximum number of elements that can be stored in the list
 *
 * @return ZOO_LIST_HANDLE Handle to the newly created linked list
 *         NULL if creation fails
 *
 * This function allocates and initializes a new linked list structure used for
 * SMB operations. The list is bounded by the specified maximum length to prevent
 * unbounded growth and memory exhaustion.
 */
ZOO_LIST_HANDLE zoo_list_create(ZOO_SIZE_T max_list_length)
{

    if (max_list_length == 0)
    {
        return NULL;
    }

    ZOO_LIST_STRUCT* list = (ZOO_LIST_STRUCT*)malloc(sizeof(ZOO_LIST_STRUCT));

    if (!list)
    {
        return NULL;
    }

    if (!ZOO_MUTEX_INIT(&list->mutex))
    {
        free(list);
        return NULL;
    }

    if (!ZOO_COND_INIT(&list->cond))
    {
        ZOO_MUTEX_DESTROY(&list->mutex);
        free(list);
        return NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->max_list_length = max_list_length;
    return (ZOO_LIST_HANDLE)list;
}

/**
 * @brief Adds an element to the back (end) of the list
 *
 * @param list Handle to the linked list where the element will be added
 * @param data Pointer to the data to be added to the list
 * @return ZOO_ERROR_T Returns error code indicating success or failure
 *         - ZOO_OK: Element successfully added
 *         - ZOO_ERROR_BUFFER_INVALID_HANDLE: Invalid list handle provided
 *         - ZOO_ERROR_NULL_POINTER: Data pointer is NULL
 *         - ZOO_ERROR_BUFFER_ALLOCATION_FAILED: Memory allocation failed
 */
ZOO_ERROR_T zoo_list_push_back(ZOO_LIST_HANDLE list, const void* data)
{
    if (!list || !data)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return ZOO_ERROR_MUTEX_LOCK_FAILED;
    }

    if (is_list_full(linked_list))
    {
        unlock_list(linked_list);
        return ZOO_ERROR_BUFFER_FULL;
    }

    NODE_STRUCT* node = malloc(sizeof(NODE_STRUCT));

    if (!node)
    {
        unlock_list(linked_list);
        return ZOO_ERROR_OUT_OF_MEMORY;
    }

    node->data = (void*)data;
    node->next = NULL;

    if (!linked_list->head)
    {
        linked_list->head = node;
        linked_list->tail = node;
    }
    else
    {
        linked_list->tail->next = node;
        linked_list->tail = node;
    }

    linked_list->size++;

    unlock_list(linked_list);
    return ZOO_OK;
}

/**
 * @brief Pushes a new element at the front of the linked list
 *
 * @param list Handle to the linked list where the element will be added
 * @param data Pointer to the data to be stored in the new element
 *
 * @return true if the operation was successful
 * @return false if the operation failed (e.g., memory allocation error)
 */
ZOO_ERROR_T zoo_list_push_front(ZOO_LIST_HANDLE list, const void* data)
{
    if (!list || !data)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return ZOO_ERROR_MUTEX_LOCK_FAILED;
    }

    if (is_list_full(linked_list))
    {
        unlock_list(linked_list);
        return ZOO_ERROR_BUFFER_FULL;
    }

    NODE_STRUCT* node = malloc(sizeof(NODE_STRUCT));

    if (!node)
    {
        unlock_list(linked_list);
        return ZOO_ERROR_OUT_OF_MEMORY;
    }

    node->data = (void*)data;
    node->next = linked_list->head;

    if (!linked_list->head)
    {
        linked_list->tail = node;
    }
    linked_list->head = node;
    linked_list->size++;

    unlock_list(linked_list);
    return ZOO_OK;
}

/**
 * @brief Removes and returns the last element from the list
 *
 * @param list Handle to the linked list
 * @return void* Pointer to the removed element's data, NULL if list is empty
 *
 * @note The caller is responsible for freeing the returned memory if required
 */
void* zoo_list_pop_back(ZOO_LIST_HANDLE list)
{

    if (!list)
    {
        return NULL;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return NULL;
    }

    if (!linked_list->head)
    {
        unlock_list(linked_list);
        return NULL;
    }

    void* data;

    // Special case: only one node
    if (linked_list->head == linked_list->tail)
    {
        data = linked_list->head->data;
        free(linked_list->head);
        linked_list->head = NULL;
        linked_list->tail = NULL;
        linked_list->size--;
        unlock_list(linked_list);
        return data;
    }

    // Find the second-to-last node
    NODE_STRUCT* current = linked_list->head;
    while (current->next != linked_list->tail)
    {
        current = current->next;
    }

    // Remove the last node
    data = linked_list->tail->data;
    free(linked_list->tail);
    linked_list->tail = current;
    linked_list->tail->next = NULL;
    linked_list->size--;

    unlock_list(linked_list);
    return data;
}

/**
 * @brief Removes and returns the data from the front of the linked list
 *
 * @param list The handle to the linked list
 * @return void* Pointer to the data that was at the front of the list, or NULL if the list is empty
 *
 * @note The caller is responsible for freeing the returned data if necessary
 */
void* zoo_list_pop_front(ZOO_LIST_HANDLE list)
{
    if (!list)
    {
        return NULL;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return NULL;
    }

    if (!linked_list->head)
    {
        unlock_list(linked_list);
        return NULL;
    }

    NODE_STRUCT* node = linked_list->head;
    void* data = node->data;
    linked_list->head = node->next;

    if (!linked_list->head)
    {
        linked_list->tail = NULL;
    }

    linked_list->size--;
    free(node);
    unlock_list(linked_list);
    return data;
}

/**
 * @brief Clears all elements from the linked list
 *
 * @param list Handle to the linked list to be cleared
 *
 * This function removes all nodes from the list and frees their memory,
 * but preserves the list structure itself. The list can be reused after
 * clearing. Thread-safe operation is ensured through mutex locking.
 */
void zoo_list_clear(ZOO_LIST_HANDLE list)
{

    if (!list)
    {
        return;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return;
    }

    ZOO_SIZE_T nodes_cleared = 0;
    NODE_STRUCT* current = linked_list->head;
    while (current)
    {
        NODE_STRUCT* next = current->next;
        free(current);
        current = next;
        nodes_cleared++;
    }

    linked_list->head = NULL;
    linked_list->tail = NULL;
    linked_list->size = 0;

    unlock_list(linked_list);
}

/**
 * @brief Gets the element at the specified index in the list
 *
 * @param list Handle to the linked list
 * @param index Position of the element to retrieve (0-based)
 * @param data Pointer to store the retrieved element
 * @return ZOO_ERROR_T Status code
 */
void* zoo_list_at(
    ZOO_LIST_HANDLE list,
    ZOO_SIZE_T index)
{

    if (!list)
    {
        return NULL;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return NULL;
    }

    if (index >= linked_list->size)
    {
        unlock_list(linked_list);
        return NULL;
    }

    NODE_STRUCT* current = linked_list->head;
    for (ZOO_SIZE_T i = 0; i < index; i++)
    {
        current = current->next;
    }

    void* data = (void*)current->data;

    unlock_list(linked_list);
    return data;
}

/**
 * @brief Returns the last element of the linked list.
 *
 * @param list Handle to the linked list.
 * @return void* Pointer to the last element in the list. Returns NULL if the list is empty.
 */
void* zoo_list_back(ZOO_LIST_HANDLE list)
{

    if (!list)
    {
        return NULL;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return NULL;
    }

    if (!linked_list->tail)
    {
        unlock_list(linked_list);
        return NULL;
    }

    void* data = (void*)linked_list->tail->data;

    unlock_list(linked_list);
    return data;
}

/**
 * @brief Gets the current size of the list
 *
 * @param list Handle to the linked list
 * @return ZOO_SIZE_T Current number of elements in the list
 */
ZOO_SIZE_T zoo_list_size(ZOO_LIST_HANDLE list)
{
    if (!list)
    {
        return 0;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return 0;
    }

    ZOO_SIZE_T size = linked_list->size;
    unlock_list(linked_list);

    return size;
}

/**
 * @brief Gets the maximum capacity of the list
 *
 * @param list Handle to the linked list
 * @return ZOO_SIZE_T Maximum number of elements the list can hold
 */
ZOO_SIZE_T zoo_list_capacity(ZOO_LIST_HANDLE list)
{
    if (!list)
    {
        return 0;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return 0;
    }

    ZOO_SIZE_T capacity = linked_list->max_list_length;
    unlock_list(linked_list);

    return capacity;
}

/**
 * @brief Gets the first element in the list without removing it
 *
 * @param list Handle to the linked list
 * @param data Pointer to store the front element
 * @return ZOO_ERROR_T Status code
 */
void* zoo_list_front(
    ZOO_LIST_HANDLE list)
{

    if (!list)
    {
        return NULL;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return NULL;
    }

    if (!linked_list->head)
    {
        unlock_list(linked_list);
        return NULL;
    }

    void* data = (void*)linked_list->head->data;

    unlock_list(linked_list);
    return data;
}

/**
 * @brief Checks if the linked list is empty
 *
 * @param list Handle to the linked list to check
 * @return ZOO_BOOL true if list is empty or invalid, false otherwise
 *
 * This function performs a thread-safe check of the list's empty status.
 * Returns true if the list contains no elements or if the list handle
 * is invalid. The operation is protected by mutex locking.
 */
ZOO_BOOL zoo_list_empty(ZOO_LIST_HANDLE list)
{

    if (!list)
    {
        return true;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return true;
    }

    ZOO_BOOL is_empty = (linked_list->size == 0);
    unlock_list(linked_list);

    return is_empty;
}

/**
 * @brief Destroys a linked list and frees all allocated memory
 *
 * This function deallocates all memory associated with the given linked list,
 * including all nodes and their contents. After calling this function, the list
 * handle becomes invalid and should not be used.
 *
 * @param list The linked list handle to be destroyed
 * @note After calling this function, the list handle should not be used anymore
 */
void zoo_list_destroy(ZOO_LIST_HANDLE list)
{
    if (!list)
    {
        return;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    zoo_list_clear(list);

    ZOO_MUTEX_DESTROY(&linked_list->mutex);
    ZOO_COND_DESTROY(&linked_list->cond);
    free(linked_list);
}

/**
 * @brief Checks if the linked list is full
 *
 * @param list Handle to the linked list to check
 * @return ZOO_BOOL true if list is full or invalid, false otherwise
 *
 * This function performs a thread-safe check of the list's full status.
 * Returns true if the list has reached its maximum capacity or if the 
 * list handle is invalid. The operation is protected by mutex locking.
 */
ZOO_BOOL zoo_list_full(ZOO_LIST_HANDLE list)
{
    if (!list)
    {
        return true;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return true;
    }

    ZOO_BOOL is_full = (linked_list->size >= linked_list->max_list_length);
    unlock_list(linked_list);

    return is_full;
}

/**
 * @brief Inserts a new element at the specified index in the linked list
 *
 * @param list  Handle to the linked list where the element will be inserted
 * @param index Position at which to insert the new element (0-based)
 * @param data  Pointer to the data to be inserted
 *
 * @note The function assumes the list handle and data pointer are valid
 * @note If index is greater than the list size, the element is appended at the end
 */
void zoo_list_insert(ZOO_LIST_HANDLE list, ZOO_SIZE_T index, const void* data)
{

    if (!list || !data)
    {
        return;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return;
    }

    if (is_list_full(linked_list))
    {
        unlock_list(linked_list);
        return;
    }

    NODE_STRUCT* new_node = malloc(
        sizeof(NODE_STRUCT));

    if (!new_node)
    {
        unlock_list(linked_list);
        return;
    }

    new_node->data = (void*)data;

    // Insert at head if index is 0 or list is empty
    if (index == 0 || !linked_list->head)
    {
        new_node->next = linked_list->head;
        linked_list->head = new_node;
        if (!linked_list->tail)
        {
            linked_list->tail = new_node;
        }
    }
    else
    {
        // Find insertion point
        NODE_STRUCT* current = linked_list->head;
        NODE_STRUCT* prev = NULL;
        ZOO_SIZE_T current_index = 0;

        while (current && current_index < index)
        {
            prev = current;
            current = current->next;
            current_index++;
        }

        // Insert the node
        new_node->next = current;
        prev->next = new_node;

        // Update tail if inserting at the end
        if (!current)
        {
            linked_list->tail = new_node;
        }
    }

    linked_list->size++;
    unlock_list(linked_list);
}

/**
 * @brief Erases an element at the specified index from the list
 *
 * @param list Handle to the linked list
 * @param index Position of the element to erase (0-based index)
 * @return ZOO_ERROR_T Status code indicating success or failure
 *         - ZOO_OK: Element successfully erased
 *         - ZOO_ERROR_INVALID_PARAM: Invalid list handle or index
 *         - ZOO_ERROR_MUTEX_LOCK_FAILED: Failed to acquire mutex lock
 *         - ZOO_SMB_ERROR_OUT_OF_RANGE: Position exceeds list size
 */
ZOO_ERROR_T zoo_list_erase(ZOO_LIST_HANDLE list, ZOO_SIZE_T index)
{

    if (!list)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return ZOO_ERROR_MUTEX_LOCK_FAILED;
    }

    // Check if index is valid
    if (index >= linked_list->size)
    {
        unlock_list(linked_list);
        return ZOO_ERROR_BUFFER_OUT_OF_RANGE;
    }

    NODE_STRUCT* current = linked_list->head;
    NODE_STRUCT* prev = NULL;

    // Special case: erasing head node
    if (index == 0)
    {
        linked_list->head = current->next;
        if (!linked_list->head)
        {
            linked_list->tail = NULL;
        }
        free(current);
        linked_list->size--;

        unlock_list(linked_list);
        return ZOO_OK;
    }

    // Traverse to the node at index
    for (ZOO_SIZE_T i = 0; i < index; i++)
    {
        prev = current;
        current = current->next;
    }

    // Update links
    prev->next = current->next;
    if (current == linked_list->tail)
    {
        linked_list->tail = prev;
    }

    // Free the node
    free(current);
    linked_list->size--;

    unlock_list(linked_list);
    return ZOO_OK;
}

/**
 * @brief Removes an element from the linked list that matches the provided data.
 *
 * This function searches through the linked list for an element containing data
 * that matches the provided data parameter and removes it from the list.
 *
 * @param list The handle to the linked list from which to remove the element
 * @param data Pointer to the data to search for and remove from the list
 *
 * @return ZOO_ERROR_T Returns an error code indicating the success or failure
 *         of the removal operation. Returns success if the element was found and
 *         removed, or an appropriate error code if the element was not found or
 *         if other errors occurred during the removal process.
 *
 * @note The caller is responsible for ensuring that the data pointer remains valid
 *       during the function call. The function will compare the data using the
 *       comparison method configured for the list.
 *
 * @warning If multiple elements contain matching data, only the first occurrence
 *          will be removed.
 */
ZOO_ERROR_T zoo_list_remove(ZOO_LIST_HANDLE list, const void* data)
{

    if (!list || !data)
    {
        return ZOO_ERROR_INVALID_PARAM;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return ZOO_ERROR_MUTEX_LOCK_FAILED;
    }

    NODE_STRUCT* current = linked_list->head;
    NODE_STRUCT* prev = NULL;

    while (current)
    {
        if (current->data == data)  // Assuming direct pointer comparison
        {
            // Found the node to remove
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                linked_list->head = current->next;  // Removing head
            }

            if (current == linked_list->tail)
            {
                linked_list->tail = prev;  // Update tail if needed
            }

            free(current);
            linked_list->size--;

            unlock_list(linked_list);
            return ZOO_OK;
        }

        prev = current;
        current = current->next;
    }

    unlock_list(linked_list);
    return ZOO_ERROR_NOT_FOUND;
}

/**
 * @brief Removes the first element from the linked list that matches a given condition.
 *
 * This function iterates through the linked list referenced by `list` and removes the first node
 * for which the provided predicate function returns true. The removed node's data pointer is returned.
 *
 * @param list The handle to the linked list.
 * @param predicate A function pointer that takes a node's data and returns a non-zero value if the node should be removed.
 * @param user_data Additional data to pass to the predicate function.
 * @return A pointer to the data of the removed node if a match is found and removed, or NULL if no such node exists.
 */
void* zoo_list_remove_if(ZOO_LIST_HANDLE list, ZOO_BOOL (*compare)(const void* data, const void* target), const void* target)
{
    if (!list || !compare || !target)
    {
        return NULL;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return NULL;
    }

    NODE_STRUCT* current = linked_list->head;
    NODE_STRUCT* prev = NULL;

    while (current)
    {
        if (compare(current->data, target))
        {
            // Found the node to remove
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                linked_list->head = current->next;  // Removing head
            }

            if (current == linked_list->tail)
            {
                linked_list->tail = prev;  // Update tail if needed
            }

            void* removed_data = current->data;
            free(current);
            linked_list->size--;

            unlock_list(linked_list);
            return removed_data;
        }

        prev = current;
        current = current->next;
    }

    unlock_list(linked_list);
    return NULL;
}

/**
 * @brief Searches for an element in a linked list using a comparison function.
 *
 * Iterates through the linked list referenced by `list`, applying the `compare`
 * function to each element's data and the provided `target`. Returns a pointer
 * to the data of the first element for which `compare` returns true.
 *
 * @param list      The handle to the linked list to search.
 * @param compare   A function pointer to the comparison function. Should return
 *                  true if the element matches the target, false otherwise.
 * @param target    A pointer to the target data to search for.
 *
 * @return A pointer to the data of the matching element if found, or NULL if no match is found.
 */
void* zoo_list_find_if(ZOO_LIST_HANDLE list, ZOO_BOOL (*compare)(const void* data, const void* target), const void* target)
{
    if (!list || !compare || !target)
    {
        return NULL;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return NULL;
    }

    NODE_STRUCT* current = linked_list->head;
    while (current)
    {
        if (compare(current->data, target))
        {
            unlock_list(linked_list);
            return current->data;
        }
        current = current->next;
    }

    unlock_list(linked_list);
    return NULL;
}

/**
 * @brief Iterates through all elements in the linked list and applies a callback function to each element.
 *
 * This function traverses the entire linked list and calls the provided callback function
 * for each node in the list, passing the node's data and user-provided data as arguments.
 *
 * @param list The handle to the linked list to iterate through
 * @param callback Function pointer to be called for each element in the list.
 *                 The callback receives two parameters:
 *                 - data: Pointer to the data stored in the current list node
 *                 - user_data: User-provided data passed through from the user_data parameter
 * @param user_data Optional user data that will be passed to the callback function for each iteration.
 *                  Can be NULL if no additional data is needed.
 *
 * @note The callback function should not modify the list structure during iteration.
 * @note If list is NULL or empty, the function returns without calling the callback.
 */
void zoo_list_foreach(ZOO_LIST_HANDLE list, void (*callback)(void* data, void* user_data), void* user_data)
{

    if (!list || !callback)
    {
        return;
    }

    ZOO_LIST_STRUCT* linked_list = (ZOO_LIST_STRUCT*)list;

    if (!lock_list(linked_list))
    {
        return;
    }

    NODE_STRUCT* current = linked_list->head;
    while (current)
    {
        callback(current->data, user_data);
        current = current->next;
    }

    unlock_list(linked_list);
}

#pragma GCC diagnostic pop
