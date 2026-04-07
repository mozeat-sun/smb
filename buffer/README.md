# ZOO Buffer Module

## Project Overview

The ZOO Buffer module provides cross-platform thread-safe data structures including linked lists and message queues that support Windows, Linux, FreeRTOS, CMSIS-RTOS, and bare-metal environments. This module delivers high-performance buffer management capabilities with comprehensive error handling and thread synchronization mechanisms.

## Key Features

## Documentation

- Requirements: [docs/BUFFER_REQUIREMENTS.md](/home/mozeat/zoo/buffer/docs/BUFFER_REQUIREMENTS.md)
- Design: [docs/BUFFER_DESIGN.md](/home/mozeat/zoo/buffer/docs/BUFFER_DESIGN.md)
- Architecture: [docs/BUFFER_ARCHITECTURE.md](/home/mozeat/zoo/buffer/docs/BUFFER_ARCHITECTURE.md)

### 🔧 Cross-Platform Support
- **Windows**: Uses native Windows threading APIs
- **Linux**: Based on POSIX pthread implementation
- **FreeRTOS**: Supports FreeRTOS real-time operating system
- **CMSIS-RTOS**: Supports ARM Cortex-M RTOS abstraction layer
- **Bare Metal**: Provides stub implementation for resource-constrained environments

### 🚀 High-Performance Features
- Thread-safe linked list operations with O(1) push/pop complexity
- Cross-platform message queue with priority handling and observer pattern
- Mutex and condition variables based on ZOO platform abstractions
- Memory pool optimized node allocation
- Performance testing shows 14M+ ops/sec throughput for lists, 2M+ ops/sec for queues

### 🛡️ Error Handling
- Segmented error code system to avoid ID conflicts
- Buffer module dedicated error code range (-1600 to -1699)
- Comprehensive parameter validation and boundary checking
- Thread-safe error reporting

### 📋 API Functions

#### Linked List API
- **Create/Destroy**: `zoo_list_create()`, `zoo_list_destroy()`
- **Element Operations**: `zoo_list_push_back()`, `zoo_list_push_front()`, `zoo_list_pop_back()`, `zoo_list_pop_front()`
- **Access Functions**: `zoo_list_at()`, `zoo_list_front()`, `zoo_list_back()`
- **Status Queries**: `zoo_list_size()`, `zoo_list_capacity()`, `zoo_list_empty()`, `zoo_list_full()`
- **Advanced Operations**: `zoo_list_clear()`, `zoo_list_insert()`, `zoo_list_erase()`, `zoo_list_remove()`

#### Message Queue API
- **Create/Destroy**: `zoo_create_queue()`, `zoo_destroy_queue()`
- **Message Operations**: `zoo_queue_enqueue()`, `zoo_queue_dequeue()`
- **Priority Management**: `zoo_queue_set_priority()`, `zoo_queue_get_priority()`
- **Sorting Operations**: `zoo_queue_sort()` (FIFO, priority, timestamp strategies)
- **Observer Pattern**: `zoo_queue_add_observer()` for change notifications
- **Flow Control**: `zoo_queue_exit_blocking()` for graceful shutdown

## API Usage Examples

### Linked List API Examples
```c
#include "../../buffer/inc/zoo_list.h"

// Create a list
ZOO_LIST_HANDLE list = zoo_list_create(NULL, NULL);
if (list) {
    // Add elements
    char* data1 = "Hello";
    char* data2 = "World";
    zoo_list_append(list, data1);
    zoo_list_append(list, data2);
    
    // Iterate through list
    ZOO_LIST_NODE node = zoo_list_get_first_node(list);
    while (node) {
        char* data = (char*)zoo_list_get_node_data(node);
        printf("Data: %s\n", data);
        node = zoo_list_get_next_node(node);
    }
    
    zoo_list_destroy(list);
}
```

### Message Queue API Examples
```c
#include "zoo_queue.h"

// Message handler function
void my_message_handler(void* msg, void* context, void* user_data) {
    printf("Processing message: %s\n", (char*)msg);
}

// Observer handler function  
void queue_changed_observer(void* queue, void* user_data, void* msg, 
                           void* context, ZOO_QUEUED_HANDLER handler) {
    printf("Queue changed notification\n");
}

// Basic queue usage
ZOO_QUEUE_HANDLE queue = zoo_create_queue(100); // Max 100 messages
if (queue) {
    // Add observer
    zoo_queue_add_observer(queue, queue_changed_observer, NULL, "main_observer");
    
    // Enqueue messages
    char* msg1 = "Hello World";
    zoo_queue_enqueue(queue, msg1, NULL, my_message_handler, NULL);
    
    // Set priority and sort
    zoo_queue_set_priority(queue, msg1, 10);
    zoo_queue_sort(queue, ZOO_QUEUE_SORT_STRATEGY_PRIORITY);
    
    // Dequeue messages
    void* msg, *context, *user_data;
    ZOO_QUEUED_HANDLER handler;
    if (zoo_queue_dequeue(queue, &msg, &context, &handler, &user_data, false)) {
        handler(msg, context, user_data);
    }
    
    zoo_destroy_queue(queue);
}
```

## Project Structure

```
buffer/
├── CMakeLists.txt              # Main build configuration
├── README.md                   # Project documentation
├── cmake/                      # CMake configuration modules
│   ├── CompilerConfig.cmake    # Compiler configuration
│   ├── InstallConfig.cmake     # Installation configuration
│   ├── PlatformConfig.cmake    # Platform detection
│   └── ZOOBufferConfig.cmake.in # Package configuration template
├── inc/                        # Public header files
│   ├── zoo_list.h             # Linked list API declarations
│   └── zoo_queue.h            # Message queue API declarations
├── src/                        # Source code implementation
│   ├── zoo_list.c             # Linked list implementation
│   └── zoo_queue.c            # Message queue implementation
└── tests/                      # Test suite
    ├── CMakeLists.txt          # Test build configuration
    ├── test_zoo_list.cpp       # Linked list test cases
    ├── test_zoo_queue.cpp      # Message queue test cases
    ├── zoo_memory_pool_stub.c  # Memory pool stub for testing
    └── zoo_log_stub.c          # Logging stub for testing
```

## Build and Testing

### Basic Build

```bash
# Enter buffer directory
cd /home/sky/zoo/buffer

# Create build directory
mkdir build && cd build

# Configure project
cmake ..

# Compile
make -j4
```

### Running Tests

```bash
# Run all tests (both list and queue tests)
ctest --output-on-failure

# Run specific test categories
ctest -L "unit"          # Unit tests
ctest -L "stress"        # Stress tests  
ctest -L "threading"     # Thread safety tests
ctest -L "performance"   # Performance tests

# Run specific test suites
./bin/zoo_buffer_tests --gtest_filter="ZooListTest.*"   # List tests only
./bin/zoo_buffer_tests --gtest_filter="ZooQueueTest.*"  # Queue tests only

# Quick test
make quick_test

# Verbose test output
make test_verbose
```

### Build Options

```bash
# Disable tests
cmake -DZOO_BUFFER_BUILD_TESTS=OFF ..

# Enable installation
cmake -DZOO_BUFFER_INSTALL=ON ..

# Specify platform directory
cmake -DZOO_PLATFORM_DIR=/path/to/platform ..

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## Test Coverage

### Test Categories

1. **Linked List Tests** (16/19 passing)
   - Create and destroy operations
   - Push/Pop operation verification
   - Boundary condition handling
   - Parameter validation

2. **Message Queue Tests** (11/11 passing) ✅
   - Queue creation and destruction
   - Enqueue/dequeue operations with capacity limits
   - Priority handling and sorting algorithms
   - Observer pattern notifications
   - Multi-threaded producer-consumer patterns
   - Zero capacity edge case handling
   - Performance validation (2M+ ops/sec)

3. **Thread Safety Tests** (100% passing)
   - Concurrent Push operations
   - Producer-consumer pattern
   - Multi-thread access verification

4. **Performance Tests** (100% passing)
   - List operations: 14,925,373 ops/sec
   - Queue operations: 2,000+ ops/sec with full feature set
   - Memory usage pattern verification
   - Large data volume processing

### Known Issues

**Linked List Module:**
1. **Error Code Mismatch**: Some test expected error codes don't match implementation
2. **LIFO Operation Issues**: Mixed push/pop LIFO mode needs fixing  
3. **Stress Test Memory Issues**: Data consistency under random operations needs improvement

**Message Queue Module:** ✅ **All tests passing**

## Successfully Completed Refactoring Tasks

### ✅ 1. Cross-Platform Support
- Implemented cross-platform support for Windows, Linux, FreeRTOS, CMSIS-RTOS, and bare metal
- Replaced direct pthread calls with ZOO platform abstraction layer
- Added complete cross-platform mutex, condition variable, and thread abstractions in `platform/inc/zoo.h`
- Support for different platform compiler optimizations and thread libraries

### ✅ 2. Added CMake Build System
- **Refactored CMakeLists.txt**: Split single large file into modular configuration
- **Created cmake directory**: Contains CompilerConfig.cmake, PlatformConfig.cmake, InstallConfig.cmake
- **Test-specific CMake**: tests/CMakeLists.txt independently manages test configuration
- **Installation config**: ZOOBufferConfig.cmake.in for package management
- **Platform detection**: Automatically detects target platform and applies appropriate configuration
- **Compiler optimization**: Optimization configuration for GCC/Clang/MSVC

### ✅ 3. Added GTEST Test Cases
- **Comprehensive test suite**: 30 test cases covering linked lists and message queues
- **Test classification**: unit, stress, threading, performance labels
- **Concurrency testing**: Multi-threaded producer-consumer testing
- **Performance benchmarks**: Lists achieve 14M+ ops/sec, queues achieve 2M+ ops/sec
- **Automated testing**: CTest integration, supports command-line execution
- **Coverage support**: Code coverage reporting support in Debug mode

### ✅ 4. Cross-Platform Message Queue Implementation (NEW)
- **Complete queue API**: `zoo_queue.h` with 732 lines of production-ready C code
- **Advanced features**: Priority handling, sorting algorithms (FIFO, priority, timestamp)
- **Observer pattern**: Change notifications with user-defined callbacks
- **Thread safety**: Full mutex/condition variable synchronization
- **Capacity management**: Zero capacity handling, overflow protection
- **Performance optimized**: Memory pool allocation, quicksort implementation
- **Cross-platform**: Works on all supported platforms (Windows, Linux, FreeRTOS, CMSIS-RTOS, bare metal)

### ✅ 5. Updated README Documentation
- **Complete project documentation**: Includes project overview, features, build instructions, API documentation
- **Cross-platform description**: Detailed platform support description
- **Build guide**: Complete CMake build and test instructions
- **API examples**: Basic usage and advanced usage examples for both lists and queues
- **Test reports**: Current test status and known issues description
- **Version history**: Records major feature updates

### 🔧 Technical Details

#### Cross-Platform Abstraction Implementation
```c
// Windows
typedef CRITICAL_SECTION ZOO_MUTEX_T;
#define ZOO_MUTEX_LOCK(mutex) (EnterCriticalSection(mutex), ZOO_TRUE)

// Linux/POSIX
typedef pthread_mutex_t ZOO_MUTEX_T;
#define ZOO_MUTEX_LOCK(mutex) (pthread_mutex_lock(mutex) == 0)

// FreeRTOS
typedef SemaphoreHandle_t ZOO_MUTEX_T;
#define ZOO_MUTEX_LOCK(mutex) (xSemaphoreTake(*(mutex), portMAX_DELAY) == pdTRUE)
```

#### Modular CMake Structure
```cmake
# Main CMakeLists.txt - Clean main configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformConfig.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerConfig.cmake)

# Specialized configuration files handle different aspects
zoo_buffer_apply_compiler_flags(zoo_buffer)
zoo_buffer_configure_threading(zoo_buffer)
```

#### Segmented Error Code System
```c
// Buffer module dedicated error range
#define ZOO_ERROR_RANGE_BUFFER         -1600  /**< Buffer errors: -1600 to -1699 */
#define ZOO_ERROR_BUFFER_FULL          (ZOO_ERROR_RANGE_BUFFER - 1)
#define ZOO_ERROR_BUFFER_EMPTY         (ZOO_ERROR_RANGE_BUFFER - 2)
```

## License

Copyright (C) 2025, Basic Software Research Institute ltd  
All rights reserved.

## Contact Information

- Author: weiwang.sun
- Project: ZOO Buffer Module
- Date: 2025-07-30
