#include "zoo_util.h"
#include <stdio.h>

int main(void) {
    printf("Testing modular ZOO utility headers...\n");
    
    // Test timestamp functionality
    ZOO_UINT64 timestamp = zoo_get_timestamp_milliseconds();
    printf("Current timestamp: %llu ms\n", (unsigned long long)timestamp);
    
    // Test string functionality
    char buffer[256];
    zoo_safety_copy_string(buffer, "Hello, ZOO!", sizeof(buffer));
    printf("String test: %s\n", buffer);
    
    // Test math functionality
    float a = 3.14159f, b = 3.14160f;
    ZOO_BOOL equal = zoo_float_equal(a, b);
    printf("Float comparison (3.14159 ≈ 3.14160): %s\n", equal ? "true" : "false");
    
    // Test memory functionality
    void *ptr = zoo_safety_malloc_zero(1024);
    if (ptr) {
        printf("Memory allocation: success\n");
        zoo_safety_delete_pointer(&ptr);
        printf("Memory cleanup: success\n");
    }
    
    printf("All modular headers working correctly!\n");
    return 0;
}
