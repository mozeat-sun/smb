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

set(Unity_FOUND FALSE)

get_filename_component(_zoo_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(_unity_candidate_dirs)
if(DEFINED Unity_ROOT AND Unity_ROOT)
    list(APPEND _unity_candidate_dirs "${Unity_ROOT}")
endif()
if(DEFINED ENV{UNITY_ROOT} AND NOT "$ENV{UNITY_ROOT}" STREQUAL "")
    list(APPEND _unity_candidate_dirs "$ENV{UNITY_ROOT}")
endif()
if(DEFINED ZOO_THIRDPARTY_DIR AND ZOO_THIRDPARTY_DIR)
    list(APPEND _unity_candidate_dirs "${ZOO_THIRDPARTY_DIR}/test/unity")
endif()
list(APPEND _unity_candidate_dirs
    "${_zoo_repo_root}/thirdparty/unity"
    "${_zoo_repo_root}/thirdparty/test/unity"
    "${_zoo_repo_root}/third_party/unity"
    "${_zoo_repo_root}/third_party/test/unity"
)

foreach(_candidate IN LISTS _unity_candidate_dirs)
    if(EXISTS "${_candidate}/src/unity.h" AND EXISTS "${_candidate}/src/unity.c")
        set(Unity_ROOT_DIR "${_candidate}")
        set(Unity_INCLUDE_DIR "${_candidate}/src")
        set(Unity_SOURCE "${_candidate}/src/unity.c")
        set(Unity_FOUND TRUE)
        break()
    elseif(EXISTS "${_candidate}/unity.h" AND EXISTS "${_candidate}/unity.c")
        set(Unity_ROOT_DIR "${_candidate}")
        set(Unity_INCLUDE_DIR "${_candidate}")
        set(Unity_SOURCE "${_candidate}/unity.c")
        set(Unity_FOUND TRUE)
        break()
    endif()
endforeach()

if(Unity_FOUND)
    if(NOT TARGET Unity::Unity)
        add_library(Unity::Unity INTERFACE IMPORTED)
        set_target_properties(Unity::Unity PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Unity_INCLUDE_DIR}"
        )
    endif()

    set(Unity_LIBRARIES Unity::Unity)

    if(NOT Unity_FIND_QUIETLY)
        message(STATUS "Found Unity: ${Unity_INCLUDE_DIR}")
    endif()
else()
    if(Unity_FIND_REQUIRED)
        string(JOIN "\n  " _unity_candidates_pretty ${_unity_candidate_dirs})
        message(FATAL_ERROR "Unity test framework not found. Checked:\n  ${_unity_candidates_pretty}")
    endif()
endif()

mark_as_advanced(Unity_ROOT_DIR Unity_INCLUDE_DIR Unity_SOURCE Unity_LIBRARIES)
