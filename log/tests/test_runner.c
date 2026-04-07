#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

// Test function declarations
void test_log_initialization_and_shutdown(void);
void test_log_level_setting(void);
void test_log_target_setting(void);
void test_log_basic_logging(void);
void test_log_formatted_messages(void);
void test_log_level_filtering(void);
void test_log_config_functions(void);
void test_log_error_codes(void);
void test_log_thread_safety(void);
void test_log_performance(void);

// Platform detection
#if defined(__x86_64__) || defined(_M_X64)
#define PLATFORM_STRING "X86_64"
#elif defined(__i386) || defined(_M_IX86)
#define PLATFORM_STRING "X86"
#elif defined(__aarch64__)
#define PLATFORM_STRING "ARM64"
#elif defined(__arm__)
#define PLATFORM_STRING "ARM"
#else
#define PLATFORM_STRING "UNKNOWN"
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
static const char *get_platform_name(void)
{
#if defined(__x86_64__) || defined(__amd64__)
    return "X86_64";
#elif defined(__i386__)
    return "X86";
#elif defined(__aarch64__)
    return "ARM64";
#elif defined(__arm__)
    return "ARM";
#else
    return "Unknown";
#endif
}

static const char *get_os_name(void)
{
#if defined(__linux__)
    return "Linux";
#elif defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown";
#endif
}

int main(void)
{
    printf("=== ZOO Log Test Suite ===\n");
    printf("Platform: %s\n", get_platform_name());
    printf("OS: %s\n", get_os_name());
    printf("Unity Framework Version: %d.%d.%d\n",
           UNITY_VERSION_MAJOR, UNITY_VERSION_MINOR, UNITY_VERSION_BUILD);
    printf("\n");

    UNITY_BEGIN();

    printf("=== Running Core Log Tests ===\n");
    RUN_TEST(test_log_initialization_and_shutdown);
    RUN_TEST(test_log_level_setting);
    RUN_TEST(test_log_target_setting);
    RUN_TEST(test_log_basic_logging);
    RUN_TEST(test_log_formatted_messages);
    RUN_TEST(test_log_level_filtering);
    RUN_TEST(test_log_config_functions);
    RUN_TEST(test_log_error_codes);

    printf("\n=== Running Thread Safety Tests ===\n");
    RUN_TEST(test_log_thread_safety);

    printf("\n=== Running Performance Tests ===\n");
    RUN_TEST(test_log_performance);

    printf("\n=== Test Summary ===\n");
    return UNITY_END();
}
