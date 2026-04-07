#[[
==============================================================================
Installation Configuration for ZOO Buffer Module
==============================================================================
Description: CMake installation rules and package configuration
Features:
- Library and header installation
- CMake package configuration files
- Cross-platform installation paths

Author: weiwang.sun
Date: 2025-07-30
==============================================================================
]]

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ==============================================================================
# INSTALLATION CONFIGURATION
# ==============================================================================

# Function to configure installation for the zoo_buffer target
function(zoo_buffer_configure_installation)
    # Install library
    install(TARGETS zoo_buffer
        EXPORT ZOOBufferTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zoo
    )
    
    # Install headers
    install(DIRECTORY inc/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zoo
        FILES_MATCHING PATTERN "*.h"
    )
    
    # Install CMake configuration files
    install(EXPORT ZOOBufferTargets
        FILE ZOOBufferTargets.cmake
        NAMESPACE ZOO::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZOOBuffer
    )
    
    # Configure package config file
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZOOBufferConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ZOOBufferConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZOOBuffer
    )
    
    # Generate version file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/ZOOBufferConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    # Install package config files
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ZOOBufferConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ZOOBufferConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZOOBuffer
    )
    
    message(STATUS "ZOO Buffer Module - Installation configured")
    message(STATUS "ZOO Buffer Module - Install with: cmake --build . --target install")
endfunction()
