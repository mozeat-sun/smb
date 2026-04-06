#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Zoo::zoo_util_static" for configuration "Release"
set_property(TARGET Zoo::zoo_util_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Zoo::zoo_util_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libzoo_util.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS Zoo::zoo_util_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_Zoo::zoo_util_static "${_IMPORT_PREFIX}/lib/libzoo_util.a" )

# Import target "Zoo::zoo_util_shared" for configuration "Release"
set_property(TARGET Zoo::zoo_util_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Zoo::zoo_util_shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libzoo_util.so.1.0.0"
  IMPORTED_SONAME_RELEASE "libzoo_util.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS Zoo::zoo_util_shared )
list(APPEND _IMPORT_CHECK_FILES_FOR_Zoo::zoo_util_shared "${_IMPORT_PREFIX}/lib/libzoo_util.so.1.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
