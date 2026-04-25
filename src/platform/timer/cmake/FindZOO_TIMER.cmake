# ==============================================================================
# ZOO Timer Module CMake Find Script
# ==============================================================================

find_path(ZOO_TIMER_INCLUDE_DIR
    NAMES zoo_timer.h
    PATHS
        /usr/include
        /usr/local/include
        ${CMAKE_INSTALL_PREFIX}/include
    PATH_SUFFIXES zoo timer
)

find_library(ZOO_TIMER_LIBRARY
    NAMES zoo_timer libzoo_timer
    PATHS
        /usr/lib
        /usr/local/lib
        ${CMAKE_INSTALL_PREFIX}/lib
    PATH_SUFFIXES zoo timer
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZOO_TIMER
    REQUIRED_VARS ZOO_TIMER_LIBRARY ZOO_TIMER_INCLUDE_DIR
    VERSION_VAR ZOO_TIMER_VERSION
)

if(ZOO_TIMER_FOUND)
    set(ZOO_TIMER_LIBRARIES ${ZOO_TIMER_LIBRARY})
    set(ZOO_TIMER_INCLUDE_DIRS ${ZOO_TIMER_INCLUDE_DIR})
    
    if(NOT TARGET zoo::timer)
        add_library(zoo::timer UNKNOWN IMPORTED)
        set_target_properties(zoo::timer PROPERTIES
            IMPORTED_LOCATION "${ZOO_TIMER_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZOO_TIMER_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(ZOO_TIMER_INCLUDE_DIR ZOO_TIMER_LIBRARY)
