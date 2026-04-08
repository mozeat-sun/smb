/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Utilities
 * Component ID: ZOO_STRING
 * File name: zoo_string.h
 * Description: String manipulation utilities and safe string operations for ZOO
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     AI Assistant    Extracted from zoo_util.h
 ******************************************************************************/
#ifndef ZOO_STRING_H
#define ZOO_STRING_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo.h"
#include <string.h>
#include <stdlib.h>

    // ==============================================================================
    // SAFE STRING MANIPULATION INLINE FUNCTIONS
    // ==============================================================================

    /**
     * @brief Safely copy string with null termination guarantee
     * @param dest Destination buffer
     * @param src Source string
     * @param dest_size Size of destination buffer
     * @return ZOO_OK on success, negative error code on failure
     * @note Always null-terminates the destination string
     */
    static ZOO_FORCE_INLINE ZOO_INT32 zoo_safety_copy_string(char *dest, ZOO_STRING src, ZOO_SIZE_T dest_size)
    {
        if (dest == NULL || src == NULL || dest_size == 0)
        {
            return -1; // Invalid parameters
        }

        if (dest_size == 1)
        {
            dest[0] = '\0';
            return ZOO_OK;
        }

        ZOO_SIZE_T src_len = strlen(src);
        ZOO_SIZE_T copy_len = (src_len < dest_size - 1) ? src_len : dest_size - 1;

        if (copy_len > 0)
        {
            memcpy(dest, src, copy_len);
        }
        dest[copy_len] = '\0';

        return ZOO_OK;
    }

    /**
     * @brief Safely concatenate strings with buffer overflow protection
     * @param dest Destination buffer
     * @param src Source string to append
     * @param dest_size Total size of destination buffer
     * @return ZOO_OK on success, negative error code on failure
     * @note Always maintains null termination
     */
    static ZOO_FORCE_INLINE ZOO_INT32 zoo_safety_concat_string(char *dest, ZOO_STRING src, ZOO_SIZE_T dest_size)
    {
        if (dest == NULL || src == NULL || dest_size == 0)
        {
            return -1; // Invalid parameters
        }

        ZOO_SIZE_T dest_len = strlen(dest);
        if (dest_len >= dest_size)
        {
            return -2; // Destination already full
        }

        ZOO_SIZE_T remaining = dest_size - dest_len - 1; // -1 for null terminator
        ZOO_SIZE_T src_len = strlen(src);
        ZOO_SIZE_T copy_len = (src_len < remaining) ? src_len : remaining;

        if (copy_len > 0)
        {
            memcpy(dest + dest_len, src, copy_len);
        }
        dest[dest_len + copy_len] = '\0';

        return ZOO_OK;
    }

    /**
     * @brief Safely duplicate a string with memory allocation
     * @param src Source string to duplicate
     * @return Pointer to newly allocated string, or NULL on failure
     * @note Caller is responsible for freeing the returned pointer
     */
    static ZOO_FORCE_INLINE char *zoo_safety_duplicate_string(ZOO_STRING src)
    {
        if (src == NULL)
        {
            return NULL;
        }

        ZOO_SIZE_T src_len = strlen(src);
        char *dest = (char *)malloc(src_len + 1);

        if (dest == NULL)
        {
            return NULL; // Memory allocation failed
        }

        memcpy(dest, src, src_len);
        dest[src_len] = '\0';

        return dest;
    }

    /**
     * @brief Safely compare strings with length limit
     * @param str1 First string
     * @param str2 Second string
     * @param max_len Maximum number of characters to compare
     * @return 0 if equal, negative if str1 < str2, positive if str1 > str2
     * @note NULL-safe comparison
     */
    static ZOO_FORCE_INLINE ZOO_INT32 zoo_safety_compare_string(ZOO_STRING str1, ZOO_STRING str2, ZOO_SIZE_T max_len)
    {
        if (str1 == NULL || str2 == NULL)
        {
            if (str1 == str2)
                return 0;
            return (str1 == NULL) ? -1 : 1;
        }

        if (max_len == 0)
        {
            return 0;
        }

        return strncmp(str1, str2, max_len);
    }

    /**
     * @brief Find substring in string with length limit
     * @param haystack String to search in
     * @param needle Substring to find
     * @param haystack_len Maximum length to search in haystack
     * @return Pointer to first occurrence or NULL if not found
     * @note NULL-safe search
     */
    static ZOO_FORCE_INLINE ZOO_STRING zoo_safety_find_substring(ZOO_STRING haystack, ZOO_STRING needle, ZOO_SIZE_T haystack_len)
    {
        if (haystack == NULL || needle == NULL || haystack_len == 0)
        {
            return NULL;
        }

        ZOO_SIZE_T needle_len = strlen(needle);
        if (needle_len == 0)
        {
            return haystack;
        }

        if (needle_len > haystack_len)
        {
            return NULL;
        }

        for (ZOO_SIZE_T i = 0; i <= haystack_len - needle_len; i++)
        {
            if (strncmp(haystack + i, needle, needle_len) == 0)
            {
                return haystack + i;
            }
        }

        return NULL;
    }

    /**
     * @brief Get safe string length with maximum limit
     * @param str String to measure
     * @param max_len Maximum length to check
     * @return Length of string, or max_len if string is longer
     * @note Returns 0 for NULL string
     */
    static ZOO_FORCE_INLINE ZOO_SIZE_T zoo_safety_string_length(ZOO_STRING str, ZOO_SIZE_T max_len)
    {
        if (str == NULL)
        {
            return 0;
        }

        ZOO_SIZE_T len = 0;
        while (len < max_len && str[len] != '\0')
        {
            len++;
        }

        return len;
    }

    /**
     * @brief Check if string is empty or NULL
     * @param str String to check
     * @return ZOO_TRUE if string is NULL or empty, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_string_is_empty(ZOO_STRING str)
    {
        return (str == NULL || str[0] == '\0');
    }

    /**
     * @brief Check if two strings are the same (case-sensitive)
     * @param str1 First string
     * @param str2 Second string
     * @return ZOO_TRUE if strings are identical, ZOO_FALSE otherwise
     * @note NULL-safe comparison: two NULL strings are considered the same
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_string_is_the_same(ZOO_STRING str1, ZOO_STRING str2)
    {
        // Handle NULL cases
        if (str1 == NULL || str2 == NULL)
        {
            return (str1 == str2); // Both NULL = same, one NULL = different
        }

        // Use strcmp for comparison
        return (strcmp(str1, str2) == 0);
    }

    /**
     * @brief Trim whitespace from beginning and end of string
     * @param str String to trim (modified in place)
     * @return Pointer to trimmed string
     * @note Modifies the original string
     */
    static ZOO_FORCE_INLINE char *zoo_string_trim(char *str)
    {
        if (str == NULL)
        {
            return NULL;
        }

        // Trim leading whitespace
        while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r'))
        {
            str++;
        }

        if (*str == '\0')
        {
            return str;
        }

        // Trim trailing whitespace
        char *end = str + strlen(str) - 1;
        while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        {
            end--;
        }
        *(end + 1) = '\0';

        return str;
    }

    /**
     * @brief Convert string to lowercase in place
     * @param str String to convert
     * @return Pointer to converted string
     * @note Modifies the original string
     */
    static ZOO_FORCE_INLINE char *zoo_string_to_lower(char *str)
    {
        if (str == NULL)
        {
            return NULL;
        }

        char *p = str;
        while (*p)
        {
            if (*p >= 'A' && *p <= 'Z')
            {
                *p = *p + ('a' - 'A');
            }
            p++;
        }

        return str;
    }

    /**
     * @brief Convert string to uppercase in place
     * @param str String to convert
     * @return Pointer to converted string
     * @note Modifies the original string
     */
    static ZOO_FORCE_INLINE char *zoo_string_to_upper(char *str)
    {
        if (str == NULL)
        {
            return NULL;
        }

        char *p = str;
        while (*p)
        {
            if (*p >= 'a' && *p <= 'z')
            {
                *p = *p - ('a' - 'A');
            }
            p++;
        }

        return str;
    }

    /**
     * @brief Check if string starts with prefix
     * @param str String to check
     * @param prefix Prefix to look for
     * @return ZOO_TRUE if str starts with prefix, ZOO_FALSE otherwise
     * @note NULL-safe comparison
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_string_starts_with(ZOO_STRING str, ZOO_STRING prefix)
    {
        if (str == NULL || prefix == NULL)
        {
            return ZOO_FALSE;
        }

        ZOO_SIZE_T prefix_len = strlen(prefix);
        if (prefix_len == 0)
        {
            return ZOO_TRUE;
        }

        return strncmp(str, prefix, prefix_len) == 0;
    }

    /**
     * @brief Check if string ends with suffix
     * @param str String to check
     * @param suffix Suffix to look for
     * @return ZOO_TRUE if str ends with suffix, ZOO_FALSE otherwise
     * @note NULL-safe comparison
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_string_ends_with(ZOO_STRING str, ZOO_STRING suffix)
    {
        if (str == NULL || suffix == NULL)
        {
            return ZOO_FALSE;
        }

        ZOO_SIZE_T str_len = strlen(str);
        ZOO_SIZE_T suffix_len = strlen(suffix);

        if (suffix_len == 0)
        {
            return ZOO_TRUE;
        }

        if (suffix_len > str_len)
        {
            return ZOO_FALSE;
        }

        return strcmp(str + str_len - suffix_len, suffix) == 0;
    }

    /**
     * @brief Replace all occurrences of a character in string
     * @param str String to modify
     * @param old_char Character to replace
     * @param new_char Replacement character
     * @return Number of replacements made
     * @note Modifies the original string
     */
    static ZOO_FORCE_INLINE ZOO_SIZE_T zoo_string_replace_char(char *str, char old_char, char new_char)
    {
        if (str == NULL)
        {
            return 0;
        }

        ZOO_SIZE_T count = 0;
        char *p = str;

        while (*p)
        {
            if (*p == old_char)
            {
                *p = new_char;
                count++;
            }
            p++;
        }

        return count;
    }

#ifdef __cplusplus
}
#endif

#endif /* ZOO_STRING_H */
