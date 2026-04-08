#include "unity.h"
#include "cmock.h"  // Add CMock support
#include <stdio.h>

// Platform detection
#if defined(__x86_64__) || defined(__amd64__)
    #define PLATFORM_STRING "X86_64"
#elif defined(__i386__)
    #define PLATFORM_STRING "X86"
#elif defined(__aarch64__)
    #define PLATFORM_STRING "ARM64"
#elif defined(__arm__)
    #define PLATFORM_STRING "ARM"
#else
    #define PLATFORM_STRING "Unknown"
#endif

#ifdef __linux__
    #define OS_STRING "Linux"
#elif defined(_WIN32)
    #define OS_STRING "Windows"
#elif defined(__APPLE__)
    #define OS_STRING "macOS"
#else
    #define OS_STRING "Unknown"
#endif

// Test runner setup and teardown
void setUp(void);
void tearDown(void);

// Platform detection helpers
const char* get_platform_name(void) {
    return PLATFORM_STRING;
}

const char* get_os_name(void) {
    return OS_STRING;
}

// Test function declarations
void test_memory_pool_creation_and_destruction(void);
void test_basic_allocation_and_free(void);
void test_memory_write_and_read(void);
void test_zero_size_allocation(void);
void test_null_pointer_free(void);
void test_memory_usage_statistics(void);
void test_multiple_small_allocations(void);
void test_memory_pool_validation(void);
void test_basic_thread_safety(void);
void test_allocation_patterns(void);
void test_final_cleanup(void);

int main(void) {
    printf("=== ZOO Memory Pool Test Suite ===\n");
    printf("Platform: %s\n", get_platform_name());
    printf("OS: %s\n", get_os_name());
    printf("Unity Framework Version: %d.%d.%d\n", 
           UNITY_VERSION_MAJOR, UNITY_VERSION_MINOR, UNITY_VERSION_BUILD);
    printf("CMock Support: Available\n");
    printf("\n");
    
    UNITY_BEGIN();
    
    printf("=== Running Core Memory Pool Tests ===\n");
    RUN_TEST(test_memory_pool_creation_and_destruction);
    RUN_TEST(test_basic_allocation_and_free);
    RUN_TEST(test_memory_write_and_read);
    RUN_TEST(test_zero_size_allocation);
    RUN_TEST(test_null_pointer_free);
    RUN_TEST(test_memory_usage_statistics);
    RUN_TEST(test_multiple_small_allocations);
    RUN_TEST(test_memory_pool_validation);
    
    printf("\n=== Running Advanced Tests ===\n");
    RUN_TEST(test_allocation_patterns);
    
    printf("\n=== Running Thread Safety Tests ===\n");
    RUN_TEST(test_basic_thread_safety);
    
    printf("\n=== Final Cleanup ===\n");
    RUN_TEST(test_final_cleanup);
    
    printf("\n=== Test Summary ===\n");
    return UNITY_END();
}
