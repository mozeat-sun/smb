
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was ZooUtilConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

# ZooUtil CMake Configuration File
# This file sets up the ZooUtil library for use with find_package()

include(CMakeFindDependencyMacro)

# Find required dependencies
find_dependency(Threads)

# Include the targets file
include("${CMAKE_CURRENT_LIST_DIR}/ZooUtilTargets.cmake")

# Provide legacy variables for compatibility
set(ZooUtil_LIBRARIES ZooUtil::zoo_util_static ZooUtil::zoo_util_shared)
set(ZooUtil_INCLUDE_DIRS "${PACKAGE_PREFIX_DIR}/include")

# Check if targets exist
if(NOT TARGET ZooUtil::zoo_util_static)
    set(ZooUtil_FOUND FALSE)
    set(ZooUtil_NOT_FOUND_MESSAGE "ZooUtil static library target not found")
    return()
endif()

if(NOT TARGET ZooUtil::zoo_util_shared)
    set(ZooUtil_FOUND FALSE) 
    set(ZooUtil_NOT_FOUND_MESSAGE "ZooUtil shared library target not found")
    return()
endif()

set(ZooUtil_FOUND TRUE)

# Print status message
if(NOT ZooUtil_FIND_QUIETLY)
    message(STATUS "Found ZooUtil: ${PACKAGE_PREFIX_DIR} (found version \"1.0.0\")")
endif()
