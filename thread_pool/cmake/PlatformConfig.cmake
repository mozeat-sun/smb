# ==============================================================================
# ZOO Thread Pool - Platform Detection and Configuration
# ==============================================================================

# Platform detection function
function(zoo_thread_pool_detect_platform)
    message(STATUS "Detecting platform for ZOO Thread Pool...")
    
    # Detect architecture
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ZOO_PLATFORM_64BIT ON PARENT_SCOPE)
        set(ZOO_PLATFORM_32BIT OFF PARENT_SCOPE)
        message(STATUS "Architecture: 64-bit")
    else()
        set(ZOO_PLATFORM_64BIT OFF PARENT_SCOPE)
        set(ZOO_PLATFORM_32BIT ON PARENT_SCOPE)
        message(STATUS "Architecture: 32-bit")
    endif()
    
    # Detect operating system
    if(WIN32)
        set(ZOO_OS_WINDOWS ON PARENT_SCOPE)
        set(ZOO_OS_LINUX OFF PARENT_SCOPE)
        set(ZOO_OS_NAME "Windows" PARENT_SCOPE)
        set(ZOO_HAS_THREADING ON PARENT_SCOPE)
        set(ZOO_HAS_POSIX OFF PARENT_SCOPE)
        message(STATUS "Operating System: Windows")
        
        # Windows-specific definitions
        add_definitions(-DZOO_OS_WINDOWS -D_WIN32_WINNT=0x0601)
        
    elseif(UNIX AND NOT APPLE)
        set(ZOO_OS_LINUX ON PARENT_SCOPE)
        set(ZOO_OS_WINDOWS OFF PARENT_SCOPE)
        set(ZOO_OS_NAME "Linux" PARENT_SCOPE)
        set(ZOO_HAS_THREADING ON PARENT_SCOPE)
        set(ZOO_HAS_POSIX ON PARENT_SCOPE)
        message(STATUS "Operating System: Linux")
        
        # Linux-specific definitions
        add_definitions(-DZOO_OS_LINUX -D_GNU_SOURCE)
        
    elseif(APPLE)
        set(ZOO_OS_MACOS ON PARENT_SCOPE)
        set(ZOO_OS_NAME "macOS" PARENT_SCOPE)
        set(ZOO_HAS_THREADING ON PARENT_SCOPE)
        set(ZOO_HAS_POSIX ON PARENT_SCOPE)
        message(STATUS "Operating System: macOS")
        
    else()
        set(ZOO_OS_UNKNOWN ON PARENT_SCOPE)
        set(ZOO_OS_NAME "Unknown" PARENT_SCOPE)
        set(ZOO_HAS_THREADING OFF PARENT_SCOPE)
        set(ZOO_HAS_POSIX OFF PARENT_SCOPE)
        message(WARNING "Unknown operating system detected")
    endif()
    
    # Detect processor
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
        set(ZOO_PLATFORM_X86_64 ON PARENT_SCOPE)
        set(ZOO_PLATFORM_NAME "x86_64" PARENT_SCOPE)
        message(STATUS "Processor: x86_64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i386|i686|x86)$")
        set(ZOO_PLATFORM_X86_32 ON PARENT_SCOPE)
        set(ZOO_PLATFORM_NAME "x86_32" PARENT_SCOPE)
        message(STATUS "Processor: x86_32")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        set(ZOO_PLATFORM_ARM_CORTEX_A64 ON PARENT_SCOPE)
        set(ZOO_PLATFORM_NAME "ARM64" PARENT_SCOPE)
        message(STATUS "Processor: ARM64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        set(ZOO_PLATFORM_ARM_CORTEX_A32 ON PARENT_SCOPE)
        set(ZOO_PLATFORM_NAME "ARM32" PARENT_SCOPE)
        message(STATUS "Processor: ARM32")
    else()
        set(ZOO_PLATFORM_UNKNOWN ON PARENT_SCOPE)
        set(ZOO_PLATFORM_NAME "Unknown" PARENT_SCOPE)
        message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR} (Unknown)")
    endif()
endfunction()

# Threading configuration function
function(zoo_thread_pool_configure_threading target)
    if(ZOO_HAS_THREADING)
        message(STATUS "Configuring threading for ${target}")
        
        if(ZOO_OS_WINDOWS)
            # Windows threading support
            target_compile_definitions(${target} PRIVATE 
                ZOO_HAS_THREADING=1
                ZOO_OS_WINDOWS=1
            )
            target_link_libraries(${target} PRIVATE kernel32)
            
        elseif(ZOO_OS_LINUX OR ZOO_HAS_POSIX)
            # POSIX threading support
            target_compile_definitions(${target} PRIVATE 
                ZOO_HAS_THREADING=1
                ZOO_HAS_POSIX=1
                _POSIX_C_SOURCE=200809L
                _DEFAULT_SOURCE=1
            )
            find_package(Threads REQUIRED)
            target_link_libraries(${target} PRIVATE Threads::Threads)
            
            # Additional libraries for Linux
            if(ZOO_OS_LINUX)
                target_link_libraries(${target} PRIVATE rt m)
            endif()
        endif()
    else()
        message(STATUS "Threading disabled for ${target}")
        target_compile_definitions(${target} PRIVATE ZOO_HAS_THREADING=0)
    endif()
endfunction()
