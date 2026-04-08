/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_UTILITY
 * File name: zoo_smb_util.h
 * Description: SMB utility functions
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     assistant         created
 ******************************************************************************/

#ifndef ZOO_SMB_UTIL_H
#define ZOO_SMB_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo_smb_types.h"
#include "zoo_timestamp.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Create a name from 3 components
 * @param part1 First part of the name
 * @param part2 Second part of the name  
 * @param transport_type Transport type
 * @return Formatted name string (static buffer)
 */
static inline const char* zoo_smb_make_name_3(const char* part1, const char* part2, ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type)
{
    static char name_buffer[256];
    const char* transport_str = ZOO_SMB_TRANSPORT_TYPE_TO_STRING(transport_type);
    
    snprintf(name_buffer, sizeof(name_buffer), "%s_%s_%s", 
             part1 ? part1 : "NULL", 
             part2 ? part2 : "NULL", 
             transport_str);
    
    return name_buffer;
}

/**
 * @brief Create a name from 4 components
 * @param part1 First part of the name
 * @param part2 Second part of the name  
 * @param part3 Third part of the name
 * @param part4 Fourth part of the name
 * @return Formatted name string (static buffer)
 */
static inline const char* zoo_smb_make_name_4(const char* part1, const char* part2, const char* part3, const char* part4)
{
    static char name_buffer[256];
    
    snprintf(name_buffer, sizeof(name_buffer), "%s_%s_%s_%s", 
             part1 ? part1 : "NULL", 
             part2 ? part2 : "NULL", 
             part3 ? part3 : "NULL",
             part4 ? part4 : "NULL");
    
    return name_buffer;
}

/**
 * @brief Create a unique ID
 * @return Unique ID based on timestamp and random component
 */
static inline uint64_t zoo_smb_create_unique_id(void)
{
    static uint32_t counter = 0;
    uint64_t timestamp = zoo_get_timestamp_milliseconds();
    return (timestamp << 16) | (++counter & 0xFFFF);
}

/**
 * @brief Get current time in milliseconds
 * @return Current time in milliseconds
 */
static inline uint64_t zoo_smb_get_current_time_ms(void)
{
    return zoo_get_timestamp_milliseconds();
}

/**
 * @brief Check if two strings are the same
 * @param str1 First string
 * @param str2 Second string
 * @return true if strings are equal, false otherwise
 */
static inline bool zoo_smb_is_same_string(const char* str1, const char* str2)
{
    if (str1 == NULL && str2 == NULL) return true;
    if (str1 == NULL || str2 == NULL) return false;
    return strcmp(str1, str2) == 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ZOO_SMB_UTIL_H */
