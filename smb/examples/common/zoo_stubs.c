/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Examples
 * Component id: zoo_stubs_
 * File name: zoo_stubs.c
 * Description: Minimal stub implementations for missing ZOO dependencies
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                created
 ******************************************************************************/

#include "zoo_stubs.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Global log level
static ZOO_LOG_LEVEL_ENUM g_log_level = ZOO_LOG_LEVEL_INFO;

// Log level to string mapping
static const char* log_level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "OFF"
};

/**
 * @brief Set the global log level
 */
void zoo_log_set_level(ZOO_LOG_LEVEL_ENUM level)
{
    g_log_level = level;
}

/**
 * @brief Set log output target (stub: accepts value, no-op)
 */
void zoo_log_set_target(ZOO_LOG_TARGET_ENUM target)
{
    (void)target;
}

/**
 * @brief Get current timestamp string
 */
static void get_timestamp(char* buffer, size_t size)
{
    struct timeval tv;
    struct tm* tm_info;
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             tv.tv_usec / 1000);
}

/**
 * @brief Log a message
 */
void zoo_log_message(ZOO_LOG_LEVEL_ENUM level, const char* file, int line, 
                     const char* func, const char* format, ...)
{
    if (level < g_log_level || level >= ZOO_LOG_LEVEL_OFF) {
        return;
    }
    
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    // Print log header
    printf("[%s] [%s] %s:%d (%s): ", 
           timestamp, log_level_strings[level], file, line, func);
    
    // Print the actual message
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
}

/**
 * @brief Initialize memory pool (stub)
 */
ZOO_ERROR_TYPE zoo_memory_pool_init(void)
{
    return ZOO_SMB_OK;
}

/**
 * @brief Cleanup memory pool (stub)
 */
void zoo_memory_pool_cleanup(void)
{
    // Nothing to do in stub
}

/**
 * @brief Allocate memory from pool (just use malloc)
 */
void* zoo_memory_pool_alloc(size_t size)
{
    return malloc(size);
}

/**
 * @brief Free memory to pool (just use free)
 */
void zoo_memory_pool_free(void* ptr)
{
    free(ptr);
}

/**
 * @brief Create a simple linked list (stub)
 */
zoo_list_t* zoo_list_create(void)
{
    zoo_list_t* list = (zoo_list_t*)malloc(sizeof(zoo_list_t));
    if (list) {
        list->head = NULL;
        list->tail = NULL;
        list->size = 0;
    }
    return list;
}

/**
 * @brief Destroy a linked list (stub)
 */
void zoo_list_destroy(zoo_list_t* list)
{
    if (list) {
        zoo_list_node_t* current = list->head;
        while (current) {
            zoo_list_node_t* next = current->next;
            free(current);
            current = next;
        }
        free(list);
    }
}

/**
 * @brief Add item to list (stub)
 */
ZOO_ERROR_TYPE zoo_list_add(zoo_list_t* list, void* data)
{
    if (!list) return ZOO_SMB_ERROR_INVALID_PARAM;
    
    zoo_list_node_t* node = (zoo_list_node_t*)malloc(sizeof(zoo_list_node_t));
    if (!node) return ZOO_SMB_ERROR_ALLOCATION_FAILED;
    
    node->data = data;
    node->next = NULL;
    
    if (!list->head) {
        list->head = list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    
    list->size++;
    return ZOO_SMB_OK;
}

/**
 * @brief Get list size (stub)
 */
size_t zoo_list_size(const zoo_list_t* list)
{
    return list ? list->size : 0;
}

/**
 * @brief Simple configuration reader (stub)
 */
const char* zoo_config_get_string(const char* section, const char* key, const char* default_value)
{
    // Return default values for common config keys
    if (strcmp(key, "server_address") == 0) return "127.0.0.1";
    if (strcmp(key, "server_port") == 0) return "8080";
    if (strcmp(key, "transport_type") == 0) return "tcp";
    if (strcmp(key, "log_level") == 0) return "info";
    
    return default_value;
}

/**
 * @brief Get config integer (stub)
 */
int zoo_config_get_int(const char* section, const char* key, int default_value)
{
    const char* str_value = zoo_config_get_string(section, key, NULL);
    if (str_value) {
        return atoi(str_value);
    }
    return default_value;
}

/**
 * @brief Load config file (stub)
 */
ZOO_ERROR_TYPE zoo_config_load(const char* filename)
{
    // Stub - always succeed
    printf("[STUB] Loading config file: %s\n", filename ? filename : "default");
    return ZOO_SMB_OK;
}
