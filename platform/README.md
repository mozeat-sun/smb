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

Copyright (C) 2025, ZOO Ltd. All rights reserved.

## Technical Support

- **Project Repository**: https://git.nevint.com/PERD/Software_PlatForm/poweros/soa_platform/zos
- **Issue Reporting**: Submit through GitLab Issues
- **Technical Discussion**: Internal technical forum

## Contributing

1. Fork the project repository
2. Create a feature branch (`git checkout -b feature/new-feature`)
3. Commit your changes (`git commit -am 'Add new feature'`)
4. Push to the branch (`git push origin feature/new-feature`)
5. Create a Pull Request

---

*ZOO Platform - Providing unified abstraction layer for embedded and cross-platform development*
