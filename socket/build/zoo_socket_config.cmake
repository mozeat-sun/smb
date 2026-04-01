################################################################################
# Copyright (C) 2025, ZOO Ltd
#
# Product: ZOO
# Module: Socket
# File name: zoo_socket_config.cmake.in
# Description: CMake package configuration file template
#
# Change History:
# Version   Date           Author          Description
# -------   ----------     -----------     ---------------------------------
# 1.0       2025-08-05     assistant       Initial creation
################################################################################


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was zoo_socket_config.cmake.in                            ########

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

# Include the targets file
if(NOT TARGET zoo::zoo_socket_static AND NOT TARGET zoo::zoo_socket_shared)
    include("${CMAKE_CURRENT_LIST_DIR}/zoo_socket_targets.cmake")
endif()

# Set component variables
set(zoo_socket_VERSION 1.0.0)

# Check required components
check_required_components(zoo_socket)

# Provide imported targets
if(TARGET zoo::zoo_socket_static)
    set(zoo_socket_FOUND TRUE)
    set(zoo_socket_LIBRARIES zoo::zoo_socket_static)
    set(zoo_socket_INCLUDE_DIRS $<TARGET_PROPERTY:zoo::zoo_socket_static,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

if(TARGET zoo::zoo_socket_shared)
    set(zoo_socket_FOUND TRUE)
    if(NOT DEFINED zoo_socket_LIBRARIES)
        set(zoo_socket_LIBRARIES zoo::zoo_socket_shared)
        set(zoo_socket_INCLUDE_DIRS $<TARGET_PROPERTY:zoo::zoo_socket_shared,INTERFACE_INCLUDE_DIRECTORIES>)
    endif()
endif()

# Provide usage information
if(zoo_socket_FOUND AND NOT zoo_socket_FIND_QUIETLY)
    message(STATUS "Found zoo_socket: ${zoo_socket_VERSION}")
endif()
