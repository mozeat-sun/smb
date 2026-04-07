# ==============================================================================
# ZOO Thread Pool - Compiler Configuration
# ==============================================================================

# Compiler-specific optimizations and flags
function(zoo_thread_pool_apply_compiler_flags target)
    message(STATUS "Applying compiler flags for ${target}")
    
    # Common flags for all compilers
    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Debug>:ZOO_DEBUG=1>
        $<$<CONFIG:Release>:ZOO_RELEASE=1>
        $<$<CONFIG:Release>:NDEBUG>
    )
    
    # GCC and Clang
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            # Warning flags
            -Wall
            -Wextra
            -Wstrict-prototypes
            -Wmissing-prototypes
            -Wold-style-definition
            -Wpointer-arith
            -Wcast-qual
            -Wcast-align
            -Wwrite-strings
            -Wconversion
            -Wsign-conversion
            -Wnested-externs
            -Winline
            -Wdisabled-optimization
            
            # Optimization flags
            $<$<CONFIG:Debug>:-O0>
            $<$<CONFIG:Debug>:-g3>
            $<$<CONFIG:Debug>:-ggdb>
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Release>:-DNDEBUG>
            $<$<CONFIG:Release>:-fomit-frame-pointer>
            
            # Security flags
            -fstack-protector-strong
            -D_FORTIFY_SOURCE=2
            
            # Threading flags
            -pthread
        )
        
        # GCC-specific flags
        if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE
                -Wlogical-op
                -Wduplicated-cond
                -Wduplicated-branches
                -Wnull-dereference
                -Wjump-misses-init
                
                # Performance flags for Release
                $<$<CONFIG:Release>:-flto>
                $<$<CONFIG:Release>:-fwhole-program>
            )
            
            # Link-time optimization for Release builds
            if(CMAKE_BUILD_TYPE STREQUAL "Release")
                target_link_options(${target} PRIVATE -flto -fwhole-program)
            endif()
        endif()
        
        # Clang-specific flags
        if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${target} PRIVATE
                -Weverything
                -Wno-padded
                -Wno-switch-enum
                -Wno-covered-switch-default
                -Wno-declaration-after-statement
                -Wno-disabled-macro-expansion
                
                # Performance flags for Release
                $<$<CONFIG:Release>:-flto>
            )
            
            if(CMAKE_BUILD_TYPE STREQUAL "Release")
                target_link_options(${target} PRIVATE -flto)
            endif()
        endif()
        
    # Microsoft Visual C++
    elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE
            # Warning level
            /W4
            /WX-  # Don't treat warnings as errors by default
            
            # Disable specific warnings
            /wd4201  # nonstandard extension used: nameless struct/union
            /wd4204  # nonstandard extension used: non-constant aggregate initializer
            /wd4214  # nonstandard extension used: bit field types other than int
            /wd4996  # 'function': was declared deprecated
            
            # Optimization flags
            $<$<CONFIG:Debug>:/Od>
            $<$<CONFIG:Debug>:/Zi>
            $<$<CONFIG:Debug>:/RTC1>
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Release>:/Oi>
            $<$<CONFIG:Release>:/GL>
            
            # Security flags
            /GS
            /SDL
        )
        
        # MSVC-specific definitions
        target_compile_definitions(${target} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_DEPRECATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
        
        # Link-time code generation for Release builds
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            target_link_options(${target} PRIVATE /LTCG)
        endif()
        
    # Intel C++ Compiler
    elseif(CMAKE_C_COMPILER_ID STREQUAL "Intel")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wcheck
            $<$<CONFIG:Debug>:-O0>
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Release>:-ipo>
        )
    endif()
    
    # Platform-specific optimizations
    if(ZOO_PLATFORM_X86_64 OR ZOO_PLATFORM_X86_32)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target} PRIVATE
                $<$<CONFIG:Release>:-march=native>
                $<$<CONFIG:Release>:-mtune=native>
            )
        endif()
    endif()
    
    # Memory debugging options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target} PRIVATE
                -fsanitize=address
                -fsanitize=undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target} PRIVATE
                -fsanitize=address
                -fsanitize=undefined
            )
        endif()
    endif()
endfunction()

# Function to set up coverage flags
function(zoo_thread_pool_setup_coverage target)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            --coverage
            -fprofile-arcs
            -ftest-coverage
        )
        target_link_options(${target} PRIVATE --coverage)
        message(STATUS "Coverage enabled for ${target}")
    endif()
endfunction()
