#include "unity.h"
#include "zoo_util.h"

// Forward declarations for all test functions
extern int main_timestamp_tests(void);
extern int main_string_tests(void);
extern int main_memory_tests(void);
extern int main_math_tests(void);

void setUp(void)
{
    // Global setup if needed
}

void tearDown(void)
{
    // Global teardown if needed
}

void test_zoo_util_initialization(void)
{
    // Test utility module initialization
    ZOO_ERROR_T result = zoo_util_initialize();
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    
    // Test finalization
    result = zoo_util_finalize();
    TEST_ASSERT_EQUAL(ZOO_OK, result);
}

int main(void)
{
    UNITY_BEGIN();
    
    // Run basic initialization test
    RUN_TEST(test_zoo_util_initialization);
    
    printf("\n=== Running ZOO Util Unity Tests ===\n\n");
    
    // Run all module tests
    printf("Running Timestamp Tests...\n");
    int timestamp_result = main_timestamp_tests();
    
    printf("Running String Tests...\n");
    int string_result = main_string_tests();
    
    printf("Running Memory Tests...\n");
    int memory_result = main_memory_tests();
    
    printf("Running Math Tests...\n");
    int math_result = main_math_tests();
    
    // Print summary
    printf("\n=== Test Results Summary ===\n");
    printf("Timestamp Tests: %s\n", timestamp_result == 0 ? "PASSED" : "FAILED");
    printf("String Tests: %s\n", string_result == 0 ? "PASSED" : "FAILED");
    printf("Memory Tests: %s\n", memory_result == 0 ? "PASSED" : "FAILED");
    printf("Math Tests: %s\n", math_result == 0 ? "PASSED" : "FAILED");
    
    int overall_result = UNITY_END();
    
    // Return non-zero if any test suite failed
    if (timestamp_result != 0 || string_result != 0 || 
        memory_result != 0 || math_result != 0 || overall_result != 0)
    {
        return 1;
    }
    
    return 0;
}
