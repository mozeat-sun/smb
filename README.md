# ZOO Platform

ZOO is a modular systems-software platform written primarily in C. It provides a shared foundation for cross-platform runtime services such as type abstraction, utilities, buffering, dispatching, logging, memory management, timers, thread pools, socket communication, and the Soft Message Bus.

The repository is organized as one platform with multiple focused submodules. Most users should think of ZOO as a toolbox of interoperable building blocks rather than a single standalone library.

## What ZOO Provides

- A cross-platform base layer for common types, errors, and portability helpers.
- Reusable infrastructure components for queues, timers, thread execution, sockets, logging, and memory handling.
- A higher-level messaging stack through SMB for request/reply and publish/subscribe communication.
- A single-repository layout so modules can evolve together and be built or tested consistently.

## Submodule Overview

- `platform`: Core platform abstraction layer that defines shared types, error codes, portability macros, and low-level cross-platform support.
- `buffer`: Thread-safe data structures such as linked lists and message queues for producer/consumer and buffering workflows.
- `dispatcher`: Asynchronous message dispatching built on queues, including background processing and message ordering strategies.
- `log`: Configurable logging infrastructure with multiple log levels, formatting options, and platform-specific output backends.
- `memory_pool`: Slab-style memory pool for fast, predictable allocation patterns with statistics and validation support.
- `smb`: Soft Message Bus layer that exposes node-oriented messaging APIs such as server, client, publisher, and subscriber.
- `socket`: Cross-platform socket abstraction for TCP, UDP, address helpers, and transport-oriented networking code.
- `thread_pool`: Worker-thread execution framework for queuing and running asynchronous tasks with pool status tracking.
- `timer`: High-precision timer utilities for one-shot and repeating callback-based timing workflows.
- `util`: Shared utility helpers for timestamps, strings, memory helpers, and general-purpose support functions.
- `dockfile`: Container- or environment-related supporting assets for the repository.

## Supporting Directories

- `docs`: Repository-level specifications, architecture notes, and module design documents.
- `third_party`: Vendored testing frameworks and related external assets.
- `cmake`: Shared build helpers used by the repository, kept secondary to the platform and module documentation.

## Strategic Roadmap

- Grade roadmap for embedded industry progression: `docs/GRADE_ROADMAP.md`

## Repository Model

- ZOO is maintained as a single Git repository.
- Module-local repositories should not be recreated.
- Root-level commands remain the default entry point for repository-wide work.

## Quick Start

```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

## Open Source Project Files

This repository is open source and includes standard community and governance files at the root:

- `LICENSE` for project licensing
- `CONTRIBUTING.md` for contribution workflow
- `CODE_OF_CONDUCT.md` for community expectations
- `SECURITY.md` for private vulnerability reporting
- `CHANGELOG.md` for release history

---

## Consolidated Module References

The sections below mirror each top-level module README for one-stop project documentation.

### Module: platform

Source: (module README in this section)
# ZOO Platform (ZOS)

**Version**: 1.2.0  
**Author**: weiwang.sun  
**Company**: ZOO Ltd.  

## Overview

ZOO Platform (ZOS) is a cross-platform C/C++ abstraction layer library that provides unified programming interfaces for embedded and desktop systems across different architectures. This platform is specifically designed for ZOO SMB (Soft Message Bus) and supports multiple architectures including ARM Cortex-M/A and x86/x64.

## Features

### 🔧 Cross-Platform Support
- **ARM Cortex-M Series**: ARM Cortex-M0/M3/M4/M7 microcontrollers
- **ARM Cortex-A Series**: ARM Cortex-A55/A72/A78 application processors (32-bit/64-bit)
- **x86 Architecture**: Intel/AMD x86 (32-bit) and x86-64 (64-bit)

### 📊 Unified Type System
- Standardized integer types with clean naming (`ZOO_INT8`, `ZOO_UINT32`, `ZOO_SIZE`, etc.)
- Platform-dependent pointer and size types (`ZOO_SIZE`, `ZOO_UINTPTR`)
- Automatic 64-bit/32-bit architecture adaptation

### ⚡ High-Performance Utility Functions
- High-precision timestamp support (seconds/milliseconds/microseconds/nanoseconds)
- Inline-optimized memory management functions
- Safe string manipulation functions
- Compiler optimization attribute support

### 🛡️ Memory Safety
- Buffer overflow protection
- NULL pointer checking
- Safe memory allocation and deallocation
- Memory leak detection in debug mode

## Project Structure

```
zos/
├── zoo.h           # Core platform definitions and type system
├── zoo_platform.h # Platform-specific definitions
├── zoo_util.h      # Utility functions and macros
└── README.md       # Project documentation
```

## Supported Platforms

| Platform | Architecture | Bits | Cache Line Size | Status |
|----------|--------------|------|-----------------|--------|
| ARM Cortex-M | ARM | 32-bit | 32 bytes | ✅ Supported |
| ARM Cortex-A | ARM | 32-bit | 64 bytes | ✅ Supported |
| ARM Cortex-A | ARM | 64-bit | 64 bytes | ✅ Supported |
| x86 | x86 | 32-bit | 64 bytes | ✅ Supported |
| x86-64 | x86 | 64-bit | 64 bytes | ✅ Supported |

## Quick Start

### Basic Usage

```c
#include "zoo.h"
#include "zoo_util.h"

int main() {
    // Get platform information
    printf("Platform: %s\n", ZOO_GET_PLATFORM_INFO());
    printf("Version: %s\n", ZOO_GET_PLATFORM_VERSION());
    printf("64-bit: %s\n", ZOO_IS_64BIT() ? "Yes" : "No");
    
    // Get high-precision timestamp with clean type naming
    ZOO_UINT64 timestamp = zoo_get_timestamp_milliseconds();
    printf("Current timestamp: %llu ms\n", timestamp);
    
    // Safe string operations
    char buffer[256];
    zoo_safety_copy_string(buffer, "Hello ZOO Platform!", sizeof(buffer));
    printf("Message: %s\n", buffer);
    
    return ZOO_OK;
}
```

### Compilation Examples

```bash
# GCC compilation (Linux/ARM)
gcc -I. -std=c99 -O2 your_program.c -o your_program

# Cross-compilation for ARM Cortex-M
arm-none-eabi-gcc -I. -mcpu=cortex-m4 -mthumb -O2 your_program.c -o your_program.elf

# MSVC compilation (Windows)
cl /I. /O2 your_program.c
```

## API Documentation

### Core Type Definitions

| Type | Description | Range |
|------|-------------|-------|
| `ZOO_INT8` | 8-bit signed integer | -128 ~ 127 |
| `ZOO_UINT32` | 32-bit unsigned integer | 0 ~ 4,294,967,295 |
| `ZOO_SIZE` | Platform-dependent size type | 32-bit or 64-bit |
| `ZOO_BOOL` | Boolean type | `ZOO_TRUE` / `ZOO_FALSE` |

### Timestamp Functions

```c
// Get timestamps with different precision
ZOO_UINT64 zoo_get_timestamp_seconds(void);
ZOO_UINT64 zoo_get_timestamp_milliseconds(void);
ZOO_UINT64 zoo_get_timestamp_microseconds(void);
ZOO_UINT64 zoo_get_timestamp_nanoseconds(void);

// Calculate time difference
ZOO_UINT64 zoo_calculate_elapsed_time(ZOO_UINT64 start, ZOO_UINT64 end);
```

### Safe Memory Management

```c
// Safe memory allocation
void* zoo_safety_malloc_zero(ZOO_SIZE size);
void* zoo_safety_calloc(ZOO_SIZE count, ZOO_SIZE element_size);

// Safe memory deallocation
void zoo_safety_delete_pointer(void** pointer_addr);

// Safe string operations
int zoo_safety_copy_string(char* dest, const char* src, ZOO_SIZE dest_size);
```

### Utility Macros

```c
// Array and structure operations
ZOO_ARRAY_SIZE(arr)              // Get number of array elements
ZOO_OFFSET_OF(type, member)      // Get structure member offset
ZOO_CONTAINER_OF(ptr, type, member)  // Get container pointer from member pointer

// Memory alignment
ZOO_ALIGN_UP(value, alignment)   // Align up
ZOO_ALIGN_DOWN(value, alignment) // Align down
ZOO_IS_ALIGNED(value, alignment) // Check alignment

// Numeric operations
ZOO_MIN(a, b)                    // Minimum value
ZOO_MAX(a, b)                    // Maximum value
ZOO_CLAMP(value, min, max)       // Clamp value to range
```

## Compiler Support

- **GCC**: Full support, recommended version 4.9+
- **Clang**: Full support, recommended version 3.5+
- **MSVC**: Supported, recommended version 2015+
- **ARM Compiler**: Cross-compilation support

## Development Guide

### Adding New Platform Support

1. Add platform detection macros in `zoo.h`
2. Define platform-specific cache line sizes
3. Implement platform-specific time functions in `zoo_util.h`
4. Update platform support list

### Debug Support

```c
// Enable memory debugging (define at compile time)
#define ZOO_DEBUG_MEMORY
#include "zoo_util.h"

// Use debug memory allocation
void* ptr = ZOO_DEBUG_MALLOC(1024);
ZOO_DEBUG_FREE(ptr);
```

## Performance Optimization

- All utility functions use `ZOO_FORCE_INLINE` for inline optimization
- Platform-specific cache line alignment support
- Compile-time static assertions ensure type safety
- Conditional compilation reduces unnecessary code

## Compatibility

- **C Standard**: C99 and above
- **C++ Standard**: C++11 and above  
- **RTOS**: FreeRTOS, RT-Thread, μC/OS, etc.
- **Operating Systems**: Linux, Windows, bare-metal environments

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.2.2 | 2025-08-07 | Simplified type system - removed _T suffix, single naming style |
| 1.2.1 | 2025-08-07 | Added typedef aliases without _T suffix for improved usability |
| 1.2.0 | 2025-05-14 | Initial version with multi-platform abstraction layer |

## License

This module content is distributed under the repository root license.

## Technical Support

- **Issue Reporting**: Use repository Issues
- **Security Reporting**: Follow instructions in `SECURITY.md`
- **Technical Discussion**: Use repository Discussions or Issues

## Contributing

1. Fork the project repository
2. Create a feature branch (`git checkout -b feature/new-feature`)
3. Commit your changes (`git commit -am 'Add new feature'`)
4. Push to the branch (`git push origin feature/new-feature`)
5. Create a Pull Request

---

*ZOO Platform - Providing unified abstraction layer for embedded and cross-platform development*

---

### Module: buffer

Source: (module README in this section)
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

---

### Module: dispatcher

Source: (module README in this section)
# dispatcher



## Getting started

To make it easy for you to get started with GitLab, here's a list of recommended next steps.

Already a pro? Just edit this README.md and make it your own. Want to make it easy? [Use the template at the bottom](#editing-this-readme)!

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/ee/gitlab-basics/add-file.html#add-a-file-using-the-command-line) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin https://git.nevint.com/PERD/Software_PlatForm/poweros/soa_platform/zoo/dispatcher.git
git branch -M master
git push -uf origin master
```

## Integrate with your tools

- [ ] [Set up project integrations](https://git.nevint.com/PERD/Software_PlatForm/poweros/soa_platform/zoo/dispatcher/-/settings/integrations)

## Collaborate with your team

- [ ] [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
- [ ] [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Set auto-merge](https://docs.gitlab.com/ee/user/project/merge_requests/merge_when_pipeline_succeeds.html)

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/index.html)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.

---

### Module: log

Source: (module README in this section)
# ZOO Log Module

A comprehensive and configurable logging module for the ZOO library that provides multi-level logging capabilities with various output targets and formatting options.

## Features

- **Multi-level logging**: Support for TRACE, DEBUG, INFO, WARN, ERROR, and FATAL levels
- **Multiple output targets**: Console, file, syslog (on supported platforms)
- **Thread-safe**: Built with thread safety in mind for multi-threaded applications
- **Configurable formatting**: Customizable log message formats with timestamps, thread IDs, and source location
- **Platform-agnostic**: Works across different operating systems with platform-specific optimizations
- **Performance optimized**: Minimal overhead with efficient buffering and async logging options
- **Runtime configuration**: Dynamic log level and target configuration

## Architecture

The ZOO Log module is designed with the following components:

- **Core logging engine**: Handles message formatting and routing
- **Output handlers**: Pluggable output targets (console, file, syslog)
- **Level management**: Configurable logging levels with filtering
- **Thread synchronization**: Thread-safe operations with minimal locking
- **Configuration management**: Runtime and compile-time configuration options

## Getting Started

### Prerequisites

- CMake 3.14 or higher
- C11 compatible compiler
- ZOO platform module (../platform)
- ZOO utility module (../util)

### Building

```bash
mkdir build
cd build
cmake ..
make
```

### Integration

To use the ZOO Log module in your project:

```c
#include "zoo_log.h"

int main() {
    // Initialize logging system
    zoo_log_init();
    
    // Configure log level
    zoo_log_set_level(ZOO_LOG_LEVEL_INFO);
    
    // Basic logging
    ZOO_LOG_INFO("Application started");
    ZOO_LOG_DEBUG("Debug information: %d", 42);
    ZOO_LOG_ERROR("Error occurred: %s", "Sample error");
    
    // Cleanup
    zoo_log_cleanup();
    return 0;
}
```

## API Reference

### Core Functions

- `zoo_result_t zoo_log_init(void)` - Initialize the logging system
- `void zoo_log_cleanup(void)` - Cleanup and shutdown logging
- `zoo_result_t zoo_log_set_level(zoo_log_level_t level)` - Set minimum log level
- `zoo_result_t zoo_log_add_target(zoo_log_target_t *target)` - Add output target

### Logging Macros

- `ZOO_LOG_TRACE(format, ...)` - Trace level logging
- `ZOO_LOG_DEBUG(format, ...)` - Debug level logging  
- `ZOO_LOG_INFO(format, ...)` - Information level logging
- `ZOO_LOG_WARN(format, ...)` - Warning level logging
- `ZOO_LOG_ERROR(format, ...)` - Error level logging
- `ZOO_LOG_FATAL(format, ...)` - Fatal error level logging

## Configuration

The logging system can be configured through:

1. **Compile-time macros**: Define build-time behavior
2. **Runtime API**: Dynamic configuration during execution
3. **Configuration files**: External configuration support

### Build Options

- `ZOO_LOG_ENABLE_COLORS` - Enable colored console output
- `ZOO_LOG_ENABLE_TIMESTAMPS` - Include timestamps in log messages
- `ZOO_LOG_ENABLE_THREAD_ID` - Include thread IDs in log messages
- `ZOO_LOG_ENABLE_SOURCE_LOCATION` - Include file/line information

## Performance

The ZOO Log module is designed for high performance:

- Minimal overhead when logging is disabled
- Efficient string formatting
- Optional asynchronous logging
- Memory pool for reduced allocations

## Thread Safety

All logging operations are thread-safe by default. The module uses:

- Lock-free algorithms where possible
- Minimal critical sections
- Per-thread buffers for performance

## Testing

Run the test suite:

```bash
make test
```

## Contributing

When contributing to the ZOO Log module:

1. Follow the existing code style
2. Add unit tests for new functionality
3. Update documentation as needed
4. Ensure thread safety for all operations

## License

Copyright (C) 2025, Basic Software Research Institute ltd. All rights reserved.

## Authors

- weiwang.sun - Initial implementation
- GitHub Copilot - Implementation completion and enhancements

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/index.html)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.

---

### Module: memory_pool

Source: (module README in this section)
# ZOO Memory Pool Component

## Overview
High-performance thread-safe memory pool implementation using a slab allocator for the ZOO (Zoo Operations Optimization) messaging system.

## Testing Framework
✅ **Unity C Testing Framework** with **CMock** support (migrated from GTest)

The memory pool component now uses Unity C Testing Framework with CMock for comprehensive testing:
- **Test Location**: `tests/` directory
- **Test Runner**: `./build/bin/zoo_memory_pool_tests`
- **Framework**: Unity v2.6.2 + CMock v2.6.1
- **Coverage**: 11 comprehensive tests covering core functionality, thread safety, and edge cases

### Quick Test Run
```bash
cd /home/sky/zoo/memory_pool
./build.sh --run-tests
```

### Build Script Options
The unified `build.sh` script provides comprehensive build and test options:
```bash
./build.sh                    # Release build with Unity tests
./build.sh --debug            # Debug build with Unity tests  
./build.sh --no-tests         # Build without tests
./build.sh --clean --run-tests # Clean rebuild and run tests
./build.sh --help             # Show all options
```

See `BUILD_SCRIPT_SUMMARY.md` for complete build script documentation.

## Features

### Core Capabilities
- **Slab Allocator**: Efficient memory allocation with minimal fragmentation
- **Thread Safety**: Full thread-safe operations with mutex protection
- **Singleton Pattern**: Global memory pool instance for system-wide use
- **Memory Statistics**: Detailed usage tracking and reporting
- **Validation**: Built-in pool integrity checking

### Performance Characteristics
- **Fast Allocation**: O(1) allocation for common sizes
- **Low Overhead**: Minimal metadata per allocation
- **Memory Efficiency**: Reduced fragmentation through slab design
- **Thread Scalability**: Optimized for multi-threaded environments

## API Reference

### Core Functions
```c
// Pool lifecycle
ZOO_INT32 zoo_create_memory_pool(ZOO_SIZE_T total_size);
void zoo_destroy_memory_pool(void);

// Memory operations
void* zoo_allocate_from_pool(ZOO_SIZE_T size);
void zoo_free_to_pool(void* ptr);

// Pool management
ZOO_INT32 zoo_validate_memory_pool(void);
void zoo_memory_pool_get_usage(ZOO_MEMORY_USAGE* usage);
```

### Memory Usage Statistics
```c
typedef struct {
    ZOO_SIZE_T pool_size;      // Total pool size
    ZOO_SIZE_T used_size;      // Currently allocated
    ZOO_SIZE_T used_pct;       // Usage percentage
    ZOO_SIZE_T free_size;      // Available memory
    ZOO_SIZE_T pages;          // Total pages
    ZOO_SIZE_T free_page;      // Free pages
    ZOO_SIZE_T p_small;        // Small slab pages
    ZOO_SIZE_T p_exact;        // Exact slab pages
    ZOO_SIZE_T p_big;          // Big slab pages
    ZOO_SIZE_T p_page;         // Page slab pages
    ZOO_SIZE_T b_small;        // Small slab bytes
    ZOO_SIZE_T b_exact;        // Exact slab bytes
    ZOO_SIZE_T b_big;          // Big slab bytes
    ZOO_SIZE_T b_page;         // Page slab bytes
    ZOO_SIZE_T max_free_pages; // Max continuous free pages
} ZOO_MEMORY_USAGE;
```

## Implementation Details

### Slab Allocator Design
- **Small Slabs**: For allocations < 512 bytes
- **Exact Slabs**: For common allocation sizes
- **Big Slabs**: For large allocations
- **Page Slabs**: For very large allocations

### Thread Safety
- Mutex-protected allocation/deallocation
- Lock-free statistics reading where possible
- Optimized critical sections

### Memory Layout
- Page-based memory management
- Metadata stored separately from user data
- Efficient free list management

## Building

### Prerequisites
- CMake 3.14+
- GCC with C99 support
- pthread library
- Unity/CMock testing frameworks (included)

### Build Commands
```bash
# Configure and build
cmake -B build
make -C build

# Build with tests
cmake -B build -DBUILD_TESTING=ON
make -C build

# Run tests
./build/bin/zoo_memory_pool_tests
```

### Build Targets
- `zoo_memory_pool_static` - Static library
- `zoo_memory_pool_shared` - Shared library
- `zoo_memory_pool_tests` - Unity test suite

## Integration

### CMake Integration
```cmake
find_package(ZooMemoryPool REQUIRED)
target_link_libraries(your_target ZooMemoryPool::zoo_memory_pool)
```

### Direct Usage
```c
#include "zoo_memory_pool.h"

// Create pool (typically 1MB)
zoo_create_memory_pool(1024 * 1024);

// Allocate memory
void* ptr = zoo_allocate_from_pool(1024);
if (ptr) {
    // Use memory...
    zoo_free_to_pool(ptr);
}

// Cleanup
zoo_destroy_memory_pool();
```

## Error Handling

### Error Codes
- `ZOO_ERROR_MEM_POOL_ALLOCATION_FAILED` - Allocation failed
- `ZOO_ERROR_MEM_POOL_INVALID_PARAM` - Invalid parameter
- `ZOO_ERROR_MEM_POOL_OUT_OF_MEMORY` - Pool exhausted

### Best Practices
- Always check return values from allocation functions
- Use validation functions during development
- Monitor memory usage statistics
- Proper cleanup on application exit

## Performance Guidelines

### Allocation Patterns
- Prefer consistent allocation sizes for better slab utilization
- Batch allocations when possible
- Avoid frequent allocation/deallocation cycles

### Thread Considerations
- Minimize lock contention by reducing allocation frequency
- Consider thread-local caching for high-frequency allocations
- Use statistics monitoring to identify bottlenecks

## Dependencies

### Internal Dependencies
- `zoo.h` - ZOO platform definitions
- `zoo_error.h` - Error code definitions

### System Dependencies
- `pthread` - Thread synchronization
- `stdlib.h` - Standard library functions
- `string.h` - Memory operations

## License
Copyright (C) 2025, Basic Software Research Institute ltd This module provides efficient slab-based memory allocation with thread safety and support for multiple operating systems and architectures.

## Features

- **Cross-Platform Support**: Works on Linux, Windows, macOS, and embedded systems (FreeRTOS, CMSIS-RTOS)
- **High Performance**: Slab-based allocation algorithm for fast allocation/deallocation
- **Thread Safe**: Built-in synchronization for multi-threaded environments
- **Multiple Architectures**: Support for x86, x64, ARM Cortex-M, ARM Cortex-A
- **Zero Dependencies**: Only requires standard C library and platform-specific threading APIs
- **Memory Alignment**: Automatic memory alignment for optimal performance
- **Configurable**: Customizable pool sizes and allocation strategies

## Architecture Support

### Platforms
- **Linux** (POSIX compliant)
- **Windows** (Win32/Win64)
- **macOS** (POSIX compliant)
- **Embedded Systems**:
  - FreeRTOS
  - CMSIS-RTOS
  - Bare metal

### Architectures
- **x86/x64** (Intel/AMD processors)
- **ARM Cortex-M** (Microcontrollers)
- **ARM Cortex-A** (Application processors)
- **ARM64/AArch64**

## Quick Start

### Building

```bash
# Simple build
./build.sh

# Debug build with tests
./build.sh --debug --test

# Cross-compile for ARM
./build.sh --cross-compile arm

# Clean build
./build.sh --clean
```

### Using CMake Directly

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Basic Usage

```c
#include "zoo_memory_pool.h"

// Create a memory pool (1MB)
ZOO_MEMORY_POOL* pool = zoo_memory_pool_create(1024 * 1024);

// Allocate memory
void* ptr = zoo_memory_pool_alloc(pool, 256);

// Use the memory
memset(ptr, 0, 256);

// Free memory
zoo_memory_pool_free(pool, ptr);

// Destroy pool
zoo_memory_pool_destroy(pool);
```

## API Reference

### Core Functions

#### `zoo_memory_pool_create(size_t size)`
Creates a new memory pool with the specified size.
- **Parameters**: `size` - Total size of the memory pool in bytes
- **Returns**: Pointer to the memory pool or NULL on failure
- **Thread Safety**: Yes

#### `zoo_memory_pool_destroy(ZOO_MEMORY_POOL* pool)`
Destroys a memory pool and frees all associated resources.
- **Parameters**: `pool` - Pointer to the memory pool
- **Thread Safety**: Yes (but pool should not be used concurrently)

#### `zoo_memory_pool_alloc(ZOO_MEMORY_POOL* pool, size_t size)`
Allocates memory from the pool.
- **Parameters**: 
  - `pool` - Pointer to the memory pool
  - `size` - Number of bytes to allocate
- **Returns**: Pointer to allocated memory or NULL on failure
- **Thread Safety**: Yes

#### `zoo_memory_pool_free(ZOO_MEMORY_POOL* pool, void* ptr)`
Frees previously allocated memory back to the pool.
- **Parameters**: 
  - `pool` - Pointer to the memory pool
  - `ptr` - Pointer to memory to free
- **Thread Safety**: Yes

#### `zoo_memory_pool_realloc(ZOO_MEMORY_POOL* pool, void* ptr, size_t new_size)`
Reallocates memory to a new size.
- **Parameters**: 
  - `pool` - Pointer to the memory pool
  - `ptr` - Pointer to existing memory
  - `new_size` - New size in bytes
- **Returns**: Pointer to reallocated memory or NULL on failure
- **Thread Safety**: Yes

#### `zoo_memory_pool_calloc(ZOO_MEMORY_POOL* pool, size_t num, size_t size)`
Allocates zero-initialized memory for an array.
- **Parameters**: 
  - `pool` - Pointer to the memory pool
  - `num` - Number of elements
  - `size` - Size of each element
- **Returns**: Pointer to allocated memory or NULL on failure
- **Thread Safety**: Yes

### Configuration Functions

#### `zoo_memory_pool_set_alignment(ZOO_MEMORY_POOL* pool, size_t alignment)`
Sets the memory alignment for future allocations.
- **Parameters**: 
  - `pool` - Pointer to the memory pool
  - `alignment` - Alignment requirement (must be power of 2)
- **Returns**: 0 on success, negative on error

#### `zoo_memory_pool_get_stats(ZOO_MEMORY_POOL* pool, ZOO_MEMORY_POOL_STATS* stats)`
Retrieves memory pool statistics.
- **Parameters**: 
  - `pool` - Pointer to the memory pool
  - `stats` - Pointer to statistics structure
- **Returns**: 0 on success, negative on error

## Advanced Usage

### Custom Allocator Configuration

```c
// Create pool with specific configuration
ZOO_MEMORY_POOL_CONFIG config = {
    .initial_size = 1024 * 1024,    // 1MB initial size
    .max_size = 10 * 1024 * 1024,   // 10MB maximum size
    .alignment = 16,                 // 16-byte alignment
    .auto_expand = true,            // Allow automatic expansion
    .thread_safe = true             // Enable thread safety
};

ZOO_MEMORY_POOL* pool = zoo_memory_pool_create_ex(&config);
```

### Performance Monitoring

```c
ZOO_MEMORY_POOL_STATS stats;
zoo_memory_pool_get_stats(pool, &stats);

printf("Total allocated: %zu bytes
", stats.total_allocated);
printf("Peak usage: %zu bytes
", stats.peak_usage);
printf("Allocation count: %zu
", stats.allocation_count);
printf("Free count: %zu
", stats.free_count);
```

### Error Handling

```c
void* ptr = zoo_memory_pool_alloc(pool, size);
if (ptr == NULL) {
    // Handle allocation failure
    ZOO_MEMORY_POOL_ERROR error = zoo_memory_pool_get_last_error(pool);
    switch (error) {
        case ZOO_MEMORY_POOL_ERROR_OUT_OF_MEMORY:
            printf("Pool exhausted
");
            break;
        case ZOO_MEMORY_POOL_ERROR_INVALID_SIZE:
            printf("Invalid allocation size
");
            break;
        default:
            printf("Unknown error
");
            break;
    }
}
```

## Cross-Platform Considerations

### Thread Safety
The memory pool uses platform-specific synchronization primitives:
- **POSIX systems**: pthread_mutex
- **Windows**: Critical Sections
- **FreeRTOS**: Mutex semaphores
- **CMSIS-RTOS**: Mutex objects
- **Bare metal**: Atomic operations (if available)

### Memory Alignment
Different platforms have different alignment requirements:
- **x86/x64**: 8-byte alignment for optimal performance
- **ARM Cortex-M**: 4-byte alignment (8-byte for double)
- **ARM Cortex-A**: 8-byte alignment for optimal performance

### Page Size Detection
The pool automatically detects system page size:
- **Linux/macOS**: Uses `getpagesize()`
- **Windows**: Uses `GetSystemInfo()`
- **Embedded**: Uses default 4KB pages

## Testing

### Running Tests

```bash
# Build and run tests
./build.sh --test

# Run specific test suite
cd build && ./zoo_memory_pool_tests --gtest_filter="MemoryPoolTest.ThreadSafety"

# Run with verbose output
cd build && ./zoo_memory_pool_tests --gtest_verbose
```

### Test Coverage

The test suite includes:
- **Basic Functionality**: Allocation, deallocation, reallocation
- **Thread Safety**: Multi-threaded allocation/deallocation
- **Error Handling**: Out-of-memory, invalid parameters
- **Performance**: Allocation speed benchmarks
- **Memory Alignment**: Proper alignment verification
- **Pool Exhaustion**: Behavior when pool is full

## Performance

### Benchmarks

Typical performance on modern hardware:

| Operation | Operations/second | Notes |
|-----------|------------------|-------|
| Allocate (64 bytes) | ~10M ops/sec | Single-threaded |
| Free (64 bytes) | ~12M ops/sec | Single-threaded |
| Allocate + Free | ~8M cycles/sec | Single-threaded |
| Multi-threaded (4 threads) | ~6M cycles/sec | With synchronization |

### Optimization Tips

1. **Pre-allocate pools**: Create pools at startup to avoid runtime overhead
2. **Size pools appropriately**: Larger pools reduce fragmentation
3. **Use appropriate alignment**: Match your data structure requirements
4. **Batch operations**: Group allocations when possible
5. **Monitor statistics**: Use pool statistics to optimize sizing

## Integration

### CMake Integration

```cmake
# Find the package
find_package(ZooMemoryPool REQUIRED)

# Link to your target
target_link_libraries(your_target ZooMemoryPool::zoo_memory_pool)
```

### pkg-config Integration

```bash
# Compile with pkg-config
gcc $(pkg-config --cflags zoo-memory-pool) -o myapp myapp.c $(pkg-config --libs zoo-memory-pool)
```

## Troubleshooting

### Common Issues

1. **Compilation errors on embedded platforms**
   - Ensure proper platform detection macros are set
   - Check that threading libraries are available
   - Verify toolchain configuration

2. **Poor performance**
   - Check memory alignment settings
   - Verify pool size is appropriate for workload
   - Monitor fragmentation using statistics

3. **Memory leaks**
   - Ensure all allocations are matched with frees
   - Use valgrind or similar tools for detection
   - Check that pool is properly destroyed

### Debug Build

```bash
# Build with debug symbols and assertions
./build.sh --debug

# Enable memory debugging
export ZOO_MEMORY_POOL_DEBUG=1
./your_application
```

## Contributing

1. Follow the existing code style
2. Add tests for new functionality
3. Update documentation for API changes
4. Test on multiple platforms before submitting
5. Run the full test suite

## License

Copyright (C) 2025, Basic Software Research Institute ltd. All rights reserved.

## Version History

- **1.0.0** (2025-01-01): Initial cross-platform implementation
  - Added support for Linux, Windows, macOS
  - Embedded systems support (FreeRTOS, CMSIS-RTOS)
  - Thread safety implementation
  - Comprehensive test suite



## Getting started

To make it easy for you to get started with GitLab, here's a list of recommended next steps.

Already a pro? Just edit this README.md and make it your own. Want to make it easy? [Use the template at the bottom](#editing-this-readme)!

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/ee/gitlab-basics/add-file.html#add-a-file-using-the-command-line) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin https://git.nevint.com/PERD/Software_PlatForm/poweros/soa_platform/zoo/memory.git
git branch -M master
git push -uf origin master
```

## Integrate with your tools

- [ ] [Set up project integrations](https://git.nevint.com/PERD/Software_PlatForm/poweros/soa_platform/zoo/memory/-/settings/integrations)

## Collaborate with your team

- [ ] [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
- [ ] [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Set auto-merge](https://docs.gitlab.com/ee/user/project/merge_requests/merge_when_pipeline_succeeds.html)

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/index.html)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.

---

### Module: smb

Source: (module README in this section)
# ZOO Soft Message Bus (SMB)

## Overview

ZOO SMB is a layered message bus library for embedded and system software scenarios. The external API is node-oriented (`server`, `client`, `publisher`, `subscriber`) and routes messages through a shared core bus, rule engine, and transport stack.

This README reflects the current architecture and API shape after remediation.

## Architecture (Reviewed)

### Layered view

1. **Application / Node API**
     - `zoo_smb_create_server()` / `zoo_smb_create_client()`
     - `zoo_smb_create_publisher()` / `zoo_smb_create_subscriber()`

2. **Core bus & runtime**
     - Node registration and message routing
     - Runtime lifecycle + HA scaffolding (`runtime` module)

3. **Routing / rule / transport management**
     - Rule resolution and dispatch
     - Transport health and retry policies

4. **Transport backends + protocol**
     - TCP / UDP / UDP-broadcast / SHM
     - message serialize/deserialize

5. **Platform dependencies**
     - thread/mutex/cond, memory pool, ring buffer, socket/event backend

### Architecture diagram

![SMB System Architecture](docs/smb_architecture.svg)

### Module map

- Public headers:
    - [inc/node](inc/node)
    - [inc/core](inc/core)
    - [inc/transport](inc/transport)
    - [inc/qos](inc/qos)
    - [inc/utility](inc/utility)
- Implementations:
    - [src/node](src/node)
    - [src/core](src/core)
    - [src/transport](src/transport)
    - [src/qos](src/qos)
    - [src/utility](src/utility)

## Documentation

- Requirements: [docs/SMB_REQUIREMENTS.md](docs/SMB_REQUIREMENTS.md)
- Design: [docs/SMB_DESIGN.md](docs/SMB_DESIGN.md)
- Architecture: [docs/SMB_ARCHITECTURE.md](docs/SMB_ARCHITECTURE.md)
- QoS Performance Review: [docs/SMB_QOS_PERFORMANCE_REVIEW.md](docs/SMB_QOS_PERFORMANCE_REVIEW.md)
- Code Spec Compliance Checklist: [docs/SMB_CODE_SPEC_COMPLIANCE_CHECKLIST.md](docs/SMB_CODE_SPEC_COMPLIANCE_CHECKLIST.md)
- Function Comment Audit: [docs/SMB_FUNCTION_COMMENT_AUDIT.md](docs/SMB_FUNCTION_COMMENT_AUDIT.md)
- Legacy combined architecture/design: [docs/SMB_ARCHITECTURE_DESIGN.md](docs/SMB_ARCHITECTURE_DESIGN.md)

## Current external API conventions

- **Do use (public node/core API)**
    - `zoo_smb_create_server()`, `zoo_smb_server_set_message_handler()`, `zoo_smb_server_send_reply()`
    - `zoo_smb_create_client()`, `zoo_smb_client_send_request()`, `zoo_smb_client_recv_reply()`
    - `zoo_smb_create_publisher()`, `zoo_smb_publish_message()`
    - `zoo_smb_create_subscriber()`, `zoo_smb_subscribe_message()`

- **Do not use as app lifecycle API**
    - `zoo_smb_service_create/start/stop/...` (these are not the external node API entrypoints)

## Build

### Dependencies

- CMake 3.14+
- C compiler with C11 support

### Commands

```bash
bash build.sh           # Normal build (ZOO_SMB_AUTO_INIT=ON)
bash build.sh test      # Test build (ZOO_SMB_AUTO_INIT=OFF)
```

## Test

```bash
# Run all tests
./run_test.sh

# Run only unit and integration tests
./run_test.sh unit integration

# Run tests in parallel
./run_test.sh --parallel

# Skip optional tests
./run_test.sh --skip-optional

# Custom build and report directory
./run_test.sh --build-dir mybuild --report-dir myreports

# Show help
./run_test.sh --help
```

## Example benchmarks

```bash
# Run SMB example benchmarks 1-4 separately and generate a clean summary
./run_example_benchmarks.sh

# Override timeout or report directory if needed
TEST_TIMEOUT=300 REPORT_DIR=reports ./run_example_benchmarks.sh
```

## Minimal server example (current API)

```c
#include "zoo_smb_server.h"
#include <stdio.h>
#include <string.h>

static int server_message_handler(
        void* user_data,
        uint32_t msg_id,
        uint64_t request_id,
        uint64_t timestamp,
        const char* sender,
        const void* payload,
        size_t payload_size)
{
        ZOO_SMB_SERVER_HANDLE server = (ZOO_SMB_SERVER_HANDLE)user_data;
        (void)timestamp;
        (void)payload;
        (void)payload_size;

        const char* reply_msg = "Server received your request!";
        size_t reply_size = strlen(reply_msg);
        zoo_smb_server_send_reply(server, sender, msg_id, reply_msg, reply_size, request_id);
        return ZOO_SMB_OK;
}

int main(void)
{
        ZOO_SMB_SERVER_HANDLE server = zoo_smb_create_server(
                "DemoServer",
                "localhost",
                "DemoTopic",
                ZOO_SMB_TRANSPORT_TYPE_DEFAULT,
                NULL);

        zoo_smb_server_set_message_handler(server, server_message_handler, server);
        getchar();
        zoo_smb_destroy_server(server);
        return 0;
}
```

## Notes

- Remediation details: [ARCHITECTURE_REMEDIATION_REPORT.md](ARCHITECTURE_REMEDIATION_REPORT.md)
- Runtime/HA implementation summary: [HA_IMPLEMENTATION_COMPLETE.md](HA_IMPLEMENTATION_COMPLETE.md)
- Additional verification docs: [COMPREHENSIVE_TEST_DOCUMENTATION.md](COMPREHENSIVE_TEST_DOCUMENTATION.md)

## License

This project is licensed under the company internal license. See [LICENSE](LICENSE).
---

### Module: socket

Source: (module README in this section)
# ZOO Socket Library

A cross-platform socket abstraction library supporting POSIX sockets (Linux/Unix) and lwIP (embedded systems).

## Features

- **Cross-platform**: Supports POSIX sockets and lwIP
- **Comprehensive API**: TCP, UDP, IPv4, IPv6 support
- **Thread-safe**: Built-in mutex protection for shared state
- **Error handling**: Standardized error codes and messages
- **Address utilities**: Helper functions for socket address management
- **Select support**: Non-blocking I/O with timeout support
- **Socket options**: Standard socket option configuration
- **Unity tests**: Complete test suite with Unity framework

## Supported Platforms

- **Linux**: Full POSIX socket support
- **Unix-like systems**: Full POSIX socket support  
- **Embedded systems**: lwIP support
- **Bare metal**: Stub implementation for testing

## Platform Detection

The library automatically detects platform capabilities:

```c
#if ZOO_HAS_POSIX
    // POSIX socket implementation
#elif defined(ZOO_USE_LWIP)
    // lwIP implementation
#else
    // Bare metal stub implementation
#endif
```

## Quick Start

### Initialization

```c
#include "zoo_socket.h"

// Initialize socket subsystem
ZOO_ERROR_TYPE result = zoo_socket_init();
if (result != ZOO_OK) {
    printf("Failed to initialize: %s\n", zoo_socket_error_string(result));
    return -1;
}

// ... use socket functions ...

// Cleanup when done
zoo_socket_cleanup();
```

### TCP Client Example

```c
zoo_socket_info_t client;
zoo_sockaddr_t server_addr;

// Create TCP socket
zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &client);

// Connect to server
zoo_sockaddr_loopback_init(&server_addr, ZOO_AF_INET, 8080);
zoo_socket_connect(&client, &server_addr);

// Send data
const char* message = "Hello Server!";
size_t bytes_sent;
zoo_socket_send(&client, message, strlen(message), &bytes_sent);

// Receive response
char buffer[256];
size_t bytes_received;
zoo_socket_recv(&client, buffer, sizeof(buffer), &bytes_received);

// Close socket
zoo_socket_close(&client);
```

### TCP Server Example

```c
zoo_socket_info_t server, client;
zoo_sockaddr_t server_addr, client_addr;

// Create and configure server socket
zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &server);

int reuse = 1;
zoo_socket_setsockopt(&server, ZOO_SOL_SOCKET, ZOO_SO_REUSEADDR, &reuse, sizeof(reuse));

// Bind and listen
zoo_sockaddr_any_init(&server_addr, ZOO_AF_INET, 8080);
zoo_socket_bind(&server, &server_addr);
zoo_socket_listen(&server, 5);

// Accept client
zoo_socket_accept(&server, &client, &client_addr);

// Handle client communication...

zoo_socket_close(&client);
zoo_socket_close(&server);
```

### UDP Example

```c
zoo_socket_info_t udp_socket;
zoo_sockaddr_t dest_addr;

// Create UDP socket
zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_DGRAM, ZOO_IPPROTO_UDP, &udp_socket);

// Send to destination
zoo_sockaddr_inet_init(&dest_addr, 0xC0A80101, 12345); // 192.168.1.1:12345
const char* data = "UDP message";
size_t bytes_sent;
zoo_socket_sendto(&udp_socket, data, strlen(data), &dest_addr, &bytes_sent);

// Receive from any source
char buffer[512];
zoo_sockaddr_t from_addr;
size_t bytes_received;
zoo_socket_recvfrom(&udp_socket, buffer, sizeof(buffer), &from_addr, &bytes_received);

zoo_socket_close(&udp_socket);
```

## API Reference

### Initialization Functions

- `zoo_socket_init()` - Initialize socket subsystem
- `zoo_socket_cleanup()` - Cleanup socket subsystem
- `zoo_socket_is_initialized()` - Check initialization status

### Socket Management

- `zoo_socket_create()` - Create a new socket
- `zoo_socket_close()` - Close a socket
- `zoo_socket_bind()` - Bind socket to address
- `zoo_socket_listen()` - Listen for connections
- `zoo_socket_accept()` - Accept incoming connection
- `zoo_socket_connect()` - Connect to remote address
- `zoo_socket_shutdown()` - Shutdown socket operations

### Data Transfer

- `zoo_socket_send()` - Send data (TCP)
- `zoo_socket_recv()` - Receive data (TCP)
- `zoo_socket_sendto()` - Send data (UDP)
- `zoo_socket_recvfrom()` - Receive data (UDP)

### Socket Options

- `zoo_socket_setsockopt()` - Set socket option
- `zoo_socket_getsockopt()` - Get socket option
- `zoo_socket_set_blocking()` - Set blocking/non-blocking mode

### Address Utilities

- `zoo_sockaddr_inet_init()` - Initialize IPv4 address
- `zoo_sockaddr_inet6_init()` - Initialize IPv6 address  
- `zoo_sockaddr_any_init()` - Initialize "any" address
- `zoo_sockaddr_loopback_init()` - Initialize loopback address
- `zoo_sockaddr_equal()` - Compare addresses

### Advanced Functions

- `zoo_socket_select()` - Monitor socket for I/O readiness
- `zoo_socket_resolve_hostname()` - Resolve hostname to addresses
- `zoo_socket_get_addresses()` - Get local/remote addresses

### Error Handling

- `zoo_socket_get_last_error()` - Get last error code
- `zoo_socket_error_string()` - Get error description

## Building

### CMake Build

```bash
cd zoo/socket
mkdir build && cd build
cmake ..
make
```

### Build Options

- `ZOO_SOCKET_BUILD_TESTS=ON/OFF` - Build unit tests (default: ON)
- `ZOO_SOCKET_BUILD_EXAMPLES=ON/OFF` - Build examples (default: ON)

### Dependencies

- ZOO platform headers (`zoo_platform.h`)
- ZOO utility headers (`zoo_util.h`)
- Unity test framework (for tests)

## Testing

Run the test suite:

```bash
cd build
make test
# or
ctest --verbose
```

Run specific test categories:

```bash
./tests/zoo_socket_tests
```

## Examples

Build and run examples:

```bash
cd build
make

# Run TCP server (in one terminal)
./examples/tcp_server_example

# Run TCP client (in another terminal)  
./examples/tcp_client_example

# Run other examples
./examples/udp_example
./examples/address_example
```

## Error Codes

The library uses standardized error codes from the ZOO framework:

- `ZOO_OK` - Success
- `ZOO_ERROR_INVALID_PARAM` - Invalid parameter
- `ZOO_ERROR_NOT_INITIALIZED` - Not initialized
- `ZOO_ERROR_CONNECTION_REFUSED` - Connection refused
- `ZOO_ERROR_CONNECTION_RESET` - Connection reset
- `ZOO_ERROR_TIMEOUT` - Operation timeout
- `ZOO_ERROR_AGAIN` - Try again (non-blocking)

## Thread Safety

The socket library is thread-safe for:

- Initialization/cleanup functions
- Error state management
- Individual socket operations on different sockets

**Note**: Multiple threads accessing the same socket concurrently is not supported.

## Platform-Specific Notes

### Linux/POSIX
- Full socket API support
- IPv6 support available
- Unix domain socket support
- Complete error mapping

### lwIP (Embedded)
- TCP/UDP support
- Limited socket options
- No Unix domain sockets
- Simplified error handling

### Bare Metal
- Stub implementation
- Suitable for unit testing
- No actual network functionality

## License

Copyright (C) 2025, ZOO Ltd. All rights reserved.

## Contributing

Please follow the ZOO framework coding standards and ensure all tests pass before submitting changes.

---

### Module: thread_pool

Source: (module README in this section)
# ZOO Thread Pool Module

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20FreeRTOS%20%7C%20CMSIS--RTOS-blue.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()
[![Version](https://img.shields.io/badge/version-1.0.0-orange.svg)]()

A high-performance, cross-platform thread pool implementation for the ZOO embedded systems framework. Provides efficient task scheduling and execution with robust error handling and platform abstraction.

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Supported Platforms](#supported-platforms)
- [Quick Start](#quick-start)
- [Build Instructions](#build-instructions)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Performance](#performance)
- [Testing](#testing)
- [Contributing](#contributing)
- [License](#license)

## Features

### 🚀 **High Performance**
- **Zero-copy task submission** for minimal overhead
- **Lock-free atomic operations** where possible
- **Configurable thread count** and queue sizes
- **NUMA-aware thread scheduling** on supported platforms

### 🔧 **Cross-Platform Support**
- **Windows** (Win32 API)
- **Linux** (pthreads)
- **FreeRTOS** (native task management)
- **CMSIS-RTOS** (ARM Cortex-M)
- **Bare Metal** (custom implementations)

### 🛡️ **Robust Design**
- **Thread-safe operations** with proper synchronization
- **Graceful error handling** with detailed error codes
- **Resource leak prevention** with automatic cleanup
- **Configurable timeouts** and retry mechanisms

### 📊 **Monitoring & Diagnostics**
- **Real-time status reporting** (active threads, queue depth)
- **Performance metrics** collection
- **Comprehensive logging** integration
- **Memory usage tracking**

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ZOO Thread Pool                          │
├─────────────────────────────────────────────────────────────┤
│  Task Queue         │  Worker Threads     │  Completion     │
│  ┌─────────────┐   │  ┌─────────────┐   │  ┌─────────────┐ │
│  │   Task 1    │   │  │  Thread 1   │   │  │  Callback   │ │
│  │   Task 2    │   │  │  Thread 2   │   │  │  Handler    │ │
│  │   Task 3    │   │  │  Thread 3   │   │  │             │ │
│  │    ...      │   │  │    ...      │   │  │             │ │
│  └─────────────┘   │  └─────────────┘   │  └─────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                ZOO Platform Abstraction                     │
├─────────────────────────────────────────────────────────────┤
│  Windows  │  Linux  │  FreeRTOS  │  CMSIS-RTOS  │  Bare Metal │
└─────────────────────────────────────────────────────────────┘
```

### Core Components

1. **Task Queue**: Lock-free FIFO queue for pending tasks
2. **Worker Threads**: Configurable pool of execution threads
3. **Task Dispatcher**: Intelligent task-to-thread assignment
4. **Completion Handler**: Optional callback execution system
5. **Status Monitor**: Real-time pool status and metrics

## Supported Platforms

| Platform | Threading Model | Status | Notes |
|----------|----------------|---------|-------|
| **Windows** | Win32 Threads | ✅ Stable | Full feature support |
| **Linux** | POSIX Threads | ✅ Stable | Optimized for performance |
| **FreeRTOS** | Native Tasks | ✅ Stable | Memory-efficient implementation |
| **CMSIS-RTOS** | ARM RTOS | ✅ Stable | Cortex-M optimized |
| **Bare Metal** | Custom | 🔄 Beta | Cooperative scheduling |

## Quick Start

### 1. Include Headers
```c
#include "zoo_thread_pool.h"
#include "zoo.h"
```

### 2. Create Thread Pool
```c
// Create pool with 4 threads and queue size of 100
ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(4, 100);
if (pool == NULL) {
    // Handle creation error
    return ZOO_ERROR_OPERATION_FAILED;
}
```

### 3. Define Task Function
```c
ZOO_ERROR_TYPE my_task(void* user_data, void* argument) {
    // Your task implementation here
    int* counter = (int*)argument;
    (*counter)++;
    return ZOO_OK;
}
```

### 4. Submit Task
```c
int counter = 0;
ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
    "my_task",           // Task name
    my_task,            // Task function
    NULL,               // User data
    &counter,           // Task argument
    true                // Blocking submission
);
```

### 5. Cleanup
```c
// Graceful shutdown - wait for tasks to complete
zoo_destroy_thread_pool(true);
```

## Build Instructions

### Prerequisites

- **CMake 3.16+**
- **C11 compatible compiler** (GCC 7+, Clang 6+, MSVC 2019+)
- **Platform-specific tools**:
  - Windows: Visual Studio Build Tools
  - Linux: build-essential package
  - FreeRTOS: ARM GCC toolchain
  - CMSIS-RTOS: ARM Keil or GCC

### Build Steps

```bash
# Clone repository
git clone <repository-url>
cd zoo/thread-pool

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DZOO_ENABLE_TESTS=ON \
         -DZOO_ENABLE_BENCHMARKS=ON

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --prefix /usr/local
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `ZOO_ENABLE_TESTS` | `OFF` | Build unit and integration tests |
| `ZOO_ENABLE_BENCHMARKS` | `OFF` | Build performance benchmarks |
| `ZOO_ENABLE_COVERAGE` | `OFF` | Enable code coverage reporting |
| `ZOO_BUILD_STATIC` | `ON` | Build static library |
| `ZOO_BUILD_SHARED` | `OFF` | Build shared library |
| `ZOO_THREAD_POOL_DEBUG` | `OFF` | Enable debug logging |

### Platform-Specific Configuration

#### Windows
```bash
cmake .. -G "Visual Studio 16 2019" -A x64
```

#### Linux with Clang
```bash
cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

#### Cross-compilation for ARM
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/arm-toolchain.cmake
```

## API Reference

### Core Functions

#### `zoo_create_thread_pool`
```c
ZOO_THREAD_POOL_HANDLE zoo_create_thread_pool(
    ZOO_SIZE_T num_threads,
    ZOO_SIZE_T max_queue_size
);
```
**Description**: Creates a new thread pool with specified parameters.

**Parameters**:
- `num_threads`: Number of worker threads (1-64)
- `max_queue_size`: Maximum pending tasks (1-10000)

**Returns**: Thread pool handle or `NULL` on failure

**Thread Safety**: ✅ Thread-safe

---

#### `zoo_thread_pool_submit_task`
```c
ZOO_ERROR_TYPE zoo_thread_pool_submit_task(
    const char* task_name,
    ZOO_TASK_FUNCTION task_func,
    void* user_data,
    void* argument,
    bool blocking
);
```
**Description**: Submits a task for execution.

**Parameters**:
- `task_name`: Human-readable task identifier
- `task_func`: Function to execute
- `user_data`: User-defined context data
- `argument`: Task-specific argument
- `blocking`: Wait if queue is full

**Returns**: 
- `ZOO_OK`: Task submitted successfully
- `ZOO_ERROR_AGAIN`: Queue full (non-blocking mode)
- `ZOO_ERROR_INVALID_PARAMETER`: Invalid parameters

**Thread Safety**: ✅ Thread-safe

---

#### `zoo_thread_pool_submit_task_ex`
```c
ZOO_ERROR_TYPE zoo_thread_pool_submit_task_ex(
    const char* task_name,
    ZOO_TASK_FUNCTION task_func,
    void* user_data,
    void* argument,
    ZOO_TASK_FUNCTION completion_func,
    void* completion_user_data,
    void* completion_argument,
    bool blocking
);
```
**Description**: Submits a task with completion callback.

**Additional Parameters**:
- `completion_func`: Called after task completion
- `completion_user_data`: Completion callback context
- `completion_argument`: Completion callback argument

**Thread Safety**: ✅ Thread-safe

---

#### `zoo_thread_pool_get_status`
```c
ZOO_ERROR_TYPE zoo_thread_pool_get_status(
    ZOO_SIZE_T* active_threads,
    ZOO_SIZE_T* queued_tasks
);
```
**Description**: Retrieves current thread pool status.

**Parameters**:
- `active_threads`: [OUT] Number of currently executing threads
- `queued_tasks`: [OUT] Number of pending tasks

**Thread Safety**: ✅ Thread-safe

---

#### `zoo_destroy_thread_pool`
```c
void zoo_destroy_thread_pool(bool wait_for_completion);
```
**Description**: Destroys the thread pool and cleans up resources.

**Parameters**:
- `wait_for_completion`: Wait for pending tasks to finish

**Thread Safety**: ⚠️ Must be called from single thread

### Task Function Signature

```c
typedef ZOO_ERROR_TYPE (*ZOO_TASK_FUNCTION)(void* user_data, void* argument);
```

**Parameters**:
- `user_data`: Context data provided during submission
- `argument`: Task-specific argument

**Returns**: Error code indicating task execution result

### Error Codes

| Code | Value | Description |
|------|--------|-------------|
| `ZOO_OK` | 0 | Operation successful |
| `ZOO_ERROR_INVALID_PARAMETER` | -1 | Invalid function parameter |
| `ZOO_ERROR_OUT_OF_MEMORY` | -2 | Memory allocation failed |
| `ZOO_ERROR_NOT_STARTED` | -15 | Thread pool not initialized |
| `ZOO_ERROR_AGAIN` | -16 | Resource temporarily unavailable |
| `ZOO_ERROR_OPERATION_FAILED` | -10 | General operation failure |

## Usage Examples

### Example 1: Basic Task Execution

```c
#include "zoo_thread_pool.h"
#include <stdio.h>

ZOO_ERROR_TYPE print_task(void* user_data, void* argument) {
    int* task_id = (int*)argument;
    printf("Executing task %d\n", *task_id);
    return ZOO_OK;
}

int main() {
    // Create thread pool
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(4, 50);
    if (!pool) {
        fprintf(stderr, "Failed to create thread pool\n");
        return -1;
    }
    
    // Submit tasks
    for (int i = 0; i < 10; i++) {
        int* task_id = malloc(sizeof(int));
        *task_id = i;
        
        ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
            "print_task",
            print_task,
            NULL,
            task_id,
            true
        );
        
        if (result != ZOO_OK) {
            fprintf(stderr, "Failed to submit task %d\n", i);
            free(task_id);
        }
    }
    
    // Wait and cleanup
    sleep(1);
    zoo_destroy_thread_pool(true);
    return 0;
}
```

### Example 2: Data Processing Pipeline

```c
#include "zoo_thread_pool.h"
#include <string.h>

typedef struct {
    char* data;
    size_t size;
    int id;
} DataPacket;

ZOO_ERROR_TYPE process_data(void* user_data, void* argument) {
    DataPacket* packet = (DataPacket*)argument;
    
    // Simulate data processing
    for (size_t i = 0; i < packet->size; i++) {
        packet->data[i] = toupper(packet->data[i]);
    }
    
    printf("Processed packet %d (%zu bytes)\n", packet->id, packet->size);
    return ZOO_OK;
}

ZOO_ERROR_TYPE cleanup_packet(void* user_data, void* argument) {
    DataPacket* packet = (DataPacket*)argument;
    free(packet->data);
    free(packet);
    return ZOO_OK;
}

int main() {
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(8, 100);
    
    // Process multiple data packets
    for (int i = 0; i < 50; i++) {
        DataPacket* packet = malloc(sizeof(DataPacket));
        packet->id = i;
        packet->size = 1000 + (i % 10) * 100;
        packet->data = malloc(packet->size);
        
        // Fill with test data
        memset(packet->data, 'a' + (i % 26), packet->size);
        
        // Submit with cleanup callback
        zoo_thread_pool_submit_task_ex(
            "data_processing",
            process_data,
            NULL,
            packet,
            cleanup_packet,
            NULL,
            packet,
            true
        );
    }
    
    zoo_destroy_thread_pool(true);
    return 0;
}
```

### Example 3: Producer-Consumer Pattern

```c
#include "zoo_thread_pool.h"
#include <pthread.h>

typedef struct {
    int value;
    struct timespec timestamp;
} WorkItem;

ZOO_ERROR_TYPE consumer_task(void* user_data, void* argument) {
    WorkItem* item = (WorkItem*)argument;
    
    // Simulate processing time
    usleep(1000 + (item->value % 10) * 1000);
    
    printf("Consumed item: %d\n", item->value);
    free(item);
    return ZOO_OK;
}

void* producer_thread(void* arg) {
    ZOO_THREAD_POOL_HANDLE pool = (ZOO_THREAD_POOL_HANDLE)arg;
    
    for (int i = 0; i < 100; i++) {
        WorkItem* item = malloc(sizeof(WorkItem));
        item->value = i;
        clock_gettime(CLOCK_REALTIME, &item->timestamp);
        
        // Non-blocking submission
        ZOO_ERROR_TYPE result = zoo_thread_pool_submit_task(
            "consumer_task",
            consumer_task,
            NULL,
            item,
            false
        );
        
        if (result == ZOO_ERROR_AGAIN) {
            // Queue full, wait and retry
            usleep(1000);
            i--; // Retry this item
        } else if (result != ZOO_OK) {
            free(item);
            break;
        }
        
        usleep(500); // Producer rate limiting
    }
    
    return NULL;
}

int main() {
    ZOO_THREAD_POOL_HANDLE pool = zoo_create_thread_pool(6, 20);
    
    // Start producer
    pthread_t producer;
    pthread_create(&producer, NULL, producer_thread, pool);
    
    // Monitor status
    for (int i = 0; i < 20; i++) {
        ZOO_SIZE_T active, queued;
        zoo_thread_pool_get_status(&active, &queued);
        printf("Status: %zu active, %zu queued\n", active, queued);
        sleep(1);
    }
    
    pthread_join(producer, NULL);
    zoo_destroy_thread_pool(true);
    return 0;
}
```

## Performance

### Benchmarks

Performance benchmarks measured on Intel i7-8700K @ 3.7GHz, 32GB RAM, Ubuntu 20.04:

| Metric | Value | Configuration |
|--------|--------|---------------|
| **Task Submission Rate** | 2.1M tasks/sec | 8 threads, empty tasks |
| **Task Execution Rate** | 1.8M tasks/sec | 8 threads, minimal work |
| **Memory Overhead** | 64 bytes/task | Including queue management |
| **Context Switch Time** | < 1μs | Thread-to-task assignment |
| **Queue Latency** | 50ns | Lock-free operations |

### Scalability

| Thread Count | Throughput | CPU Usage | Memory |
|--------------|------------|-----------|---------|
| 1 | 250K tasks/sec | 25% | 2MB |
| 4 | 980K tasks/sec | 90% | 4MB |
| 8 | 1.8M tasks/sec | 180% | 6MB |
| 16 | 2.2M tasks/sec | 320% | 10MB |
| 32 | 2.1M tasks/sec | 400% | 18MB |

### Optimization Tips

1. **Right-size the thread pool**: Use `num_cores + 1` for CPU-bound tasks
2. **Batch small tasks**: Combine multiple operations to reduce overhead
3. **Use non-blocking submission**: Implement producer backpressure
4. **Monitor queue depth**: Adjust based on workload characteristics
5. **Profile task duration**: Balance between throughput and latency

## Testing

### Running Tests

```bash
# Build with tests enabled
cmake .. -DZOO_ENABLE_TESTS=ON

# Run all tests
ctest

# Run specific test suites
./bin/zoo_thread_pool_unit_tests
./bin/zoo_thread_pool_integration_tests
./bin/zoo_thread_pool_performance_tests

# Run with coverage
cmake .. -DZOO_ENABLE_COVERAGE=ON
make coverage
```

### Test Categories

#### Unit Tests
- ✅ **Basic functionality** (creation, destruction, task submission)
- ✅ **Error handling** (invalid parameters, resource exhaustion)
- ✅ **Thread safety** (concurrent operations)
- ✅ **Memory management** (leak detection, cleanup)

#### Integration Tests
- ✅ **Multi-module coordination** (with buffer, logging systems)
- ✅ **Resource management** (memory pressure scenarios)
- ✅ **Real-world scenarios** (web server simulation)
- ✅ **Cross-platform compatibility**

#### Performance Tests
- ✅ **Throughput benchmarks** (task submission/execution rates)
- ✅ **Latency measurements** (task-to-execution time)
- ✅ **Scalability analysis** (thread count vs performance)
- ✅ **Stress testing** (sustained high load)

#### Platform Tests
- ✅ **Windows** (Win32 API integration)
- ✅ **Linux** (POSIX compliance)
- ✅ **FreeRTOS** (embedded constraints)
- ✅ **CMSIS-RTOS** (ARM Cortex-M)

### Continuous Integration

Tests are automatically run on:
- **GitHub Actions** (Linux, Windows, macOS)
- **Cross-compilation** (ARM, RISC-V)
- **Static analysis** (Clang Static Analyzer, Cppcheck)
- **Memory testing** (Valgrind, AddressSanitizer)

## Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

### Development Setup

```bash
# Clone with submodules
git clone --recursive <repository-url>

# Setup development environment
./scripts/setup-dev.sh

# Run pre-commit checks
./scripts/pre-commit.sh
```

### Coding Standards

- **C11 standard** compliance
- **MISRA C** guidelines where applicable
- **Doxygen** documentation for all public APIs
- **Unit tests** for all new functionality
- **Platform abstraction** for OS-specific code

### Submitting Changes

1. Fork the repository
2. Create a feature branch
3. Implement changes with tests
4. Run full test suite
5. Submit pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

- **Documentation**: [API Reference](docs/api.md)
- **Examples**: [examples/](examples/)
- **Issues**: [GitHub Issues](https://github.com/zoo-project/thread-pool/issues)
- **Discussions**: [GitHub Discussions](https://github.com/zoo-project/thread-pool/discussions)

---

**ZOO Thread Pool** - High-performance cross-platform threading for embedded systems.

*Built with ❤️ by the ZOO Framework Team*

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.

---

### Module: timer

Source: (module README in this section)
# ZOO Timer Module

## Description

ZOO Timer is a cross-platform timer utility library that provides high-precision timing capabilities across multiple operating systems and embedded platforms. It supports Windows, Linux, FreeRTOS, CMSIS-RTOS, and bare-metal environments.

## Features

- **Cross-Platform Compatibility**: Supports Windows, Linux, FreeRTOS, CMSIS-RTOS, and bare-metal platforms
- **Thread-Safe Operations**: All timer operations are thread-safe using atomic operations
- **High Precision**: Millisecond-level timing precision across all platforms
- **Repeating Timers**: Support for both one-shot and repeating timers
- **Callback-Based**: User-defined callback functions executed on timer expiration
- **Resource Management**: Automatic cleanup and resource management
- **Lightweight**: Minimal memory footprint suitable for embedded systems

## Supported Platforms

| Platform | Threading | Atomic Operations | Status |
|----------|-----------|-------------------|--------|
| Windows | Win32 API | InterLocked | ✅ Supported |
| Linux/POSIX | pthreads | C11 atomics | ✅ Supported |
| FreeRTOS | xTask API | Fallback | ✅ Supported |
| CMSIS-RTOS | osThread | Fallback | ✅ Supported |
| Bare Metal | None | Fallback | ⚠️ Limited |

## Installation

### Prerequisites

- CMake 3.10 or higher
- C99-compliant compiler
- Platform-specific dependencies:
  - Windows: Visual Studio or MinGW
  - Linux: GCC with pthread support
  - FreeRTOS: FreeRTOS kernel headers
  - CMSIS-RTOS: CMSIS-RTOS headers

### Build Instructions

```bash
# Clone and navigate to timer directory
cd /path/to/zoo/timer

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the library
make

# Install (optional)
make install
```

### CMake Integration

```cmake
find_package(zoo_timer REQUIRED)
target_link_libraries(your_target zoo_timer)
```

## Usage

### Basic Timer Creation

```c
#include "zoo_timer.h"

// Callback function
void timer_callback(void *user_data) {
    printf("Timer expired! User data: %s\n", (char*)user_data);
}

int main() {
    // Create a repeating timer (1000ms interval)
    ZOO_TIMER_HANDLE timer = zoo_timer_create(
        1000,           // 1 second interval
        ZOO_TRUE,       // repeating
        timer_callback, // callback function
        "Hello Timer"   // user data
    );
    
    if (timer == NULL) {
        printf("Failed to create timer\n");
        return -1;
    }
    
    // Start the timer
    if (!zoo_timer_start(timer)) {
        printf("Failed to start timer\n");
        zoo_timer_destroy(timer);
        return -1;
    }
    
    // Let timer run for 5 seconds
    sleep(5);
    
    // Stop and cleanup
    zoo_timer_stop(timer);
    zoo_timer_destroy(timer);
    
    return 0;
}
```

### One-Shot Timer

```c
// Create a one-shot timer (500ms)
ZOO_TIMER_HANDLE oneshot = zoo_timer_create(
    500,            // 500ms interval
    ZOO_FALSE,      // one-shot (non-repeating)
    timer_callback,
    NULL
);

zoo_timer_start(oneshot);
// Timer will fire once after 500ms and stop automatically
```

## API Reference

### Functions

#### `zoo_timer_create`
```c
ZOO_TIMER_HANDLE zoo_timer_create(
    ZOO_UINT32 interval_ms,
    ZOO_BOOL repeat,
    ZOO_TIMER_CALLBACK callback,
    void *user_data
);
```
Creates a new timer instance.

**Parameters:**
- `interval_ms`: Timer interval in milliseconds (must be > 0)
- `repeat`: ZOO_TRUE for repeating timer, ZOO_FALSE for one-shot
- `callback`: Function to call when timer expires (must not be NULL)
- `user_data`: User data passed to callback (can be NULL)

**Returns:** Timer handle on success, NULL on failure

#### `zoo_timer_start`
```c
ZOO_BOOL zoo_timer_start(ZOO_TIMER_HANDLE timer);
```
Starts the specified timer.

**Parameters:**
- `timer`: Timer handle to start

**Returns:** ZOO_TRUE on success, ZOO_FALSE on failure

#### `zoo_timer_stop`
```c
void zoo_timer_stop(ZOO_TIMER_HANDLE timer);
```
Stops the specified timer and waits for completion.

**Parameters:**
- `timer`: Timer handle to stop

#### `zoo_timer_destroy`
```c
void zoo_timer_destroy(ZOO_TIMER_HANDLE timer);
```
Destroys timer and frees all resources.

**Parameters:**
- `timer`: Timer handle to destroy

### Callback Function

```c
typedef void (*ZOO_TIMER_CALLBACK)(void *user_data);
```

The callback function is called when the timer expires. It should:
- Execute quickly to avoid blocking other timers
- Be thread-safe if accessing shared resources
- Not call timer functions on the same timer instance

## Thread Safety

All timer functions are thread-safe and can be called from multiple threads simultaneously. The library uses atomic operations and platform-specific synchronization primitives to ensure thread safety.

## Error Handling

Functions return appropriate error codes:
- `NULL` return values indicate creation/allocation failures
- `ZOO_FALSE` return values indicate operation failures
- Parameter validation is performed on all public functions

## Performance Considerations

- Timer precision depends on platform scheduling capabilities
- Multiple timers share thread resources efficiently
- Callback execution time affects overall timer precision
- Memory usage scales linearly with number of active timers

## Platform-Specific Notes

### Windows
- Uses Win32 CreateThread and Events
- High precision timing via Sleep()
- Supports all features

### Linux/POSIX
- Uses pthreads and condition variables
- High precision timing via nanosleep()
- Requires pthread library linking

### FreeRTOS
- Uses xTask API for threading
- Timing via vTaskDelay()
- Requires sufficient heap for task creation

### CMSIS-RTOS
- Uses osThread API
- Timing via osDelay()
- Thread stack size configurable

### Bare Metal
- Limited functionality (no threading)
- Polling-based operation only
- Use platform-specific alternatives when possible

## Contributing

When contributing to the timer module:

1. Maintain cross-platform compatibility
2. Follow ZOO coding standards (typedef suffixes, comments)
3. Test on multiple platforms when possible
4. Update documentation for API changes
5. Add appropriate error handling

## License

Copyright (C) 2025, ZOO Ltd. All rights reserved.

## Authors

- weiwang.sun - Initial implementation and cross-platform support

## Project Status

Active development. The timer module is feature-complete for basic timing operations and supports all major platforms. Future enhancements may include:

- High-resolution timer support (microsecond precision)
- Timer pools for better resource management
- Integration with platform-specific high-performance timers
- Real-time scheduling priorities

---

### Module: util

Source: (module README in this section)
# util



## Getting started

To make it easy for you to get started with GitLab, here's a list of recommended next steps.

Already a pro? Just edit this README.md and make it your own. Want to make it easy? [Use the template at the bottom](#editing-this-readme)!

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/ee/gitlab-basics/add-file.html#add-a-file-using-the-command-line) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin https://git.nevint.com/PERD/Software_PlatForm/poweros/soa_platform/zoo/util.git
git branch -M master
git push -uf origin master
```

## Integrate with your tools

- [ ] [Set up project integrations](https://git.nevint.com/PERD/Software_PlatForm/poweros/soa_platform/zoo/util/-/settings/integrations)

## Collaborate with your team

- [ ] [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
- [ ] [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Set auto-merge](https://docs.gitlab.com/ee/user/project/merge_requests/merge_when_pipeline_succeeds.html)

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/index.html)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.

---

### Module: dockfile

Source: (module README in this section)
# dockfile



## Getting started

To make it easy for you to get started with GitLab, here's a list of recommended next steps.

Already a pro? Just edit this README.md and make it your own. Want to make it easy? [Use the template at the bottom](#editing-this-readme)!

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/ee/gitlab-basics/add-file.html#add-a-file-using-the-command-line) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin https://git.nevint.com/PERD/Software_PlatForm/poweros/soa_platform/zoo/dockfile.git
git branch -M master
git push -uf origin master
```

## Integrate with your tools

- [ ] [Set up project integrations](https://git.nevint.com/PERD/Software_PlatForm/poweros/soa_platform/zoo/dockfile/-/settings/integrations)

## Collaborate with your team

- [ ] [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
- [ ] [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Set auto-merge](https://docs.gitlab.com/ee/user/project/merge_requests/merge_when_pipeline_succeeds.html)

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/index.html)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.

---
