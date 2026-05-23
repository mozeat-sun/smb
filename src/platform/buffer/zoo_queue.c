/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: buffer
 * Component id: queue
 * File name: zoo_queue.c
 * Description: Cross-platform message queue implementation for ZOO
 *              Supports Windows, Linux, FreeRTOS, CMSIS-RTOS, and bare metal
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun         created
 * 1.1       2025-07-31     weiwang.sun         added cross-platform support
 ******************************************************************************/

// Suppress compiler warnings for unused mutex return values
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"

#include "zoo_queue.h"
#include "zoo_memory_pool.h"
#include "zoo_list.h"
#include "zoo_log.h"
#include <string.h>

/**
 * @brief Queue node structure for storing message information
 */
typedef struct QUEUE_NODE_STRUCT
{
    void* msg;                          /**< Message data */
    void* context;                      /**< Message context */
    ZOO_QUEUED_HANDLER handler;         /**< Callback handler */
    void* user_data;                    /**< User data */
    ZOO_INT32 priority;                 /**< Message priority */
    ZOO_TIME_T arrived_timestamp;       /**< Arrival timestamp */
} QUEUE_NODE_STRUCT;

/**
 * @brief Queue observer structure for change notifications
 */
typedef struct QUEUE_OBSERVER_STRUCT
{
    char name[MAX_QUEUE_OBSERVER_NAME_LENGTH];  /**< Observer name */
    ON_QUEUE_CHANGED_HANDLER handler;           /**< Change handler */
    void* user_data;                            /**< User data */
} QUEUE_OBSERVER_STRUCT;

/**
 * @brief Main queue structure
 */
typedef struct ZOO_QUEUE_STRUCT
{
    ZOO_LIST_HANDLE messages;           /**< Message list */
    ZOO_LIST_HANDLE observers;          /**< Observer list */
    ZOO_SIZE_T max_queue_size;          /**< Maximum queue size */
    ZOO_ATOMIC_BOOL exit_flag;          /**< Exit flag for blocking operations */
    ZOO_MUTEX_T messages_mutex;         /**< Mutex for message operations */
    ZOO_MUTEX_T observers_mutex;        /**< Mutex for observer operations */
    ZOO_COND_T cond;                    /**< Condition variable for blocking */
} ZOO_QUEUE_STRUCT;

/**
 * @brief Compare messages based on strategy
 * @param a First message node
 * @param b Second message node  
 * @param strategy Comparison strategy
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
static ZOO_INT32 compare_messages(void* a, void* b, ZOO_INT32 strategy)
{
    if (!a || !b) return 0;
    
    QUEUE_NODE_STRUCT* node_a = (QUEUE_NODE_STRUCT*)a;
    QUEUE_NODE_STRUCT* node_b = (QUEUE_NODE_STRUCT*)b;
    
    switch (strategy)
    {
        case ZOO_QUEUE_SORT_STRATEGY_PRIORITY:
            if (node_a->priority < node_b->priority) return -1;
            if (node_a->priority > node_b->priority) return 1;
            return 0;
            
        case ZOO_QUEUE_SORT_STRATEGY_TIMESTAMP:
            if (node_a->arrived_timestamp < node_b->arrived_timestamp) return -1;
            if (node_a->arrived_timestamp > node_b->arrived_timestamp) return 1;
            return 0;
            
        case ZOO_QUEUE_SORT_STRATEGY_FIFO:
        default:
            return 0; // FIFO maintains insertion order
    }
}

/**
 * @brief Notify all observers about a queue change
 * @param queue Pointer to the message queue
 * @param msg Pointer to the message data
 * @param context Message context
 * @param handler Message handler
 */
static void notify_observers(
    ZOO_QUEUE_STRUCT* queue,
    void* msg,
    void* context,
    ZOO_QUEUED_HANDLER handler)
{
    if (!queue || !queue->observers) return;
    
    ZOO_MUTEX_LOCK(&queue->observers_mutex);
    ZOO_SIZE_T observer_count = zoo_list_size(queue->observers);
    
    for (ZOO_SIZE_T i = 0; i < observer_count; i++)
    {
        QUEUE_OBSERVER_STRUCT* observer = (QUEUE_OBSERVER_STRUCT*)zoo_list_at(queue->observers, i);
        if (observer && observer->handler)
        {
            observer->handler(queue, observer->user_data, msg, context, handler);
        }
    }
    ZOO_MUTEX_UNLOCK(&queue->observers_mutex);
}

/**
 * @brief Create a new message queue
 * @param max_queue_size Maximum size of the queue
 * @return Handle to the created message queue, or NULL on error
 */
ZOO_QUEUE_HANDLE zoo_create_queue(ZOO_SIZE_T max_queue_size)
{
    ZOO_LOG_TRACE("Creating message queue, max_queue_size=%zu", max_queue_size);

    ZOO_QUEUE_STRUCT* queue = (ZOO_QUEUE_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_QUEUE_STRUCT));
    if (!queue)
    {
        return NULL;
    }

    // Initialize atomic exit flag
    ZOO_ATOMIC_INIT(&queue->exit_flag, ZOO_FALSE);
    queue->max_queue_size = max_queue_size;

    // Create message and observer lists
    // For zero capacity, still create list with minimal size but enforce capacity in enqueue
    ZOO_SIZE_T list_size = (max_queue_size == 0) ? 64 : max_queue_size;
    queue->messages = zoo_list_create(list_size);
    queue->observers = zoo_list_create(256); // Fixed size for observers

    if (!queue->messages || !queue->observers)
    {
        if (queue->messages) zoo_list_destroy(queue->messages);
        if (queue->observers) zoo_list_destroy(queue->observers);
        zoo_free_to_pool(queue);
        return NULL;
    }

    // Initialize synchronization primitives
    if (!ZOO_MUTEX_INIT(&queue->messages_mutex))
    {
        zoo_list_destroy(queue->messages);
        zoo_list_destroy(queue->observers);
        zoo_free_to_pool(queue);
        return NULL;
    }

    if (!ZOO_MUTEX_INIT(&queue->observers_mutex))
    {
        ZOO_MUTEX_DESTROY(&queue->messages_mutex);
        zoo_list_destroy(queue->messages);
        zoo_list_destroy(queue->observers);
        zoo_free_to_pool(queue);
        return NULL;
    }

    if (!ZOO_COND_INIT(&queue->cond))
    {
        ZOO_MUTEX_DESTROY(&queue->messages_mutex);
        ZOO_MUTEX_DESTROY(&queue->observers_mutex);
        zoo_list_destroy(queue->messages);
        zoo_list_destroy(queue->observers);
        zoo_free_to_pool(queue);
        return NULL;
    }
    return queue;
}

/**
 * @brief Destroy a message queue and free all resources
 * @param queue_handle Handle to the message queue to destroy
 */
void zoo_destroy_queue(ZOO_QUEUE_HANDLE queue_handle)
{

    if (!queue_handle)
    {
        return;
    }

    ZOO_QUEUE_STRUCT* queue = (ZOO_QUEUE_STRUCT*)queue_handle;

    // Signal exit to any waiting threads
    ZOO_ATOMIC_STORE(&queue->exit_flag, ZOO_TRUE);
    ZOO_COND_BROADCAST(&queue->cond);

    // Give some time for threads to exit
    ZOO_SLEEP_MS(200);

    // Clean up message list
    if (queue->messages)
    {
        ZOO_MUTEX_LOCK(&queue->messages_mutex);
        
        // Free all message nodes
        ZOO_SIZE_T size = zoo_list_size(queue->messages);
        for (ZOO_SIZE_T i = 0; i < size; i++)
        {
            QUEUE_NODE_STRUCT* node = (QUEUE_NODE_STRUCT*)zoo_list_at(queue->messages, i);
            if (node)
            {
                zoo_free_to_pool(node);
            }
        }
        
        zoo_list_destroy(queue->messages);
        ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
    }

    // Clean up observer list
    if (queue->observers)
    {
        ZOO_MUTEX_LOCK(&queue->observers_mutex);
        
        // Free all observer nodes
        ZOO_SIZE_T size = zoo_list_size(queue->observers);
        for (ZOO_SIZE_T i = 0; i < size; i++)
        {
            QUEUE_OBSERVER_STRUCT* observer = (QUEUE_OBSERVER_STRUCT*)zoo_list_at(queue->observers, i);
            if (observer)
            {
                zoo_free_to_pool(observer);
            }
        }
        
        zoo_list_destroy(queue->observers);
        ZOO_MUTEX_UNLOCK(&queue->observers_mutex);
    }

    // Destroy synchronization primitives
    ZOO_MUTEX_DESTROY(&queue->messages_mutex);
    ZOO_MUTEX_DESTROY(&queue->observers_mutex);
    ZOO_COND_DESTROY(&queue->cond);

    // Free queue structure
    zoo_free_to_pool(queue);
}

/**
 * @brief Enqueue a message into the message queue
 * @param queue_handle Handle to the message queue
 * @param msg Message data
 * @param context Message context
 * @param handler Message handler
 * @param user_data User data
 * @return ZOO_TRUE if successful, ZOO_FALSE otherwise
 */
ZOO_BOOL zoo_queue_enqueue(
    ZOO_QUEUE_HANDLE queue_handle,
    void* msg,
    void* context,
    ZOO_QUEUED_HANDLER handler,
    void* user_data)
{

    if (!queue_handle || !msg || !handler)
    {
        return ZOO_FALSE;
    }

    ZOO_QUEUE_STRUCT* queue = (ZOO_QUEUE_STRUCT*)queue_handle;

    ZOO_MUTEX_LOCK(&queue->messages_mutex);

    // Check if queue is full
    if (zoo_list_size(queue->messages) >= queue->max_queue_size)
    {
        ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
        return ZOO_FALSE;
    }

    // Create new message node
    QUEUE_NODE_STRUCT* node = (QUEUE_NODE_STRUCT*)zoo_allocate_from_pool(sizeof(QUEUE_NODE_STRUCT));
    if (!node)
    {
        ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
        return ZOO_FALSE;
    }

    // Initialize node
    node->msg = msg;
    node->context = context;
    node->handler = handler;
    node->user_data = user_data;
    node->priority = 0;
    node->arrived_timestamp = ZOO_TIME_GET();

    // Add to list
    ZOO_ERROR_T result = zoo_list_push_back(queue->messages, node);
    if (result != ZOO_OK)
    {
        zoo_free_to_pool(node);
        ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
        return ZOO_FALSE;
    }

    ZOO_MUTEX_UNLOCK(&queue->messages_mutex);

    // Notify observers and signal waiting threads
    notify_observers(queue, msg, context, handler);
    ZOO_COND_SIGNAL(&queue->cond);

    return ZOO_TRUE;
}

/**
 * @brief Dequeue a message from the message queue
 * @param queue_handle Handle to the message queue
 * @param[out] msg Pointer to store message data
 * @param[out] context Pointer to store message context  
 * @param[out] handler Pointer to store message handler
 * @param[out] user_data Pointer to store user data
 * @param block_if_empty Whether to block if queue is empty
 * @return ZOO_TRUE if successful, ZOO_FALSE otherwise
 */
bool zoo_queue_dequeue(
    ZOO_QUEUE_HANDLE queue_handle,
    void** msg,
    void** context,
    ZOO_QUEUED_HANDLER* handler,
    void** user_data,
    bool block_if_empty)
{
    if (!queue_handle || !msg || !context || !handler)
    {
        return ZOO_FALSE;
    }

    ZOO_QUEUE_STRUCT* queue = (ZOO_QUEUE_STRUCT*)queue_handle;
    ZOO_MUTEX_LOCK(&queue->messages_mutex);

    // Wait for messages if blocking is enabled
    while (queue->messages && zoo_list_size(queue->messages) == 0 && 
           block_if_empty && !ZOO_ATOMIC_LOAD(&queue->exit_flag))
    {
        ZOO_COND_WAIT(&queue->cond, &queue->messages_mutex);
    }

    // Check exit condition
    if (ZOO_ATOMIC_LOAD(&queue->exit_flag))
    {
        ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
        return ZOO_FALSE;
    }

    // Try to get a message
    QUEUE_NODE_STRUCT* node = (QUEUE_NODE_STRUCT*)zoo_list_pop_front(queue->messages);
    if (!node)
    {
        ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
        return ZOO_FALSE;
    }

    // Extract message data
    *msg = node->msg;
    *context = node->context;
    *handler = node->handler;
    if (user_data) *user_data = node->user_data;

    // Free the node
    zoo_free_to_pool(node);
    ZOO_MUTEX_UNLOCK(&queue->messages_mutex);

    return ZOO_TRUE;
}

/**
 * @brief Exit blocking state of the message queue
 * @param queue_handle Handle to the message queue
 */
void zoo_queue_exit_blocking(ZOO_QUEUE_HANDLE queue_handle)
{
    if (!queue_handle)
    {
        return;
    }

    ZOO_QUEUE_STRUCT* queue = (ZOO_QUEUE_STRUCT*)queue_handle;
    
    ZOO_MUTEX_LOCK(&queue->messages_mutex);
    ZOO_ATOMIC_STORE(&queue->exit_flag, ZOO_TRUE);
    ZOO_COND_BROADCAST(&queue->cond);
    ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
    
    ZOO_SLEEP_MS(200);
}

/**
 * @brief Find an existing observer in the queue
 * @param queue Pointer to the queue structure
 * @param name Name of the observer to search for
 * @param handler Handler function of the observer to search for
 * @return ZOO_TRUE if observer exists, ZOO_FALSE otherwise
 */
static bool find_existing_observer(
    ZOO_QUEUE_STRUCT* queue,
    const char* name,
    ON_QUEUE_CHANGED_HANDLER handler)
{
    ZOO_SIZE_T observer_count = zoo_list_size(queue->observers);
    for (ZOO_SIZE_T i = 0; i < observer_count; i++)
    {
        QUEUE_OBSERVER_STRUCT* existing = (QUEUE_OBSERVER_STRUCT*)zoo_list_at(queue->observers, i);
        if (existing &&
            strcmp(existing->name, name) == 0 &&
            existing->handler == handler)
        {
            return ZOO_TRUE;
        }
    }
    return ZOO_FALSE;
}

/**
 * @brief Add an observer to the message queue
 * @param queue_handle Handle to the message queue
 * @param handler Observer handler function
 * @param user_data User data for the observer
 * @param name Name of the observer
 */
void zoo_queue_add_observer(
    ZOO_QUEUE_HANDLE queue_handle,
    ON_QUEUE_CHANGED_HANDLER handler,
    void* user_data,
    const char* name)
{

    if (!queue_handle || !handler || !name)
    {
        return;
    }

    ZOO_QUEUE_STRUCT* queue = (ZOO_QUEUE_STRUCT*)queue_handle;

    ZOO_MUTEX_LOCK(&queue->observers_mutex);

    // Check if observer already exists
    if (find_existing_observer(queue, name, handler))
    {
        ZOO_MUTEX_UNLOCK(&queue->observers_mutex);
        return;
    }

    // Create observer node
    QUEUE_OBSERVER_STRUCT* observer = (QUEUE_OBSERVER_STRUCT*)zoo_allocate_from_pool(sizeof(QUEUE_OBSERVER_STRUCT));
    if (!observer)
    {
        ZOO_MUTEX_UNLOCK(&queue->observers_mutex);
        return;
    }

    // Initialize observer
    strncpy(observer->name, name, MAX_QUEUE_OBSERVER_NAME_LENGTH - 1);
    observer->name[MAX_QUEUE_OBSERVER_NAME_LENGTH - 1] = '\0';
    observer->handler = handler;
    observer->user_data = user_data;

    // Add to observers list
    if (zoo_list_push_back(queue->observers, observer) != ZOO_OK)
    {
        zoo_free_to_pool(observer);
        ZOO_MUTEX_UNLOCK(&queue->observers_mutex);
        return;
    }

    ZOO_MUTEX_UNLOCK(&queue->observers_mutex);
}

/**
 * @brief Set priority for a message in the queue
 * @param queue_handle Handle to the message queue
 * @param msg Pointer to the message
 * @param priority New priority value
 * @return ZOO_TRUE if successful, ZOO_FALSE otherwise
 */
bool zoo_queue_set_priority(
    ZOO_QUEUE_HANDLE queue_handle,
    void* msg,
    ZOO_INT32 priority)
{

    if (!queue_handle || !msg)
    {
        return ZOO_FALSE;
    }

    ZOO_QUEUE_STRUCT* queue = (ZOO_QUEUE_STRUCT*)queue_handle;
    ZOO_MUTEX_LOCK(&queue->messages_mutex);

    ZOO_SIZE_T size = zoo_list_size(queue->messages);
    for (ZOO_SIZE_T i = 0; i < size; i++)
    {
        QUEUE_NODE_STRUCT* node = (QUEUE_NODE_STRUCT*)zoo_list_at(queue->messages, i);
        if (node && node->msg == msg)
        {
            node->priority = priority;
            ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
            return ZOO_TRUE;
        }
    }

    ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
    return ZOO_FALSE;
}

/**
 * @brief Get priority of a message in the queue
 * @param queue_handle Handle to the message queue
 * @param msg Pointer to the message
 * @return Priority value, or -1 if not found
 */
ZOO_INT32 zoo_queue_get_priority(
    ZOO_QUEUE_HANDLE queue_handle,
    void* msg)
{
    if (!queue_handle || !msg)
    {
        return -1;
    }

    ZOO_QUEUE_STRUCT* queue = (ZOO_QUEUE_STRUCT*)queue_handle;
    ZOO_MUTEX_LOCK(&queue->messages_mutex);

    ZOO_SIZE_T size = zoo_list_size(queue->messages);
    for (ZOO_SIZE_T i = 0; i < size; i++)
    {
        QUEUE_NODE_STRUCT* node = (QUEUE_NODE_STRUCT*)zoo_list_at(queue->messages, i);
        if (node && node->msg == msg)
        {
            ZOO_INT32 priority = node->priority;
            ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
            return priority;
        }
    }

    ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
    return -1;
}

/**
 * @brief Partition function for quicksort
 * @param nodes Array of message nodes to be sorted
 * @param low Starting index of the partition
 * @param high Ending index of the partition
 * @param strategy Comparison strategy to use
 * @return The index of the pivot element after partitioning
 */
static ZOO_SIZE_T partition(void** nodes, ZOO_SIZE_T low, ZOO_SIZE_T high, ZOO_INT32 strategy)
{
    void* pivot = nodes[high];
    ZOO_SIZE_T i = low;

    for (ZOO_SIZE_T j = low; j < high; j++)
    {
        if (compare_messages(nodes[j], pivot, strategy) <= 0)
        {
            // Swap nodes[i] and nodes[j]
            void* temp = nodes[i];
            nodes[i] = nodes[j];
            nodes[j] = temp;
            i++;
        }
    }

    // Swap nodes[i] and nodes[high] (pivot)
    void* temp = nodes[i];
    nodes[i] = nodes[high];
    nodes[high] = temp;
    
    return i;
}

/**
 * @brief Quick sort function for message nodes
 * @param nodes Array of message nodes to be sorted
 * @param low Starting index of the array
 * @param high Ending index of the array
 * @param strategy Comparison strategy to use
 */
static void quick_sort(void** nodes, ZOO_SIZE_T low, ZOO_SIZE_T high, ZOO_INT32 strategy)
{
    if (low < high)
    {
        ZOO_SIZE_T pi = partition(nodes, low, high, strategy);

        // Handle recursion for left partition
        if (pi > 0)
        {
            quick_sort(nodes, low, pi - 1, strategy);
        }

        // Handle right partition
        if (pi < high)
        {
            quick_sort(nodes, pi + 1, high, strategy);
        }
    }
}

/**
 * @brief Sort the message queue based on specified strategy
 * @param queue_handle Handle to the message queue
 * @param strategy Sorting strategy to use
 */
void zoo_queue_sort(
    ZOO_QUEUE_HANDLE queue_handle,
    ZOO_INT32 strategy)
{

    if (!queue_handle)
    {
        return;
    }

    if (strategy == ZOO_QUEUE_SORT_STRATEGY_NONE ||
        strategy == ZOO_QUEUE_SORT_STRATEGY_FIFO)
    {
        return;
    }

    ZOO_QUEUE_STRUCT* queue = (ZOO_QUEUE_STRUCT*)queue_handle;

    ZOO_MUTEX_LOCK(&queue->messages_mutex);

    ZOO_SIZE_T size = zoo_list_size(queue->messages);
    if (size <= 1)
    {
        ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
        return;
    }

    // Create temporary array for sorting
    void** nodes = (void**)zoo_allocate_from_pool(sizeof(void*) * size);
    if (!nodes)
    {
        ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
        return;
    }

    // Copy nodes to array
    for (ZOO_SIZE_T i = 0; i < size; i++)
    {
        nodes[i] = zoo_list_at(queue->messages, i);
        if (!nodes[i])
        {
            zoo_free_to_pool(nodes);
            ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
            return;
        }
    }

    // Sort using quicksort
    quick_sort(nodes, 0, size - 1, strategy);

    // Clear and rebuild list
    zoo_list_clear(queue->messages);
    for (ZOO_SIZE_T i = 0; i < size; i++)
    {
        if (zoo_list_push_back(queue->messages, nodes[i]) != ZOO_OK)
        {
            zoo_free_to_pool(nodes);
            ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
            return;
        }
    }

    zoo_free_to_pool(nodes);
    ZOO_MUTEX_UNLOCK(&queue->messages_mutex);
}

#pragma GCC diagnostic pop
