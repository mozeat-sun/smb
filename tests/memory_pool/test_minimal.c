#include "unity.h"
#include "zoo_memory_pool.h"

void setUp(void) {
}

void tearDown(void) {
    zoo_destroy_memory_pool();
}

void test_simple_creation(void) {
    // Just try to create and destroy
    int result = zoo_create_memory_pool(1024 * 1024);
    TEST_ASSERT_EQUAL_INT(0, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_simple_creation);
    return UNITY_END();
}
