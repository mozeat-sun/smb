#include "unity.h"
#include "zoo_util.h"
#include <math.h>

// Unity requires setUp and tearDown functions
void setUp(void)
{
    // Set up code here
}

void tearDown(void)
{
    // Clean up code here
}

void test_zoo_float_comparison_functions(void)
{
    // Test float zero checking
    ZOO_BOOL result = zoo_float_is_zero(0.0f);
    TEST_ASSERT_TRUE(result);
    
    result = zoo_float_is_zero(0.000001f);
    TEST_ASSERT_FALSE(result); // Boundary value is not strictly within epsilon
    
    result = zoo_float_is_zero(1.0f);
    TEST_ASSERT_FALSE(result);
    
    // Test float equality
    result = zoo_float_equal(3.14159f, 3.14159f);
    TEST_ASSERT_TRUE(result);
    
    result = zoo_float_equal(3.14159f, 3.14160f);
    TEST_ASSERT_FALSE(result); // Difference exceeds float epsilon (1e-6)
    
    result = zoo_float_equal(3.14159f, 3.15f);
    TEST_ASSERT_FALSE(result);
}

void test_zoo_double_comparison_functions(void)
{
    // Test double zero checking
    ZOO_BOOL result = zoo_double_is_zero(0.0);
    TEST_ASSERT_TRUE(result);
    
    result = zoo_double_is_zero(0.000000000000001);
    TEST_ASSERT_FALSE(result); // Boundary value equals epsilon; API uses strict <
    
    result = zoo_double_is_zero(1.0);
    TEST_ASSERT_FALSE(result);
    
    // Test double equality
    result = zoo_double_equal(3.141592653589793, 3.141592653589793);
    TEST_ASSERT_TRUE(result);
    
    result = zoo_double_equal(3.141592653589793, 3.141592653589794);
    TEST_ASSERT_TRUE(result); // Within epsilon
}

void test_zoo_float_special_values(void)
{
    // Test NaN detection
    float nan_val = 0.0f / 0.0f;
    ZOO_BOOL result = zoo_float_is_nan(nan_val);
    TEST_ASSERT_TRUE(result);
    
    result = zoo_float_is_nan(1.0f);
    TEST_ASSERT_FALSE(result);
    
    // Test infinity detection
    float inf_val = 1.0f / 0.0f;
    result = zoo_float_is_infinite(inf_val);
    TEST_ASSERT_TRUE(result);
    
    result = zoo_float_is_infinite(1.0f);
    TEST_ASSERT_FALSE(result);
    
    // Test finite values
    result = zoo_float_is_finite(1.0f);
    TEST_ASSERT_TRUE(result);
    
    result = zoo_float_is_finite(inf_val);
    TEST_ASSERT_FALSE(result);
    
    result = zoo_float_is_finite(nan_val);
    TEST_ASSERT_FALSE(result);
}

void test_zoo_integer_math_functions(void)
{
    // Test absolute value
    int result = zoo_abs_int(-5);
    TEST_ASSERT_EQUAL(5, result);
    
    result = zoo_abs_int(5);
    TEST_ASSERT_EQUAL(5, result);
    
    // Test min/max
    result = zoo_min_int(5, 10);
    TEST_ASSERT_EQUAL(5, result);
    
    result = zoo_max_int(5, 10);
    TEST_ASSERT_EQUAL(10, result);
    
    // Test clamping
    result = zoo_clamp_int(15, 5, 10);
    TEST_ASSERT_EQUAL(10, result);
    
    result = zoo_clamp_int(3, 5, 10);
    TEST_ASSERT_EQUAL(5, result);
    
    result = zoo_clamp_int(7, 5, 10);
    TEST_ASSERT_EQUAL(7, result);
}

void test_zoo_float_math_functions(void)
{
    // Test min/max
    float result = zoo_min_float(3.14f, 2.71f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.71f, result);
    
    result = zoo_max_float(3.14f, 2.71f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.14f, result);
    
    // Test clamping
    result = zoo_clamp_float(15.0f, 5.0f, 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, result);
    
    result = zoo_clamp_float(3.0f, 5.0f, 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, result);
    
    result = zoo_clamp_float(7.0f, 5.0f, 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, result);
}

void test_zoo_interpolation_functions(void)
{
    // Test linear interpolation
    float result = zoo_lerp_float(0.0f, 10.0f, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, result);
    
    result = zoo_lerp_float(0.0f, 10.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
    
    result = zoo_lerp_float(0.0f, 10.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, result);
    
    // Test value mapping
    result = zoo_map_float(5.0f, 0.0f, 10.0f, 0.0f, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, result);
}

void test_zoo_power_functions(void)
{
    // Test power of 2
    unsigned int result = zoo_power_of_2(3);
    TEST_ASSERT_EQUAL(8, result);
    
    result = zoo_power_of_2(0);
    TEST_ASSERT_EQUAL(1, result);
    
    // Test power of 2 checking
    ZOO_BOOL is_power = zoo_is_power_of_2(8);
    TEST_ASSERT_TRUE(is_power);
    
    is_power = zoo_is_power_of_2(9);
    TEST_ASSERT_FALSE(is_power);
    
    // Test next power of 2
    result = zoo_next_power_of_2(5);
    TEST_ASSERT_EQUAL(8, result);
    
    result = zoo_next_power_of_2(8);
    TEST_ASSERT_EQUAL(8, result);
}

void test_zoo_angle_conversion(void)
{
    // Test degree to radian conversion
    TEST_ASSERT_DOUBLE_WITHIN(0.001, M_PI, zoo_deg_to_rad(180.0));
    TEST_ASSERT_DOUBLE_WITHIN(0.001, M_PI / 2.0, zoo_deg_to_rad(90.0));

    // Test radian to degree conversion
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 180.0, zoo_rad_to_deg(M_PI));
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 90.0, zoo_rad_to_deg(M_PI / 2.0));
}

void test_zoo_integer_division(void)
{
    int quotient, remainder;
    
    // Test safe division
    int result = zoo_safe_div_int(10, 3, &quotient, &remainder);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_EQUAL(3, quotient);
    TEST_ASSERT_EQUAL(1, remainder);
    
    // Test division by zero
    result = zoo_safe_div_int(10, 0, &quotient, &remainder);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_zoo_gcd_lcm_functions(void)
{
    // Test greatest common divisor
    unsigned int result = zoo_gcd(12, 18);
    TEST_ASSERT_EQUAL(6, result);
    
    result = zoo_gcd(17, 19);
    TEST_ASSERT_EQUAL(1, result);
    
    // Test least common multiple
    result = zoo_lcm(4, 6);
    TEST_ASSERT_EQUAL(12, result);
    
    result = zoo_lcm(7, 5);
    TEST_ASSERT_EQUAL(35, result);
}

int main_math_tests(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_zoo_float_comparison_functions);
    RUN_TEST(test_zoo_double_comparison_functions);
    RUN_TEST(test_zoo_float_special_values);
    RUN_TEST(test_zoo_integer_math_functions);
    RUN_TEST(test_zoo_float_math_functions);
    RUN_TEST(test_zoo_interpolation_functions);
    RUN_TEST(test_zoo_power_functions);
    RUN_TEST(test_zoo_angle_conversion);
    RUN_TEST(test_zoo_integer_division);
    RUN_TEST(test_zoo_gcd_lcm_functions);
    
    return UNITY_END();
}

int main(void)
{
    return main_math_tests();
}
