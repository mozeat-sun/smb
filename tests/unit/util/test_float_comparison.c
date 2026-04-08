#include "../platform/inc/zoo.h"
#include "inc/zoo_util.h"
#include <stdio.h>

int main() {
    // 测试浮点比较函数
    float f1 = 1.000001f;
    float f2 = 1.000000f;
    float f3 = 1.00001f;
    
    printf("Float comparison tests:\n");
    printf("f1 = %f, f2 = %f, f3 = %f\n", f1, f2, f3);
    printf("f1 == f2 (should be true): %s\n", zoo_float_equals(f1, f2) ? "true" : "false");
    printf("f1 == f3 (should be false): %s\n", zoo_float_equals(f1, f3) ? "true" : "false");
    printf("f1 < f3 (should be true): %s\n", zoo_float_less_than(f1, f3) ? "true" : "false");
    printf("f3 > f1 (should be true): %s\n", zoo_float_greater_than(f3, f1) ? "true" : "false");
    
    // 测试double比较
    double d1 = 2.0000001;  // 差异在10e-7范围内
    double d2 = 2.0000000;
    double d3 = 2.00001;    // 差异在10e-5范围内，应该不相等
    
    printf("\nDouble comparison tests:\n");
    printf("d1 = %.10f, d2 = %.10f, d3 = %.10f\n", d1, d2, d3);
    printf("d1 - d2 = %.10e\n", d1 - d2);
    printf("d1 - d3 = %.10e\n", d1 - d3);
    printf("d1 == d2 (should be true): %s\n", zoo_double_equals(d1, d2) ? "true" : "false");
    printf("d1 == d3 (should be false): %s\n", zoo_double_equals(d1, d3) ? "true" : "false");
    
    // 测试零值比较
    printf("\nZero comparison tests:\n");
    printf("0.0000001f is zero (should be true): %s\n", zoo_float_is_zero(0.0000001f) ? "true" : "false");
    printf("0.00001f is zero (should be false): %s\n", zoo_float_is_zero(0.00001f) ? "true" : "false");
    
    // 测试绝对值和限制函数
    printf("\nUtility function tests:\n");
    printf("abs(-3.14f) = %f\n", zoo_float_abs(-3.14f));
    printf("clamp(5.0f, 1.0f, 4.0f) = %f\n", zoo_float_clamp(5.0f, 1.0f, 4.0f));
    printf("clamp(-1.0f, 1.0f, 4.0f) = %f\n", zoo_float_clamp(-1.0f, 1.0f, 4.0f));
    
    return 0;
}
