# ZooConfig.cmake.in
# Configuration file for the ZOO library package


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was ZooConfig.cmake.in                            ########

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

# Set up import targets
include("${CMAKE_CURRENT_LIST_DIR}/ZooTargets.cmake" OPTIONAL)

# Provide legacy variables for compatibility
set(ZOO_FOUND TRUE)
set(ZOO_VERSION "1.0.0")
set(ZOO_INCLUDE_DIRS "/usr/local/include/zoo")

# Check if all requested components are available
check_required_components(Zoo)

# Display found message
if(NOT Zoo_FIND_QUIETLY)
    message(STATUS "Found ZOO: ${ZOO_VERSION} (${ZOO_INCLUDE_DIRS})")
endif()
