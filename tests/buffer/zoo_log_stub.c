/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: buffer
 * Component id: queue_test
 * File name: zoo_log_stub.c
 * Description: Minimal stub implementation for log functions
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-31     weiwang.sun         created
 ******************************************************************************/

#include <stdio.h>
#include <stdarg.h>

// Forward declarations for log functions (avoiding zoo_log.h dependency)
void zoo_log_trace(const char *file, int line_num, const char *func_name, const char *format, ...);
void zoo_log_debug(const char *file, int line_num, const char *func_name, const char *format, ...);
void zoo_log_info(const char *file, int line_num, const char *func_name, const char *format, ...);
void zoo_log_warn(const char *file, int line_num, const char *func_name, const char *format, ...);
void zoo_log_error(const char *file, int line_num, const char *func_name, const char *format, ...);

// Simple stub implementation using printf
void zoo_log_trace(const char *file, int line_num, const char *func_name, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[TRACE] %s:%d %s() ", file, line_num, func_name);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void zoo_log_debug(const char *file, int line_num, const char *func_name, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[DEBUG] %s:%d %s() ", file, line_num, func_name);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void zoo_log_info(const char *file, int line_num, const char *func_name, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[INFO] %s:%d %s() ", file, line_num, func_name);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void zoo_log_warn(const char *file, int line_num, const char *func_name, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[WARN] %s:%d %s() ", file, line_num, func_name);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void zoo_log_error(const char *file, int line_num, const char *func_name, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[ERROR] %s:%d %s() ", file, line_num, func_name);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
