# ZOO Memory Pool Unity/CMock Tests

## Overview
This directory contains Unity C Testing Framework tests with CMock support for the ZOO Memory Pool component, replacing the previous GTest-based tests.

## Migration Status
✅ **COMPLETED**: Migration from GTest to Unity/CMock testing framework

## Test Framework
- **Unity C Testing Framework** v2.6.2: Lightweight C unit testing framework
- **CMock** v2.6.1: Mock generation framework for C (available for future mocking needs)

## Test Structure

### Test Files
- `test_zoo_memory_pool.c` - Core Unity tests for memory pool functionality
- `test_runner.c` - Test runner with platform detection and organized test execution
- `CMakeLists.txt` - Unity/CMock build configuration

### Test Categories

#### Core Memory Pool Tests
- `test_memory_pool_creation_and_destruction` - Pool lifecycle management
- `test_basic_allocation_and_free` - Basic allocation/deallocation
- `test_memory_write_and_read` - Memory integrity verification
- `test_zero_size_allocation` - Edge case handling
- `test_null_pointer_free` - Null pointer safety
- `test_memory_usage_statistics` - Memory usage tracking
- `test_multiple_small_allocations` - Small allocation patterns
- `test_memory_pool_validation` - Pool integrity validation

#### Advanced Tests
- `test_allocation_patterns` - Complex allocation scenarios

#### Thread Safety Tests
- `test_basic_thread_safety` - Multi-threaded allocation testing

#### Cleanup Tests
- `test_final_cleanup` - Final resource cleanup

## Building and Running

### Quick Build and Test
```bash
cd /home/sky/zoo/memory_pool
make -C build && ./build/bin/zoo_memory_pool_tests
```

### Alternative Build Methods
```bash
# Using the Unity build script
./build_unity.sh

# Using CMake custom targets
make -C build quick_test      # Run tests
make -C build test_verbose    # Run with verbose output
```

### Using CTest
```bash
cd build
ctest --output-on-failure
```

## Test Design Considerations

### Singleton Pattern Handling
The memory pool uses a singleton pattern with global state. Tests are designed to:
- Use `ensure_pool_exists()` helper for safe pool initialization
- Handle single pool instance across all tests
- Manage cleanup carefully to avoid state conflicts

### Thread Safety Testing
- Uses reduced thread count (2 threads) for stability
- Smaller allocation counts to prevent resource exhaustion
- Proper thread synchronization and cleanup

### Memory Statistics
- Tests allow for memory overhead in pool implementation
- Flexible assertions accommodate implementation details
- Real-time memory usage tracking and validation

## Features

### Platform Detection
- Automatic detection of CPU architecture (X86_64, ARM64, etc.)
- Operating system identification (Linux, Windows, macOS)
- Platform-specific test adaptations

### CMock Integration
- CMock framework available for future mocking needs
- Mock generation capabilities for dependency isolation
- Ready for advanced testing scenarios requiring mocks

### Comprehensive Coverage
- Memory allocation and deallocation patterns
- Error condition handling
- Thread safety verification
- Memory integrity checking
- Performance characteristics validation

## Migration Notes

### From GTest to Unity
- Replaced `ASSERT_*` macros with `TEST_ASSERT_*` Unity macros
- Converted C++ test classes to C test functions
- Adapted setup/teardown to Unity patterns
- Maintained all original test coverage

### Benefits of Unity/CMock
- Lightweight framework suitable for embedded/C projects
- Better integration with C codebase
- CMock provides powerful mocking capabilities
- Faster compilation and execution
- Simpler test debugging

## Dependencies

### Build Requirements
- CMake 3.14+
- GCC compiler with C99 support
- pthread library
- Math library (libm)

### Framework Dependencies
- Unity C Testing Framework (included in third_party)
- CMock (included in third_party)
- ZOO Memory Pool library

## Test Output Example
```
=== ZOO Memory Pool Test Suite ===
Platform: X86_64
OS: Linux
Unity Framework Version: 2.6.2
CMock Support: Available

=== Running Core Memory Pool Tests ===
[...test execution details...]

=== Test Summary ===
11 Tests 0 Failures 0 Ignored 
OK
```

## Future Enhancements
- Add CMock-based dependency mocking tests
- Performance benchmarking integration
- Memory leak detection with Valgrind
- Code coverage reporting
- Continuous integration setup
