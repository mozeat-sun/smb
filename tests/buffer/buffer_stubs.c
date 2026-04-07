/*******************************************************************************
 * Buffer List and Queue Stub Implementation
 * Simple stub for testing Unity framework integration
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>

// Basic type definitions
typedef int ZOO_ERROR_T;

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

// Error codes - define the ones we need for testing (matching test_zoo_list.c)
#define ZOO_ERROR_OK 0
#define ZOO_ERROR_INVALID_PARAM -1
#define ZOO_ERROR_EMPTY -2
#define ZOO_ERROR_FULL -3
#define ZOO_ERROR_MEMORY -4

// Use the same error code definition as the zoo.h header file
// Since the tests include zoo.h directly, we don't need to redefine ZOO_ERROR_T

// Simple list implementation
// List structure
typedef struct zoo_list_node {
    void* data;
    size_t data_size;
    struct zoo_list_node* next;
} zoo_list_node_t;

typedef struct {
    zoo_list_node_t* head;
    int size;
    int max_size;
    pthread_mutex_t mutex;
} zoo_list_t;

// Simple queue implementation  
typedef struct zoo_queue_node {
    void* data;
    size_t data_size;
    struct zoo_queue_node* next;
} zoo_queue_node_t;

typedef struct zoo_queue {
    zoo_queue_node_t* front;
    zoo_queue_node_t* rear;
    int size;
    int max_size;
    pthread_mutex_t mutex;
} zoo_queue_t;

// List functions
ZOO_ERROR_T zoo_list_create(zoo_list_t** list, int max_size) {
    if (!list || max_size <= 0) return ZOO_ERROR_INVALID_PARAM;
    
    zoo_list_t* new_list = (zoo_list_t*)malloc(sizeof(zoo_list_t));
    if (!new_list) return ZOO_ERROR_MEMORY;
    
    new_list->head = NULL;
    new_list->size = 0;
    new_list->max_size = max_size;
    
    if (pthread_mutex_init(&new_list->mutex, NULL) != 0) {
        free(new_list);
        return ZOO_ERROR_MEMORY;
    }
    
    *list = new_list;
    return ZOO_ERROR_OK;
}

ZOO_ERROR_T zoo_list_destroy(zoo_list_t* list) {
    if (!list) return ZOO_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&list->mutex);
    
    // Free all nodes
    zoo_list_node_t* current = list->head;
    while (current) {
        zoo_list_node_t* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    
    pthread_mutex_unlock(&list->mutex);
    pthread_mutex_destroy(&list->mutex);
    free(list);
    return ZOO_ERROR_OK;
}

ZOO_ERROR_T zoo_list_push(zoo_list_t* list, const void* data, size_t size) {
    if (!list || !data || size == 0) return ZOO_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&list->mutex);
    
    if (list->size >= list->max_size) {
        pthread_mutex_unlock(&list->mutex);
        return ZOO_ERROR_FULL;
    }
    
    zoo_list_node_t* new_node = (zoo_list_node_t*)malloc(sizeof(zoo_list_node_t));
    if (!new_node) {
        pthread_mutex_unlock(&list->mutex);
        return ZOO_ERROR_MEMORY;
    }
    
    new_node->data = malloc(size);
    if (!new_node->data) {
        free(new_node);
        pthread_mutex_unlock(&list->mutex);
        return ZOO_ERROR_MEMORY;
    }
    
    memcpy(new_node->data, data, size);
    new_node->data_size = size;
    new_node->next = list->head;
    list->head = new_node;
    list->size++;
    
    pthread_mutex_unlock(&list->mutex);
    return ZOO_ERROR_OK;
}

ZOO_ERROR_T zoo_list_pop(zoo_list_t* list, void* data, size_t size) {
    if (!list || !data || size == 0) return ZOO_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&list->mutex);
    
    if (list->size == 0) {
        pthread_mutex_unlock(&list->mutex);
        return ZOO_ERROR_EMPTY;
    }
    
    zoo_list_node_t* node = list->head;
    if (node->data_size != size) {
        pthread_mutex_unlock(&list->mutex);
        return ZOO_ERROR_INVALID_PARAM;
    }
    
    memcpy(data, node->data, size);
    list->head = node->next;
    list->size--;
    
    free(node->data);
    free(node);
    
    pthread_mutex_unlock(&list->mutex);
    return ZOO_ERROR_OK;
}

int zoo_list_size(zoo_list_t* list) {
    if (!list) return -1;
    
    pthread_mutex_lock(&list->mutex);
    int size = list->size;
    pthread_mutex_unlock(&list->mutex);
    
    return size;
}

int zoo_list_is_empty(zoo_list_t* list) {
    if (!list) return 0;
    
    pthread_mutex_lock(&list->mutex);
    int empty = (list->size == 0);
    pthread_mutex_unlock(&list->mutex);
    
    return empty;
}

int zoo_list_is_full(zoo_list_t* list) {
    if (!list) return 0;
    
    pthread_mutex_lock(&list->mutex);
    int full = (list->size >= list->max_size);
    pthread_mutex_unlock(&list->mutex);
    
    return full;
}

// Queue functions
ZOO_ERROR_T zoo_queue_create(zoo_queue_t** queue, int max_size) {
    if (!queue || max_size <= 0) return ZOO_ERROR_INVALID_PARAM;
    
    zoo_queue_t* new_queue = (zoo_queue_t*)malloc(sizeof(zoo_queue_t));
    if (!new_queue) return ZOO_ERROR_MEMORY;
    
    new_queue->front = NULL;
    new_queue->rear = NULL;
    new_queue->size = 0;
    new_queue->max_size = max_size;
    
    if (pthread_mutex_init(&new_queue->mutex, NULL) != 0) {
        free(new_queue);
        return ZOO_ERROR_MEMORY;
    }
    
    *queue = new_queue;
    return ZOO_ERROR_OK;
}

ZOO_ERROR_T zoo_queue_destroy(zoo_queue_t* queue) {
    if (!queue) return ZOO_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&queue->mutex);
    
    // Free all nodes
    zoo_queue_node_t* current = queue->front;
    while (current) {
        zoo_queue_node_t* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    free(queue);
    return ZOO_ERROR_OK;
}

ZOO_ERROR_T zoo_queue_enqueue(zoo_queue_t* queue, const void* data, size_t size) {
    if (!queue || !data || size == 0) return ZOO_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->size >= queue->max_size) {
        pthread_mutex_unlock(&queue->mutex);
        return ZOO_ERROR_FULL;
    }
    
    zoo_queue_node_t* new_node = (zoo_queue_node_t*)malloc(sizeof(zoo_queue_node_t));
    if (!new_node) {
        pthread_mutex_unlock(&queue->mutex);
        return ZOO_ERROR_INVALID_PARAM;
    }
    
    new_node->data = malloc(size);
    if (!new_node->data) {
        free(new_node);
        pthread_mutex_unlock(&queue->mutex);
        return ZOO_ERROR_INVALID_PARAM;
    }
    
    memcpy(new_node->data, data, size);
    new_node->data_size = size;
    new_node->next = NULL;
    
    if (!queue->rear) {
        queue->front = queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }
    
    queue->size++;
    pthread_mutex_unlock(&queue->mutex);
    return ZOO_ERROR_OK;
}

ZOO_ERROR_T zoo_queue_dequeue(zoo_queue_t* queue, void* data, size_t size) {
    if (!queue || !data || size == 0) return ZOO_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return ZOO_ERROR_EMPTY;
    }
    
    zoo_queue_node_t* node = queue->front;
    if (node->data_size != size) {
        pthread_mutex_unlock(&queue->mutex);
        return ZOO_ERROR_INVALID_PARAM;
    }
    
    memcpy(data, node->data, size);
    
    queue->front = node->next;
    if (!queue->front) queue->rear = NULL;
    
    free(node->data);
    free(node);
    queue->size--;
    
    pthread_mutex_unlock(&queue->mutex);
    return ZOO_ERROR_OK;
}

int zoo_queue_size(zoo_queue_t* queue) {
    if (!queue) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    int size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    
    return size;
}

int zoo_queue_is_empty(zoo_queue_t* queue) {
    if (!queue) return 0;
    
    pthread_mutex_lock(&queue->mutex);
    int empty = (queue->size == 0);
    pthread_mutex_unlock(&queue->mutex);
    
    return empty;
}

int zoo_queue_is_full(zoo_queue_t* queue) {
    if (!queue) return 0;
    
    pthread_mutex_lock(&queue->mutex);
    int full = (queue->size >= queue->max_size);
    pthread_mutex_unlock(&queue->mutex);
    
    return full;
}
