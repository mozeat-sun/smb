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
