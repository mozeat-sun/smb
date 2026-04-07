#.rst:
# FindUnity
# ---------
#
# Find Unity test framework in ZOO thirdparty directory
#
# This module defines:
#
# ::
#
#   Unity_FOUND       - True if Unity is found
#   Unity_INCLUDE_DIR - Unity include directory
#   Unity_SOURCE      - Unity source file
#   Unity_LIBRARIES   - Unity library (for interface)
#
# You can add Unity to your project with:
#
# ::
#
#   find_package(Unity REQUIRED)
#   target_link_libraries(your_test_target ${Unity_LIBRARIES})
#   target_include_directories(your_test_target PRIVATE ${Unity_INCLUDE_DIR})
#

# Find the Unity directory relative to ZOO root
get_filename_component(ZOO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
while(NOT EXISTS "${ZOO_ROOT_DIR}/thirdparty" AND NOT ${ZOO_ROOT_DIR} STREQUAL "/")
    get_filename_component(ZOO_ROOT_DIR ${ZOO_ROOT_DIR} DIRECTORY)
endwhile()

set(Unity_ROOT_DIR "${ZOO_ROOT_DIR}/thirdparty/test/unity/src")

# Check if Unity exists
if(EXISTS "${Unity_ROOT_DIR}/unity.h" AND EXISTS "${Unity_ROOT_DIR}/unity.c")
    set(Unity_FOUND TRUE)
    set(Unity_INCLUDE_DIR "${Unity_ROOT_DIR}")
    set(Unity_SOURCE "${Unity_ROOT_DIR}/unity.c")
    
    # Create an interface library for Unity
    if(NOT TARGET Unity::Unity)
        add_library(Unity::Unity INTERFACE IMPORTED)
        set_target_properties(Unity::Unity PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Unity_INCLUDE_DIR}"
            INTERFACE_SOURCES "${Unity_SOURCE}"
        )
    endif()
    
    set(Unity_LIBRARIES Unity::Unity)
    
    if(NOT Unity_FIND_QUIETLY)
        message(STATUS "Found Unity: ${Unity_ROOT_DIR}")
    endif()
else()
    set(Unity_FOUND FALSE)
    if(Unity_FIND_REQUIRED)
        message(FATAL_ERROR "Unity test framework not found in ${Unity_ROOT_DIR}")
    endif()
endif()

# Mark as advanced
mark_as_advanced(Unity_INCLUDE_DIR Unity_SOURCE Unity_LIBRARIES)
