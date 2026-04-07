/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Examples
 * Component id: zoo_stubs_
 * File name: zoo_stubs.h
 * Description: Minimal stub implementations for missing ZOO dependencies
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                created
 ******************************************************************************/

#ifndef ZOO_STUBS_H
#define ZOO_STUBS_H

#include "zoo_platform.h"
#include "utility/zoo_smb_error.h"
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------
// Log Level Definitions
// ----------------------
typedef enum {
    ZOO_LOG_LEVEL_TRACE = 0,
    ZOO_LOG_LEVEL_DEBUG = 1,
    ZOO_LOG_LEVEL_INFO = 2,
    ZOO_LOG_LEVEL_WARN = 3,
    ZOO_LOG_LEVEL_ERROR = 4,
    ZOO_LOG_LEVEL_FATAL = 5,
    ZOO_LOG_LEVEL_OFF = 6
} ZOO_LOG_LEVEL_ENUM;

typedef enum {
    ZOO_LOG_TARGET_CONSOLE = 0,
    ZOO_LOG_TARGET_FILE = 1,
    ZOO_LOG_TARGET_BOTH = 2
} ZOO_LOG_TARGET_ENUM;

// ----------------------
// Logging Functions
// ----------------------
void zoo_log_set_level(ZOO_LOG_LEVEL_ENUM level);
void zoo_log_set_target(ZOO_LOG_TARGET_ENUM target);
void zoo_log_message(ZOO_LOG_LEVEL_ENUM level, const char* file, int line, 
                     const char* func, const char* format, ...);

// Logging macros
#define ZOO_LOG_TRACE(format, ...) zoo_log_message(ZOO_LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define ZOO_LOG_DEBUG(format, ...) zoo_log_message(ZOO_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define ZOO_LOG_INFO(format, ...)  zoo_log_message(ZOO_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define ZOO_LOG_WARN(format, ...)  zoo_log_message(ZOO_LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define ZOO_LOG_ERROR(format, ...) zoo_log_message(ZOO_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define ZOO_LOG_FATAL(format, ...) zoo_log_message(ZOO_LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

// ----------------------
// Memory Pool Functions
// ----------------------
ZOO_ERROR_TYPE zoo_memory_pool_init(void);
void zoo_memory_pool_cleanup(void);
void* zoo_memory_pool_alloc(size_t size);
void zoo_memory_pool_free(void* ptr);

// ----------------------
// List Data Structure
// ----------------------
typedef struct zoo_list_node {
    void* data;
    struct zoo_list_node* next;
} zoo_list_node_t;

typedef struct {
    zoo_list_node_t* head;
    zoo_list_node_t* tail;
    size_t size;
} zoo_list_t;

zoo_list_t* zoo_list_create(void);
void zoo_list_destroy(zoo_list_t* list);
ZOO_ERROR_TYPE zoo_list_add(zoo_list_t* list, void* data);
size_t zoo_list_size(const zoo_list_t* list);

// ----------------------
// Configuration Functions
// ----------------------
const char* zoo_config_get_string(const char* section, const char* key, const char* default_value);
int zoo_config_get_int(const char* section, const char* key, int default_value);
ZOO_ERROR_TYPE zoo_config_load(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // ZOO_STUBS_H
