/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 *
 * Product: ZOO
 * Module: Utilities
 * Component ID: ZOO_MATH
 * File name: zoo_math.h
 * Description: Mathematical utilities and floating point operations for ZOO
 *
 * Change History:
 * Version   Date           Author          Description
 * -------   ----------     -----------     ---------------------------------
 * 1.0       2025-08-01     AI Assistant    Extracted from zoo_util.h
 ******************************************************************************/
#ifndef ZOO_MATH_H
#define ZOO_MATH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zoo.h"
#include <math.h>
#include <limits.h>

    // ==============================================================================
    // MATHEMATICAL CONSTANTS
    // ==============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

#define ZOO_MATH_EPSILON_FLOAT 1e-6f
#define ZOO_MATH_EPSILON_DOUBLE 1e-15

    // ==============================================================================
    // FLOATING POINT COMPARISON INLINE FUNCTIONS
    // ==============================================================================

    /**
     * @brief Check if a ZOO_FLOAT value is approximately zero
     * @param value Float value to check
     * @return ZOO_TRUE if approximately zero, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_float_is_zero(ZOO_FLOAT value)
    {
        return (fabsf(value) < ZOO_MATH_EPSILON_FLOAT);
    }

    /**
     * @brief Check if a ZOO_DOUBLE value is approximately zero
     * @param value Double value to check
     * @return ZOO_TRUE if approximately zero, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_double_is_zero(ZOO_DOUBLE value)
    {
        return (fabs(value) < ZOO_MATH_EPSILON_DOUBLE);
    }

    /**
     * @brief Check if two ZOO_FLOAT values are approximately equal
     * @param a First ZOO_FLOAT value
     * @param b Second ZOO_FLOAT value
     * @return ZOO_TRUE if approximately equal, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_float_equal(ZOO_FLOAT a, ZOO_FLOAT b)
    {
        return (fabsf(a - b) < ZOO_MATH_EPSILON_FLOAT);
    }

    /**
     * @brief Check if two ZOO_DOUBLE values are approximately equal
     * @param a First ZOO_DOUBLE value
     * @param b Second ZOO_DOUBLE value
     * @return ZOO_TRUE if approximately equal, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_double_equal(ZOO_DOUBLE a, ZOO_DOUBLE b)
    {
        return (fabs(a - b) < ZOO_MATH_EPSILON_DOUBLE);
    }

    /**
     * @brief Check if two ZOO_FLOAT values are approximately equal with custom epsilon
     * @param a First ZOO_FLOAT value
     * @param b Second ZOO_FLOAT value
     * @param epsilon Custom epsilon for comparison
     * @return ZOO_TRUE if approximately equal, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_float_equal_epsilon(ZOO_FLOAT a, ZOO_FLOAT b, ZOO_FLOAT epsilon)
    {
        return (fabsf(a - b) < epsilon);
    }

    /**
     * @brief Check if two ZOO_DOUBLE values are approximately equal with custom epsilon
     * @param a First ZOO_DOUBLE value
     * @param b Second ZOO_DOUBLE value
     * @param epsilon Custom epsilon for comparison
     * @return ZOO_TRUE if approximately equal, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_double_equal_epsilon(ZOO_DOUBLE a, ZOO_DOUBLE b, ZOO_DOUBLE epsilon)
    {
        return (fabs(a - b) < epsilon);
    }

    /**
     * @brief Check if a ZOO_FLOAT value is finite (not infinite or NaN)
     * @param value Float value to check
     * @return ZOO_TRUE if finite, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_float_is_finite(ZOO_FLOAT value)
    {
        return isfinite(value);
    }

    /**
     * @brief Check if a ZOO_DOUBLE value is finite (not infinite or NaN)
     * @param value Double value to check
     * @return ZOO_TRUE if finite, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_double_is_finite(ZOO_DOUBLE value)
    {
        return isfinite(value);
    }

    /**
     * @brief Check if a ZOO_FLOAT value is NaN (Not a Number)
     * @param value Float value to check
     * @return ZOO_TRUE if NaN, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_float_is_nan(ZOO_FLOAT value)
    {
        return isnan(value);
    }

    /**
     * @brief Check if a ZOO_DOUBLE value is NaN (Not a Number)
     * @param value Double value to check
     * @return ZOO_TRUE if NaN, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_double_is_nan(ZOO_DOUBLE value)
    {
        return isnan(value);
    }

    /**
     * @brief Check if a ZOO_FLOAT value is infinite
     * @param value Float value to check
     * @return ZOO_TRUE if infinite, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_float_is_infinite(ZOO_FLOAT value)
    {
        return isinf(value);
    }

    /**
     * @brief Check if a ZOO_DOUBLE value is infinite
     * @param value Double value to check
     * @return ZOO_TRUE if infinite, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_double_is_infinite(ZOO_DOUBLE value)
    {
        return isinf(value);
    }

    // ==============================================================================
    // MATHEMATICAL UTILITY INLINE FUNCTIONS
    // ==============================================================================

    /**
     * @brief Get the absolute value of an integer
     * @param value Integer value
     * @return Absolute value
     */
    static ZOO_FORCE_INLINE int zoo_abs_int(int value)
    {
        return (value < 0) ? -value : value;
    }

    /**
     * @brief Get the absolute value of a long integer
     * @param value Long integer value
     * @return Absolute value
     */
    static ZOO_FORCE_INLINE long zoo_abs_long(long value)
    {
        return (value < 0) ? -value : value;
    }

    /**
     * @brief Get the minimum of two integers
     * @param a First integer
     * @param b Second integer
     * @return Minimum value
     */
    static ZOO_FORCE_INLINE int zoo_min_int(int a, int b)
    {
        return (a < b) ? a : b;
    }

    /**
     * @brief Get the maximum of two integers
     * @param a First integer
     * @param b Second integer
     * @return Maximum value
     */
    static ZOO_FORCE_INLINE int zoo_max_int(int a, int b)
    {
        return (a > b) ? a : b;
    }

    /**
     * @brief Get the minimum of two ZOO_FLOAT values
     * @param a First ZOO_FLOAT
     * @param b Second ZOO_FLOAT
     * @return Minimum value
     */
    static ZOO_FORCE_INLINE ZOO_FLOAT zoo_min_float(ZOO_FLOAT a, ZOO_FLOAT b)
    {
        return (a < b) ? a : b;
    }

    /**
     * @brief Get the maximum of two ZOO_FLOAT values
     * @param a First ZOO_FLOAT
     * @param b Second ZOO_FLOAT
     * @return Maximum value
     */
    static ZOO_FORCE_INLINE ZOO_FLOAT zoo_max_float(ZOO_FLOAT a, ZOO_FLOAT b)
    {
        return (a > b) ? a : b;
    }

    /**
     * @brief Get the minimum of two ZOO_DOUBLE values
     * @param a First ZOO_DOUBLE
     * @param b Second ZOO_DOUBLE
     * @return Minimum value
     */
    static ZOO_FORCE_INLINE ZOO_DOUBLE zoo_min_double(ZOO_DOUBLE a, ZOO_DOUBLE b)
    {
        return (a < b) ? a : b;
    }

    /**
     * @brief Get the maximum of two ZOO_DOUBLE values
     * @param a First ZOO_DOUBLE
     * @param b Second ZOO_DOUBLE
     * @return Maximum value
     */
    static ZOO_FORCE_INLINE ZOO_DOUBLE zoo_max_double(ZOO_DOUBLE a, ZOO_DOUBLE b)
    {
        return (a > b) ? a : b;
    }

    /**
     * @brief Clamp an integer value between min and max
     * @param value Value to clamp
     * @param min_val Minimum allowed value
     * @param max_val Maximum allowed value
     * @return Clamped value
     */
    static ZOO_FORCE_INLINE int zoo_clamp_int(int value, int min_val, int max_val)
    {
        if (value < min_val)
            return min_val;
        if (value > max_val)
            return max_val;
        return value;
    }

    /**
     * @brief Clamp a ZOO_FLOAT value between min and max
     * @param value Value to clamp
     * @param min_val Minimum allowed value
     * @param max_val Maximum allowed value
     * @return Clamped value
     */
    static ZOO_FORCE_INLINE ZOO_FLOAT zoo_clamp_float(ZOO_FLOAT value, ZOO_FLOAT min_val, ZOO_FLOAT max_val)
    {
        if (value < min_val)
            return min_val;
        if (value > max_val)
            return max_val;
        return value;
    }

    /**
     * @brief Clamp a ZOO_DOUBLE value between min and max
     * @param value Value to clamp
     * @param min_val Minimum allowed value
     * @param max_val Maximum allowed value
     * @return Clamped value
     */
    static ZOO_FORCE_INLINE ZOO_DOUBLE zoo_clamp_double(ZOO_DOUBLE value, ZOO_DOUBLE min_val, ZOO_DOUBLE max_val)
    {
        if (value < min_val)
            return min_val;
        if (value > max_val)
            return max_val;
        return value;
    }

    /**
     * @brief Linear interpolation between two ZOO_FLOAT values
     * @param a Starting value
     * @param b Ending value
     * @param t Interpolation factor (0.0 to 1.0)
     * @return Interpolated value
     */
    static ZOO_FORCE_INLINE ZOO_FLOAT zoo_lerp_float(ZOO_FLOAT a, ZOO_FLOAT b, ZOO_FLOAT t)
    {
        return a + t * (b - a);
    }

    /**
     * @brief Linear interpolation between two ZOO_DOUBLE values
     * @param a Starting value
     * @param b Ending value
     * @param t Interpolation factor (0.0 to 1.0)
     * @return Interpolated value
     */
    static ZOO_FORCE_INLINE ZOO_DOUBLE zoo_lerp_double(ZOO_DOUBLE a, ZOO_DOUBLE b, ZOO_DOUBLE t)
    {
        return a + t * (b - a);
    }

    /**
     * @brief Map a value from one range to another (ZOO_FLOAT)
     * @param value Input value
     * @param in_min Input range minimum
     * @param in_max Input range maximum
     * @param out_min Output range minimum
     * @param out_max Output range maximum
     * @return Mapped value
     */
    static ZOO_FORCE_INLINE ZOO_FLOAT zoo_map_float(ZOO_FLOAT value, ZOO_FLOAT in_min, ZOO_FLOAT in_max,
                                                    ZOO_FLOAT out_min, ZOO_FLOAT out_max)
    {
        if (zoo_float_equal(in_max, in_min))
        {
            return out_min;
        }

        ZOO_FLOAT t = (value - in_min) / (in_max - in_min);
        return zoo_lerp_float(out_min, out_max, t);
    }

    /**
     * @brief Map a value from one range to another (ZOO_DOUBLE)
     * @param value Input value
     * @param in_min Input range minimum
     * @param in_max Input range maximum
     * @param out_min Output range minimum
     * @param out_max Output range maximum
     * @return Mapped value
     */
    static ZOO_FORCE_INLINE ZOO_DOUBLE zoo_map_double(ZOO_DOUBLE value, ZOO_DOUBLE in_min, ZOO_DOUBLE in_max,
                                                      ZOO_DOUBLE out_min, ZOO_DOUBLE out_max)
    {
        if (zoo_double_equal(in_max, in_min))
        {
            return out_min;
        }

        ZOO_DOUBLE t = (value - in_min) / (in_max - in_min);
        return zoo_lerp_double(out_min, out_max, t);
    }

    /**
     * @brief Calculate power of 2 for integer (2^n)
     * @param n Exponent
     * @return 2^n, or 0 if n is negative or would overflow
     */
    static ZOO_FORCE_INLINE unsigned int zoo_power_of_2(int n)
    {
        if (n < 0 || n >= 32)
        {
            return 0; // Overflow or invalid
        }

        return 1U << n;
    }

    /**
     * @brief Check if an integer is a power of 2
     * @param value Integer to check
     * @return ZOO_TRUE if power of 2, ZOO_FALSE otherwise
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_is_power_of_2(unsigned int value)
    {
        return (value != 0) && ((value & (value - 1)) == 0);
    }

    /**
     * @brief Round up to the next power of 2
     * @param value Input value
     * @return Next power of 2 greater than or equal to value
     */
    static ZOO_FORCE_INLINE unsigned int zoo_next_power_of_2(unsigned int value)
    {
        if (value == 0)
            return 1;

        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value++;

        return value;
    }

    /**
     * @brief Convert degrees to radians
     * @param degrees Angle in degrees
     * @return Angle in radians
     */
    static ZOO_FORCE_INLINE ZOO_DOUBLE zoo_deg_to_rad(ZOO_DOUBLE degrees)
    {
        return degrees * M_PI / 180.0;
    }

    /**
     * @brief Convert radians to degrees
     * @param radians Angle in radians
     * @return Angle in degrees
     */
    static ZOO_FORCE_INLINE ZOO_DOUBLE zoo_rad_to_deg(ZOO_DOUBLE radians)
    {
        return radians * 180.0 / M_PI;
    }

    /**
     * @brief Safe integer division with remainder
     * @param dividend Dividend
     * @param divisor Divisor
     * @param quotient Pointer to store quotient result
     * @param remainder Pointer to store remainder result
     * @return ZOO_OK on success, negative error code on failure
     */
    static ZOO_FORCE_INLINE int zoo_safe_div_int(int dividend, int divisor,
                                                 int *quotient, int *remainder)
    {
        if (divisor == 0)
        {
            return -1; // Division by zero
        }

        if (quotient != NULL)
        {
            *quotient = dividend / divisor;
        }

        if (remainder != NULL)
        {
            *remainder = dividend % divisor;
        }

        return ZOO_OK;
    }

    /**
     * @brief Calculate integer square root (Newton's method)
     * @param value Input value
     * @return Integer square root
     */
    static ZOO_FORCE_INLINE unsigned int zoo_isqrt(unsigned int value)
    {
        if (value == 0)
            return 0;

        unsigned int x = value;
        unsigned int y = (x + 1) / 2;

        while (y < x)
        {
            x = y;
            y = (x + value / x) / 2;
        }

        return x;
    }

    /**
     * @brief Calculate greatest common divisor using Euclidean algorithm
     * @param a First integer
     * @param b Second integer
     * @return Greatest common divisor
     */
    static ZOO_FORCE_INLINE unsigned int zoo_gcd(unsigned int a, unsigned int b)
    {
        while (b != 0)
        {
            unsigned int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }

    /**
     * @brief Calculate least common multiple
     * @param a First integer
     * @param b Second integer
     * @return Least common multiple
     */
    static ZOO_FORCE_INLINE unsigned int zoo_lcm(unsigned int a, unsigned int b)
    {
        if (a == 0 || b == 0)
            return 0;
        return (a / zoo_gcd(a, b)) * b;
    }

    // ==============================================================================
    // UUID GENERATION FUNCTIONS
    // ==============================================================================

    /**
     * @brief Generate a simple 64-bit UUID based on timestamp and random seed
     * @return ZOO_INT64 UUID value
     * @note This is a simple UUID implementation, not cryptographically secure
     */
    static ZOO_FORCE_INLINE ZOO_INT64 zoo_generate_uuid64(void)
    {
        // Simple UUID generation using timestamp and basic random
        static ZOO_UINT32 seed = 1;
        
        // Get current timestamp (using a simple counter approach for embedded systems)
        static ZOO_UINT32 timestamp_counter = 0;
        timestamp_counter++;
        
        // Simple linear congruential generator
        seed = seed * 1103515245 + 12345;
        
        // Combine timestamp and random for upper 32 bits
        ZOO_UINT32 upper = (timestamp_counter << 16) | (seed & 0xFFFF);
        
        // Generate random for lower 32 bits
        seed = seed * 1103515245 + 12345;
        ZOO_UINT32 lower = seed;
        
        return ((ZOO_INT64)upper << 32) | lower;
    }

    /**
     * @brief Generate a 64-bit UUID with custom seed
     * @param seed Pointer to seed value (will be modified)
     * @return ZOO_INT64 UUID value
     */
    static ZOO_FORCE_INLINE ZOO_INT64 zoo_generate_uuid64_seeded(ZOO_UINT32 *seed)
    {
        if (seed == NULL)
        {
            return zoo_generate_uuid64();
        }
        
        static ZOO_UINT32 timestamp_counter = 0;
        timestamp_counter++;
        
        // Use provided seed
        *seed = *seed * 1103515245 + 12345;
        ZOO_UINT32 upper = (timestamp_counter << 16) | (*seed & 0xFFFF);
        
        *seed = *seed * 1103515245 + 12345;
        ZOO_UINT32 lower = *seed;
        
        return ((ZOO_INT64)upper << 32) | lower;
    }

    /**
     * @brief Generate a monotonic increasing 64-bit UUID
     * @return ZOO_INT64 UUID value (always increasing)
     */
    static ZOO_FORCE_INLINE ZOO_INT64 zoo_generate_uuid64_monotonic(void)
    {
        static ZOO_INT64 last_uuid = 0;
        last_uuid++;
        return last_uuid;
    }

    /**
     * @brief Check if a 64-bit UUID is valid (non-zero)
     * @param uuid UUID to validate
     * @return ZOO_TRUE if valid, ZOO_FALSE if invalid
     */
    static ZOO_FORCE_INLINE ZOO_BOOL zoo_uuid64_is_valid(ZOO_INT64 uuid)
    {
        return (uuid != 0);
    }

    /**
     * @brief Compare two 64-bit UUIDs
     * @param uuid1 First UUID
     * @param uuid2 Second UUID
     * @return 0 if equal, negative if uuid1 < uuid2, positive if uuid1 > uuid2
     */
    static ZOO_FORCE_INLINE int zoo_uuid64_compare(ZOO_INT64 uuid1, ZOO_INT64 uuid2)
    {
        if (uuid1 < uuid2) return -1;
        if (uuid1 > uuid2) return 1;
        return 0;
    }

#ifdef __cplusplus
}
#endif

#endif /* ZOO_MATH_H */
