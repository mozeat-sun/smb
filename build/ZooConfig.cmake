# ZooConfig.cmake.in
# Configuration file for the ZOO umbrella package


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

include(CMakeFindDependencyMacro)

set(ZOO_FOUND TRUE)
set(ZOO_VERSION "1.0.0")
set_and_check(ZOO_INCLUDE_DIRS "${PACKAGE_PREFIX_DIR}/include/zoo")

# Root package currently acts as an umbrella package for installed module configs.
# Individual module targets are exported by their respective module packages.
set(ZOO_SUPPORTED_COMPONENTS
    Platform
    Util
    Log
    MemoryPool
    Buffer
    Socket
    ThreadPool
    Dispatcher
    Timer
    SMB
)

set(_zoo_component_missing FALSE)
foreach(_component IN LISTS Zoo_FIND_COMPONENTS)
    if(NOT _component IN_LIST ZOO_SUPPORTED_COMPONENTS)
        set(Zoo_FOUND FALSE)
        set(_zoo_component_missing TRUE)
        if(NOT Zoo_FIND_QUIETLY)
            message(WARNING "ZOO component '${_component}' is not supported by the umbrella package")
        endif()
    endif()
endforeach()

check_required_components(Zoo)

if(NOT Zoo_FIND_QUIETLY AND Zoo_FOUND)
    message(STATUS "Found ZOO: ${ZOO_VERSION} (${ZOO_INCLUDE_DIRS})")
endif()
