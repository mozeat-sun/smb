/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: buffer
 * Component id: list
 * File name: zoo_list.h
 * Description: Generic singly linked list interface for ZOO buffer module
 *              Provides a thread-safe linked list implementation with dynamic
 *              memory management using a memory pool.
 *              Supports basic operations like push, pop, front, back, size,
 *              and clear. The list can hold any type of data, with the size
 *              of each element specified at creation time.
 *              The implementation is designed to be efficient and safe for
 *              concurrent access, using mutexes for synchronization.
 *              The list can be used for FIFO operations and supports random
 *              access to elements.
 *              The list is implemented using a node structure that contains
 *              a pointer to the data and a pointer to the next node.
 *              The list structure contains pointers to the head and tail nodes,
 *              the current size, and a maximum length limit.
 *              The list is allocated from a memory pool to reduce fragmentation
 *              and improve performance.
 *              The implementation is designed to be portable and can be used
 *              across different platforms with minimal changes.
 *              The list can be used in various applications, including
 *              message queues, task scheduling, and data buffering.
 *              The list is designed to be extensible and can be modified to
 *              support additional features like sorting, searching, and
 *              filtering.
 *              The implementation is based on the principles of data abstraction
 *              and encapsulation, providing a clean interface for users while
 *              hiding the implementation details.
 *              The list is designed to be efficient in terms of memory usage
 *              and performance, with a focus on minimizing overhead and
 *              maximizing throughput.
 *              The implementation is tested for correctness and performance,
 *              with unit tests provided to ensure reliability and stability.
 *              The list is designed to be easy to use and integrate into existing
 *              codebases, with clear documentation and examples provided.
 *              The implementation follows best practices for software development,
 *              including code reviews, version control, and continuous integration.
 *              The list is part of the ZOO library, which provides a comprehensive
 *              set of utilities and components for building scalable and efficient
 *              applications.
 *              The implementation is open source and licensed under the ZOO license,
 *              allowing users to modify and distribute the code freely while
 *              adhering to the terms of the license.
 *              Provides APIs for list creation, destruction, element access,
 *              insertion, deletion, and traversal.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-23     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_LIST_H
#define ZOO_LIST_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo.h"
#include "zoo_error.h"
#include <stddef.h>
#include <stdbool.h>

    typedef struct ZOO_LIST_STRUCT* ZOO_LIST_HANDLE;

    /**
     * @brief Creates a new linked list for storing elements of a specified size.
     *
     * Allocates and initializes a new ZOO_LIST structure capable of holding
     * elements, each of size `data_size` bytes.
     *
     * @param max_list_length The maximum list length for the linked list, in bytes.
     * @param data_size The size, in bytes, of each element to be stored in the list.
     * @return Pointer to the newly created ZOO_LIST, or NULL on failure.
     */
    ZOO_LIST_HANDLE zoo_list_create(ZOO_SIZE_T max_list_length);

    /**
     * @brief Destroys a ZOO_LIST and frees all associated memory.
     *
     * This function deallocates all nodes in the linked list and releases any resources
     * held by the list. After calling this function, the list pointer should not be used
     * unless it is reinitialized.
     *
     * @param list Pointer to the ZOO_LIST to be destroyed.
     */
    void zoo_list_destroy(ZOO_LIST_HANDLE list);

    /**
     * @brief Appends a new element to the end of the linked list.
     *
     * Adds a new node containing the provided data pointer to the end of the specified
     * ZOO_SMB_LINKED_LIST. The data pointer is stored directly; the data is not copied.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST to which the data will be appended.
     * @param data Pointer to the data to be added to the list.
     */
    ZOO_ERROR_T zoo_list_push_back(ZOO_LIST_HANDLE list, const void* data);

    /**
     * @brief Inserts a new element at the front of the linked list.
     *
     * Adds a new node containing the provided data pointer to the front of the specified
     * ZOO_SMB_LINKED_LIST. The data pointer is stored directly; the data is not copied.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST to which the data will be prepended.
     * @param data Pointer to the data to be added to the list.
     */
    ZOO_ERROR_T zoo_list_push_front(ZOO_LIST_HANDLE list, const void* data);

    /**
     * @brief Removes the last element from the linked list.
     *
     * Deletes the last node from the specified ZOO_SMB_LINKED_LIST and frees its memory.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST from which the last element will be removed.
     */
    void* zoo_list_pop_back(ZOO_LIST_HANDLE list);

    /**
     * @brief Removes the first element from the linked list.
     *
     * Deletes the first node from the specified ZOO_SMB_LINKED_LIST and frees its memory.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST from which the first element will be removed.
     */
    void* zoo_list_pop_front(ZOO_LIST_HANDLE list);

    /**
     * @brief Retrieves the data pointer of the first element in the list.
     *
     * Returns a pointer to the data stored in the first node of the list, or NULL if the list is empty.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST.
     * @return Pointer to the data of the first element, or NULL if the list is empty.
     */
    void* zoo_list_front(ZOO_LIST_HANDLE list);

    /**
     * @brief Retrieves the data pointer of the last element in the list.
     *
     * Returns a pointer to the data stored in the last node of the list, or NULL if the list is empty.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST.
     * @return Pointer to the data of the last element, or NULL if the list is empty.
     */
    void* zoo_list_back(ZOO_LIST_HANDLE list);

    /**
     * @brief Returns the number of elements in the linked list.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST.
     * @return The number of elements in the list.
     */
    ZOO_SIZE_T zoo_list_size(ZOO_LIST_HANDLE list);

    /**
     * @brief Checks if the linked list is empty.
     *
     * @param list Pointer to the ZOO_LIST.
     * @return true if the list is empty, false otherwise.
     */
    ZOO_BOOL zoo_list_empty(ZOO_LIST_HANDLE list);

    /**
     * @brief Checks if the linked list is full.
     *
     * @param list Pointer to the ZOO_LIST.
     * @return true if the list is full, false otherwise.
     */
    ZOO_BOOL zoo_list_full(ZOO_LIST_HANDLE list);

    /**
     * @brief Returns the maximum capacity of the linked list.
     *
     * @param list Pointer to the ZOO_LIST.
     * @return The maximum number of elements the list can hold.
     */
    ZOO_SIZE_T zoo_list_capacity(ZOO_LIST_HANDLE list);

    /**
     * @brief Removes all elements from the linked list.
     *
     * Deletes all nodes in the specified ZOO_SMB_LINKED_LIST and frees their memory.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST to be cleared.
     */
    void zoo_list_clear(ZOO_LIST_HANDLE list);

    /**
     * @brief Retrieves the data pointer at the specified index in the list.
     *
     * Returns a pointer to the data stored at the given index, or NULL if the index is out of bounds.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST.
     * @param index Zero-based index of the element to retrieve.
     * @return Pointer to the data at the specified index, or NULL if out of bounds.
     */
    void* zoo_list_at(ZOO_LIST_HANDLE list, ZOO_SIZE_T index);

    /**
     * @brief Inserts a new element at the specified index in the list.
     *
     * Adds a new node containing the provided data pointer at the given index in the list.
     * If the index is greater than the list size, the element is appended to the end.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST.
     * @param index Zero-based index at which to insert the new element.
     * @param data Pointer to the data to be inserted.
     */
    void zoo_list_insert(ZOO_LIST_HANDLE list, ZOO_SIZE_T index, const void* data);

    /**
     * @brief Removes the element at the specified index from the list.
     *
     * Deletes the node at the given index and frees its memory. If the index is out of bounds,
     * the function does nothing.
     *
     * @param list Pointer to the ZOO_SMB_LINKED_LIST.
     * @param index Zero-based index of the element to remove.
     */
    ZOO_ERROR_T zoo_list_erase(ZOO_LIST_HANDLE list, ZOO_SIZE_T index);

    /**
     * @brief Removes an element from a linked list
     *
     * This function removes an element from the specified linked list. The element
     * to be removed is typically identified by its position, value, or handle.
     *
     * @param list Handle to the linked list from which to remove the element
     *
     * @return ZOO_ERROR_T Returns an error code indicating the success or
     *         failure of the removal operation. Possible return values may include:
     *         - Success code if the element was successfully removed
     *         - Error code if the list is invalid, empty, or element not found
     *
     * @note The caller should check the return value to ensure the operation completed successfully
     * @warning Removing an element from an invalid or empty list may result in undefined behavior
     */
    ZOO_ERROR_T zoo_list_remove(ZOO_LIST_HANDLE list,
                                           const void* data);

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
    void* zoo_list_remove_if(ZOO_LIST_HANDLE list,
                                           ZOO_BOOL (*compare)(const void* data, const void* target),
                                           const void* target);                                            
    /**
     * @brief Searches for an element in a linked list that matches a given condition.
     *
     * This function traverses the linked list referenced by `list` and applies the
     * `compare` function to each element's data and the provided `target`. If the
     * `compare` function returns true for an element, a pointer to that element's
     * data is returned.
     *
     * @param list      The handle to the linked list to search.
     * @param compare   A function pointer that takes two `const void*` arguments:
     *                  the element's data and the target value. Should return true
     *                  if the element matches the target, false otherwise.
     * @param target    A pointer to the value to compare against each element.
     *
     * @return A pointer to the data of the first matching element, or NULL if no match is found.
     */
    void* zoo_list_find_if(ZOO_LIST_HANDLE list, ZOO_BOOL (*compare)(const void* data, const void* target), const void* target);

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
    void zoo_list_foreach(ZOO_LIST_HANDLE list, void (*callback)(void* data, void* user_data), void* user_data);
#ifdef __cplusplus
}
#endif
#endif  // ZOO_SMB_LINKED_LIST_H