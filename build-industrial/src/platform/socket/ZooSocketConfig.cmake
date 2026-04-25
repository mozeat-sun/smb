
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was ZooSocketConfig.cmake.in                            ########

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

include(CMakeFindDependencyMacro)

if(NOT TARGET Zoo::zoo_socket_static AND NOT TARGET Zoo::zoo_socket_shared)
    include("${CMAKE_CURRENT_LIST_DIR}/ZooSocketTargets.cmake")
endif()

set(ZooSocket_FOUND FALSE)
set(ZooSocket_VERSION "1.0.0")

if(TARGET Zoo::zoo_socket_static)
    set(ZooSocket_FOUND TRUE)
    set(ZooSocket_LIBRARIES Zoo::zoo_socket_static)
    set(ZooSocket_INCLUDE_DIRS "$<TARGET_PROPERTY:Zoo::zoo_socket_static,INTERFACE_INCLUDE_DIRECTORIES>")
endif()

if(TARGET Zoo::zoo_socket_shared AND NOT ZooSocket_LIBRARIES)
    set(ZooSocket_FOUND TRUE)
    set(ZooSocket_LIBRARIES Zoo::zoo_socket_shared)
    set(ZooSocket_INCLUDE_DIRS "$<TARGET_PROPERTY:Zoo::zoo_socket_shared,INTERFACE_INCLUDE_DIRECTORIES>")
endif()

# Backward-compatibility variables
set(zoo_socket_FOUND ${ZooSocket_FOUND})
set(zoo_socket_VERSION ${ZooSocket_VERSION})
set(zoo_socket_LIBRARIES ${ZooSocket_LIBRARIES})
set(zoo_socket_INCLUDE_DIRS ${ZooSocket_INCLUDE_DIRS})

check_required_components(ZooSocket)

if(ZooSocket_FOUND AND NOT ZooSocket_FIND_QUIETLY)
    message(STATUS "Found ZooSocket: ${ZooSocket_VERSION}")
endif()
