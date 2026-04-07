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
