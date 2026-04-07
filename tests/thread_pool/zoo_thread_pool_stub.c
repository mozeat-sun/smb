/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Thread Pool
 * File name: zoo_thread_pool_stub.c
 * Description: Minimal stub implementation for testing Unity migration
 * 
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01    AI Assistant      Created for Unity testing
 ******************************************************************************/

#include "zoo_thread_pool.h"
#include "zoo.h"
#include "zoo_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Simple global thread pool for testing
static ZOO_BOOL pool_initialized = ZOO_FALSE;
static pthread_t worker_threads[8];
static ZOO_SIZE_T thread_count = 0;
static ZOO_BOOL shutdown_flag = ZOO_FALSE;

// Simple task structure
typedef struct {
    ZOO_TASK_FUNC function;
    void *user_data;
    void *argument;
    ZOO_TASK_FUNC callback;
    void *callback_user_data;
    void *callback_argument;
} simple_task_t;

// Simple queue (for basic testing)
static simple_task_t task_queue[100];
static int queue_head = 0;
static int queue_tail = 0;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Worker thread function
static void* worker_thread(void *arg)
{
    (void)arg;
    
    while (!shutdown_flag) {
        simple_task_t task;
        ZOO_BOOL has_task = ZOO_FALSE;
        
        pthread_mutex_lock(&queue_mutex);
        
        while (queue_head == queue_tail && !shutdown_flag) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        
        if (!shutdown_flag && queue_head != queue_tail) {
            task = task_queue[queue_head];
            queue_head = (queue_head + 1) % 100;
            has_task = ZOO_TRUE;
        }
        
        pthread_mutex_unlock(&queue_mutex);
        
        if (has_task && task.function) {
            // Execute task
            task.function(task.user_data, task.argument);
            
            // Execute callback if present
            if (task.callback) {
                task.callback(task.callback_user_data, task.callback_argument);
            }
        }
    }
    
    return NULL;
}

ZOO_THREAD_POOL_HANDLE zoo_create_thread_pool(ZOO_SIZE_T thread_count_param,
                                              ZOO_SIZE_T max_queue_size)
{
    if (thread_count_param == 0 || max_queue_size == 0) {
        return NULL;
    }
    
    if (pool_initialized) {
        return (ZOO_THREAD_POOL_HANDLE)1; // Return dummy handle
    }
    
    thread_count = (thread_count_param > 8) ? 8 : thread_count_param;
    shutdown_flag = ZOO_FALSE;
    queue_head = 0;
    queue_tail = 0;
    
    // Create worker threads
    for (ZOO_SIZE_T i = 0; i < thread_count; i++) {
        if (pthread_create(&worker_threads[i], NULL, worker_thread, NULL) != 0) {
            // Cleanup on failure
            shutdown_flag = ZOO_TRUE;
            pthread_cond_broadcast(&queue_cond);
            
            for (ZOO_SIZE_T j = 0; j < i; j++) {
                pthread_join(worker_threads[j], NULL);
            }
            return NULL;
        }
    }
    
    pool_initialized = ZOO_TRUE;
    return (ZOO_THREAD_POOL_HANDLE)1; // Return dummy handle
}

ZOO_ERROR_TYPE zoo_thread_pool_submit_task(
    const char *task_name,
    ZOO_TASK_FUNC function,
    void *user_data,
    void *argument,
    ZOO_BOOL block)
{
    (void)task_name; // Suppress unused parameter warning
    
    if (!pool_initialized || !function) {
        return ZOO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&queue_mutex);
    
    int next_tail = (queue_tail + 1) % 100;
    
    // Check if queue is full
    if (next_tail == queue_head) {
        pthread_mutex_unlock(&queue_mutex);
        
        if (block) {
            // For blocking mode, wait a bit and try again
            usleep(1000);
            return zoo_thread_pool_submit_task(task_name, function, user_data, argument, block);
        } else {
            return ZOO_ERROR_RESOURCE_BUSY;
        }
    }
    
    // Add task to queue
    task_queue[queue_tail].function = function;
    task_queue[queue_tail].user_data = user_data;
    task_queue[queue_tail].argument = argument;
    task_queue[queue_tail].callback = NULL;
    task_queue[queue_tail].callback_user_data = NULL;
    task_queue[queue_tail].callback_argument = NULL;
    
    queue_tail = next_tail;
    
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
    
    return ZOO_OK;
}

ZOO_ERROR_TYPE zoo_thread_pool_submit_task_ex(
    const char *task_name,
    ZOO_TASK_FUNC function,
    void *f_user_data,
    void *f_argument,
    ZOO_TASK_FUNC call_after_func,
    void *caf_user_data,
    void *caf_argument,
    ZOO_BOOL block)
{
    (void)task_name; // Suppress unused parameter warning
    
    if (!pool_initialized || !function) {
        return ZOO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&queue_mutex);
    
    int next_tail = (queue_tail + 1) % 100;
    
    // Check if queue is full
    if (next_tail == queue_head) {
        pthread_mutex_unlock(&queue_mutex);
        
        if (block) {
            // For blocking mode, wait a bit and try again
            usleep(1000);
            return zoo_thread_pool_submit_task_ex(task_name, function, f_user_data, f_argument,
                                                 call_after_func, caf_user_data, caf_argument, block);
        } else {
            return ZOO_ERROR_RESOURCE_BUSY;
        }
    }
    
    // Add task to queue
    task_queue[queue_tail].function = function;
    task_queue[queue_tail].user_data = f_user_data;
    task_queue[queue_tail].argument = f_argument;
    task_queue[queue_tail].callback = call_after_func;
    task_queue[queue_tail].callback_user_data = caf_user_data;
    task_queue[queue_tail].callback_argument = caf_argument;
    
    queue_tail = next_tail;
    
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
    
    return ZOO_OK;
}

void zoo_destroy_thread_pool(ZOO_BOOL graceful)
{
    if (!pool_initialized) {
        return;
    }
    
    pthread_mutex_lock(&queue_mutex);
    shutdown_flag = ZOO_TRUE;
    pthread_cond_broadcast(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
    
    // Wait for threads to finish
    for (ZOO_SIZE_T i = 0; i < thread_count; i++) {
        pthread_join(worker_threads[i], NULL);
    }
    
    if (graceful) {
        // In graceful mode, we already waited for threads
        // In non-graceful mode, we also wait but could be more aggressive
    }
    
    pool_initialized = ZOO_FALSE;
    thread_count = 0;
    queue_head = 0;
    queue_tail = 0;
}

ZOO_ERROR_TYPE zoo_thread_pool_get_status(ZOO_SIZE_T *active_threads,
                                         ZOO_SIZE_T *queued_tasks)
{
    if (!pool_initialized) {
        return ZOO_ERROR_INVALID_PARAM;
    }
    
    if (active_threads) {
        *active_threads = thread_count;
    }
    
    if (queued_tasks) {
        pthread_mutex_lock(&queue_mutex);
        int count = queue_tail - queue_head;
        if (count < 0) count += 100;
        *queued_tasks = (ZOO_SIZE_T)count;
        pthread_mutex_unlock(&queue_mutex);
    }
    
    return ZOO_OK;
}
