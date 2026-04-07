#include "unity.h"
#include "zoo_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Test configuration
static const char *test_log_file = "/tmp/zoo_log_test.log";

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

// Helper functions
static void cleanup_test_files(void)
{
    unlink(test_log_file);
}

// Unity setup and teardown
void setUp(void)
{
    cleanup_test_files();
    zoo_log_shutdown();
}

void tearDown(void)
{
    zoo_log_shutdown();
    cleanup_test_files();
}

// ============================================================================
// LOG CORE TESTS
// ============================================================================

void test_log_initialization_and_shutdown(void)
{
    // Test successful initialization with NULL config (uses defaults)
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    // Test shutdown
    zoo_log_shutdown();

    // Test shutdown when not initialized (should be safe)
    zoo_log_shutdown();

    // Test initialization with specific config
    ZOO_LOG_CONFIG_STRUCT config = ZOO_LOG_CONFIG_STRUCT_DEFAULT;
    config.level = ZOO_LOG_LEVEL_DEBUG;
    config.target = ZOO_LOG_TARGET_CONSOLE;

    result = zoo_log_init(&config);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
}

void test_log_level_setting(void)
{
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    // Test setting valid log levels
    zoo_log_set_level(ZOO_LOG_LEVEL_DEBUG);
    zoo_log_set_level(ZOO_LOG_LEVEL_INFO);
    zoo_log_set_level(ZOO_LOG_LEVEL_ERROR);

    // Note: zoo_log_set_level is void, so we can't check return values
    // We just verify it doesn't crash
}

void test_log_target_setting(void)
{
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    // Test setting different targets
    zoo_log_set_target(ZOO_LOG_TARGET_CONSOLE);
    zoo_log_set_target(ZOO_LOG_TARGET_SYSLOG);
    zoo_log_set_target(ZOO_LOG_TARGET_BOTH);

    // Note: zoo_log_set_target is void, so we can't check return values
    // We just verify it doesn't crash
}

void test_log_basic_logging(void)
{
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    zoo_log_set_level(ZOO_LOG_LEVEL_DEBUG);

    // Test different log levels using the convenience macros
    ZOO_LOG_DEBUG("%s", "Test debug message");
    ZOO_LOG_INFO("%s", "Test info message");
    ZOO_LOG_WARN("%s", "Test warning message");
    ZOO_LOG_ERROR("%s", "Test error message");

    // Test direct function calls
    zoo_log_debug(__FILE__, __LINE__, __func__, "Direct debug call");
    zoo_log_info(__FILE__, __LINE__, __func__, "Direct info call");
    zoo_log_warn(__FILE__, __LINE__, __func__, "Direct warn call");
    zoo_log_error(__FILE__, __LINE__, __func__, "Direct error call");

    // Note: Since we're using console output, we can't easily verify
    // the content, but we ensure the functions don't crash
}

void test_log_formatted_messages(void)
{
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    zoo_log_set_level(ZOO_LOG_LEVEL_DEBUG);

    // Test formatted logging
    int test_value = 42;
    const char *test_string = "test_string";

    ZOO_LOG_INFO("Formatted message: value=%d, string=%s", test_value, test_string);
    zoo_log_info(__FILE__, __LINE__, __func__, "Direct formatted: %d %s", test_value, test_string);

    // Test with various format specifiers
    ZOO_LOG_DEBUG("Multiple formats: int=%d, hex=0x%x, float=%.2f, char=%c",
                  42, 255, 3.14f, 'A');
}

void test_log_level_filtering(void)
{
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    // Set log level to INFO (should filter out DEBUG messages)
    zoo_log_set_level(ZOO_LOG_LEVEL_INFO);

    ZOO_LOG_DEBUG("%s", "This debug message should be filtered");
    ZOO_LOG_INFO("%s", "This info message should appear");
    ZOO_LOG_ERROR("%s", "This error message should appear");

    // Set log level to ERROR (should filter out DEBUG and INFO)
    zoo_log_set_level(ZOO_LOG_LEVEL_ERROR);

    ZOO_LOG_DEBUG("%s", "This debug message should be filtered");
    ZOO_LOG_INFO("%s", "This info message should be filtered");
    ZOO_LOG_ERROR("%s", "This error message should appear");
}

void test_log_config_functions(void)
{
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    // Test getting config
    const ZOO_LOG_CONFIG_STRUCT *config = zoo_log_get_config();
    TEST_ASSERT_NOT_NULL(config);

    // Test setting new config
    ZOO_LOG_CONFIG_STRUCT new_config = ZOO_LOG_CONFIG_STRUCT_DEFAULT;
    new_config.level = ZOO_LOG_LEVEL_WARN;
    new_config.target = ZOO_LOG_TARGET_CONSOLE;

    zoo_log_set_config(&new_config);

    // Verify config was applied by testing log level behavior
    ZOO_LOG_INFO("%s", "This info message should be filtered with WARN level");
    ZOO_LOG_WARN("%s", "This warn message should appear");
}

void test_log_error_codes(void)
{
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    zoo_log_set_level(ZOO_LOG_LEVEL_ERROR);

    // Test error code logging functions
    zoo_log_error_code(-1, "Error with code: %s", "INVALID_PARAM");
    zoo_log_fatal_code(-2, "Fatal error with code: %s", "OUT_OF_MEMORY");

    // These functions should not crash
}

// ============================================================================
// LOG THREADING TESTS
// ============================================================================

typedef struct
{
    int thread_id;
    int message_count;
} thread_data_t;

static void *log_thread_worker(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;

    for (int i = 0; i < data->message_count; i++)
    {
        ZOO_LOG_INFO("Thread %d message %d", data->thread_id, i);
        // Small delay to increase chance of thread interleaving
        usleep(100);
    }

    return NULL;
}

void test_log_thread_safety(void)
{
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    zoo_log_set_level(ZOO_LOG_LEVEL_INFO);

    const int num_threads = 4;
    const int messages_per_thread = 10;
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    // Create threads
    for (int i = 0; i < num_threads; i++)
    {
        thread_data[i].thread_id = i;
        thread_data[i].message_count = messages_per_thread;

        int ret = pthread_create(&threads[i], NULL, log_thread_worker, &thread_data[i]);
        TEST_ASSERT_EQUAL(0, ret);
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Note: We can't easily verify message content with console output,
    // but the test passes if no crashes occur during concurrent logging
}

// ============================================================================
// LOG PERFORMANCE TESTS
// ============================================================================

void test_log_performance(void)
{
    ZOO_INT32 result = zoo_log_init(NULL);
    TEST_ASSERT_EQUAL(ZOO_OK, result);

    zoo_log_set_level(ZOO_LOG_LEVEL_INFO);

    const int num_messages = 1000;

    // Log many messages to test performance
    for (int i = 0; i < num_messages; i++)
    {
        ZOO_LOG_INFO("Performance test message %d", i);
    }

    // Test should complete without hanging or crashing
}
