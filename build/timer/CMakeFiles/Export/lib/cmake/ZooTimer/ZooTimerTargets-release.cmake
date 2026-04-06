#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Zoo::zoo_timer_static" for configuration "Release"
set_property(TARGET Zoo::zoo_timer_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Zoo::zoo_timer_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libzoo_timer.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS Zoo::zoo_timer_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_Zoo::zoo_timer_static "${_IMPORT_PREFIX}/lib/libzoo_timer.a" )

# Import target "Zoo::zoo_timer_shared" for configuration "Release"
set_property(TARGET Zoo::zoo_timer_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Zoo::zoo_timer_shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libzoo_timer.so.1.0.0"
  IMPORTED_SONAME_RELEASE "libzoo_timer.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS Zoo::zoo_timer_shared )
list(APPEND _IMPORT_CHECK_FILES_FOR_Zoo::zoo_timer_shared "${_IMPORT_PREFIX}/lib/libzoo_timer.so.1.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
