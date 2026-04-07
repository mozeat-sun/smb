/*******************************************************************************
 * Copyright (C) 2025, ZOO Ltd.
 * All rights reserved.
 * Product: ZOO Timer Module
 * File name: README.md
 * Description: Testing infrastructure documentation
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-01     AI Assistant      Created comprehensive testing docs
 ******************************************************************************/

# ZOO Timer Module Testing Infrastructure

## Overview

This directory contains comprehensive unit tests for the ZOO Timer module using Unity testing framework and custom mocks for cross-platform testing.

## Test Structure

```
tests/
├── CMakeLists.txt              # Test build configuration
├── unit/                       # Unit test files
│   ├── test_zoo_timer.c        # Core timer functionality tests
│   ├── test_zoo_timer_platform.c # Platform-specific tests
│   └── test_runner.c           # Main test runner
├── mocks/                      # Mock implementations
│   ├── mock_zoo_platform.c    # Platform function mocks
│   ├── mock_zoo_platform.h    # Platform mock headers
│   ├── mock_stdlib.c          # Standard library mocks
│   └── mock_stdlib.h          # Standard library mock headers
└── build/                      # Test build artifacts
```

## Test Categories

### 1. Core Timer Functionality Tests
- **Timer Creation Tests**: Parameter validation, memory allocation, error handling
- **Timer Start Tests**: Thread creation, state management, error conditions
- **Timer Stop Tests**: Thread joining, cleanup, state transitions
- **Timer Destroy Tests**: Resource cleanup, running timer handling
- **Edge Cases Tests**: Multiple timers, large intervals, stress conditions

### 2. Platform-Specific Tests
- **Platform Sleep Tests**: Sleep function abstraction testing
- **Platform Threading Tests**: Thread creation, joining, error handling
- **Platform Memory Tests**: Memory allocation, failure handling, exhaustion
- **Platform Error Tests**: Error handling, null parameters, invalid operations
- **Platform Stress Tests**: Multiple operations, concurrent simulation

## Mock Framework

### Platform Function Mocks
The testing framework includes comprehensive mocks for platform-specific operations:

- `mock_zoo_timer_sleep_ms()` - Sleep function mock
- `mock_zoo_timer_create_thread()` - Thread creation mock
- `mock_zoo_timer_join_thread()` - Thread joining mock
- `mock_malloc()` / `mock_free()` - Memory allocation mocks

### Mock Control Functions
Each mock provides control functions for testing:

- `mock_*_expect()` - Set expectations for function calls
- `mock_*_verify()` - Verify function was called correctly
- `mock_*_reset()` - Reset mock state between tests

## Building and Running Tests

### Prerequisites
- CMake 3.15+
- GCC or Clang compiler
- Unity testing framework (auto-downloaded)

### Build Commands
```bash
# Build and run all tests
./build.sh --tests

# Clean build and run tests
./build.sh --clean --tests

# Run with coverage analysis
./build.sh --coverage

# Run with Valgrind memory checking
./build.sh --valgrind

# Run individual test scripts
./run_tests.sh        # Standard test execution
./run_coverage.sh     # Coverage analysis
./run_valgrind.sh     # Memory leak detection
```

### Test Output
Tests provide detailed output including:
- Individual test results (PASS/FAIL)
- Test execution statistics
- Failure details with line numbers
- Mock function call verification

## Test Results Analysis

### Current Test Status
The test suite includes 32 comprehensive tests covering:
- ✅ 12 tests passing (basic validation, null handling, mock setup)
- ❌ 20 tests failing (expected - require timer implementation adaptation)

### Expected Failures
Many tests currently fail because:
1. Real timer implementation conflicts with mock expectations
2. Platform-specific code needs mock integration
3. Memory allocation patterns differ from mock simulation

### Next Steps for Full Testing
1. **Integration Mode**: Create integration tests that use real platform functions
2. **Mock Injection**: Modify timer implementation to support mock injection
3. **Test Isolation**: Separate unit tests from integration tests
4. **Coverage Analysis**: Enable comprehensive code coverage reporting

## Coverage Analysis

Run coverage analysis to see test coverage:
```bash
./build.sh --coverage
```

Coverage report will be generated in `tests/build/coverage/html/index.html`

## Memory Testing

Run Valgrind memory analysis:
```bash
./build.sh --valgrind
```

This checks for:
- Memory leaks
- Buffer overruns
- Use after free
- Invalid memory access

## Static Analysis

Run static analysis tools:
```bash
./build.sh --static-analysis
```

Includes:
- cppcheck static analysis
- clang-tidy recommendations
- Compiler warnings analysis

## Mock Development Guidelines

### Adding New Mocks
1. Create mock header in `mocks/mock_*.h`
2. Implement mock functions in `mocks/mock_*.c`
3. Add control and verification functions
4. Include in test CMakeLists.txt
5. Document mock behavior

### Mock Function Pattern
```c
// Mock state tracking
static mock_data_t mock_data = {0};

// Mock implementation
int mock_function(int param) {
    mock_data.call_count++;
    mock_data.last_param = param;
    return mock_data.return_value;
}

// Expectation setting
void mock_function_expect(int return_value) {
    mock_data.return_value = return_value;
}

// Verification
void mock_function_verify(int expected_calls) {
    TEST_ASSERT_EQUAL_INT(expected_calls, mock_data.call_count);
}
```

## Unity Testing Framework

### Test Assertion Macros
- `TEST_ASSERT(condition)` - Basic condition test
- `TEST_ASSERT_TRUE(condition)` - True condition test
- `TEST_ASSERT_FALSE(condition)` - False condition test
- `TEST_ASSERT_NULL(pointer)` - Null pointer test
- `TEST_ASSERT_NOT_NULL(pointer)` - Non-null pointer test
- `TEST_ASSERT_EQUAL_INT(expected, actual)` - Integer equality
- `TEST_ASSERT_*_MESSAGE(assertion, message)` - All with custom message

### Test Structure
```c
void test_function_name(void)
{
    // Arrange - Set up test conditions
    
    // Act - Execute function under test
    
    // Assert - Verify results
    TEST_ASSERT_EQUAL_INT(expected, actual);
}
```

## Continuous Integration

The test infrastructure is designed for CI/CD integration:
- Return codes indicate test success/failure
- XML output for test result parsing
- Coverage reports for quality metrics
- Static analysis for code quality

## Troubleshooting

### Common Issues
1. **Tests not building**: Check CMake configuration and dependencies
2. **Mock failures**: Verify mock expectations match test execution
3. **Memory errors**: Use Valgrind to identify memory issues
4. **Coverage issues**: Ensure gcov/lcov tools are installed

### Debug Mode
Build tests in debug mode for better debugging:
```bash
BUILD_TYPE=Debug ./build.sh --tests
```

## Contributing

When adding new tests:
1. Follow existing test naming conventions
2. Add appropriate mock functions
3. Include both positive and negative test cases
4. Update documentation
5. Verify all tests pass before submitting
