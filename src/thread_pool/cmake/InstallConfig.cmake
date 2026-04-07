# ==============================================================================
# ZOO Thread Pool - Enhanced Installation Configuration
# Comprehensive installation management with multiple options
# ==============================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Installation paths configuration
set(ZOO_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/zoo)
set(ZOO_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(ZOO_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(ZOO_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}/zoo-thread-pool)
set(ZOO_INSTALL_DOCDIR ${CMAKE_INSTALL_DOCDIR}/zoo-thread-pool)
set(ZOO_INSTALL_MANDIR ${CMAKE_INSTALL_MANDIR})
set(ZOO_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/ZOOThreadPool)

# Installation configuration function
function(zoo_thread_pool_configure_installation)
    message(STATUS "Configuring comprehensive installation for ZOO Thread Pool")
    
    # ==============================================================================
    # LIBRARY INSTALLATION
    # ==============================================================================
    
    # Install main library
    install(TARGETS zoo_thread_pool
        EXPORT ZOOThreadPoolTargets
        LIBRARY DESTINATION ${ZOO_INSTALL_LIBDIR}
            COMPONENT Runtime
        ARCHIVE DESTINATION ${ZOO_INSTALL_LIBDIR}
            COMPONENT Development
        RUNTIME DESTINATION ${ZOO_INSTALL_BINDIR}
            COMPONENT Runtime
        INCLUDES DESTINATION ${ZOO_INSTALL_INCLUDEDIR}
    )
    
    # ==============================================================================
    # HEADER FILES INSTALLATION
    # ==============================================================================
    
    # Install primary headers
    install(FILES
        inc/zoo_thread_pool.h
        DESTINATION ${ZOO_INSTALL_INCLUDEDIR}
        COMPONENT Development
    )
    
    # Install platform headers if available
    if(EXISTS "${ZOO_PLATFORM_DIR}/inc")
        install(DIRECTORY "${ZOO_PLATFORM_DIR}/inc/"
            DESTINATION ${ZOO_INSTALL_INCLUDEDIR}
            COMPONENT Development
            FILES_MATCHING 
            PATTERN "*.h"
            PATTERN "*.hpp"
        )
        message(STATUS "Will install platform headers from: ${ZOO_PLATFORM_DIR}/inc")
    endif()
    
    # Install memory pool headers if available
    if(EXISTS "${ZOO_MEMORY_POOL_DIR}/inc/zoo_memory_pool.h")
        install(FILES
            "${ZOO_MEMORY_POOL_DIR}/inc/zoo_memory_pool.h"
            DESTINATION ${ZOO_INSTALL_INCLUDEDIR}
            COMPONENT Development
        )
        message(STATUS "Will install memory pool header")
    endif()
    
    # ==============================================================================
    # EXAMPLES INSTALLATION
    # ==============================================================================
    
    if(ZOO_ENABLE_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/examples/"
            DESTINATION ${ZOO_INSTALL_DATADIR}/examples
            COMPONENT Examples
            FILES_MATCHING 
            PATTERN "*.c"
            PATTERN "*.cpp"
            PATTERN "*.h"
            PATTERN "*.md"
            PATTERN "CMakeLists.txt"
        )
        message(STATUS "Will install examples")
    endif()
    
    # ==============================================================================
    # DOCUMENTATION INSTALLATION
    # ==============================================================================
    
    # Install README and LICENSE
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        DESTINATION ${ZOO_INSTALL_DOCDIR}
        COMPONENT Documentation
        OPTIONAL
    )
    
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION ${ZOO_INSTALL_DOCDIR}
        COMPONENT Documentation
        OPTIONAL
    )
    
    # Install generated documentation if available
    if(ZOO_ENABLE_DOCUMENTATION AND TARGET doc)
        install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs/html/"
            DESTINATION ${ZOO_INSTALL_DOCDIR}/html
            COMPONENT Documentation
            OPTIONAL
        )
    endif()
    
    # ==============================================================================
    # PKG-CONFIG FILE INSTALLATION
    # ==============================================================================
    
    # Generate pkg-config file
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/zoo-thread-pool.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/zoo-thread-pool.pc"
        @ONLY
    )
    
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/zoo-thread-pool.pc"
        DESTINATION ${ZOO_INSTALL_LIBDIR}/pkgconfig
        COMPONENT Development
    )
    
    # ==============================================================================
    # CMAKE CONFIG FILES INSTALLATION
    # ==============================================================================
    
    # Set up variables for config file generation
    set(INCLUDE_INSTALL_DIR ${ZOO_INSTALL_INCLUDEDIR})
    set(LIB_INSTALL_DIR ${ZOO_INSTALL_LIBDIR})
    
    # Generate package config files
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZOOThreadPoolConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ZOOThreadPoolConfig.cmake"
        INSTALL_DESTINATION "${ZOO_INSTALL_CMAKEDIR}"
        PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )
    
    # Generate version file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/ZOOThreadPoolConfigVersion.cmake"
        VERSION ${ZOO_THREAD_POOL_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    # Install config files
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ZOOThreadPoolConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ZOOThreadPoolConfigVersion.cmake"
        DESTINATION ${ZOO_INSTALL_CMAKEDIR}
        COMPONENT Development
    )
    
    # Install export targets
    install(EXPORT ZOOThreadPoolTargets
        FILE ZOOThreadPoolTargets.cmake
        NAMESPACE ZOO::
        DESTINATION ${ZOO_INSTALL_CMAKEDIR}
        COMPONENT Development
    )
    
    # ==============================================================================
    # TOOLS AND UTILITIES INSTALLATION
    # ==============================================================================
    
    # Install benchmark tools if built
    if(ZOO_ENABLE_BENCHMARKS AND TARGET zoo_thread_pool_benchmark)
        install(TARGETS zoo_thread_pool_benchmark
            RUNTIME DESTINATION ${ZOO_INSTALL_BINDIR}
            COMPONENT Tools
        )
    endif()
    
    # Install test utilities if built
    if(ZOO_ENABLE_TESTS)
        if(TARGET zoo_thread_pool_tests)
            install(TARGETS zoo_thread_pool_tests
                RUNTIME DESTINATION ${ZOO_INSTALL_BINDIR}
                COMPONENT Testing
                OPTIONAL
            )
        endif()
        
        if(TARGET zoo_thread_pool_integration_tests)
            install(TARGETS zoo_thread_pool_integration_tests
                RUNTIME DESTINATION ${ZOO_INSTALL_BINDIR}
                COMPONENT Testing
                OPTIONAL
            )
        endif()
    endif()
    
    # ==============================================================================
    # SYSTEM INTEGRATION FILES
    # ==============================================================================
    
    # Install systemd service files if on Linux
    if(UNIX AND NOT APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/etc/systemd")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/etc/systemd/"
            DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/systemd/system
            COMPONENT System
            OPTIONAL
        )
    endif()
    
    # Install configuration files
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/etc/zoo")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/etc/zoo/"
            DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/zoo
            COMPONENT Configuration
            OPTIONAL
        )
    endif()
    
    # ==============================================================================
    # INSTALLATION SUMMARY
    # ==============================================================================
    
    message(STATUS "")
    message(STATUS "Installation Configuration Summary:")
    message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "  Libraries: ${ZOO_INSTALL_LIBDIR}")
    message(STATUS "  Headers: ${ZOO_INSTALL_INCLUDEDIR}")
    message(STATUS "  Binaries: ${ZOO_INSTALL_BINDIR}")
    message(STATUS "  Documentation: ${ZOO_INSTALL_DOCDIR}")
    message(STATUS "  Examples: ${ZOO_INSTALL_DATADIR}/examples")
    message(STATUS "  CMake files: ${ZOO_INSTALL_CMAKEDIR}")
    message(STATUS "")
    
endfunction()

# ==============================================================================
# UNINSTALLATION SUPPORT
# ==============================================================================

# Create uninstall target
function(zoo_thread_pool_add_uninstall_target)
    if(NOT TARGET uninstall)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            @ONLY
        )
        
        add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
            COMMENT "Uninstalling ZOO Thread Pool"
        )
    endif()
endfunction()

# ==============================================================================
# COMPONENT-BASED INSTALLATION
# ==============================================================================

# Function to configure installation components
function(zoo_thread_pool_configure_components)
    # Define installation components
    set(CPACK_COMPONENTS_ALL Runtime Development Documentation Examples Tools Testing System Configuration)
    
    # Component descriptions
    set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime Libraries")
    set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Runtime libraries required to run applications using ZOO Thread Pool")
    set(CPACK_COMPONENT_RUNTIME_GROUP "Runtime")
    set(CPACK_COMPONENT_RUNTIME_REQUIRED TRUE)
    
    set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Files")
    set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Header files and static libraries for development")
    set(CPACK_COMPONENT_DEVELOPMENT_GROUP "Development")
    set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS Runtime)
    
    set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
    set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "API documentation and user guides")
    set(CPACK_COMPONENT_DOCUMENTATION_GROUP "Documentation")
    
    set(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME "Examples")
    set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "Example code and tutorials")
    set(CPACK_COMPONENT_EXAMPLES_GROUP "Development")
    set(CPACK_COMPONENT_EXAMPLES_DEPENDS Development)
    
    set(CPACK_COMPONENT_TOOLS_DISPLAY_NAME "Tools and Utilities")
    set(CPACK_COMPONENT_TOOLS_DESCRIPTION "Benchmarking and debugging tools")
    set(CPACK_COMPONENT_TOOLS_GROUP "Tools")
    set(CPACK_COMPONENT_TOOLS_DEPENDS Runtime)
    
    set(CPACK_COMPONENT_TESTING_DISPLAY_NAME "Testing Suite")
    set(CPACK_COMPONENT_TESTING_DESCRIPTION "Unit and integration test executables")
    set(CPACK_COMPONENT_TESTING_GROUP "Development")
    set(CPACK_COMPONENT_TESTING_DEPENDS Development)
    
    set(CPACK_COMPONENT_SYSTEM_DISPLAY_NAME "System Integration")
    set(CPACK_COMPONENT_SYSTEM_DESCRIPTION "System service files and integration scripts")
    set(CPACK_COMPONENT_SYSTEM_GROUP "System")
    
    set(CPACK_COMPONENT_CONFIGURATION_DISPLAY_NAME "Configuration Files")
    set(CPACK_COMPONENT_CONFIGURATION_DESCRIPTION "Default configuration files and templates")
    set(CPACK_COMPONENT_CONFIGURATION_GROUP "Configuration")
    
    # Component groups
    set(CPACK_COMPONENT_GROUP_RUNTIME_DESCRIPTION "Essential runtime components")
    set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION "Development and building tools")
    set(CPACK_COMPONENT_GROUP_DOCUMENTATION_DESCRIPTION "Documentation and help files")
    set(CPACK_COMPONENT_GROUP_TOOLS_DESCRIPTION "Additional utilities and tools")
    set(CPACK_COMPONENT_GROUP_SYSTEM_DESCRIPTION "System integration components")
    set(CPACK_COMPONENT_GROUP_CONFIGURATION_DESCRIPTION "Configuration and setup files")
endfunction()

message(STATUS "Enhanced ZOO Thread Pool installation configuration loaded")
